"""Rebuild the UI portrait material using explicit, verified material pin names."""
import unreal as u
tools=u.AssetToolsHelpers.get_asset_tools();path='/Game/IsekaiClass/UI/Materials/M_UI_DownedPortrait'
mat=u.load_asset(path)
if not mat:mat=tools.create_asset('M_UI_DownedPortrait',path.rsplit('/',1)[0],u.Material,u.MaterialFactoryNew())
mat.set_editor_property('material_domain',u.MaterialDomain.MD_UI);mat.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
u.MaterialEditingLibrary.delete_all_material_expressions(mat)
tex=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionTextureSampleParameter2D,-550,0);tex.set_editor_property('parameter_name','Portrait');tex.set_editor_property('texture',u.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
weights=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionConstant3Vector,-550,200);weights.set_editor_property('constant',u.LinearColor(.2126,.7152,.0722,1))
gray=u.MaterialEditingLibrary.create_material_expression(mat,u.MaterialExpressionDotProduct,-200,0)
pins=u.MaterialEditingLibrary.get_material_expression_input_names(gray);assert len(pins)==2,pins
assert u.MaterialEditingLibrary.connect_material_expressions(tex,'RGB',gray,pins[0])
assert u.MaterialEditingLibrary.connect_material_expressions(weights,'',gray,pins[1])
assert u.MaterialEditingLibrary.connect_material_property(gray,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
assert u.MaterialEditingLibrary.connect_material_property(tex,'A',u.MaterialProperty.MP_OPACITY)
u.MaterialEditingLibrary.recompile_material(mat);assert u.EditorAssetLibrary.save_loaded_asset(mat,False);u.log('PARTY_PORTRAIT_MATERIAL_OK '+str(pins))
