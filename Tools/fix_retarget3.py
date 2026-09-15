"""Round 3 retarget fix: pelvis translation OFF (source translation units are
broken ~100x; rotations carry the technique), re-batch, verify pelvis stays
home and strikes still read."""
import unreal

E = unreal.EditorAssetLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()

rtg = unreal.load_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn.RTG_CMU_to_Quinn")
rc = unreal.IKRetargeterController.get_controller(rtg)
if rc.get_num_retarget_ops() == 0:
    rc.add_default_ops()
rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)

pelvis_idx = rc.get_index_of_op_by_name("Pelvis Motion")
oc = rc.get_op_controller(pelvis_idx)
s = oc.get_settings()
s.set_editor_property("translation_alpha", 0.0)
s.set_editor_property("rotation_alpha", 1.0)
oc.set_settings(s)
check = oc.get_settings()
unreal.log("FIX3 pelvis translation_alpha=%.1f rotation_alpha=%.1f ops=%d" % (
    check.get_editor_property("translation_alpha"),
    check.get_editor_property("rotation_alpha"),
    rc.get_num_retarget_ops()))

saved = E.save_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn", only_if_is_dirty=False)
unreal.log("FIX3 retargeter saved=%s" % saved)

# Wipe old output (asset registry list; EditorAssetLibrary listing is flaky)
old = [str(a.package_name) for a in ar.get_assets_by_path("/Game/Anims/CMU_Manny", recursive=False)]
unreal.log("FIX3 deleting %d old clips" % len(old))
for pkg in old:
    E.delete_asset(pkg)

skm_cmu = unreal.load_asset("/Game/Anims/CMU/12_04.12_04")
skm_quinn = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
datas = [d for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
         if str(d.package_name).startswith("/Game/Anims/CMU/")]
unreal.log("FIX3 retargeting %d" % len(datas))
op = unreal.IKRetargetBatchOperation()
result = op.duplicate_and_retarget(datas, skm_cmu, skm_quinn, rtg,
    search="", replace="", prefix="MNY_", suffix="",
    target_path="/Game/Anims/CMU_Manny", use_source_path=False,
    include_referenced_assets=False, overwrite_existing_files=True)
unreal.log("FIX3 batch produced %d" % len(result))
E.save_directory("/Game/Anims/CMU_Manny")

# Verify: pelvis near home at all times; hands still strike (rotation-driven)
OPTS = unreal.AnimPoseEvaluationOptions()
def bp(seq, b, t):
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD).translation

for clip in ["MNY_135_06", "MNY_135_09"]:
    seq = unreal.load_asset("/Game/Anims/CMU_Manny/%s.%s" % (clip, clip))
    if not seq:
        unreal.log("FIX3 %s MISSING" % clip)
        continue
    pmax = 0.0
    hmax = 0.0
    prevh = None
    t = 0.5
    while t < 18.0:
        p = bp(seq, "pelvis", t)
        h = bp(seq, "hand_r", t) - p
        pmax = max(pmax, unreal.Vector(p.x, p.y, 0).length())
        if prevh:
            hmax = max(hmax, (h - prevh).length() / 0.2)
        prevh = h
        t += 0.2
    unreal.log("FIX3 %s pelvis max horiz offset %.0f cm (want <10); hand rel speed max %.0f cm/s (want >300)" % (clip, pmax, hmax))
unreal.log("FIX3 COMPLETE")
