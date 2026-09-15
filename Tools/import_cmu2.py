"""CMU import, take 2: drive Interchange directly with an animation-only
pipeline. First file with no skeleton (Interchange creates one from the FBX
joint hierarchy), rest bound to that skeleton."""
import os
import unreal

PROJECT_ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT_ROOT, "Import", "CMU")
DEST = "/Game/Anims/CMU"

files = sorted(f for f in os.listdir(SRC) if f.endswith(".fbx"))
tools = unreal.AssetToolsHelpers.get_asset_tools()


def make_pipeline(skeleton):
    p = unreal.InterchangeGenericAssetsPipeline()
    common = p.get_editor_property("common_skeletal_meshes_and_animations_properties")
    common.set_editor_property("import_only_animations", True)
    if skeleton:
        common.set_editor_property("skeleton", skeleton)
    anim = p.get_editor_property("animation_pipeline")
    anim.set_editor_property("import_animations", True)
    anim.set_editor_property("import_bone_tracks", True)
    mats = p.get_editor_property("material_pipeline")
    mats.set_editor_property("import_materials", False)
    return p


def make_task(filename, skeleton):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SRC, filename)
    t.destination_path = DEST
    t.automated = True
    t.save = True
    t.replace_existing = True
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(make_pipeline(skeleton))
    t.options = override
    return t


unreal.log("CMU2 first file: %s" % files[0])
tools.import_asset_tasks([make_task(files[0], None)])

skel = None
for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    if isinstance(unreal.EditorAssetLibrary.load_asset(path), unreal.Skeleton):
        skel = unreal.EditorAssetLibrary.load_asset(path)
        break
unreal.log("CMU2 skeleton after first import: %s" % (skel.get_path_name() if skel else "NONE"))

if skel:
    tools.import_asset_tasks([make_task(f, skel) for f in files[1:]])
    anims = [p for p in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True)
             if isinstance(unreal.EditorAssetLibrary.load_asset(p), unreal.AnimSequence)]
    unreal.log("CMU2 anim sequences: %d / %d" % (len(anims), len(files)))
    unreal.EditorAssetLibrary.save_directory(DEST)
unreal.log("CMU2 COMPLETE")
