# Impor efek suara (ArtSource/Audio/*.wav), suara karakter (Audio/Voice) dan lagu radio (Audio/Music) ke Unreal.
# Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/import_audio.py" -unattended -nullrhi
# File LOOP_* otomatis diset berulang. File suara karakter yang belum ada cukup dilewati.

import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT, "ArtSource", "Audio")
tools = unreal.AssetToolsHelpers.get_asset_tools()


def import_folder(folder, dest):
    if not os.path.isdir(folder):
        return []
    tasks = []
    for f in sorted(os.listdir(folder)):
        if not f.lower().endswith(".wav"):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(folder, f))
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("automated", True)
        t.set_editor_property("save", True)
        t.set_editor_property("replace_existing", True)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    names = []
    for t in tasks:
        for p in t.get_editor_property("imported_object_paths"):
            a = unreal.load_asset(p)
            if isinstance(a, unreal.SoundWave):
                if a.get_name().startswith("LOOP_"):
                    a.set_editor_property("looping", True)
                    unreal.EditorAssetLibrary.save_loaded_asset(a)
                names.append(a.get_name())
    return names


sfx = import_folder(SRC, "/Game/Audio")
vo = import_folder(os.path.join(SRC, "Voice"), "/Game/Audio/Voice")
mus = import_folder(os.path.join(SRC, "Music"), "/Game/Audio/Music")
unreal.log("JR_AUDIO_DONE sfx=%d voice=%d music=%d %s" % (len(sfx), len(vo), len(mus), ",".join(sfx)))
