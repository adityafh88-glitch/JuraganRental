# Impor semua aset dari ArtSource ke Unreal.
# Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/import_assets.py" -unattended -nullrhi
# Hasil: /Game/Props, /Game/Characters, /Game/Characters/Animations + laporan di Saved/ImportReport.json

import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ART = os.path.join(PROJECT, "ArtSource")
REPORT = os.path.join(PROJECT, "Saved", "ImportReport.json")

# Pakai importer FBX klasik agar opsi skeleton dan animasi di bawah dihormati.
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")

tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"props": {}, "characters": {}, "animations": [], "errors": []}


def run(tasks):
    tools.import_asset_tasks(tasks)
    paths = []
    for t in tasks:
        paths.extend(list(t.get_editor_property("imported_object_paths")))
    return paths


def make_task(path, dest, options):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", path)
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("options", options)
    return t


def units(o):
    """Konversi meter (Blender) ke cm (Unreal) di semua jenis data impor."""
    for key in ("static_mesh_import_data", "skeletal_mesh_import_data", "anim_sequence_import_data"):
        o.get_editor_property(key).set_editor_property("convert_scene_unit", True)
    return o


def static_options():
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
    return o


def skeletal_options(skeleton=None):
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", True)
    o.set_editor_property("import_as_skeletal", True)
    o.set_editor_property("import_animations", False)
    o.set_editor_property("import_materials", True)
    o.set_editor_property("import_textures", False)
    o.set_editor_property("create_physics_asset", False)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    if skeleton:
        o.set_editor_property("skeleton", skeleton)
    return o


def anim_options(skeleton):
    o = unreal.FbxImportUI()
    o.set_editor_property("import_mesh", False)
    o.set_editor_property("import_animations", True)
    o.set_editor_property("import_materials", False)
    o.set_editor_property("skeleton", skeleton)
    o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    return o  # panjang animasi memakai default (waktu yang diekspor dari Blender)


def bounds_cm(asset):
    try:
        b = asset.get_bounds()
        e = b.box_extent
        return [round(e.x * 2, 1), round(e.y * 2, 1), round(e.z * 2, 1)]
    except Exception as ex:  # skeletal mesh
        try:
            b = asset.get_imported_bounds()
            e = b.box_extent
            return [round(e.x * 2, 1), round(e.y * 2, 1), round(e.z * 2, 1)]
        except Exception:
            return str(ex)


# ---------- Props ----------
prop_dir = os.path.join(ART, "Props")
tasks = [make_task(os.path.join(prop_dir, f), "/Game/Props", units(static_options()))
         for f in sorted(os.listdir(prop_dir)) if f.lower().endswith(".fbx")]
for p in run(tasks):
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        report["props"][a.get_name()] = bounds_cm(a)

# ---------- Karakter ----------
char_dir = os.path.join(ART, "Characters")
files = sorted(f for f in os.listdir(char_dir) if f.lower().endswith(".fbx"))
first = "SK_Pemain.fbx"
skeleton = None
for p in run([make_task(os.path.join(char_dir, first), "/Game/Characters", units(skeletal_options()))]):
    a = unreal.load_asset(p)
    if isinstance(a, unreal.SkeletalMesh):
        skeleton = a.get_editor_property("skeleton")
        report["characters"][a.get_name()] = bounds_cm(a)
if not skeleton:
    report["errors"].append("Skeleton tidak terbentuk dari SK_Pemain")
else:
    report["skeleton"] = skeleton.get_path_name()
    rest = [make_task(os.path.join(char_dir, f), "/Game/Characters", units(skeletal_options(skeleton))) for f in files if f != first]
    for p in run(rest):
        a = unreal.load_asset(p)
        if isinstance(a, unreal.SkeletalMesh):
            report["characters"][a.get_name()] = bounds_cm(a)

    # ---------- Animasi ----------
    anim_dir = os.path.join(char_dir, "Animations")
    atasks = [make_task(os.path.join(anim_dir, f), "/Game/Characters/Animations", units(anim_options(skeleton)))
              for f in sorted(os.listdir(anim_dir)) if f.lower().endswith(".fbx")]
    for p in run(atasks):
        a = unreal.load_asset(p)
        if isinstance(a, unreal.AnimSequence):
            report["animations"].append({"name": a.get_name(), "length_s": round(a.get_play_length(), 2)})

os.makedirs(os.path.dirname(REPORT), exist_ok=True)
with open(REPORT, "w", encoding="utf-8") as fh:
    json.dump(report, fh, indent=2)
unreal.log("JR_IMPORT_DONE props=%d characters=%d animations=%d" % (len(report["props"]), len(report["characters"]), len(report["animations"])))
