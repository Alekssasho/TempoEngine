use std::env;
use std::path::PathBuf;

fn main() {
    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("../../Tempest/World/Components/Components.h")
        // We don't need any function or methods
        .ignore_functions()
        .ignore_methods()
        //.layout_tests(false) // This could be used to disable layout tests
        .clang_arg("-DBINDGEN")
        // Add input dir for Math.h and glm headers
        .clang_arg("-I../../Tempest")
        .clang_arg("-I../../ThirdParty/glm/include")
        // Tell that the header is c++
        .clang_arg("-xc++")
        // Export only Tempest types in the header
        .whitelist_type("Tempest::.*")
        // But disable glm export as we will be using our rust glm types
        .blacklist_type("glm::.*")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/component_bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("component_bindings.rs"))
        .expect("Couldn't write bindings!");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=../../Tempest/World/Components/Components.h");
}
