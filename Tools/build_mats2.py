import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary


def get_or_create(name, folder="/Game/Bending/VFX"):
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path), path
    at = unreal.AssetToolsHelpers.get_asset_tools()
    return at.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew()), path


def const3(mat, rgb, x, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, x, y)
    e.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return e


def const1(mat, v, x, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", v)
    return e


# Lightning: blinding blue-white
mat, path = get_or_create("M_Lightning")
MEL.delete_all_material_expressions(mat)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("two_sided", True)
b = const3(mat, (0.85, 0.9, 1.0), -400, -200)
MEL.connect_material_property(b, "", unreal.MaterialProperty.MP_BASE_COLOR)
e = const3(mat, (16.0, 20.0, 40.0), -400, 0)
MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
o = const1(mat, 0.95, -400, 200)
MEL.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat)
EAL.save_asset(path)
unreal.log("MAT built M_Lightning")

# Sky temple recolor: white -> warm sandstone so pale air VFX and the gray
# player read against the architecture
mat, path = get_or_create("M_AirStone")
MEL.delete_all_material_expressions(mat)
b = const3(mat, (0.42, 0.3, 0.19), -400, -200)
MEL.connect_material_property(b, "", unreal.MaterialProperty.MP_BASE_COLOR)
r = const1(mat, 0.75, -400, 0)
MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(mat)
EAL.save_asset(path)
unreal.log("MAT recolored M_AirStone to sandstone")
unreal.log("MATS2 COMPLETE")
