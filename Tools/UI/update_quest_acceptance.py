"""Keep quest presentation data consistent with the accepted-only journal."""
import csv
import io
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
def accepted_only(text):
    reader = csv.DictReader(io.StringIO(text))
    rows = list(reader)
    for row in rows:
        row['bListBeforeAccept'] = 'False'
    output = io.StringIO(newline='')
    writer = csv.DictWriter(output, reader.fieldnames, lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue()

source = root / 'Data/Quest/DT_IC_Quest.csv'
original_source = source.read_text(encoding='utf-8-sig')
updated_source = accepted_only(original_source)
if updated_source != original_source:
    source.write_text(updated_source, encoding='utf-8', newline='')
table = u.load_asset('/Game/IsekaiClass/Data/DataTable/Quest/DT_IC_Quest')
assert table
current = u.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
assert u.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, accepted_only(current))
assert u.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
u.log('IC_QUEST_ACCEPTED_ONLY_DATA_OK')
