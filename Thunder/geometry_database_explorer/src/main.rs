use std::time::Instant;

use egui::FontDefinitions;
use egui_wgpu_backend::{RenderPass, ScreenDescriptor};
use egui_winit_platform::{Platform, PlatformDescriptor};
use futures_lite::future::block_on;
use mesh_shader::ShaderConstants;
use wgpu::util::DeviceExt;
use winit::event::Event::*;
use winit::event::WindowEvent::*;
use winit::event_loop::ControlFlow;

use std::collections::hash_map::HashMap;
use std::io::Read;
use structopt::StructOpt;

const INITIAL_WIDTH: u32 = 1280;
const INITIAL_HEIGHT: u32 = 720;
const OUTPUT_FORMAT: wgpu::TextureFormat = wgpu::TextureFormat::Bgra8UnormSrgb;

const MESH_RADIUS_MULTIPLIER: f32 = 1.2;

#[derive(StructOpt)]
struct CommandLineOptions {
    /// Input file to read geometry database
    #[structopt(short, long, parse(from_os_str))]
    input_database: std::path::PathBuf,
}

unsafe fn any_as_u8_slice<T: Sized>(p: &T) -> &[u8] {
    ::std::slice::from_raw_parts((p as *const T) as *const u8, ::std::mem::size_of::<T>())
}

struct OrbitCamera {
    pub azimuth: f32,
    pub polar: f32,
    pub radius: f32,
}

impl OrbitCamera {
    fn create_view_matrix(&self) -> glam::Mat4 {
        let eye = glam::vec3(
            self.radius * self.azimuth.cos() * self.polar.sin(),
            self.radius * self.azimuth.sin() * self.polar.sin(),
            self.radius * self.polar.cos(),
        );
        glam::Mat4::look_at_rh(eye, glam::vec3(0.0, 0.0, 0.0), glam::vec3(0.0, 1.0, 0.0))
    }
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

    let meshlet_buffer = database.meshlet_buffer().unwrap();
    let mesh_dimensions = {
        if let Some(mappings) = database.mappings() {
            let vertex_buffer = database.vertex_buffer().unwrap();
            let vertex_buffer_vec3 = unsafe {
                ::std::slice::from_raw_parts(
                    vertex_buffer.as_ptr() as *const glam::Vec3,
                    vertex_buffer.len() / std::mem::size_of::<glam::Vec3>(),
                )
            };
            let mut map = HashMap::new();
            for (index, mesh) in mappings.iter().enumerate() {
                let biggest_dimensions = meshlet_buffer[mesh.meshlets_offset() as usize
                    ..(mesh.meshlets_offset() + mesh.meshlets_count()) as usize]
                    .iter()
                    .map(|meshlet| {
                        let first_vertex = meshlet.vertex_offset();
                        let mesh_slice = &vertex_buffer_vec3[(first_vertex as usize)
                            ..((first_vertex + meshlet.vertex_count()) as usize)];
                        let min_sizes = mesh_slice
                            .iter()
                            .fold(glam::Vec3::default(), |init, vertex| init.min(*vertex));
                        let max_sizes = mesh_slice
                            .iter()
                            .fold(glam::Vec3::default(), |init, vertex| init.max(*vertex));
                        (min_sizes, max_sizes)
                    })
                    .reduce(|a, b| (a.0.min(b.0), a.1.max(b.1)))
                    .unwrap();
                let biggest_dimension = (biggest_dimensions.1 - biggest_dimensions.0).max_element();
                map.insert(index, biggest_dimension);
            }
            map
        } else {
            HashMap::new()
        }
    };

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
    let index_buffer_data: Vec<u16> = database
        .meshlet_indices_buffer()
        .unwrap()
        .iter()
        .map(|index| *index as u16)
        .collect();

    let index_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: Some("Index buffer"),
        contents: unsafe { index_buffer_data.as_slice().align_to::<u8>().1 },
        usage: wgpu::BufferUsage::INDEX,
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
        array_stride: (std::mem::size_of::<f32>() * 6) as u64,
        step_mode: wgpu::InputStepMode::Vertex,
        attributes: &[
            wgpu::VertexAttribute {
                format: wgpu::VertexFormat::Float3,
                offset: 0,
                shader_location: 0,
            },
            wgpu::VertexAttribute {
                format: wgpu::VertexFormat::Float3,
                offset: (std::mem::size_of::<f32>() * 3) as u64,
                shader_location: 1,
            },
        ],
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
        depth_stencil: Some(wgpu::DepthStencilState {
            format: wgpu::TextureFormat::Depth32Float,
            depth_write_enabled: true,
            depth_compare: wgpu::CompareFunction::Less,
            stencil: wgpu::StencilState::default(),
            bias: wgpu::DepthBiasState::default(),
            clamp_depth: false,
        }),
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

