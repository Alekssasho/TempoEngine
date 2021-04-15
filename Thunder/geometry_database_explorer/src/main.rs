use std::time::Instant;

use egui::FontDefinitions;
use egui_wgpu_backend::{RenderPass, ScreenDescriptor};
use egui_winit_platform::{Platform, PlatformDescriptor};
use futures_lite::future::block_on;
use mesh_shader::ShaderConstants;
use wgpu::util::DeviceExt;
use winit::event::Event::*;
use winit::event_loop::ControlFlow;

use std::io::Read;
use structopt::StructOpt;

const INITIAL_WIDTH: u32 = 1280;
const INITIAL_HEIGHT: u32 = 720;
const OUTPUT_FORMAT: wgpu::TextureFormat = wgpu::TextureFormat::Bgra8UnormSrgb;

#[derive(StructOpt)]
struct CommandLineOptions {
    /// Input file to read geometry database
    #[structopt(short, long, parse(from_os_str))]
    input_database: std::path::PathBuf,
}

unsafe fn any_as_u8_slice<T: Sized>(p: &T) -> &[u8] {
    ::std::slice::from_raw_parts((p as *const T) as *const u8, ::std::mem::size_of::<T>())
}

fn main() {
    let opt = CommandLineOptions::from_args();
    let mut database_file =
        std::fs::File::open(opt.input_database.clone()).expect("Valid database file");
    let mut database_file_contents = Vec::new();
    database_file
        .read_to_end(&mut database_file_contents)
        .unwrap();
    // Leak is necessery in order to transfer lifetime to 'static. event_loop.run is taking 'static lifetime
    // Can use EventLoopExtRunReturn::run_return if this turned to be a problem.
    let database =
        data_definition_generated::get_root_as_geometry_database(database_file_contents.leak());

    // Start the event loop
    let event_loop = winit::event_loop::EventLoop::new();
    let window = winit::window::WindowBuilder::new()
        .with_decorations(true)
        .with_resizable(true)
        .with_transparent(false)
        .with_title("Tempo Engine Geometry Database Explorer")
        .with_inner_size(winit::dpi::PhysicalSize {
            width: INITIAL_WIDTH,
            height: INITIAL_HEIGHT,
        })
        .build(&event_loop)
        .unwrap();

    let instance = wgpu::Instance::new(wgpu::BackendBit::PRIMARY);
    let surface = unsafe { instance.create_surface(&window) };

    let adapter = block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        compatible_surface: Some(&surface),
    }))
    .unwrap();

    let (mut device, mut queue) = block_on(adapter.request_device(
        &wgpu::DeviceDescriptor {
            features: wgpu::Features::PUSH_CONSTANTS,
            limits: wgpu::Limits {
                max_push_constant_size: 256,
                ..Default::default()
            },
            label: None,
        },
        None,
    ))
    .unwrap();

    let size = window.inner_size();
    let mut sc_desc = wgpu::SwapChainDescriptor {
        usage: wgpu::TextureUsage::RENDER_ATTACHMENT,
        format: OUTPUT_FORMAT,
        width: size.width as u32,
        height: size.height as u32,
        present_mode: wgpu::PresentMode::Mailbox,
    };
    let mut swap_chain = device.create_swap_chain(&surface, &sc_desc);

    // Load shaders
    let module = device.create_shader_module(&wgpu::include_spirv!(env!("mesh_shader.spv")));

    let vertex_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: Some("Vertex buffer"),
        contents: database.vertex_buffer().unwrap(),
        usage: wgpu::BufferUsage::VERTEX,
    });

    let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
        label: None,
        bind_group_layouts: &[],
        push_constant_ranges: &[wgpu::PushConstantRange {
            stages: wgpu::ShaderStage::VERTEX,
            range: 0..std::mem::size_of::<ShaderConstants>() as u32,
        }],
    });

    let vertex_buffer_layout = [wgpu::VertexBufferLayout {
        array_stride: (std::mem::size_of::<f32>() * 3) as u64,
        step_mode: wgpu::InputStepMode::Vertex,
        attributes: &[wgpu::VertexAttribute {
            format: wgpu::VertexFormat::Float3,
            offset: 0,
            shader_location: 0,
        }],
    }];

    let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
        label: None,
        layout: Some(&pipeline_layout),
        vertex: wgpu::VertexState {
            module: &module,
            entry_point: "main_vs",
            buffers: &vertex_buffer_layout,
        },
        primitive: wgpu::PrimitiveState {
            topology: wgpu::PrimitiveTopology::TriangleList,
            strip_index_format: None,
            front_face: wgpu::FrontFace::Ccw,
            cull_mode: wgpu::CullMode::None,
            polygon_mode: wgpu::PolygonMode::Fill,
        },
        depth_stencil: None,
        multisample: wgpu::MultisampleState {
            count: 1,
            mask: !0,
            alpha_to_coverage_enabled: false,
        },
        fragment: Some(wgpu::FragmentState {
            module: &module,
            entry_point: "main_fs",
            targets: &[wgpu::ColorTargetState {
                format: sc_desc.format,
                alpha_blend: wgpu::BlendState::REPLACE,
                color_blend: wgpu::BlendState::REPLACE,
                write_mask: wgpu::ColorWrite::ALL,
            }],
        }),
    });

    // We use the egui_winit_platform crate as the platform.
    let mut platform = Platform::new(PlatformDescriptor {
        physical_width: size.width as u32,
        physical_height: size.height as u32,
        scale_factor: window.scale_factor(),
        font_definitions: FontDefinitions::default(),
        style: Default::default(),
    });

    // We use the egui_wgpu_backend crate as the render backend.
    let mut egui_rpass = RenderPass::new(&device, OUTPUT_FORMAT);

    let mut current_mesh_index = 0;
    let start_time = Instant::now();
    event_loop.run(move |event, _, control_flow| {
        // Pass the winit events to the platform integration.
        platform.handle_event(&event);

        match event {
            RedrawRequested(..) => {
                platform.update_time(start_time.elapsed().as_secs_f64());

                let output_frame = match swap_chain.get_current_frame() {
                    Ok(frame) => frame,
                    Err(e) => {
                        eprintln!("Dropped frame with error: {}", e);
                        return;
                    }
                };

                // Begin to draw the UI frame.
                platform.begin_frame();
                let mut current_mesh_vertex_count = 0;
                let mut current_mesh_vertex_offset = 0;
                egui::Window::new("Geometry Explorer").show(&platform.context(), |ui| {
                    ui.label(format!("Filename: {}", opt.input_database.display()));
                    if let Some(mappings) = database.mappings() {
                        ui.label(format!("Number of meshes: {}", mappings.len()));
                        ui.separator();

                        ui.label(format!("Current mesh: {}", current_mesh_index));
                        let mesh = mappings.iter().nth(current_mesh_index).unwrap();
                        ui.label(format! {"Vertex count: {}", mesh.vertex_count()});
                        ui.label(format! {"Vertex offset: {}", mesh.vertex_offset()});
                        ui.label(format! {"Mesh index: {}", mesh.index()});

                        current_mesh_vertex_count = mesh.vertex_count();
                        current_mesh_vertex_offset = mesh.vertex_offset();

                        ui.horizontal(|ui| {
                            if ui.button("Previous").clicked() {
                                current_mesh_index = if current_mesh_index == 0 {
                                    mappings.len() - 1
                                } else {
                                    current_mesh_index - 1
                                };
                            }
                            if ui.button("Next").clicked() {
                                current_mesh_index = if current_mesh_index == (mappings.len() - 1) {
                                    0
                                } else {
                                    current_mesh_index + 1
                                };
                            }
                        });
                    }
                });

                // End the UI frame. We could now handle the output and draw the UI with the backend.
                let (_output, paint_commands) = platform.end_frame();
                let paint_jobs = platform.context().tessellate(paint_commands);

                let mut encoder = device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("encoder"),
                });
                {
                    let mut rpass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: None,
                        color_attachments: &[wgpu::RenderPassColorAttachmentDescriptor {
                            attachment: &output_frame.output.view,
                            resolve_target: None,
                            ops: wgpu::Operations {
                                load: wgpu::LoadOp::Clear(wgpu::Color::GREEN),
                                store: true,
                            },
                        }],
                        depth_stencil_attachment: None,
                    });

                    rpass.set_pipeline(&render_pipeline);
                    rpass.set_vertex_buffer(0, vertex_buffer.slice(..));

                    let push_constants = ShaderConstants {
                        view_projection_matrix: glam::Mat4::perspective_rh(
                            60f32.to_radians(),
                            1280.0 / 720.0,
                            0.1,
                            1000.0,
                        ) * glam::Mat4::look_at_rh(
                            glam::vec3(100.0, 0.0, 0.0),
                            glam::vec3(0.0, 0.0, 0.0),
                            glam::vec3(0.0, 1.0, 0.0),
                        ),
                    };

                    rpass.set_push_constants(wgpu::ShaderStage::VERTEX, 0, unsafe {
                        any_as_u8_slice(&push_constants)
                    });

                    current_mesh_vertex_offset =
                        current_mesh_vertex_offset / (std::mem::size_of::<f32>() * 3) as u32;
                    rpass.draw(
                        current_mesh_vertex_offset
                            ..(current_mesh_vertex_offset + current_mesh_vertex_count),
                        0..1,
                    );
                }

                // Upload all resources for the GPU.
                let screen_descriptor = ScreenDescriptor {
                    physical_width: sc_desc.width,
                    physical_height: sc_desc.height,
                    scale_factor: window.scale_factor() as f32,
                };
                egui_rpass.update_texture(&device, &queue, &platform.context().texture());
                egui_rpass.update_user_textures(&device, &queue);
                egui_rpass.update_buffers(&mut device, &mut queue, &paint_jobs, &screen_descriptor);

                // Record all render passes.
                egui_rpass.execute(
                    &mut encoder,
                    &output_frame.output.view,
                    &paint_jobs,
                    &screen_descriptor,
                    None,
                );

                // Submit the commands.
                queue.submit(Some(encoder.finish()));
                *control_flow = ControlFlow::Poll;
            }
            RedrawEventsCleared => {
                window.request_redraw();
            }
            WindowEvent { event, .. } => match event {
                winit::event::WindowEvent::Resized(size) => {
                    sc_desc.width = size.width;
                    sc_desc.height = size.height;
                    swap_chain = device.create_swap_chain(&surface, &sc_desc);
                }
                winit::event::WindowEvent::CloseRequested => {
                    *control_flow = ControlFlow::Exit;
                }
                _ => {}
            },
            _ => (),
        }
    });
}
