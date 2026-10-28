"""Snapshot existing, explicitly requested polish targets before Unreal authoring."""
import pathlib,json,hashlib,shutil,datetime
root=pathlib.Path(__file__).resolve().parents[2]
targets=[
 'Content/IsekaiClass/Life/Home/Blueprints/BP_IC_PlayerStorage.uasset',
 'Content/IsekaiClass/NPC/Blueprints/Containers/BP_IC_NPCChest.uasset',
 'Content/IsekaiClass/NPC/Blueprints/Containers/BP_IC_NPCBookshelf.uasset',
 'Content/IsekaiClass/NPC/Blueprints/Containers/BP_IC_NPCWardrobe.uasset',
 'Content/IsekaiClass/World/L_TestMap.umap',
 'Content/IsekaiClass/Input/Contexts/IMC_Gameplay.uasset',
 'Data/StringTables/ST_IC_Definitions.csv',
 'Content/IsekaiClass/Data/DataTable/StringTables/ST_IC_Definitions.uasset']
for name in ['QuickSlot','QuickSlots','Settings']:targets.append(f'Content/IsekaiClass/UI/Widget/WBP_IC_{name}.uasset')
for gender,code in [('Male','M'),('Female','F')]:targets.append(f'Content/IsekaiClass/Animation/Player/Main{gender}/DA_ANIM_PLAYER_{code}.uasset')
for relative in ['Life/DT_Ranching','Life/DT_IC_SleepPresentation','Life/DT_IC_FurnitureLoot','Registry/DT_IC_IDRegistry','Rewards/DT_IC_DropTable','Items/DT_IC_Item']:
    targets.extend(['Content/IsekaiClass/Data/DataTable/'+relative+'.uasset','Data/'+relative+'.csv'])
targets.extend(str(p.relative_to(root)).replace('\\','/') for p in (root/'Content/IsekaiClass/Animation/Common/Retarget').glob('IK_SRC_PlayerLife_*.uasset'))
out=root/'Saved/Polish/BeforeAssets';out.mkdir(parents=True,exist_ok=True);manifest=[]
for relative in dict.fromkeys(targets):
    source=root/relative;dest=out/relative
    if source.exists():
        if not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
        manifest.append(dict(path=relative,bytes=source.stat().st_size,sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
    else:manifest.append(dict(path=relative,new=True))
(out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps({'targets':len(manifest),'backed_up':sum('bytes' in r for r in manifest),'bytes':sum(r.get('bytes',0) for r in manifest),'manifest':str(out/'manifest.json')}))
