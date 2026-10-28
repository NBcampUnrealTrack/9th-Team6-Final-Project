import unreal as u,json,pathlib
out={}
for gender,code in [('Male','M'),('Female','F')]:
    p=u.load_asset('/Game/IsekaiClass/Animation/Player/Main'+gender+'/DA_ANIM_PLAYER_'+code);seq=p.get_editor_property('Sleep')
    pose=u.AnimPoseExtensions.get_anim_pose_at_time(seq,.1,u.AnimPoseEvaluationOptions())
    def bone(name):
        t=u.AnimPoseExtensions.get_bone_pose(pose,name,u.AnimPoseSpaces.WORLD);v=t.translation;return [v.x,v.y,v.z]
    out[gender]={'head':bone('J_Bip_C_Head'),'left_foot':bone('J_Bip_L_Foot'),'right_foot':bone('J_Bip_R_Foot'),'hips':bone('J_Bip_C_Hips')}
mesh=u.load_asset('/Game/IsekaiClass/World/SampleLevel/Medieval_House/meshes/props/SM_bed_01');out['bed_bounds']=str(mesh.get_bounding_box())
root=pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir()));(root/'Polish/SleepPose.json').write_text(json.dumps(out,indent=2),encoding='utf-8');u.log('SLEEP_POSE_AUDIT_OK')
