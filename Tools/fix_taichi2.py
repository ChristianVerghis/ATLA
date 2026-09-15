import os
import unreal
PROJECT_ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ar = unreal.AssetRegistryHelpers.get_asset_registry()
def reg_load(pkg_want, cls):
    for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine', cls), True):
        if str(d.package_name) == pkg_want:
            return d.get_asset()
    return None
skel = reg_load('/Game/Anims/CMU/12_04_Skeleton', 'Skeleton')
unreal.log('T3 skel=%s' % bool(skel))
t = unreal.AssetImportTask()
t.filename = os.path.join(PROJECT_ROOT, 'Import', 'CMU', 'converted', 'taichi_full.fbx')
t.destination_path = '/Game/Anims/CMU'
t.automated = True
t.save = True
t.replace_existing = True
opts = unreal.FbxImportUI()
opts.import_animations = True
opts.import_mesh = False
opts.import_materials = False
opts.import_textures = False
opts.skeleton = skel
opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
t.options = opts
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
found = [(str(d.package_name), d) for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine','AnimSequence'), True) if '12_04' in str(d.package_name)]
unreal.log('T3 anims: %s' % [(p.split('/')[-1], round(d.get_asset().get_play_length(),1)) for p, d in found])
for p, d in found:
    if p.startswith('/Game/Anims/CMU/'):
        inputs = unreal.IKRetargetBatchOperationInputs()
        inputs.assets_to_retarget = [d]
        inputs.source_mesh = reg_load('/Game/Anims/CMU/12_04', 'SkeletalMesh')
        inputs.target_mesh = reg_load('/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple', 'SkeletalMesh')
        inputs.ik_retarget_asset = unreal.EditorAssetLibrary.load_asset('/Game/Anims/Retarget/RTG_CMU_to_Quinn')
        inputs.prefix = 'MNY_'
        inputs.target_path = '/Game/Anims/CMU_Manny'
        inputs.include_referenced_assets = False
        inputs.overwrite_existing_files = True
        unreal.IKRetargetBatchOperation().run_batch_retarget(inputs)
        unreal.EditorAssetLibrary.save_directory('/Game/Anims/CMU_Manny')
        unreal.EditorAssetLibrary.save_directory('/Game/Anims/CMU')
out = [(str(d.package_name).split('/')[-1], round(d.get_asset().get_play_length(),1)) for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine','AnimSequence'), True) if 'MNY_12_04' in str(d.package_name)]
unreal.log('T3 retargeted: %s' % out)
unreal.log('T3 COMPLETE')
