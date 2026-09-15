import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
sys_asset = None
for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Niagara', 'NiagaraSystem'), True):
    if str(d.package_name) == '/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Small':
        sys_asset = d.get_asset()
unreal.log('PROBE asset=%s' % bool(sys_asset))
if sys_asset:
    actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_object(sys_asset, unreal.Vector(0, 0, 300))
    unreal.log('PROBE spawned=%s' % bool(actor))
    if actor:
        actor.destroy_actor()
unreal.log('PROBE COMPLETE')
