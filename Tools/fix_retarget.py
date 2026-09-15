"""Fix the broken CMU->Quinn retarget: map chains (they share exact names),
delete the static MNY_* output, and re-run the batch retarget."""
import unreal

E = unreal.EditorAssetLibrary

rtg = unreal.load_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn.RTG_CMU_to_Quinn")
rc = unreal.IKRetargeterController.get_controller(rtg)

# Map chains — names match exactly between the two auto rigs
rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
rig_q = unreal.load_asset("/Game/Anims/Retarget/IK_Quinn.IK_Quinn")
qc = unreal.IKRigController.get_controller(rig_q)
mapped = []
for ch in qc.get_retarget_chains():
    name = ch.get_editor_property("chain_name")
    src = rc.get_source_chain(name)
    if src is not None and str(src) not in ("None", ""):
        mapped.append("%s<-%s" % (name, src))
unreal.log("FIX mapped %d chains: %s" % (len(mapped), ", ".join(mapped)))

if len(mapped) < 6:
    # try fuzzy as fallback
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    mapped = [str(ch.get_editor_property("chain_name")) for ch in qc.get_retarget_chains()
              if str(rc.get_source_chain(ch.get_editor_property("chain_name"))) not in ("None", "")]
    unreal.log("FIX fuzzy fallback mapped %d" % len(mapped))

E.save_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn")

# Wipe the static output
old = E.list_assets("/Game/Anims/CMU_Manny", recursive=False)
unreal.log("FIX deleting %d static clips" % len(old))
for a in old:
    E.delete_asset(a.split(".")[0])

# Re-run the batch
skm_cmu = unreal.load_asset("/Game/Anims/CMU/12_04.12_04")
skm_quinn = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
ar = unreal.AssetRegistryHelpers.get_asset_registry()
anims = [d.get_asset() for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
         if str(d.package_name).startswith("/Game/Anims/CMU/")]
unreal.log("FIX retargeting %d source anims" % len(anims))
op = unreal.IKRetargetBatchOperation()
result = op.duplicate_and_retarget(anims, skm_cmu, skm_quinn, rtg, search="", replace="", prefix="MNY_", suffix="", remap_referenced_assets=False)
unreal.log("FIX batch result: %s" % result)
E.save_directory("/Game/Anims/CMU_Manny")

# Verify motion on the jab clip
OPTS = unreal.AnimPoseEvaluationOptions()
seq = unreal.load_asset("/Game/Anims/CMU_Manny/MNY_144_20.MNY_144_20")
if seq:
    prev = None
    mx = 0.0
    t = 0.0
    while t < 10.0:
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
        p = unreal.AnimPoseExtensions.get_bone_pose(pose, "hand_r", unreal.AnimPoseSpaces.WORLD).translation
        if prev:
            mx = max(mx, (p - prev).length() / 0.2)
        prev = p
        t += 0.2
    unreal.log("FIX verify MNY_144_20 hand_r max speed %.0f cm/s -> %s" % (mx, "ANIMATES" if mx > 100 else "STILL STATIC"))
else:
    unreal.log("FIX verify: MNY_144_20 missing after batch!")
unreal.log("FIX COMPLETE")
