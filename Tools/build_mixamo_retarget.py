"""Build the Mixamo -> Quinn retarget: IK rig with FULL chains (the round-7
lesson), retargeter with default ops, aligned retarget pose, pinned pelvis,
then batch-retarget every imported Mixamo clip.
"""
import unreal

E = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ar = unreal.AssetRegistryHelpers.get_asset_registry()

RIG_DIR = "/Game/Anims/Retarget"
OUT_DIR = "/Game/Anims/Mixamo"

# name, start bone, end bone — complete limbs, spine and neck included
CHAINS = [
    ("Spine", "Spine", "Spine2"),
    ("Neck", "Neck", "Neck"),
    ("Head", "Head", "Head"),
    ("LeftClavicle", "LeftShoulder", "LeftShoulder"),
    ("RightClavicle", "RightShoulder", "RightShoulder"),
    ("LeftArm", "LeftArm", "LeftHand"),
    ("RightArm", "RightArm", "RightHand"),
    ("LeftLeg", "LeftUpLeg", "LeftFoot"),
    ("RightLeg", "RightUpLeg", "RightFoot"),
    ("LeftFoot", "LeftToeBase", "LeftToeBase"),
    ("RightFoot", "RightToeBase", "RightToeBase"),
    ("LeftThumb", "LeftHandThumb1", "LeftHandThumb3"),
    ("LeftIndex", "LeftHandIndex1", "LeftHandIndex3"),
    ("LeftMiddle", "LeftHandMiddle1", "LeftHandMiddle3"),
    ("LeftRing", "LeftHandRing1", "LeftHandRing3"),
    ("LeftPinky", "LeftHandPinky1", "LeftHandPinky3"),
    ("RightThumb", "RightHandThumb1", "RightHandThumb3"),
    ("RightIndex", "RightHandIndex1", "RightHandIndex3"),
    ("RightMiddle", "RightHandMiddle1", "RightHandMiddle3"),
    ("RightRing", "RightHandRing1", "RightHandRing3"),
    ("RightPinky", "RightHandPinky1", "RightHandPinky3"),
]

mesh_mx = unreal.load_asset("/Game/Anims/Mixamo_Src/MixamoBase.MixamoBase")
mesh_q = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
unreal.log("MXR meshes: mixamo=%s quinn=%s" % (bool(mesh_mx), bool(mesh_q)))

rig = unreal.load_asset(RIG_DIR + "/IK_Mixamo.IK_Mixamo")
if not rig:
    rig = TOOLS.create_asset("IK_Mixamo", RIG_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
rc = unreal.IKRigController.get_controller(rig)
rc.set_skeletal_mesh(mesh_mx)
rc.set_retarget_root("Hips")

existing = {str(ch.get_editor_property("chain_name")) for ch in rc.get_retarget_chains()}
for name, sb, eb in CHAINS:
    if name in existing:
        rc.set_retarget_chain_start_bone(name, sb)
        rc.set_retarget_chain_end_bone(name, eb)
    else:
        rc.add_retarget_chain(name, sb, eb, "")
unreal.log("MXR rig chains: %d root=%s" % (len(rc.get_retarget_chains()), rc.get_retarget_root()))
E.save_asset(RIG_DIR + "/IK_Mixamo", only_if_is_dirty=False)

rig_q = unreal.load_asset(RIG_DIR + "/IK_Quinn.IK_Quinn")

rtg = unreal.load_asset(RIG_DIR + "/RTG_Mixamo_to_Quinn.RTG_Mixamo_to_Quinn")
if not rtg:
    rtg = TOOLS.create_asset("RTG_Mixamo_to_Quinn", RIG_DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
rtc = unreal.IKRetargeterController.get_controller(rtg)
rtc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rig)
rtc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig_q)
if rtc.get_num_retarget_ops() == 0:
    rtc.add_default_ops()
rtc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)

