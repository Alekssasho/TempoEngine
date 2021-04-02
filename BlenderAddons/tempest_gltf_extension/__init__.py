import bpy

bl_info = {
    "name": "Tempest glTF Extension",
    "category": "Generic",
    "version": (1, 0, 0),
    "blender": (2, 90, 0),
    'location': 'File > Export > glTF 2.0',
    'description': 'Addon which adds necessary data to run a glTF scene in Tempest Engine.',
    'tracker_url': "https://github.com/Alekssasho/TempoEngine/issues",
    'isDraft': False,
    'developer': "Aleksandar Angelov",
    'url': 'https://alekssasho.github.io/',
}

glTF_extension_name = "TEMPEST_extension"

# Support for an extension is "required" if a typical glTF viewer cannot be expected
# to load a given model without understanding the contents of the extension.
# For example, a compression scheme or new image format (with no fallback included)
# would be "required", but physics metadata or app-specific settings could be optional.
extension_is_required = False

class TempestNodeProperties(bpy.types.PropertyGroup):
    boids_enabled: bpy.props.BoolProperty(
        name="Boids",
        description='Should this object be parts of the boids system',
        default=False
        )

def register_panel():
    # Register the panel on demand, we need to be sure to only register it once
    # This is necessary because the panel is a child of the extensions panel,
    # which may not be registered when we try to register this extension
    try:
        bpy.utils.register_class(TempestNodePropertiesPanel)
    except Exception:
        pass

    # If the glTF exporter is disabled, we need to unregister the extension panel
    # Just return a function to the exporter so it can unregister the panel
    return unregister_panel


def unregister_panel():
    # Since panel is registered on demand, it is possible it is not registered
    try:
        bpy.utils.unregister_class(TempestNodePropertiesPanel)
    except Exception:
        pass

class TempestNodePropertiesPanel(bpy.types.Panel):

    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_label = "Tempest Node Properties"
    bl_context = "object"

    def draw(self, context):
        layout = self.layout
        obj = context.object

        box = layout.box()
        active_systems = box.column(heading="Active Systems")
        active_systems.prop(obj.tempest_props, "boids_enabled", toggle=True)


class glTF2ExportUserExtension:

    def __init__(self):
        # We need to wait until we create the gltf2UserExtension to import the gltf2 modules
        # Otherwise, it may fail because the gltf2 may not be loaded yet
        from io_scene_gltf2.io.com.gltf2_io_extensions import Extension
        self.Extension = Extension

    def gather_node_hook(self, gltf2_object, blender_object, export_settings):
        if gltf2_object.mesh is not None:
            if gltf2_object.extensions is None:
                gltf2_object.extensions = {}
            extension_data = {
                "boids_enabled": blender_object.tempest_props.boids_enabled
            }
            # Handle Physics
            if blender_object.rigid_body is not None and blender_object.rigid_body.collision_shape == 'MESH':
                rigid_body_data = {
                    "dynamic": blender_object.rigid_body.type == 'ACTIVE',
                    "collision_shape" : True
                }
                extension_data["physics_body"] = rigid_body_data

            gltf2_object.extensions[glTF_extension_name] = self.Extension(
                name=glTF_extension_name,
                extension=extension_data,
                required=extension_is_required
            )

def register():
    register_panel()
    bpy.utils.register_class(TempestNodeProperties)
    bpy.types.Object.tempest_props = bpy.props.PointerProperty(type=TempestNodeProperties)

def unregister():
    unregister_panel()
    bpy.utils.unregister_class(TempestNodeProperties)
    del bpy.types.Object.tempest_props

# This allows you to run the script directly from Blender's Text editor
# to test the add-on without having to install it.
if __name__ == "__main__":
    register()