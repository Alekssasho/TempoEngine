extern crate proc_macro;

use proc_macro::TokenStream;
use quote::{format_ident, quote};
#[derive(Debug)]
struct Component {
    name: syn::Ident,
    data_name: syn::Ident,
}

impl Component {
    fn get_data_name_string(&self) -> syn::Ident {
        format_ident!("{}_Name", self.data_name)
    }
}

fn gather_components(input: &syn::DeriveInput) -> Vec<Component> {
    if let syn::Data::Enum(enum_data) = &input.data {
        let mut result = Vec::new();
        for variant in &enum_data.variants {
            let data_name = if let syn::Fields::Unnamed(field) = &variant.fields {
                if let syn::Type::Path(path_type) = &field.unnamed.first().as_ref().unwrap().ty {
                    path_type.path.segments.first().unwrap().ident.clone()
                } else {
                    panic!("The field in the variant is wrong again");
                }
            } else {
                panic!("The field in the variant is wrong");
            };

            result.push(Component {
                name: variant.ident.clone(),
                data_name,
            });
        }
        result
    } else {
        panic!("Input data is not a enum");
    }
}

fn generate_components_get_component_data_function(
    name: &syn::Ident,
    components: &Vec<Component>,
) -> proc_macro2::TokenStream {
    let variant_names = components.iter().map(|comp| &comp.name);
    quote! {
        fn get_pointer_and_size(&self) -> (*const u8, usize) {
            match self {
                #(#name::#variant_names(d) => (d as *const _ as *const u8, std::mem::size_of_val(d)),)*
            }
        }
    }
}

fn generate_components_get_name_function(
    name: &syn::Ident,
    components: &Vec<Component>,
) -> proc_macro2::TokenStream {
    let variant_names = components.iter().map(|comp| &comp.name);
    let component_name_ident = components.iter().map(|comp| comp.get_data_name_string());
    quote! {
        fn get_name(&self) -> &str {
            match self {
                #(#name::#variant_names(_) => std::str::from_utf8(#component_name_ident).unwrap().trim_end_matches(char::from(0)),)*
            }
        }
    }
}

#[proc_macro_derive(Components)]
pub fn derive_components(input: TokenStream) -> TokenStream {
    let parsed_input: syn::DeriveInput = syn::parse(input).unwrap();
    let components_enum_name = parsed_input.ident.clone();
    let components = gather_components(&parsed_input);
    let get_component_data_function =
        generate_components_get_component_data_function(&components_enum_name, &components);
    let get_name_function =
        generate_components_get_name_function(&components_enum_name, &components);
    let result = quote! {
        impl #components_enum_name {
            #get_component_data_function
            #get_name_function
        }
    };

    result.into()
}

fn gather_tags(input: &syn::DeriveInput) -> Vec<syn::Ident> {
    if let syn::Data::Enum(enum_data) = &input.data {
        let mut result = Vec::new();
        for variant in &enum_data.variants {
            result.push(variant.ident.clone());
        }
        result
    } else {
        panic!("Input data is not a enum");
    }
}

fn generate_tags_get_name_function(
    name: &syn::Ident,
    tags: &Vec<syn::Ident>,
) -> proc_macro2::TokenStream {
    let tags_name_ident = tags.iter().map(|name| get_tag_name_ident(name));
    quote! {
        fn get_name(&self) -> &str {
            match self {
                #(#name::#tags => std::str::from_utf8(#tags_name_ident).unwrap().trim_end_matches(char::from(0)),)*
            }
        }
    }
}

fn get_tag_name_ident(tag_name: &syn::Ident) -> syn::Ident {
    format_ident!("Tempest_Tags_{}_Name", tag_name)
}

#[proc_macro_derive(Tags)]
pub fn derive_tags(input: TokenStream) -> TokenStream {
    let parsed_input: syn::DeriveInput = syn::parse(input).unwrap();
    let tags_enum_name = parsed_input.ident.clone();
    let tags = gather_tags(&parsed_input);
    let get_name_function = generate_tags_get_name_function(&tags_enum_name, &tags);
    let result = quote! {
        impl #tags_enum_name {
            #get_name_function
        }
    };

    result.into()
}
