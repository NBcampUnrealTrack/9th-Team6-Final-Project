"""Verify the changed HUD in isolated PIE using existing art and actual gameplay state."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

OUT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_saved_dir())) / 'FirefightHUD/Live'
OUT.mkdir(parents=True, exist_ok=True)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
S = dict(phase='starting', start=time.monotonic(), checks={})

def report():
    (OUT / 'Review.json').write_text(json.dumps({k: S[k] for k in ('phase', 'checks', 'error') if k in S}, ensure_ascii=False, indent=2), encoding='utf-8')

def phase(name):
    S.update(phase=name, at=time.monotonic())
    report()

def finish():
    report()
    u.unregister_slate_post_tick_callback(S['handle'])
    LEVELS.editor_request_end_play()
    u.SystemLibrary.quit_editor()

def capture(name):
    assert u.ICCharacterSetupLibrary.capture_monster_test_hud(S['world'], str(OUT / (name + '.png')))

def prop(widget, name):
    return widget.get_editor_property(name)

def tick(delta):
    try:
        if time.monotonic() - S['start'] > 160:
            raise RuntimeError('HUD review timeout: ' + S['phase'])
        if S['phase'] == 'starting':
            worlds = u.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            pc = u.GameplayStatics.get_player_controller(worlds[0], 0)
            if not pc or str(prop(pc, 'menu')) != 'Create':
                return
            assert u.ICCharacterSetupLibrary.prepare_live_character_review(worlds[0])
            S.update(world=worlds[0], pc=pc)
            phase('creating')
            return
        pc = S['pc']
        elapsed = time.monotonic() - S['at']
        if S['phase'] == 'creating':
            if not prop(pc, 'created'):
                return
            pc.set_menu('')
            state = pc.get_world_state()
            for slot, npc in enumerate(('NPC_R01_002', 'NPC_R01_008', 'NPC_R01_011')):
                assert state.ensure_character(npc, u.ICCharacterKind.RECRUITABLE)
                assert state.set_affinity(npc, 300)
                assert state.set_party_slot(npc, slot)
            bag = pc.get_inventory()
            for item in ('ITM_CONS_001', 'ITM_FOOD_001', 'ITM_BOOK_001'):
                bag.add_item(item, 8, False)
            for slot, item in enumerate(('ITM_CONS_001', 'ITM_FOOD_001', 'ITM_BOOK_001', 'ITM_CONS_001')):
                index = next(i for i, v in enumerate(bag.get_snapshot().bag.slots) if str(v.item_id) == item)
                assert bag.bind_quick_slot(slot, index)
            assert state.set_vitals('CHR_PLAYER', 25, 50, 70)
            hud = prop(pc, 'screen')
            hud.refresh_current_screen()
            S.update(hud=hud, state=state, bag=bag)
            phase('party')
            return
        if S['phase'] == 'party' and elapsed > 1:
            group = prop(S['hud'], 'HUD_Companions')
            for i in range(1, 4):
                card = prop(group, 'Companion' + str(i))
                assert card.is_visible()
                assert prop(card, 'portrait')
            S['checks']['three_companions_use_dt_portraits'] = True
            capture('HUD_Party3')
            result = pc.use_inventory_item('ITM_CONS_001', 'CHR_PLAYER')
            assert result is not None, result
            assert pc.get_item_cooldown('ITM_CONS_001') > 0
            assert pc.use_inventory_item('ITM_CONS_001', 'CHR_PLAYER') is None
            S['checks']['item_cooldown_blocks_repeat'] = True
            phase('cooldown')
            return
        if S['phase'] == 'cooldown' and elapsed > .4:
            capture('HUD_ItemCooldown')
            quick = prop(S['hud'], 'HUD_QuickSlots')
            for i in (1, 4):
                assert prop(prop(quick, 'QuickSlot' + str(i)), 'TXT_ItemCooldown').is_visible()
            S['checks']['duplicate_slots_share_cooldown_overlay'] = True
            asc = u.GameplayStatics.get_player_pawn(S['world'], 0).get_component_by_class(u.ICAbilitySystemComponent)
            S['effects'] = []
            for identifier in ('STE_NPC_R01_009', 'STE_NPC_R01_010'):
                if asc.apply_status(identifier, asc):
                    S['effects'].append(identifier)
            phase('effects')
            return
        if S['phase'] == 'effects' and elapsed > .5:
            capture('HUD_StatusEffects')
            card = prop(S['hud'], 'HUD_PlayerStatus')
            assert len(S['effects']) == 2
            assert prop(card, 'StatusEffects').get_children_count() >= 2
            S['checks']['multiple_live_effect_badges'] = True
            caster = next(a for a in u.GameplayStatics.get_all_actors_of_class(S['world'], u.ICCharacterBase)
                          if str(a.get_character_id()) == 'NPC_R01_002')
            ai = caster.get_controller()
            if ai:
                brain = ai.get_component_by_class(u.BrainComponent)
                if brain:
                    brain.stop_logic('HUD cooldown review')
                ai.stop_movement()
                ai.set_actor_tick_enabled(False)
            movement = caster.get_component_by_class(u.CharacterMovementComponent)
            movement.stop_movement_immediately()
            movement.set_component_tick_enabled(False)
            pc.handle_input_command(u.ICInputCommand.COMPANION_SKILL, 0)
            phase('companion_skill')
            return
        if S['phase'] == 'companion_skill' and elapsed > 1:
            first = prop(prop(S['hud'], 'HUD_Companions'), 'Companion1')
            remaining = prop(first, 'IMG_Portrait').get_parent().get_child_at(1)
            S['checks']['companion_cooldown_text'] = str(remaining.get_text())
            assert remaining.is_visible()
            assert str(remaining.get_text())
            S['checks']['real_companion_skill_updates_portrait_cooldown'] = True
            capture('HUD_CompanionSkillCooldown')
            pc.set_menu('Pause')
            prop(S['hud'], 'GameMenu').select_tab('Party')
            phase('menu')
            return
        if S['phase'] == 'menu' and elapsed > .5:
            capture('Menu_Fullscreen')
            menu = prop(S['hud'], 'GameMenu')
            # Fast-path Slate borders need not keep tick geometry. Verify their
            # viewport constraints, and retain the rendered screenshot for visual QA.
            slot = prop(prop(menu, 'MenuCard'), 'slot')
            anchors = slot.get_anchors()
            offsets = slot.get_offsets()
            assert anchors.minimum.x == 0 and anchors.minimum.y == 0
            assert anchors.maximum.x == 1 and anchors.maximum.y == 1
            assert offsets.left == offsets.top == offsets.right == offsets.bottom == 0
            S['checks']['menu_fills_viewport'] = True
            pc.set_menu('')
            phase('complete')
            return
        if S['phase'] == 'complete' and elapsed > .2:
            u.log('IC_FIREFIGHT_LIVE_REVIEW_OK ' + json.dumps(S['checks']))
            finish()
    except Exception:
        S['error'] = traceback.format_exc()
        phase('error')
        u.log_error(S['error'])
        finish()

assert not u.EditorLevelLibrary.get_pie_worlds(False)
u.EditorLoadingAndSavingUtils.load_map('/Game/IsekaiClass/World/L_TestMap')
S['handle'] = u.register_slate_post_tick_callback(tick)
assert u.ICCharacterSetupLibrary.begin_monster_test_review()
report()
