import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
DIR = "/Game/Bending/VFX"


def get_or_create(name):
    path = DIR + "/" + name
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    at = unreal.AssetToolsHelpers.get_asset_tools()
    return at.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())


def const3(mat, rgb, x, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, x, y)
    e.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return e


def const1(mat, v, x, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", v)
    return e


def build_emissive(name, base, emissive, opacity=None, roughness=0.4):
    """Simple hot emissive material; translucent when opacity is given."""
    mat = get_or_create(name)
    MEL.delete_all_material_expressions(mat)
    if opacity is not None:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property("two_sided", True)
    b = const3(mat, base, -400, -200)
    MEL.connect_material_property(b, "", unreal.MaterialProperty.MP_BASE_COLOR)
    e = const3(mat, emissive, -400, 0)
    MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    r = const1(mat, roughness, -400, 150)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if opacity is not None:
        o = const1(mat, opacity, -400, 250)
        MEL.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_asset(DIR + "/" + name)
    unreal.log("MAT built %s" % name)


def build_water():
    """Real-water read: deep blue, glassy, fresnel rim that brightens at grazing angles."""
    mat = get_or_create("M_Water")
    MEL.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)

    base = const3(mat, (0.01, 0.09, 0.32), -600, -250)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = const1(mat, 0.05, -600, -100)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = const1(mat, 1.0, -600, -20)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    fres = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -600, 100)
    fres.set_editor_property("exponent", 2.2)
    fres.set_editor_property("base_reflect_fraction", 0.08)

    # Opacity: 0.5 core -> 0.95 rim, so edges catch the eye like a real surface
    olerp = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -350, 100)
    olerp.set_editor_property("const_a", 0.5)
    olerp.set_editor_property("const_b", 0.95)
    MEL.connect_material_expressions(fres, "", olerp, "Alpha")
    MEL.connect_material_property(olerp, "", unreal.MaterialProperty.MP_OPACITY)

    # Cool cyan rim glow driven by the same fresnel
    rim = const3(mat, (0.12, 0.5, 1.1), -350, 260)
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 220)
    MEL.connect_material_expressions(rim, "", mul, "A")
    MEL.connect_material_expressions(fres, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    MEL.recompile_material(mat)
    EAL.save_asset(DIR + "/M_Water")
    unreal.log("MAT built M_Water (fresnel)")


# Fire: searing white-yellow core, hot orange glow, bright embers
build_emissive("M_FireCore", (1.0, 0.85, 0.4), (30.0, 22.0, 5.0), opacity=0.95, roughness=0.2)
build_emissive("M_Fire", (1.0, 0.35, 0.02), (10.0, 2.4, 0.12), opacity=0.62, roughness=0.3)
build_emissive("M_Ember", (1.0, 0.5, 0.08), (15.0, 4.5, 0.35), opacity=0.9, roughness=0.3)

# Air: clearly visible pale band for active techniques; ambient M_Air a touch stronger
build_emissive("M_AirBand", (0.9, 0.95, 1.0), (1.3, 1.5, 1.9), opacity=0.5, roughness=0.2)
build_emissive("M_Air", (0.92, 0.96, 1.0), (0.35, 0.4, 0.48), opacity=0.3, roughness=0.2)

build_water()
unreal.log("MATS COMPLETE")
