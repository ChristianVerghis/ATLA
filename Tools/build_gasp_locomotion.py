"""Retarget the GASP (Game Animation Sample) locomotion loops onto our
mannequin so they can drive the existing locomotion blend space.

GASP ships on SK_UEFN_Mannequin; the game runs on SK_Mannequin, so the clips
need converting. Root Motion op stays OFF — this project runs
ERootMotionMode::IgnoreRootMotion, and anything the retargeter writes into the
root bone is discarded at runtime (see Design/anim-windows.md).
"""
import unreal

E = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ar = unreal.AssetRegistryHelpers.get_asset_registry()

RIG_DIR = "/Game/Anims/Retarget"
OUT_DIR = "/Game/Anims/GASP"
SRC_DIR = "/Game/Characters/UEFN_Mannequin/Animations"

# 8-way loops at both gaits + the idle: exactly what the blend space needs
WANT = ["M_Neutral_Stand_Idle_Loop"]
for gait in ("Walk", "Run"):
    for d in ("F", "FL", "FR", "LL", "LR", "B", "BL", "BR"):
        WANT.append("M_Neutral_%s_Loop_%s" % (gait, d))

rig_src = unreal.load_asset("/Game/Characters/UEFN_Mannequin/Rigs/IK_UEFN_Mannequin.IK_UEFN_Mannequin")
rig_dst = unreal.load_asset(RIG_DIR + "/IK_Quinn.IK_Quinn")
mesh_dst = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
src_ctl = unreal.IKRigController.get_controller(rig_src)
mesh_src = src_ctl.get_skeletal_mesh()
unreal.log("GSP rigs: src=%s dst=%s srcMesh=%s" % (bool(rig_src), bool(rig_dst), mesh_src.get_name() if mesh_src else None))

rtg = unreal.load_asset(RIG_DIR + "/RTG_GASP_to_Quinn.RTG_GASP_to_Quinn")
if not rtg:
    rtg = TOOLS.create_asset("RTG_GASP_to_Quinn", RIG_DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
rtc = unreal.IKRetargeterController.get_controller(rtg)
rtc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rig_src)
rtc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig_dst)
if rtc.get_num_retarget_ops() == 0:
    rtc.add_default_ops()
rtc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)

qc = unreal.IKRigController.get_controller(rig_dst)
mapped = [str(ch.get_editor_property("chain_name")) for ch in qc.get_retarget_chains()
          if str(rtc.get_source_chain(ch.get_editor_property("chain_name"))) not in ("None", "")]
unreal.log("GSP mapped %d chains: %s" % (len(mapped), mapped))

rtc.auto_align_all_bones(unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)

# Height must live in the pelvis, never the root (IgnoreRootMotion discards it)
rm = rtc.get_index_of_op_by_name("Root Motion")
if rm >= 0:
    rtc.set_retarget_op_enabled(rm, False)

# Locomotion NEEDS vertical pelvis motion (the bob of a stride); horizontal
# stays off so clips can't drag the character off the capsule.
pelvis = rtc.get_op_controller(rtc.get_index_of_op_by_name("Pelvis Motion"))
s = pelvis.get_settings()
s.set_editor_property("translation_alpha", 1.0)
s.set_editor_property("rotation_alpha", 0.0)
s.set_editor_property("scale_horizontal", 0.0)
s.set_editor_property("scale_vertical", 1.0)
pelvis.set_settings(s)
E.save_asset(RIG_DIR + "/RTG_GASP_to_Quinn", only_if_is_dirty=False)

by_name = {}
for a in ar.get_assets_by_path(SRC_DIR, recursive=True):
    if str(a.asset_class_path.asset_name) == "AnimSequence":
        by_name[str(a.asset_name)] = a
picked = [by_name[n] for n in WANT if n in by_name]
unreal.log("GSP retargeting %d/%d clips" % (len(picked), len(WANT)))

op = unreal.IKRetargetBatchOperation()
res = op.duplicate_and_retarget(picked, mesh_src, mesh_dst, rtg,
    search="", replace="", prefix="GQ_", suffix="",
    target_path=OUT_DIR, use_source_path=False,
    include_referenced_assets=False, overwrite_existing_files=True)
unreal.log("GSP produced %d" % len(res))
E.save_directory(OUT_DIR)

# Health check: root must be at origin, pelvis must carry the height
OPTS = unreal.AnimPoseEvaluationOptions()
bad = []
for n in WANT:
    q = "GQ_" + n
    seq = unreal.load_asset("%s/%s.%s" % (OUT_DIR, q, q))
    if not seq:
        bad.append(q + ":missing")
        continue
    p = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, 0.2, OPTS)
    r = unreal.AnimPoseExtensions.get_bone_pose(p, "root", unreal.AnimPoseSpaces.LOCAL).translation
    pel = unreal.AnimPoseExtensions.get_bone_pose(p, "pelvis", unreal.AnimPoseSpaces.LOCAL).translation
    if r.size() > 5.0 or pel.z < 70.0:
        bad.append("%s(root=%.0f pelvisZ=%.0f)" % (q, r.size(), pel.z))
unreal.log("GSP health: %d clips, problems=%s" % (len(WANT), bad if bad else "NONE"))
unreal.log("GSP COMPLETE")
