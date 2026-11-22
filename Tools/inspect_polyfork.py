# Tulis ukuran (cm) dan batas model Polyfork hasil impor ke Saved/polyfork_bounds.txt,
# untuk memeriksa skala dan titik pusat.
import os

import unreal

lines = []
for path in sorted(unreal.EditorAssetLibrary.list_assets("/Game/Polyfork", recursive=True)):
    a = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box()
        lines.append("%s min=(%.0f,%.0f,%.0f) max=(%.0f,%.0f,%.0f)" % (a.get_name(), b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z))
out = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "polyfork_bounds.txt")
with open(out, "w") as f:
    f.write("\n".join(lines) + "\n")
