"""Refresh existing UI art and editable frontend music, without generating art."""
from pathlib import Path
import json
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/KeyboardPrologueReview';out.mkdir(parents=True,exist_ok=True)
base='/Game/IsekaiClass'
def save(asset):
    if isinstance(asset,u.Blueprint):
        u.BlueprintEditorLibrary.compile_blueprint(asset)
    assert u.EditorAssetLibrary.save_loaded_asset(asset,False),asset.get_path_name()

music=u.load_asset(base+'/Data/DataTable/Audio/DT_IC_BGM');assert music
rows=json.loads(u.DataTableFunctionLibrary.export_data_table_to_json_string(music))
for key,label,sound in [('BGM_MainMenu','메인 메뉴','Ocean_Waves'),('BGM_CharacterCreate','캐릭터 생성','Merchant'),('BGM_Prologue','프롤로그','Inner_Dreams')]:
    if not any(row['Name']==key for row in rows):
        path=base+'/Audio/SampleAudio/OrchestralLoops/'+sound+'.'+sound
        assert u.load_asset(path),path
        rows.append(dict(Name=key,DisplayName=label,Music=path,Volume=.6,FadeInSeconds=1.2,FadeOutSeconds=1.2,bLoop=True))
assert u.DataTableFunctionLibrary.fill_data_table_from_json_string(music,json.dumps(rows,ensure_ascii=False))
save(music)
(root/'Data/Audio/DT_IC_BGM.csv').write_text(u.DataTableFunctionLibrary.export_data_table_to_csv_string(music),encoding='utf-8-sig')

# The menu rework owns styling and explicitly excludes gameplay HUD widgets.
import runpy
runpy.run_path(str(root/'Tools/UI/setup_morpheus_ui.py'))
u.log('IC_KEYBOARD_PROLOGUE_SETUP_OK')
