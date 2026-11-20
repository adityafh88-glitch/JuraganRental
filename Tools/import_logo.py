# Impor logo (ArtSource/Logo/*.png) sebagai tekstur UI di /Game/UI/Logo.
# Header Play Store dan ikon 512 tidak diimpor (hanya untuk halaman toko).
# Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/import_logo.py" -unattended -nullrhi
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "ArtSource", "Logo")
DEST = "/Game/UI/Logo"
FILES = ["Logo_Judul.png", "KAH_K.png", "KAH_A.png", "KAH_H.png", "KAH_Gamedev.png"]

tasks = []
for name in FILES:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(SRC, name))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for name in FILES:
    path = DEST + "/" + os.path.splitext(name)[0]
    tex = unreal.EditorAssetLibrary.load_asset(path)
    if not tex:
        unreal.log_error("JR_LOGO gagal impor " + name)
        continue
    # Tekstur UI: tanpa mipmap, kompresi UI (tajam, alfa utuh), tidak di-stream.
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log("JR_LOGO ok " + path)
