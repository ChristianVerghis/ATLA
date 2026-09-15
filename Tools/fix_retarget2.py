"""Fix the CMU->Quinn retargeter: UE5.8 retargeters need an op stack —
factory-created ones have ZERO ops so nothing ever retargeted (all output
was the target A-pose). Add default ops, map chains, re-batch, verify."""
import unreal

E = unreal.EditorAssetLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()

rtg = unreal.load_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn.RTG_CMU_to_Quinn")
rc = unreal.IKRetargeterController.get_controller(rtg)

if rc.get_num_retarget_ops() == 0:
    rc.add_default_ops()
unreal.log("FIX2 ops now: %d" % rc.get_num_retarget_ops())

rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)

rig_q = unreal.load_asset("/Game/Anims/Retarget/IK_Quinn.IK_Quinn")
qc = unreal.IKRigController.get_controller(rig_q)
mapped = []
for ch in qc.get_retarget_chains():
    name = ch.get_editor_property("chain_name")
    src = rc.get_source_chain(name)
    if src is not None and str(src) not in ("None", ""):
        mapped.append(str(name))
unreal.log("FIX2 mapped %d chains: %s" % (len(mapped), ", ".join(mapped)))

E.save_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn")

# Delete the static output (asset registry — list_assets is flaky)
old = [str(a.package_name) for a in ar.get_assets_by_path("/Game/Anims/CMU_Manny", recursive=False)]
unreal.log("FIX2 deleting %d static clips" % len(old))
for pkg in old:
    E.delete_asset(pkg)

# Re-run the batch
skm_cmu = unreal.load_asset("/Game/Anims/CMU/12_04.12_04")
skm_quinn = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
anims = [d.get_asset() for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
         if str(d.package_name).startswith("/Game/Anims/CMU/")]
unreal.log("FIX2 retargeting %d source anims" % len(anims))
op = unreal.IKRetargetBatchOperation()
try:
    result = op.duplicate_and_retarget(anims, skm_cmu, skm_quinn, rtg, search="", replace="", prefix="MNY_", suffix="", remap_referenced_assets=False)
    unreal.log("FIX2 batch result: %s" % result)
except Exception as ex:
    unreal.log("FIX2 batch ERROR: %s" % ex)
E.save_directory("/Game/Anims/CMU_Manny")

# Verify motion on two clips
OPTS = unreal.AnimPoseEvaluationOptions()
for clip in ["MNY_144_20", "MNY_135_07"]:
    seq = unreal.load_asset("/Game/Anims/CMU_Manny/%s.%s" % (clip, clip))
    if not seq:
        unreal.log("FIX2 verify %s: MISSING" % clip)
        continue
    prev, mx = None, 0.0
    t = 0.0
    while t < 10.0:
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
        p = unreal.AnimPoseExtensions.get_bone_pose(pose, "hand_r", unreal.AnimPoseSpaces.WORLD).translation
        if prev:
            mx = max(mx, (p - prev).length() / 0.2)
        prev = p
        t += 0.2
    unreal.log("FIX2 verify %s hand_r max %.0f cm/s -> %s" % (clip, mx, "ANIMATES" if mx > 100 else "STILL STATIC"))
unreal.log("FIX2 COMPLETE")
