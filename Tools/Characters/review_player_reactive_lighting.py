"""Capture saved player materials in four conditions; no runtime material overrides.

Run in an isolated -ICCharacterLightingReview -ICSoftToonLightingPreview editor.
The clock and player positions change only in PIE. No world assets are saved.
"""
import hashlib
import json
import struct
import sys
import time
import traceback
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
sys.path.insert(0, str(ROOT / 'Tools/Characters'))
import setup_player_reactive_lighting as reactive
import review_player_world_lighting as review

EYE_REVIEW='-ICEyeCorrectionReview' in u.SystemLibrary.get_command_line()
OUT = reactive.OUT / ('EyeFix/Final' if EYE_REVIEW else 'Final')
OUT.mkdir(parents=True, exist_ok=True)
APPLIED = json.loads((reactive.OUT / 'Applied.json').read_text(encoding='utf-8'))
review.OUT = OUT
review.STATE['automatic_variants'] = []
POSITIONS = {
    'Outside': u.Vector(-53990, 629630, -5098.451662),
    'Inside': u.Vector(-53240, 629940, -5069.725),
}
CASES = [(sex, label, minute, POSITIONS[place])
         for sex in ['F', 'M']
         for label, minute, place in [
             ('Night_Outside', 1200, 'Outside'), ('Night_Inside', 1200, 'Inside'),
             ('Day_Outside', 600, 'Outside'), ('Day_Inside', 600, 'Inside')]]
if EYE_REVIEW:
    CASES=[case for sex in ['F','M'] for case in
           ([row for row in CASES if row[0]==sex]+[(row[0],'Closeup_'+row[1],row[2],row[3]) for row in CASES if row[0]==sex])]
CHECK = {'stage': 'waiting', 'index': 0, 'samples': [], 'graphs': {}, 'complete': False}


def report():
    result = {key: value for key, value in CHECK.items() if key not in ['handle', 'ready']}
    (OUT / 'Verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')


