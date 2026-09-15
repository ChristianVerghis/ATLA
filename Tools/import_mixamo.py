"""Import Mixamo FBX clips: first file (with skin) builds the skeleton/mesh,
the rest import as animation-only against it.

Usage: ue_run.py Tools/import_mixamo.py   (set LIMIT for a smoke test)
"""
import os
import unreal

SRC = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Import", "Mixamo")
DEST_MESH = "/Game/Anims/Mixamo_Src"
DEST_ANIM = "/Game/Anims/Mixamo_Src/Anims"
SKEL_PATH = "/Game/Anims/Mixamo_Src/MixamoBase_Skeleton.MixamoBase_Skeleton"
BASE_FBX = "Mma Kick.fbx"   # imported first, provides skin + skeleton
LIMIT = int(os.environ.get("MIXAMO_LIMIT", "0"))  # 0 = all

tools = unreal.AssetToolsHelpers.get_asset_tools()

# Interchange refuses animation-only FBX ("nothing to import") and ignores the
# legacy FbxImportUI options; the legacy importer handles both cases.
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")


def collect():
    out = []
    for root, _dirs, files in os.walk(SRC):
        for f in sorted(files):
            if f.lower().endswith(".fbx"):
                out.append(os.path.join(root, f))
    return out


def clean_name(path):
    base = os.path.splitext(os.path.basename(path))[0]
    keep = []
    for ch in base:
        keep.append(ch if (ch.isalnum() or ch == "_") else "_")
    name = "".join(keep)
    while "__" in name:
        name = name.replace("__", "_")
    return "MX_" + name.strip("_")


def make_task(path, dest, name, skeleton, as_skeletal_mesh):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = dest
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False

    opts = unreal.FbxImportUI()
    opts.set_editor_property("import_mesh", as_skeletal_mesh)
    opts.set_editor_property("import_as_skeletal", True)
    opts.set_editor_property("import_animations", True)
    opts.set_editor_property("import_materials", False)
    opts.set_editor_property("import_textures", False)
    if skeleton:
        opts.set_editor_property("skeleton", skeleton)
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

    seq_opts = opts.get_editor_property("anim_sequence_import_data")
    seq_opts.set_editor_property("import_translation", unreal.Vector(0, 0, 0))
    seq_opts.set_editor_property("convert_scene", True)
    seq_opts.set_editor_property("remove_redundant_keys", False)
    seq_opts.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)

    sk_opts = opts.get_editor_property("skeletal_mesh_import_data")
    sk_opts.set_editor_property("convert_scene", True)
    sk_opts.set_editor_property("import_morph_targets", False)

    opts.set_editor_property("create_physics_asset", False)
    task.options = opts
    return task


files = collect()
base = [f for f in files if os.path.basename(f) == BASE_FBX]
rest = [f for f in files if os.path.basename(f) != BASE_FBX]
if LIMIT:
    rest = rest[:LIMIT]
unreal.log("MXI found %d fbx (importing base + %d)" % (len(files), len(rest)))

skeleton = unreal.load_asset(SKEL_PATH)
if not skeleton and base:
    t = make_task(base[0], DEST_MESH, "MixamoBase", None, True)
    tools.import_asset_tasks([t])
    for p in t.get_editor_property("imported_object_paths") or []:
        unreal.log("MXI base imported: %s" % p)
    skeleton = unreal.load_asset(SKEL_PATH)
unreal.log("MXI skeleton: %s" % (skeleton.get_name() if skeleton else "MISSING"))

if not skeleton:
    unreal.log("MXI ABORT: no skeleton")
else:
    ok, fail = 0, []
    tasks = []
    for p in rest:
        tasks.append(make_task(p, DEST_ANIM, clean_name(p), skeleton, False))
    # import in chunks so a bad file doesn't sink everything
    CHUNK = 10
    for i in range(0, len(tasks), CHUNK):
        batch = tasks[i:i + CHUNK]
        try:
            tools.import_asset_tasks(batch)
        except Exception as ex:
            unreal.log("MXI chunk %d error: %s" % (i, ex))
        for t in batch:
            paths = t.get_editor_property("imported_object_paths") or []
            if paths:
                ok += 1
            else:
                fail.append(os.path.basename(t.filename))
        unreal.log("MXI progress %d/%d ok=%d" % (min(i + CHUNK, len(tasks)), len(tasks), ok))
    unreal.EditorAssetLibrary.save_directory(DEST_MESH)
    unreal.log("MXI DONE imported=%d failed=%d %s" % (ok, len(fail), fail[:8]))
unreal.log("MXI COMPLETE")
