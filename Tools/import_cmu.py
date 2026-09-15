"""Import the staged CMU FBX clips: first file creates the shared CMU skeleton,
the rest import animation-only against it. Dumps the bone tree for rig work."""
import os
import unreal

PROJECT_ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(PROJECT_ROOT, "Import", "CMU", "converted")
DEST = "/Game/Anims/CMU"

files = sorted(f for f in os.listdir(SRC) if f.endswith(".fbx"))
unreal.log("CMUIMP %d files" % len(files))

tools = unreal.AssetToolsHelpers.get_asset_tools()


def make_task(filename, skeleton):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SRC, filename)
    t.destination_path = DEST
    t.automated = True
    t.save = True
    t.replace_existing = True
    opts = unreal.FbxImportUI()
    opts.import_animations = True
    opts.import_materials = False
    opts.import_textures = False
    if skeleton is None:
        # First file: bring in whatever mesh/bones it has and mint the skeleton
        opts.import_mesh = True
        opts.import_as_skeletal = True
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    else:
        opts.import_mesh = False
        opts.skeleton = skeleton
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    t.options = opts
    return t


# 1) first file alone -> skeleton
tools.import_asset_tasks([make_task(files[0], None)])

skel = None
skm = None
for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if skel is None and isinstance(asset, unreal.Skeleton):
        skel = asset
    if skm is None and isinstance(asset, unreal.SkeletalMesh):
        skm = asset

if not skel:
    unreal.log_error("CMUIMP no skeleton created by first import!")
else:
    unreal.log("CMUIMP skeleton: %s  mesh: %s" % (skel.get_path_name(), skm.get_path_name() if skm else "NONE"))

    # 2) the rest as animation-only
    tools.import_asset_tasks([make_task(f, skel) for f in files[1:]])

    anims = [p for p in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True)
             if isinstance(unreal.EditorAssetLibrary.load_asset(p), unreal.AnimSequence)]
    unreal.log("CMUIMP anim sequences: %d" % len(anims))

    # 3) dump the bone tree (SkeletonModifier is the 5.4+ python route)
    try:
        mod = unreal.SkeletonModifier()
        if skm and mod.init(skm):
            names = [str(n) for n in mod.get_all_bone_names()]
            unreal.log("CMUIMP bones (%d): %s" % (len(names), ", ".join(names)))
        else:
            unreal.log("CMUIMP SkeletonModifier init failed")
    except Exception as e:
        unreal.log("CMUIMP SkeletonModifier unavailable (%s); probing alternatives" % e)
        unreal.log("CMUIMP skel attrs: %s" % [m for m in dir(skel) if "bone" in m.lower()])
        if skm:
            unreal.log("CMUIMP mesh attrs: %s" % [m for m in dir(skm) if "bone" in m.lower() or "skelet" in m.lower()])

    unreal.EditorAssetLibrary.save_directory(DEST)
    unreal.log("CMUIMP COMPLETE")
