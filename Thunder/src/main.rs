use data_definition_generated::LEVEL_EXTENSION;
use std::{
    collections::HashMap,
    path::PathBuf,
    sync::{
        atomic::{AtomicUsize, Ordering},
        Mutex,
    },
};
use structopt::StructOpt;
use tracing::Subscriber;
use tracing_subscriber::prelude::*;
use tracing_subscriber::{registry, Layer};

mod compiler;
mod resources;
mod scene;

use resources::{level::LevelResource, write_resource_to_file};

#[macro_use]
extern crate async_trait;

#[macro_use]
extern crate tracing;

#[macro_export]
macro_rules! ttrace {
    ($name:expr) => {
        let span = trace_span!($name);
        let _entered = span.enter();
    };
}

#[derive(StructOpt, Debug)]
struct CommandLineOptions {
    /// Output folder to put generated level
    #[structopt(short, long, parse(from_os_str))]
    output_folder: PathBuf,

    /// Input folder to read level from
    #[structopt(short, long, parse(from_os_str))]
    input_folder: PathBuf,

    #[structopt(short, long)]
    level_name: String,
}

struct OptickLayer {
    current_events: Mutex<HashMap<tracing::Id, u64>>,
}

impl OptickLayer {
    fn new() -> Self {
        OptickLayer {
            current_events: Mutex::new(HashMap::new()),
        }
    }
}

impl<S> Layer<S> for OptickLayer
where
    S: Subscriber + for<'a> registry::LookupSpan<'a>,
{
    fn on_enter(&self, id: &tracing::Id, ctx: tracing_subscriber::layer::Context<'_, S>) {
        if let Some(span_data) = ctx.span(id) {
            let metadata = span_data.metadata();
            let file = metadata.file().unwrap_or("<error: not available>");
            let line = metadata.line().unwrap_or(0);
            let function_name = format!("{}::{}", metadata.module_path().unwrap(), metadata.name());

            let description = optick::create_description(&function_name, file, line);
            let event_id = optick::push_event(description);
            self.current_events
                .lock()
                .unwrap()
                .insert(id.clone(), event_id);
        }
    }

    fn on_exit(&self, id: &tracing::Id, _: tracing_subscriber::layer::Context<'_, S>) {
        let map_ref = self.current_events.lock().unwrap();
        let event_id = map_ref[id];
        optick::pop_event(event_id);
    }
}

#[instrument]
async fn compile_level(opt: CommandLineOptions) {
    let level = Box::new(LevelResource::new(opt.level_name.to_string()));

    let compiled_data = compiler::compile_async(
        level,
        compiler::CompilerOptions {
            output_folder: opt.output_folder.clone(),
            input_folder: opt.input_folder.clone(),
        },
    )
    .await;

    let mut output_file_path = opt.output_folder.clone();
    output_file_path.push(format!("Level_{}", opt.level_name));
    output_file_path.set_extension(LEVEL_EXTENSION);
    write_resource_to_file(compiled_data.as_slice(), output_file_path).await;
}

#[optick_attr::capture("Thunder")]
fn main() {
    let opt = CommandLineOptions::from_args();

    tracing::subscriber::set_global_default(
        tracing_subscriber::registry().with(OptickLayer::new()),
    )
    .unwrap();

    let rt = tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .on_thread_start(|| {
            static ATOMIC_ID: AtomicUsize = AtomicUsize::new(0);
            let id = ATOMIC_ID.fetch_add(1, Ordering::SeqCst);
            optick::register_thread(&format!("Thread Pool {}", id));
        })
        .build()
        .unwrap();

    rt.block_on(compile_level(opt));
}