def validate_material(material):
    assert material.get_path_name().startswith(reactive.DEST + '/')
    assert reactive.SOURCE.effective_shading_model(material) == u.MaterialShadingModel.MSM_DEFAULT_LIT
    switches = {}
    sex='M' if '/M/' in material.get_path_name() else 'F'
    direct=reactive.use_diffuse_albedo(sex,material.get_name())
    for name in ([] if direct else ['bUseLight', 'bUseLightForce', 'bUseFilmTonemapInverse', 'bDivAdaptation']):
        value = u.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(material, name)
        assert not value, (material.get_path_name(), name, value)
        switches[name] = value
    scale = u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(material, 'ICAlbedoScale')
    assert abs(scale - APPLIED['albedo_scale']) < .00001
    if direct:
        folder='Male' if sex=='M' else 'Female'
        original=u.load_asset('/Game/IsekaiClass/Player/SamplePlayer/'+folder+'/'+material.get_name()[:-6])
        actual_texture=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(material,'ICFeatureAlbedoTexture')
        assert actual_texture==u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(original,'gltf_tex_diffuse')
    root = material
    while isinstance(root, u.MaterialInstance):
        root = root.get_editor_property('parent')
    path = root.get_path_name()
    if path not in CHECK['graphs']:
        output = u.MaterialEditingLibrary.get_material_property_input_node(root, u.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
        feature_root=root.get_name()=='M_ICAuthoredFeatureLit'
        assert output.get_editor_property('desc') == ('IC Authored Feature Surface' if feature_root else 'IC World Reactive Lit')
        names = u.MaterialEditingLibrary.get_material_expression_input_names(output)
        inputs = u.MaterialEditingLibrary.get_inputs_for_material_expression(root, output)
        pins = {str(name).replace(' ', '').lower(): node for name, node in zip(names, inputs)}
        emission = pins['emissivecolor']
        assert isinstance(emission, u.MaterialExpressionConstant)
        assert emission.get_editor_property('r') == 0.
        assert isinstance(pins['basecolor'],u.MaterialExpressionMultiply)
        if feature_root:
            assert isinstance(pins['opacitymask'],u.MaterialExpressionMultiply)
            assert root.get_editor_property('used_with_skeletal_mesh') and root.get_editor_property('used_with_morph_targets')
            assert len(u.MaterialEditingLibrary.get_material_expressions(root))==9
        CHECK['graphs'][path] = {'emissive': 0., 'base_color': 'Separate masked surface with original diffuse RGB/alpha' if feature_root else 'Original toon output * ICAlbedoScale'}
    return {'material': material.get_path_name(), 'albedo_scale': scale, 'direct_diffuse_albedo':direct,'switches': switches, 'root': path}


def tick(delta):
    try:
        if review.STATE.get('error'):
            raise RuntimeError(review.STATE['error'])
        stage = CHECK['stage']
        now = time.monotonic()
        if stage == 'waiting' and review.STATE['phase'] == 'freeze':
            assert u.ICCharacterSetupLibrary.set_ui_review_window_size(review.STATE['world'], 1600, 900)
            review.STATE['phase'] = 'reactive_native_verification'
            CHECK['stage'] = 'next'
            return
        if stage == 'next':
            if CHECK['index'] == len(CASES):
                for rel, digest in APPLIED['world_hashes'].items():
                    assert hashlib.sha256((ROOT / rel).read_bytes()).hexdigest() == digest, rel
                CHECK.update(stage='complete', complete=True, actual_preset_materials_without_review_overrides=True,
                             protected_world_files_unchanged=True, no_brightness_controller=True)
                report()
                u.unregister_slate_post_tick_callback(CHECK['handle'])
                review.finish()
                return
            sex, label, minute, point = CASES[CHECK['index']]
            player = review.STATE['player']
            player.apply_preset(1 if sex == 'F' else 0)
            body = player.get_editor_property('mesh')
            assert not list(body.get_editor_property('override_materials'))
            assert not player.get_component_by_class(u.ICPlayerToonBrightnessComponent)
            for index, slot in enumerate(body.get_skeletal_mesh_asset().get_editor_property('materials')):
                assert body.get_material(index) == slot.get_editor_property('material_interface')
                validate_material(body.get_material(index))
            player.set_actor_location_and_rotation(point, u.Rotator(), False, True)
            closeup=label.startswith('Closeup_')
            focus=point+u.Vector(0,0,52 if sex=='F' else 75) if closeup else point
            location = focus + (u.Vector(120,70,5) if closeup else u.Vector(350, 380, 75))
            review.STATE['camera'].set_world_location_and_rotation(
                location, u.MathLibrary.find_look_at_rotation(location, focus), False, True)
            review.STATE['camera'].set_editor_property('field_of_view',30 if closeup else 40)
            timer = review.STATE['pc'].get_time()
            current = timer.get_clock().game_minute
            timer.jump_game_minutes((minute - current % 1440 + 1440) % 1440)
            for cycle in u.GameplayStatics.get_all_actors_of_class(review.STATE['world'], u.ICDayNightController):
                cycle.refresh_from_clock()
            CHECK.update(stage='settle', ready=now + 7.)
            report()
            return
        if stage == 'settle' and now >= CHECK['ready']:
            sex, label, minute, point = CASES[CHECK['index']]
            world = review.STATE['world']
            suns = u.GameplayStatics.get_all_actors_of_class(world, u.DirectionalLight)
            visible = [sun for sun in suns if sun.get_component_by_class(u.DirectionalLightComponent).get_editor_property('visible')]
            assert [sun.get_name() for sun in visible] == ['DirectionalLight_0']
            cycles = u.GameplayStatics.get_all_actors_of_class(world, u.ICDayNightController)
            assert len(cycles) == 1 and [sun.get_name() for sun in cycles[0].sun_lights] == ['DirectionalLight_0']
            assert review.STATE['pc'].get_time().get_clock().game_minute % 1440 == minute
            # This is diagnostic evidence only; it never drives material brightness.
            start = point + u.Vector(0, 0, 40)
            hit = u.SystemLibrary.line_trace_single(world, start, start - visible[0].get_actor_forward_vector() * 50000.,
                u.TraceTypeQuery.ECC_VISIBILITY, True, [review.STATE['player']], u.DrawDebugTrace.NONE, True)
            blocked = bool(hit and hit.to_dict().get('blocking_hit', False))
            if 'Day' in label:
                assert blocked == label.endswith('Inside'), (label, blocked)
            name = sex + '_' + label + '.png'
            assert u.ICCharacterSetupLibrary.capture_monster_test_hud(world, str(OUT / name))
            assert struct.unpack('>II', (OUT / name).read_bytes()[16:24]) == (1600, 900)
            body = review.STATE['player'].get_editor_property('mesh')
            CHECK['samples'].append({'sex': sex, 'label': label, 'minute': minute, 'image': name,
                'position': str(point), 'sun_ray_blocked': blocked, 'lighting': review.light_state(),
                'slots': [validate_material(body.get_material(index)) for index in range(body.get_num_materials())]})
            CHECK.update(stage='next', index=CHECK['index'] + 1)
            report()
    except Exception:
        CHECK.update(stage='error', error=traceback.format_exc())
        report()
        u.log_error(CHECK['error'])
        review.STATE['phase'] = 'commands'
        u.unregister_slate_post_tick_callback(CHECK['handle'])


CHECK['handle'] = u.register_slate_post_tick_callback(tick)
report()
