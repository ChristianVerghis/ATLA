"""Build auto-generated IK rigs for the CMU skeleton and Quinn, create a
retargeter between them, and batch-duplicate all CMU anims onto the game
skeleton at /Game/Anims/CMU_Manny."""
import unreal

E = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

skm_cmu = E.load_asset("/Game/Anims/CMU/12_04")
skm_quinn = E.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
unreal.log("RTG src=%s dst=%s" % (bool(skm_cmu), bool(skm_quinn)))


def make_rig(name, mesh):
    path = "/Game/Anims/Retarget/" + name
    rig = E.load_asset(path)
    if not rig:
        rig = TOOLS.create_asset(name, "/Game/Anims/Retarget", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    ok = c.apply_auto_generated_retarget_definition()
    chains = [str(n) for n in c.get_retarget_chains()] if hasattr(c, "get_retarget_chains") else "?"
    unreal.log("RTG rig %s auto=%s root=%s chains=%s" % (name, ok, c.get_retarget_root(), chains))
    return rig


rig_cmu = make_rig("IK_CMU", skm_cmu)
rig_quinn = make_rig("IK_Quinn", skm_quinn)

rtg = E.load_asset("/Game/Anims/Retarget/RTG_CMU_to_Quinn")
if not rtg:
    rtg = TOOLS.create_asset("RTG_CMU_to_Quinn", "/Game/Anims/Retarget", unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
unreal.log("RTG rc methods: %s" % [m for m in dir(rc) if any(k in m.lower() for k in ("rig", "map", "source", "target"))])

rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rig_cmu)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig_quinn)
try:
    rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    unreal.log("RTG chains auto-mapped")
except Exception as ex:
    unreal.log("RTG auto_map issue: %s" % ex)

E.save_directory("/Game/Anims/Retarget")

# Batch retarget every CMU anim
ar = unreal.AssetRegistryHelpers.get_asset_registry()
anims = [d.get_asset() for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
         if str(d.package_name).startswith("/Game/Anims/CMU/")]
unreal.log("RTG anims to retarget: %d" % len(anims))

op = unreal.IKRetargetBatchOperation()
unreal.log("RTG op methods: %s" % [m for m in dir(op) if "retarget" in m.lower() or "duplicate" in m.lower()])
try:
    result = op.duplicate_and_retarget(anims, skm_cmu, skm_quinn, rtg, search="", replace="", prefix="MNY_", suffix="", remap_referenced_assets=False)
    unreal.log("RTG batch result: %s" % result)
except Exception as ex:
    unreal.log("RTG batch signature issue: %s" % ex)

out = [str(d.package_name) for d in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
       if "MNY_" in str(d.package_name)]
unreal.log("RTG retargeted output: %d assets (e.g. %s)" % (len(out), out[:3]))
unreal.log("RTG COMPLETE")
