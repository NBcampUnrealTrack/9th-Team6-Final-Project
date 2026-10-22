"""Render actual UI classes at seven sizes inside the fixed 16:9 game frame."""
import csv
import unreal as u
try:
    report=u.ICCharacterSetupLibrary.capture_ui_aspect_samples()
    if not report:raise RuntimeError('UI capture requires the editor with rendering enabled')
    with open(report,encoding='utf-8-sig',newline='') as stream:checks=list(csv.DictReader(stream))
    if len(checks)!=42 or any(row['ImageSaved']!='true' for row in checks):raise RuntimeError('UI capture did not produce all 42 images')
    if any(int(row['VisibleControls'])==0 for row in checks):raise RuntimeError('A captured screen has no initialized controls')
    if any(int(row['OutsideControls']) for row in checks):raise RuntimeError('A visible control extends outside the 16:9 game frame')
    u.log('IC_UI_ASPECT_REPORT '+report)
finally:
    u.SystemLibrary.quit_editor()