qc = unreal.IKRigController.get_controller(rig_q)
mapped = [str(ch.get_editor_property("chain_name")) for ch in qc.get_retarget_chains()
          if str(rtc.get_source_chain(ch.get_editor_property("chain_name"))) not in ("None", "")]
unreal.log("MXR ops=%d mapped=%d: %s" % (rtc.get_num_retarget_ops(), len(mapped), mapped))

# Align the source rest pose to the target (kills bind-pose arm twist)
rtc.auto_align_all_bones(unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)

# Pelvis FULLY pinned — translation and rotation both off, matching the CMU
# retargeter. Any hip motion from the clip drops the mesh relative to the
# capsule and the legs sink into the floor; earthbending (the one element still
# on CMU clips, which have always been pinned) was visibly the only element
# without the bug, which is what identified this setting as the cause.
# Do NOT re-enable vertical translation without foot IK to compensate.
pelvis = rtc.get_op_controller(rtc.get_index_of_op_by_name("Pelvis Motion"))
s = pelvis.get_settings()
s.set_editor_property("translation_alpha", 1.0)
s.set_editor_property("rotation_alpha", 0.0)
s.set_editor_property("scale_horizontal", 0.0)
s.set_editor_property("scale_vertical", 1.0)
s.set_editor_property("floor_constraint_weight", 1.0)
pelvis.set_settings(s)
# THE root-cause fix for the collapsing-legs bug: the Root Motion op writes the
# character's whole height into the ROOT bone (Mixamo clips came out with root
# z=95.7, pelvis z=-5). The character runs with ERootMotionMode::IgnoreRootMotion
# so gameplay movement stays code-driven — which silently discards that 95.7cm
# at runtime, dropping the hips to the floor and folding the legs. CMU clips
# were unaffected because their root is (0,0,0) with the height in the pelvis,
# which is exactly why earthbending was the only element that looked right.
# Disabling this op keeps the root at origin and leaves the height in the pelvis.
rm_idx = rtc.get_index_of_op_by_name("Root Motion")
if rm_idx >= 0:
    rtc.set_retarget_op_enabled(rm_idx, False)
    unreal.log("MXR Root Motion op disabled (height stays in the pelvis)")

# Keep the IK solver on — it was never the problem.
ik_idx = rtc.get_index_of_op_by_name("Run IK Rig")
if ik_idx >= 0:
    rtc.set_retarget_op_enabled(ik_idx, True)

chk = pelvis.get_settings()
unreal.log("MXR pelvis t=%.1f r=%.1f horiz=%.1f vert=%.1f floor=%.1f" % (
    chk.get_editor_property("translation_alpha"), chk.get_editor_property("rotation_alpha"),
    chk.get_editor_property("scale_horizontal"), chk.get_editor_property("scale_vertical"),
    chk.get_editor_property("floor_constraint_weight")))
E.save_asset(RIG_DIR + "/RTG_Mixamo_to_Quinn", only_if_is_dirty=False)

# Batch: every imported Mixamo anim (skip the shorter duplicate takes)
datas = [d for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
         if str(d.package_name).startswith("/Game/Anims/Mixamo_Src/")]
# With-skin FBX yield two takes: "_Take_001" is a static dud (0 motion) and
# "_mixamo_com" carries the real animation. Pack clips have a single take.
picked = [d for d in datas if not str(d.asset_name).endswith("_Take_001")]
unreal.log("MXR retargeting %d clips (dropped %d dud takes)" % (len(picked), len(datas) - len(picked)))

op = unreal.IKRetargetBatchOperation()
result = op.duplicate_and_retarget(picked, mesh_mx, mesh_q, rtg,
    search="", replace="", prefix="MXQ_", suffix="",
    target_path=OUT_DIR, use_source_path=False,
    include_referenced_assets=False, overwrite_existing_files=True)
unreal.log("MXR batch produced %d" % len(result))
E.save_directory(OUT_DIR)
unreal.log("MXR COMPLETE")
