extern crate proc_macro;

use proc_macro2::TokenStream;
use quote::{format_ident, quote};

struct Member<'a> {
    name: &'a syn::Ident,
    should_use_offset: bool,
}

fn gather_members(input: &syn::DeriveInput) -> Vec<Member> {
    let data_struct = if let syn::Data::Struct(data_struct) = &input.data {
        data_struct
    } else {
        panic!("Derive is not used for a struct")
    };
    let mut result = Vec::new();
    for field in &data_struct.fields {
        result.push(Member {
            name: field.ident.as_ref().unwrap(),
            should_use_offset: field.attrs.iter().any(|attr| {
                attr.parse_meta()
                    .map_or(false, |meta| meta.path().is_ident("offset"))
            }),
        });
    }

    result
}

fn generate_create(member: &Member) -> TokenStream {
    if member.should_use_offset {
        let member_name = member.name;
        let offset_name = format_ident!("{}_offset", member_name);
        quote! {
            let #offset_name = data_definition_generated::CreateInBuilder::create_in_builder(&self.#member_name, builder);
        }
    } else {
        TokenStream::new()
    }
}

fn generate_args_assign(member: &Member) -> TokenStream {
    let member_name = member.name;
    if member.should_use_offset {
        let offset_name = format_ident!("{}_offset", member_name);
        quote! {
            #member_name: Some(#offset_name)
        }
    } else {
        quote! {
            #member_name: self.#member_name
        }
    }
}

fn get_fbb_lifetime_def(lifetime: syn::Lifetime) -> syn::LifetimeDef {
    syn::LifetimeDef {
        attrs: Vec::new(),
        lifetime,
        colon_token: None,
        bounds: syn::punctuated::Punctuated::new(),
    }
}

fn fix_lifetimes(mut generics: syn::Generics) -> syn::Generics {
    let fbb_lifetime = syn::Lifetime::new("'fbb", proc_macro2::Span::call_site());

    generics
        .params
        .iter_mut()
        .for_each(|generic| match generic {
            syn::GenericParam::Lifetime(lifetime) => {
                lifetime.bounds.push(fbb_lifetime.clone());
            }
            _ => (),
        });

    generics.params = Some(syn::GenericParam::Lifetime(get_fbb_lifetime_def(
        fbb_lifetime,
    )))
    .into_iter()
    .chain(generics.params)
    .collect();
    generics
}

fn generate_where_clause(mut generics: syn::Generics) -> Option<syn::WhereClause> {
    if generics.lifetimes().count() == 0 {
        return None;
    }
    generics.make_where_clause();
    let mut clause = generics.where_clause.clone().unwrap();

    let fbb_lifetime = syn::Lifetime::new("'fbb", proc_macro2::Span::call_site());

    generics.lifetimes().for_each(|lifetime| {
        let mut bounds = syn::punctuated::Punctuated::new();
        bounds.push(fbb_lifetime.clone());
        clause
            .predicates
            .push(syn::WherePredicate::Lifetime(syn::PredicateLifetime {
                lifetime: lifetime.lifetime.clone(),
                colon_token: <syn::Token![:]>::default(),
                bounds,
            }))
    });

    Some(clause)
}

fn derive_serialize_impl(
    input: proc_macro::TokenStream,
    include_root: bool,
) -> proc_macro::TokenStream {
    let parsed_input: syn::DeriveInput = syn::parse(input).unwrap();
    let struct_name = parsed_input.ident.clone();

    // Support generics parameters of the input struct
    let (impl_generics, ty_generics, where_clause) = parsed_input.generics.split_for_impl();
    let cloned_generics = fix_lifetimes(parsed_input.generics.clone());
    let (impl_fbb_generics, _, _) = cloned_generics.split_for_impl();
    let fbb_where_clause = generate_where_clause(parsed_input.generics.clone());

    let members = gather_members(&parsed_input);
    let members_creates = members.iter().map(generate_create);
    let members_assign = members.iter().map(generate_args_assign);

    let flatbuffer_struct_name = struct_name.clone();
    let flatbuffer_struct_args_name = format_ident!("{}Args", flatbuffer_struct_name);
    let underscored_name = {
        let mut name = String::new();
        for (index, ch) in flatbuffer_struct_name.to_string().chars().enumerate() {
            if ch.is_uppercase() && index > 0 {
                name.push('_');
            }
            name.push(ch);
        }
        name.to_uppercase()
    };
    let identifier = format_ident!("{}_IDENTIFIER", underscored_name);
    let serialize_root = if include_root {
        quote! {
            fn serialize_root(self) -> Vec<u8> {
                let mut builder = data_definition_generated::flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
                let root = self.serialize(&mut builder);
                builder.finish(root, Some(data_definition_generated::#identifier));
                Vec::from(builder.finished_data())
            }
        }
    } else {
        quote! {}
    };
    let result = quote! {
        impl#impl_fbb_generics data_definition_generated::CreateInBuilder<'fbb> for #struct_name#ty_generics {
            type Item = data_definition_generated::#flatbuffer_struct_name<'fbb>;

            fn create_in_builder(
                &self,
                builder: &mut data_definition_generated::flatbuffers::FlatBufferBuilder<'fbb>,
            ) -> data_definition_generated::flatbuffers::WIPOffset<Self::Item> {
                self.serialize(builder)
            }
        }

        impl#impl_generics #struct_name#ty_generics #where_clause {
            fn serialize<'fbb>(&self, builder: &mut data_definition_generated::flatbuffers::FlatBufferBuilder<'fbb>) -> data_definition_generated::flatbuffers::WIPOffset<data_definition_generated::#flatbuffer_struct_name<'fbb>> #fbb_where_clause {
                #(#members_creates)*
                data_definition_generated::#flatbuffer_struct_name::create(
                    builder,
                    &data_definition_generated::#flatbuffer_struct_args_name {
                        #(#members_assign,)*
                    }
                )
            }
            #serialize_root
        }
    };

    result.into()
}

#[proc_macro_derive(FlatbufferSerialize, attributes(offset))]
pub fn derive_serialize(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    derive_serialize_impl(input, false)
}

#[proc_macro_derive(FlatbufferSerializeRoot, attributes(offset))]
pub fn derive_serialize_root(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    derive_serialize_impl(input, true)
}
