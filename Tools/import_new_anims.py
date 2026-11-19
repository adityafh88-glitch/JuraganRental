# Impor animasi baru dari ArtSource/Characters/Animations yang belum ada di Unreal,
# tanpa mengimpor ulang semua aset. Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/import_new_anims.py" -unattended -nullrhi

import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "ArtSource", "Characters", "Animations")
DEST = "/Game/Characters/Animations"
lib = unreal.EditorAssetLibrary
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
skeleton = unreal.load_asset("/Game/Characters/SK_Pemain_Skeleton")
tools = unreal.AssetToolsHelpers.get_asset_tools()

done = []
for f in sorted(os.listdir(SRC)):
    name, ext = os.path.splitext(f)
    if ext.lower() != ".fbx" or lib.does_asset_exist(DEST + "/" + name):
        continue
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", False)
    o.set_editor_property("import_animations", True)
    o.set_editor_property("import_materials", False)
    o.set_editor_property("skeleton", skeleton)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(SRC, f))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("options", o)
    tools.import_asset_tasks([t])
    # FBX animasi berisi mesh: buang mesh/physics sementara, rapikan nama A_X_Anim -> A_X
    base = DEST + "/" + name
    for extra in (base, base + "_PhysicsAsset"):
        if lib.does_asset_exist(extra) and not isinstance(unreal.load_asset(extra), unreal.AnimSequence):
            lib.delete_asset(extra)
    if lib.does_asset_exist(base + "_Anim"):
        lib.rename_asset(base + "_Anim", base)
    a = unreal.load_asset(base)
    if isinstance(a, unreal.AnimSequence):
        lib.save_loaded_asset(a)
        done.append("%s %.2fs" % (name, a.get_play_length()))
unreal.log("JR_NEW_ANIMS %s" % ", ".join(done))
