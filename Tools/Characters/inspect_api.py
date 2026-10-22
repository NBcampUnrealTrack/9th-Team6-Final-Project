"""Read-only discovery for the installed Unreal animation/editor API."""
import json
from pathlib import Path
import unreal as u

out={"project":u.Paths.project_dir(),"engine":u.SystemLibrary.get_engine_version()}
for name in ["IKRigController","IKRetargeterController","IKRetargetBatchOperation","IKRetargetBatchOperationInputs","BlueprintFactory","SkeletalMeshEditorSubsystem","DataTableFunctionLibrary","EditorAssetLibrary"]:
    cls=getattr(u,name,None)
    if not cls:
        out[name]="UNAVAILABLE"
        continue
    out[name]={method:getattr(cls,method).__doc__ for method in dir(cls) if any(term in method for term in ["retarget","skeletal_mesh","chain","auto_align","batch","import_data_table","create_blueprint","get_bone","get_controller","add_default","set_ik_rig"])}
out["enums"]={name:dir(getattr(u,name)) for name in ["RetargetSourceOrTarget","RetargetAutoAlignMethod","AutoMapChainType"] if hasattr(u,name)}
Path(u.Paths.project_saved_dir(),"CharacterAPI.json").write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding="utf-8")
u.log("IC_CHARACTER_API_READY")
