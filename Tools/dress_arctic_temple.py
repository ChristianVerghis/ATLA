"""Arctic: ground-mist puffs at the pool edges. Sky temple: cloud wisps below
the platform edges. Places persistent NiagaraActors and saves the level."""
import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()

def reg_sys(pkg):
    for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Niagara', 'NiagaraSystem'), True):
        if str(d.package_name) == pkg:
            return d.get_asset()
    return None

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
puff = reg_sys('/Game/NiagaraExamples/Utilities/SpriteGeneration/SmokePuffLight/NS_SmokePuffLight')
chimney = reg_sys('/Game/NiagaraExamples/FX_Smoke/NS_Chimney_Smoke')
placed = 0

# Arctic (0,0): wide flat mist puffs drifting at the pool edges
ARCTIC = [(-700, -500, 20), (600, 400, 20), (-200, 900, 20)]
# Temple (6000,6000): cloud wisps hugging the platform undersides
TEMPLE = [(5400, 5600, 180), (6600, 6300, 320), (6100, 6800, 520)]

if puff:
    for x, y, z in ARCTIC:
        a = actors.spawn_actor_from_object(puff, unreal.Vector(x, y, z))
        if a:
            a.set_actor_scale3d(unreal.Vector(4.0, 4.0, 1.2))
            a.set_actor_label('Amb_Mist_%d_%d' % (x, y))
            placed += 1
    for x, y, z in TEMPLE:
        a = actors.spawn_actor_from_object(puff, unreal.Vector(x, y, z))
        if a:
            a.set_actor_scale3d(unreal.Vector(5.0, 5.0, 1.5))
            a.set_actor_label('Amb_Cloud_%d_%d' % (x, y))
            placed += 1

unreal.log('DRESS2 placed=%d (puff=%s chimney=%s)' % (placed, bool(puff), bool(chimney)))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('DRESS2 COMPLETE')
