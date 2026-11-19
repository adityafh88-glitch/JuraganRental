# Buat material warna polos /Game/Materials/M_JR_Color (parameter "Color") yang boleh dipakai di skeletal mesh.
# Dipakai untuk mengacak warna baju/celana/rambut pelanggan saat runtime.
# Jalankan dengan Unreal Editor tertutup:
#   UnrealEditor-Cmd.exe JuraganRental.uproject -run=pythonscript -script="Tools/make_color_material.py" -unattended -nullrhi
import unreal

PATH = "/Game/Materials"
NAME = "M_JR_Color"
lib = unreal.MaterialEditingLibrary
full = "%s/%s" % (PATH, NAME)
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())

color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(0.8, 0.8, 0.8, 1.0))
lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
rough.set_editor_property("r", 0.75)
lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mat.set_editor_property("used_with_skeletal_mesh", True)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full)
unreal.log("JR_COLORMAT_DONE %s skel=%s" % (full, mat.get_editor_property("used_with_skeletal_mesh")))
