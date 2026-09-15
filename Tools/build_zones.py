import unreal

mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()


def make_mat(name, color, rough, emissive=None, translucent=False, opacity=None):
    path = "/Game/Bending/VFX/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mat = at.create_asset(name, "/Game/Bending/VFX", unreal.Material, unreal.MaterialFactoryNew())
    if translucent:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -200)
    c.set_editor_property("constant", unreal.LinearColor(*color))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 0)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive:
        e = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 150)
        e.set_editor_property("constant", unreal.LinearColor(*emissive))
        mel.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity is not None:
        o = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
        o.set_editor_property("r", opacity)
        mel.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    return unreal.EditorAssetLibrary.load_asset(path)


fire = make_mat("M_Fire", (1.0, 0.35, 0.02, 1.0), 0.4, emissive=(4.0, 1.2, 0.1, 1.0), translucent=True, opacity=0.85)
air = make_mat("M_Air", (0.92, 0.96, 1.0, 1.0), 0.2, emissive=(0.25, 0.28, 0.32, 1.0), translucent=True, opacity=0.22)
basalt = make_mat("M_Basalt", (0.05, 0.045, 0.05, 1.0), 0.85)
lava = make_mat("M_Lava", (0.6, 0.12, 0.01, 1.0), 0.6, emissive=(6.0, 1.4, 0.05, 1.0))
airstone = make_mat("M_AirStone", (0.86, 0.87, 0.92, 1.0), 0.6)
quarry = make_mat("M_Quarry", (0.34, 0.25, 0.17, 1.0), 0.9)
earth_m = unreal.EditorAssetLibrary.load_asset("/Game/Bending/VFX/M_Earth")
unreal.log("zone materials done")

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
cyl = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder")

existing = set(a.get_actor_label() for a in eas.get_all_level_actors())


def prop(label, mesh, mat, loc, scale, rot=(0, 0, 0)):
    if label in existing:
        return
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(float(loc[0]), float(loc[1]), float(loc[2])), unreal.Rotator(roll=float(rot[0]), pitch=float(rot[1]), yaw=float(rot[2])))
    a.set_actor_label(label)
    c = a.get_components_by_class(unreal.StaticMeshComponent)[0]
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    c.set_static_mesh(mesh)
    c.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(float(scale[0]), float(scale[1]), float(scale[2])))


# EARTH QUARRY @ (6000, 0)
prop("QuarryFloor", cube, quarry, (6000, 0, -52), (46, 46, 1))
boulders = [(5400, -900, 60, 2.2), (6600, -700, 90, 3.1), (6900, 400, 50, 1.8), (5300, 800, 110, 3.6),
            (6200, 1200, 70, 2.6), (5800, -1400, 80, 2.9), (6800, -1400, 55, 2.0), (5100, 100, 65, 2.4)]
for i, (x, y, z, s) in enumerate(boulders):
    prop("QuarryBoulder%d" % i, cube, earth_m, (x, y, z), (s, s * 0.85, s * 0.8), rot=(i * 13, i * 7, i * 31))
for i, (x, y) in enumerate([(5600, 500), (6500, -200), (6300, 900)]):
    prop("QuarryColumn%d" % i, cyl, quarry, (x, y, 150), (1.1, 1.1, 3.2))

# FIRE VOLCANIC FIELD @ (0, 6000)
prop("VolcanoFloor", cube, basalt, (0, 6000, -52), (46, 46, 1))
for i, (x, y, yaw, ln) in enumerate([(-800, 5400, 20, 14), (500, 6300, 70, 10), (-300, 6800, 120, 12),
                                     (900, 5700, 160, 8), (-1200, 6300, 45, 9)]):
    prop("LavaVein%d" % i, cube, lava, (x, y, -46), (ln, 0.6, 0.12), rot=(0, 0, yaw))
spires = [(-1500, 5600, 200, 4.8), (1300, 6600, 260, 6.0), (700, 5200, 170, 4.0), (-900, 6900, 230, 5.2)]
for i, (x, y, z, h) in enumerate(spires):
    prop("ObsidianSpire%d" % i, cube, basalt, (x, y, z), (1.4, 1.4, h), rot=(i * 5, i * 3, i * 47))

# AIR TEMPLE @ (6000, 6000)
prop("TempleFloor", cube, airstone, (6000, 6000, -52), (46, 46, 1))
import math
for i in range(6):
    ang = math.radians(i * 60)
    prop("TemplePillar%d" % i, cyl, airstone, (6000 + 1500 * math.cos(ang), 6000 + 1500 * math.sin(ang), 300), (1.3, 1.3, 7.0))
plats = [(5600, 5600, 260), (6400, 5800, 420), (6100, 6500, 580), (5500, 6300, 740), (6700, 6600, 900)]
for i, (x, y, z) in enumerate(plats):
    prop("SkyPlatform%d" % i, cube, airstone, (x, y, z), (3.4, 3.4, 0.3))

# Per-zone sparring dummies
bp_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
for label, loc in [("QuarryDummy", (6400, 200, 120)), ("VolcanoDummy", (400, 6200, 120)), ("TempleDummy", (6300, 6200, 120))]:
    if label not in existing:
        d = eas.spawn_actor_from_class(bp_class, unreal.Vector(float(loc[0]), float(loc[1]), float(loc[2])), unreal.Rotator(roll=0.0, pitch=0.0, yaw=180.0))
        d.set_actor_label(label)

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log("ZONES BUILT + SAVED")
