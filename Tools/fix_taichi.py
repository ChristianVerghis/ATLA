import os
import unreal
PROJECT_ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ar = unreal.AssetRegistryHelpers.get_asset_registry()
def reg_load(pkg_want, cls):
    for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine', cls), True):
        if str(d.package_name) == pkg_want:
            return d.get_asset()
    return None
t = unreal.AssetImportTask()
t.filename = os.path.join(PROJECT_ROOT, 'Import', 'CMU', 'converted', '12_04.fbx')
t.destination_path = '/Game/Anims/CMU'
t.automated = True
t.save = True
t.replace_existing = True
opts = unreal.FbxImportUI()
opts.import_animations = True
opts.import_materials = False
opts.import_textures = False
opts.import_mesh = True
opts.import_as_skeletal = True
opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
t.options = opts
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
src = None
for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine','AnimSequence'), True):
    if str(d.package_name).startswith('/Game/Anims/CMU/12_04'):
        src = d
        unreal.log('SRC5 %s = %.1f' % (str(d.package_name), d.get_asset().get_play_length()))
if src:
    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.assets_to_retarget = [src]
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
    for d in ar.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine','AnimSequence'), True):
        if 'MNY_12_04' in str(d.package_name):
            unreal.log('OUT5 %s = %.1f' % (str(d.package_name), d.get_asset().get_play_length()))
unreal.log('FIX5 COMPLETE')
