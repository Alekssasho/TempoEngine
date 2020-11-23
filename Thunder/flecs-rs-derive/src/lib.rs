extern crate proc_macro;

use proc_macro::TokenStream;
use quote::quote;
#[derive(Debug)]
struct Component {
    name: syn::Ident,
}

fn gather_components(input: &syn::DeriveInput) -> Vec<Component> {
    if let syn::Data::Enum(enum_data) = &input.data {
        let mut result = Vec::new();
        for variant in &enum_data.variants {
            result.push(Component {
                name: variant.ident.clone(),
            });
        }
        result
    } else {
        panic!("Input data is not a enum");
    }
}

fn generate_get_component_data_function(
    name: &syn::Ident,
    components: &Vec<Component>,
) -> proc_macro2::TokenStream {
    let variant_names: Vec<&syn::Ident> = components.iter().map(|comp| &comp.name).collect();
    quote! {
        fn get_pointer(&self) -> *const c_void {
            match self {
                #(#name::#variant_names(d) => d as *const _ as *const c_void,)*
            }
        }
    }
}

fn generate_get_index_function(
    name: &syn::Ident,
    components: &Vec<Component>,
) -> proc_macro2::TokenStream {
    let variant_names: Vec<&syn::Ident> = components.iter().map(|comp| &comp.name).collect();
    let indices = 0..components.len();
    quote! {
        fn get_index(&self) -> usize {
            match self {
                #(#name::#variant_names(_) => #indices,)*
            }
        }
    }
}

#[proc_macro_derive(Components)]
pub fn derive_components(input: TokenStream) -> TokenStream {
    let parsed_input: syn::DeriveInput = syn::parse(input).unwrap();
    let components_enum_name = parsed_input.ident.clone();
    let gather_components = gather_components(&parsed_input);
    let get_component_data_function =
        generate_get_component_data_function(&components_enum_name, &gather_components);
    let get_index_function = generate_get_index_function(&components_enum_name, &gather_components);
    let result = quote! {
        impl #components_enum_name {
            #get_component_data_function
            #get_index_function
        }
    };
    //println!("{}", result);
    result.into()
}
