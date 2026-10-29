"""Move the bone staff's decorations above the hand; retain reviewed offsets."""
import json
def tune_staff_socket(mesh):
    path=OUT/'SocketTransformsBeforeTuning.json'
    records=json.loads(path.read_text(encoding='utf-8')) if path.exists() else {}
    for socket in u.ObjectIterator(u.SkeletalMeshSocket):
        if socket.get_outer()!=mesh or str(socket.get_editor_property('socket_name'))!='weapon_grip_staff':continue
        key=mesh.get_path_name()
        if key not in records:records[key]=vec(socket.get_editor_property('relative_location'))
        rotation=socket.get_editor_property('relative_rotation')
        offset=u.MathLibrary.get_up_vector(rotation)*12
        socket.set_editor_property('relative_location',u.Vector(*records[key])+offset)
    path.write_text(json.dumps(records,indent=2),encoding='utf-8')
    assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
