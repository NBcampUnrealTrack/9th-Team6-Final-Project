import unreal as u,json,pathlib
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
table=u.load_asset('/Game/IsekaiClass/Data/DataTable/Life/DT_IC_SleepPresentation')
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(table))
row=next(r for r in rows if r['Name']=='Default');row.update(EnterSeconds=.8,FadeOutSeconds=.25,BlackHoldSeconds=.08,FadeInSeconds=.25,CameraOffset='(X=420,Y=450,Z=300)',CameraFocusOffset='(X=0,Y=0,Z=70)',FieldOfView=70)
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows));assert u.EditorAssetLibrary.save_loaded_asset(table,False)
(root/'Data/Life/DT_IC_SleepPresentation.csv').write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(table),encoding='utf-8-sig')
report={}
for gender,code in [('Male','M'),('Female','F')]:
    p=u.load_asset('/Game/IsekaiClass/Animation/Player/Main'+gender+'/DA_ANIM_PLAYER_'+code)
    p.set_editor_property('SleepMeshTransform',u.Transform(location=u.Vector(-55,-36,-31 if code=='M' else -25),rotation=u.Rotator(yaw=0)))
    assert u.EditorAssetLibrary.save_loaded_asset(p,False)
    report[gender]={k:dict(path=p.get_editor_property(k).get_path_name(),length=p.get_editor_property(k).get_play_length()) for k in ['SleepEnter','Sleep','SleepExit']}
(root/'Saved/Polish/SleepClips.json').write_text(json.dumps(report,indent=2),encoding='utf-8');u.log('SLEEP_CAMERA_POLISH_OK')
