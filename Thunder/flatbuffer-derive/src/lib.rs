extern crate proc_macro;

use proc_macro2::TokenStream;
use quote::{format_ident, quote};

fn gather_members(input: &syn::DeriveInput) -> Vec<&syn::Ident> {
    let data_struct = if let syn::Data::Struct(data_struct) = &input.data {
        data_struct
    } else {
        panic!("Derive is not used for a struct")
    };
    let mut result = Vec::new();
    for field in &data_struct.fields {
        result.push(field.ident.as_ref().unwrap());
    }

    result
}

fn generate_create(member: &&syn::Ident) -> TokenStream {
    let offset_name = format_ident!("{}_offset", member);
    quote! {
        let #offset_name = data_definition_generated::CreateInBuilder::create_in_builder(&self.#member, &mut builder);
    }
}

fn generate_args_assign(member: &&syn::Ident) -> TokenStream {
    let offset_name = format_ident!("{}_offset", member);
    quote! {
        #member: Some(#offset_name)
    }
}

#[proc_macro_derive(FlatbufferSerialize)]
pub fn derive_serialize(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
    let parsed_input: syn::DeriveInput = syn::parse(input).unwrap();
    let struct_name = parsed_input.ident.clone();

    let members = gather_members(&parsed_input);
    let members_creates = members.iter().map(generate_create);
    let members_assign = members.iter().map(generate_args_assign);

    let flatbuffer_struct_name = struct_name.clone();
    let flatbuffer_struct_args_name = format_ident!("{}Args", flatbuffer_struct_name);
    let identifier = format_ident!(
        "{}_IDENTIFIER",
        flatbuffer_struct_name.to_string().to_uppercase()
    );
    let result = quote! {
        impl #struct_name<'_> {
            fn serialize_root(&self) -> Vec<u8> {
                let mut builder = data_definition_generated::flatbuffers::FlatBufferBuilder::new_with_capacity(1024 * 1024);
                #(#members_creates)*
                let root = data_definition_generated::#flatbuffer_struct_name::create(
                    &mut builder,
                    &data_definition_generated::#flatbuffer_struct_args_name {
                        #(#members_assign,)*
                    }
                );
                builder.finish(root, Some(data_definition_generated::#identifier));
                Vec::from(builder.finished_data())
            }
        }
    };

    result.into()
}