    let mut depth_texture = device.create_texture(&wgpu::TextureDescriptor {
        label: Some("Depth Texture"),
        size: wgpu::Extent3d {
            width: sc_desc.width,
            height: sc_desc.height,
            depth: 1,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Depth32Float,
        usage: wgpu::TextureUsage::RENDER_ATTACHMENT,
    });

    let mut depth_texture_view = depth_texture.create_view(&wgpu::TextureViewDescriptor::default());

    let mut camera = OrbitCamera {
        azimuth: 0.0,
        polar: 0.0,
        radius: mesh_dimensions.get(&0).unwrap() * MESH_RADIUS_MULTIPLIER,
    };

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
    let mut should_move_camera = false;
    let mut last_pointer_pos = glam::vec2(0.0, 0.0);

    let mut whole_mesh = true;
    let mut current_mesh_meshlet_index: u32 = 0;

    event_loop.run(move |event, _, control_flow| {
        let _ = (&depth_texture, &depth_texture_view);
        // Pass the winit events to the platform integration.
        platform.handle_event(&event);

        match event {
            WindowEvent {
                window_id: _window_id,
                event,
            } => match event {
                MouseInput { state, button, .. } => {
                    if let winit::event::MouseButton::Other(..) = button {
                    } else {
                        if let winit::event::MouseButton::Left = button {
                            should_move_camera = state == winit::event::ElementState::Pressed;
                        }
                    }
                }
                MouseWheel { delta, .. } => {
                    match delta {
                        winit::event::MouseScrollDelta::LineDelta(_, y) => {
                            camera.radius += y;
                        }
                        winit::event::MouseScrollDelta::PixelDelta(_) => {
                            //
                        }
                    }
                }
                CursorMoved { position, .. } => {
                    let current_pos = glam::vec2(position.x as f32, position.y as f32);
                    if should_move_camera {
                        let delta = current_pos - last_pointer_pos;
                        camera.polar += (delta.x / 16.0).to_radians();
                        camera.azimuth += (delta.y / 16.0).to_radians();
                    }
                    last_pointer_pos = current_pos;
                }
                Resized(size) => {
                    sc_desc.width = size.width;
                    sc_desc.height = size.height;
                    swap_chain = device.create_swap_chain(&surface, &sc_desc);
                    depth_texture = device.create_texture(&wgpu::TextureDescriptor {
                        label: Some("Depth Texture"),
                        size: wgpu::Extent3d {
                            width: sc_desc.width,
                            height: sc_desc.height,
                            depth: 1,
                        },
                        mip_level_count: 1,
                        sample_count: 1,
                        dimension: wgpu::TextureDimension::D2,
                        format: wgpu::TextureFormat::Depth32Float,
                        usage: wgpu::TextureUsage::RENDER_ATTACHMENT,
                    });

                    depth_texture_view =
                        depth_texture.create_view(&wgpu::TextureViewDescriptor::default());
                }
                CloseRequested => {
                    *control_flow = ControlFlow::Exit;
                }
                _ => {}
            },
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
                let mut current_mesh_meshlet_count = 0;
                let mut current_mesh_meshlet_offset = 0;
                egui::Window::new("Geometry Explorer").show(&platform.context(), |ui| {
                    ui.label(format!("Filename: {}", opt.input_database.display()));
                    if let Some(mappings) = database.mappings() {
                        ui.label(format!("Number of meshes: {}", mappings.len()));
                        ui.separator();

                        // Camera settings
                        ui.label("Camera Settings:");
                        ui.horizontal(|ui| {
                            ui.label("Azimuth: ");
                            ui.drag_angle(&mut camera.azimuth);
                        });
                        ui.horizontal(|ui| {
                            ui.label("Polar: ");
                            ui.drag_angle(&mut camera.polar);
                        });
                        ui.horizontal(|ui| {
                            ui.label("Radius: ");
                            ui.add(egui::DragValue::f32(&mut camera.radius).speed(1.0));
                        });

                        ui.separator();

                        ui.label(format!("Current mesh: {}", current_mesh_index));
                        let mesh = mappings.iter().nth(current_mesh_index).unwrap();
                        ui.label(format! {"Mehslet count: {}", mesh.meshlets_count()});
                        ui.label(format! {"Mehslet offset: {}", mesh.meshlets_offset()});
                        ui.label(format! {"Mesh index: {}", mesh.index()});
                        ui.checkbox(&mut whole_mesh, "Whole mesh");

                        current_mesh_meshlet_count = mesh.meshlets_count();
                        current_mesh_meshlet_offset = mesh.meshlets_offset();

                        ui.horizontal(|ui| {
                            if ui.button("Previous").clicked() {
                                current_mesh_index = if current_mesh_index == 0 {
                                    mappings.len() - 1
                                } else {
                                    current_mesh_index - 1
                                };
                                camera.radius = mesh_dimensions.get(&current_mesh_index).unwrap()
                                    * MESH_RADIUS_MULTIPLIER;
                                current_mesh_meshlet_index = 0;
                            }
                            if ui.button("Next").clicked() {
                                current_mesh_index = if current_mesh_index == (mappings.len() - 1) {
                                    0
                                } else {
                                    current_mesh_index + 1
                                };
                                camera.radius = mesh_dimensions.get(&current_mesh_index).unwrap()
                                    * MESH_RADIUS_MULTIPLIER;
                                current_mesh_meshlet_index = 0;
                            }
                        });

                        if !whole_mesh {
                            ui.separator();
                            let meshlet = &meshlet_buffer[current_mesh_meshlet_index as usize];
                            ui.label(format! {"Current meshlet: {}", current_mesh_meshlet_index});
                            ui.label(format! {"Triangle count: {}", meshlet.triangle_count()});
                            ui.label(format! {"Triangle offset: {}", meshlet.triangle_offset()});
                            ui.label(format! {"Vertex count: {}", meshlet.vertex_count()});
                            ui.label(format! {"Vertex offset: {}", meshlet.vertex_offset()});
                            ui.horizontal(|ui| {
                                if ui.button("Previous").clicked() {
                                    current_mesh_meshlet_index = if current_mesh_meshlet_index == 0 {
                                        mappings.get(current_mesh_index).meshlets_count() - 1
                                    } else {
                                        current_mesh_meshlet_index - 1
                                    };
                                }
                                if ui.button("Next").clicked() {
                                    current_mesh_meshlet_index = if current_mesh_meshlet_index == (mappings.get(current_mesh_index).meshlets_count() - 1) {
                                        0
                                    } else {
                                        current_mesh_meshlet_index + 1
                                    };
                                }
                            });
                        }
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
                        depth_stencil_attachment: Some(
                            wgpu::RenderPassDepthStencilAttachmentDescriptor {
                                attachment: &depth_texture_view,
                                depth_ops: Some(wgpu::Operations {
                                    load: wgpu::LoadOp::Clear(1.0),
                                    store: true,
                                }),
                                stencil_ops: None,
                            },
                        ),
                    });

                    rpass.set_pipeline(&render_pipeline);
                    rpass.set_vertex_buffer(0, vertex_buffer.slice(..));
                    rpass.set_index_buffer(index_buffer.slice(..), wgpu::IndexFormat::Uint16);

                    let push_constants = ShaderConstants {
                        view_projection_matrix: glam::Mat4::perspective_rh(
                            60f32.to_radians(),
                            sc_desc.width as f32 / sc_desc.height as f32,
                            0.1,
                            1000.0,
                        ) * camera.create_view_matrix(),
                    };

                    rpass.set_push_constants(wgpu::ShaderStage::VERTEX, 0, unsafe {
                        any_as_u8_slice(&push_constants)
                    });

                    if whole_mesh {
                        for meshlet in &meshlet_buffer[current_mesh_meshlet_offset as usize
                            ..(current_mesh_meshlet_offset + current_mesh_meshlet_count) as usize]
                        {
                            rpass.draw_indexed(
                                meshlet.triangle_offset()..meshlet.triangle_offset() + meshlet.triangle_count() * 3,
                                meshlet.vertex_offset() as i32,
                                0..1,
                            )
                        }
                    } else {
                        let mappings = database.mappings().unwrap();
                        let current_meshlet = meshlet_buffer[(current_mesh_meshlet_index + mappings.get(current_mesh_index).meshlets_offset()) as usize];
                        rpass.draw_indexed(current_meshlet.triangle_offset()..current_meshlet.triangle_offset() + current_meshlet.triangle_count() * 3,
                        current_meshlet.vertex_offset() as i32,
                        0..1,)
                    }
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
            _ => (),
        }
    });
}
