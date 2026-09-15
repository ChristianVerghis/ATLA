"""Headless Blender batch: CMU FBX (bones+anim, no mesh) -> UE-importable FBX
(adds a tiny cube skinned to the root bone so UE's importer mints a skeleton).
Run: Blender --background --python blender_cmu_convert.py -- <src_dir> <dst_dir>
"""
import os
import sys

import bpy

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, DST = argv[0], argv[1]
os.makedirs(DST, exist_ok=True)

files = sorted(f for f in os.listdir(SRC) if f.endswith(".fbx"))
done = 0
for f in files:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    try:
        bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, f))
    except Exception as e:
        print("CONVERT-FAIL import", f, e)
        continue
    arm = next((o for o in bpy.data.objects if o.type == "ARMATURE"), None)
    if not arm:
        print("CONVERT-FAIL no armature", f)
        continue

    # Bake the ACTION's true range — the factory scene range (1..250) would
    # truncate every clip to 8.3s (and pad short ones)
    act = arm.animation_data.action if arm.animation_data else None
    if act:
        start, end = act.frame_range
        bpy.context.scene.frame_start = int(start)
        bpy.context.scene.frame_end = max(int(end), int(start) + 1)
        print("CONVERT-RANGE", f, int(start), int(end))

    bpy.ops.mesh.primitive_cube_add(size=0.05)
    cube = bpy.context.active_object
    mod = cube.modifiers.new("Armature", "ARMATURE")
    mod.object = arm
    root = arm.data.bones[0].name
    vg = cube.vertex_groups.new(name=root)
    vg.add(list(range(len(cube.data.vertices))), 1.0, "REPLACE")
    cube.parent = arm

    try:
        bpy.ops.export_scene.fbx(
            filepath=os.path.join(DST, f),
            add_leaf_bones=False,
            bake_anim=True,
            bake_anim_use_all_actions=False,
            bake_anim_use_nla_strips=False,
            object_types={"ARMATURE", "MESH"},
        )
        done += 1
    except Exception as e:
        print("CONVERT-FAIL export", f, e)

print("CONVERT-DONE", done, "/", len(files))
