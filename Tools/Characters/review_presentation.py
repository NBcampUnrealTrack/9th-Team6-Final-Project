"""Render measured character poses with the editor's actual animation assets."""
import unreal as u
try:
    report=u.ICCharacterSetupLibrary.inspect_character_presentation('-ICApplyCalibration' in u.SystemLibrary.get_command_line())
    if not report:raise RuntimeError('Presentation capture needs a rendering editor')
    u.log('IC_CHARACTER_PRESENTATION_REPORT '+report)
finally:
    u.SystemLibrary.quit_editor()
