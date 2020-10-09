use std::env;
use std::path::PathBuf;

fn main() {
    // Compile flecs
    cc::Build::new()
        .file("../../ThirdParty/flecs/flecs.c")
        .opt_level(3)
        .compile("flecs-rs");

    // Create binding for flecs
    let bindings = bindgen::Builder::default()
        .header("../../ThirdParty/flecs/flecs.h")
        .layout_tests(false) // This could be used to disable layout tests
        // Export only Tempest types in the header
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/flecs-binding.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("flecs-binding.rs"))
        .expect("Couldn't write bindings!");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=../../ThirdParty/flecs/flecs.h");
}
