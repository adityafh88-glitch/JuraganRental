# Impor model Polyfork (GLB, gratis untuk komersial tanpa atribusi) dari ArtSource/Polyfork ke /Game/Polyfork.
# Nama aset: SM_PF_<id tanpa akhiran acak>, mis. wall-clock-0c555c.glb -> SM_PF_Wall_Clock.
# Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/import_polyfork.py" -unattended -nullrhi
import os
import re

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "ArtSource", "Polyfork")
DEST = "/Game/Polyfork"


def asset_name(file_name):
    base = os.path.splitext(file_name)[0]
    base = re.sub(r"-[0-9a-f]{6}$", "", base)  # buang akhiran id acak
    return "SM_PF_" + "_".join(p.capitalize() for p in base.split("-"))


def pipelines():
    # Pipeline sementara (di memori): semua bagian model digabung jadi satu Static Mesh.
    p = unreal.InterchangeGenericAssetsPipeline()
    mesh = p.get_editor_property("mesh_pipeline")
    mesh.set_editor_property("combine_static_meshes_behavior", unreal.InterchangeCombineStaticMeshesBehavior.ALL)
    mesh.set_editor_property("import_skeletal_meshes", False)
    p.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
    KEEP.append(p)
    return [unreal.SoftObjectPath(p.get_path_name())]


KEEP = []
PIPES = pipelines()

done = 0
for f in sorted(os.listdir(SRC)):
    if not f.lower().endswith(".glb"):
        continue
    name = asset_name(f)
    folder = DEST + "/" + name[6:]
    if unreal.EditorAssetLibrary.does_asset_exist(DEST + "/" + name):
        continue
    src = unreal.InterchangeManager.create_source_data(os.path.join(SRC, f))
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.set_editor_property("override_pipelines", PIPES)
    assets = unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(folder, src, params)
    # Mesh gabungan dipindah ke /Game/Polyfork/SM_PF_<nama>; material tetap di subfolder.
    for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True):
        a = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(a, unreal.StaticMesh):
            unreal.EditorAssetLibrary.rename_asset(path.split(".")[0], DEST + "/" + name)
            break
    unreal.log("JR_POLYFORK " + name)
    done += 1

unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
unreal.log("JR_POLYFORK selesai %d model" % done)
