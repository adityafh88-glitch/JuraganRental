# Impor hanya props baru (FBX di ArtSource/Props yang belum ada di /Game/Props), tanpa mengimpor ulang semuanya.
# Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/import_new_props.py" -unattended -nullrhi
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
PROPS = os.path.join(PROJECT, "ArtSource", "Props")
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
tools = unreal.AssetToolsHelpers.get_asset_tools()


def options():
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", True)
    o.set_editor_property("import_as_skeletal", False)
    o.set_editor_property("import_animations", False)
    o.set_editor_property("import_materials", True)
    o.set_editor_property("import_textures", False)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    sm = o.get_editor_property("static_mesh_import_data")
    sm.set_editor_property("combine_meshes", True)
    sm.set_editor_property("auto_generate_collision", True)
    sm.set_editor_property("convert_scene_unit", True)
    return o


tasks = []
for f in sorted(os.listdir(PROPS)):
    name, ext = os.path.splitext(f)
    if ext.lower() != ".fbx" or unreal.EditorAssetLibrary.does_asset_exist("/Game/Props/" + name):
        continue
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(PROPS, f))
    t.set_editor_property("destination_path", "/Game/Props")
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("options", options())
    tasks.append(t)
tools.import_asset_tasks(tasks)
unreal.EditorAssetLibrary.save_directory("/Game/Props")
done = []
for t in tasks:
    done.extend(str(p) for p in t.get_editor_property("imported_object_paths"))
for p in done:
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box()
        unreal.log("JR_PROP %s size_cm=%s" % (a.get_name(), b.max - b.min))
unreal.log("JR_NEW_PROPS_DONE %d" % len(done))
