# Daftar aktor di L_Garasi (Saved/level_actors.txt). Dengan argumen "hapus": buang landscape gurun
# (selalu disembunyikan GameMode; hanya menambah ukuran APK dan beban HP), lalu simpan level.
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/clean_level.py hapus" -unattended -nullrhi
import os
import sys

import unreal

MAP = "/Game/Maps/L_Garasi"
remove = "hapus" in sys.argv
unreal.EditorLoadingAndSavingUtils.load_map(MAP)
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lines, removed = [], 0
for a in sub.get_all_level_actors():
    cls = a.get_class().get_name()
    lines.append("%s | %s" % (cls, a.get_actor_label()))
    if remove and ("Landscape" in cls):
        sub.destroy_actor(a)
        removed += 1
if remove:
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
lines.append("dihapus: %d" % removed)
out = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "level_actors.txt")
with open(out, "w") as f:
    f.write("\n".join(lines) + "\n")
