"""Volcanic-zone ambience: drifting embers over the lava veins + smoke columns
at the obsidian spires. Places persistent NiagaraActors and saves the level."""
import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()

def reg_sys(pkg):
    for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Niagara', 'NiagaraSystem'), True):
        if str(d.package_name) == pkg:
            return d.get_asset()
    return None

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
placed = 0

embers = reg_sys('/Game/Sparks_Embers/Niagara/NS_Embers01')
smoke = reg_sys('/Game/NiagaraExamples/FX_Smoke/NS_Smoke_Plume')

# Volcano zone centered (0, 6000); lava veins spread across the floor
EMBER_SPOTS = [(-900, 5400), (300, 6100), (900, 6700), (-400, 6600)]
SMOKE_SPOTS = [(-1200, 6200), (1100, 5700)]

if embers:
    for x, y in EMBER_SPOTS:
        a = actors.spawn_actor_from_object(embers, unreal.Vector(x, y, 60))
        if a:
            a.set_actor_scale3d(unreal.Vector(3.0, 3.0, 3.0))
            a.set_actor_label('Amb_Embers_%d_%d' % (x, y))
            placed += 1
if smoke:
    for x, y in SMOKE_SPOTS:
        a = actors.spawn_actor_from_object(smoke, unreal.Vector(x, y, 40))
        if a:
            a.set_actor_scale3d(unreal.Vector(2.2, 2.2, 2.2))
            a.set_actor_label('Amb_Smoke_%d_%d' % (x, y))
            placed += 1

unreal.log('DRESS placed=%d (embers=%s smoke=%s)' % (placed, bool(embers), bool(smoke)))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('DRESS COMPLETE')
