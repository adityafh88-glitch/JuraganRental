# Cetak pusat dan ukuran bounding box beberapa aset untuk mengecek arah depan.
import unreal

for path in ["/Game/Props/SM_TV_Tabung", "/Game/Props/SM_Kursi_Plastik", "/Game/Props/SM_Monitor_CRT", "/Game/Characters/SK_Pemain"]:
    a = unreal.load_asset(path)
    try:
        b = a.get_bounds()
    except Exception:
        b = a.get_imported_bounds()
    o, e = b.origin, b.box_extent
    unreal.log("JR_BOUNDS %s origin=(%.1f, %.1f, %.1f) extent=(%.1f, %.1f, %.1f)" % (path, o.x, o.y, o.z, e.x, e.y, e.z))
sk = unreal.load_asset("/Game/Characters/SK_Pemain")
for m in sk.get_editor_property("materials"):
    pass
sm = unreal.load_asset("/Game/Props/SM_TV_Tabung")
unreal.log("JR_SLOTS TV " + ", ".join(str(s.get_editor_property("material_slot_name")) for s in sm.get_editor_property("static_materials")))
sm = unreal.load_asset("/Game/Props/SM_Monitor_CRT")
unreal.log("JR_SLOTS CRT " + ", ".join(str(s.get_editor_property("material_slot_name")) for s in sm.get_editor_property("static_materials")))
sm = unreal.load_asset("/Game/Props/SM_Kotak_MCB")
unreal.log("JR_SLOTS MCB " + ", ".join(str(s.get_editor_property("material_slot_name")) for s in sm.get_editor_property("static_materials")))
