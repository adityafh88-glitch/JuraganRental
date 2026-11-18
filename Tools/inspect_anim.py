# Bandingkan posisi tulang di pose referensi skeleton vs frame pertama animasi.
import unreal

def v(t):
    return "(%.3f, %.3f, %.3f)" % (t.x, t.y, t.z)

sk = unreal.load_asset("/Game/Characters/SK_Pemain")
anim = unreal.load_asset("/Game/Characters/Animations/A_Idle")
try:
    mod = unreal.SkeletonModifier()
    mod.set_skeletal_mesh(sk)
    for bone in ["root", "pelvis", "thigh_l"]:
        t = mod.get_bone_transform(bone, False)
        unreal.log("JR_REF %s loc=%s scale=%s" % (bone, v(t.translation), v(t.scale3d)))
except Exception as ex:
    unreal.log("JR_REF error %s" % ex)
for bone in ["root", "pelvis", "thigh_l"]:
    try:
        t = unreal.AnimationLibrary.get_bone_pose_for_frame(anim, bone, 0, False)
        unreal.log("JR_ANIM %s loc=%s scale=%s" % (bone, v(t.translation), v(t.scale3d)))
    except Exception as ex:
        unreal.log("JR_ANIM %s error %s" % (bone, ex))
