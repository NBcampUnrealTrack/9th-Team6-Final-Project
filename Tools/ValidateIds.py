"""Validate the registry and normalized DataTable CSV exports (Python 3, stdlib)."""
from __future__ import annotations
import argparse
import csv
import glob
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
PATTERNS = {
    "Character": r"CHR_PLAYER",
    "NPC": r"NPC_R0[0-4]_(?:0[0-9]{2}|1[0-9]{2}|2[0-9]{2})",
    "Monster": r"MON_R0[0-4]_[0-9]{3}",
    "Boss": r"BOS_(?:R0[0-4]|FIN)_[0-9]{3}",
    "Skill": r"SKL_(?:WAR|ARC|MAG|HEL|NPC|MON)_[0-9]{3}",
    "Debuff": r"DBF_[0-9]{3}",
    "Item": r"ITM_(?:CONS|FOOD|BOOK|MAT|GATH|FISH|CRAFT|QST|TKT)_[0-9]{3}",
    "Equipment": r"EQP_(?:WPN_(?:SWORD|BOW|STAFF|WAND)|ARM|RNG|NCK)_[0-9]{3}",
    "Recipe": r"RCP_(?:COOK|CRAFT)_[0-9]{3}",
    "Quest": r"QST_(?:R0[0-4]_(?:MAIN|SIDE)|FIN)_[0-9]{3}",
    "Dialogue": r"DLG_NPC_R0[0-4]_[0-9]{3}_[0-9]{3}",
    "Schedule": r"SCH_NPC_R0[0-4]_[0-9]{3}_[0-9]{2}",
    "Anchor": r"ANC_R0[0-4]_[0-9]{3}",
    "SmartObject": r"SO_(?:BED|WORK|KITCHEN|FORGE|SHOP|FARM|TREE|ORE|FISH|SEAT)_R0[0-4]_[0-9]{3}",
    "Warp": r"WRP_R0[0-4]_[0-9]{3}",
    "Container": r"CNT_(?:HOME|NPC|WORLD)_R0[0-4]_[0-9]{3}",
}
REQUIRED = {"ID", "Type", "Owner", "Status", "bDeprecated"}
EMPTY = {"", "None", "NAME_None"}

def rows_from_text(text: str, source: str):
    reader = csv.DictReader(io.StringIO(text.lstrip("\ufeff")))
    if not reader.fieldnames:
        raise ValueError(f"{source}: CSV header missing")
    rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError(f"{source}: malformed CSV column count")
    return rows

def read_rows(path: Path, required=()):
    text=path.read_text(encoding="utf-8-sig")
    fields=csv.DictReader(io.StringIO(text.lstrip("\ufeff"))).fieldnames or []
    if not set(required).issubset(fields):
        raise ValueError(f"{path}: required columns missing")
    return rows_from_text(text, str(path))

def is_deprecated(row):
    return row.get("bDeprecated", "").lower() == "true"

def check_registry(rows, legacy=(), baseline=(), tables=()):
    errors, known, fname_keys = [], {}, set()
    def error(message):
        errors.append(message)
    for old_kind, source_rows in ((False, rows), (True, legacy)):
        for line, row in enumerate(source_rows, 2):
            ident = row.get("ID", "")
            where = f"{'legacy' if old_kind else 'registry'}:{line} {ident!r}"
            if not REQUIRED.issubset(row):
                error(f"{where}: required columns missing"); continue
            if not ident or len(ident) > 40:
                error(f"{where}: ID must have 1..40 characters")
            if ident.casefold() in fname_keys:
                error(f"{where}: duplicate ID")
            fname_keys.add(ident.casefold())
            known[ident] = row
            try:
                owner = int(row["Owner"])
                if not 1 <= owner <= 8:
                    raise ValueError()
            except ValueError:
                error(f"{where}: Owner must be 1..8")
            if row["bDeprecated"].lower() not in {"true", "false"}:
                error(f"{where}: bDeprecated must be true/false")
            allowed = {"Temporary", "Deprecated"} if old_kind else {"Active", "Reserved", "Deprecated"}
            if row["Status"] not in allowed or (row["Status"] == "Deprecated") != is_deprecated(row):
                error(f"{where}: Status and bDeprecated disagree")
            if old_kind:
                if row["Type"] not in set(PATTERNS) | {"Alias", "Effect"}:
                    error(f"{where}: unknown legacy Type")
                continue
            kind = row["Type"]
            if kind not in PATTERNS or not re.fullmatch(PATTERNS.get(kind, r"(?!)"), ident):
                error(f"{where}: invalid {kind} format")
                continue
            serial = 0 if kind == "Character" else int(ident.rsplit("_", 1)[1])
            try:
                start, end = int(row["RangeStart"]), int(row["RangeEnd"])
                maximum = 0 if kind == "Character" else 99 if kind == "Schedule" else 999
                minimum = 0 if kind == "Character" else 1
                if not minimum <= start <= serial <= end <= maximum:
                    raise ValueError()
                if kind == "NPC":
                    ranges = ((1,99),(101,199),(201,299))
                    if not any(a <= start <= serial <= end <= b for a,b in ranges):
                        raise ValueError()
            except (ValueError, KeyError):
                error(f"{where}: invalid allocation range or serial")
    for row in legacy:
        replacement = row.get("ReplacementID", "")
        if replacement and (replacement not in known or is_deprecated(known[replacement])):
            error(f"legacy {row['ID']}: unknown/deprecated ReplacementID {replacement}")
    for row in rows:
        if row.get("Type") in {"Dialogue", "Schedule"} and row.get("ID"):
            match = re.search(r"NPC_R0[0-4]_[0-9]{3}", row["ID"])
            if match and (match[0] not in known or is_deprecated(known[match[0]])):
                error(f"{row['ID']}: unknown/deprecated embedded NPC ID")
    for old in baseline:
        ident = old.get("ID")
        new = known.get(ident)
        if not new:
            error(f"history {ident}: allocated ID removed; retain a deprecated row")
        elif is_deprecated(old) and not is_deprecated(new):
            error(f"history {ident}: deprecated ID reactivated")
        elif any(old.get(key) != new.get(key) for key in ("Type", "Owner")):
            error(f"history {ident}: ID Type/Owner reassigned")
    defined = set()
    for source, table in tables:
        if not table:
            continue
        columns = table[0]
        primary = "ID" if "ID" in columns else "RowName" if "RowName" in columns else "---" if "---" in columns else None
        if not primary:
            error(f"{source}: requires ID, RowName, or Unreal --- primary column"); continue
        for line, row in enumerate(table, 2):
            ident = row[primary]
            where = f"{source}:{line} {ident!r}"
            if ident in defined:
                error(f"{where}: duplicate table definition")
            defined.add(ident)
            if ident not in known:
                error(f"{where}: unregistered row ID")
            elif is_deprecated(known[ident]) and not is_deprecated(row):
                error(f"{where}: deprecated ID used by active row")
            elif row.get("bDeprecated", "").lower() == "true" and not is_deprecated(known[ident]):
                error(f"{where}: deprecation must also be recorded in registry")
            if "RowName" in row and "ID" in row and row["RowName"] != ident:
                error(f"{where}: RowName must equal ID")
            if "---" in row and row["---"] != ident:
                error(f"{where}: Unreal row name must equal ID")
            for column, cell in row.items():
                if column == primary:
                    continue
                if not re.search(r"(?:id|ids)$", column, re.I):
                    continue
                if cell in EMPTY:
                    continue
                try:
                    values = json.loads(cell) if cell.startswith("[") else cell.split("|")
                    if not isinstance(values, list) or any(not isinstance(v, str) for v in values):
                        raise ValueError()
                except (json.JSONDecodeError, ValueError):
                    error(f"{where}: {column} requires a scalar, | list, or JSON string array"); continue
                for value in values:
                    value = value.strip()
                    if value in EMPTY:
                        continue
                    if value not in known:
                        error(f"{where}: {column} unknown reference {value}")
                    elif is_deprecated(known[value]):
                        error(f"{where}: {column} references deprecated {value}")
    return errors

def head_baseline():
    result = subprocess.run(
        ["git", "-C", str(ROOT), "show", "HEAD:Data/ID_Registry.csv"],
        capture_output=True, text=True, encoding="utf-8", errors="replace", check=False)
    if result.returncode == 0:
        return rows_from_text(result.stdout, "HEAD:Data/ID_Registry.csv")
    return []

class ValidatorTests(unittest.TestCase):
    def row(self, ident="ITM_FOOD_001", **changes):
        row = dict(ID=ident, Type="Item", Owner="2", RangeStart="1", RangeEnd="99", Status="Active", bDeprecated="false")
        row.update(changes); return row
    def test_valid(self):
        self.assertEqual(check_registry([self.row()]), [])
    def test_duplicate(self):
        self.assertTrue(any("duplicate" in e for e in check_registry([self.row(), self.row()])))
        self.assertTrue(any("duplicate" in e for e in check_registry([], [self.row("Food", Status="Temporary"), self.row("food", Status="Temporary")])))
    def test_bad_format_and_range(self):
        for row in (self.row("ITM FOOD 001"), self.row("ITM_FOOD_000"), self.row(RangeEnd="0"), self.row(Owner="9")):
            self.assertTrue(check_registry([row]))
    def test_npc_reserved_gaps(self):
        for ident in ("NPC_R01_100", "NPC_R01_200", "NPC_R01_300"):
            self.assertTrue(check_registry([self.row(ident, Type="NPC")]))
    def test_unknown_reference_and_rowname(self):
        table=[dict(ID="ITM_FOOD_001", RowName="bad", EffectID="DBF_999")]
        errors=check_registry([self.row()], tables=[("fixture",table)])
        self.assertTrue(any("unknown reference" in e for e in errors))
        self.assertTrue(any("RowName" in e for e in errors))
    def test_no_reuse_or_removal(self):
        dead=self.row(Status="Deprecated", bDeprecated="true")
        self.assertTrue(check_registry([self.row()], baseline=[dead]))
        self.assertTrue(check_registry([], baseline=[dead]))
        self.assertEqual(check_registry([dead], baseline=[dead]), [])
    def test_deprecated_reference(self):
        dead=self.row("ITM_FOOD_002", Status="Deprecated", bDeprecated="true")
        table=[dict(ID="ITM_FOOD_001", IngredientIDs='["ITM_FOOD_002"]')]
        self.assertTrue(check_registry([self.row(),dead], tables=[("fixture",table)]))
    def test_legacy_exception(self):
        legacy=self.row("Food", Status="Temporary")
        self.assertEqual(check_registry([self.row()], [legacy], tables=[("fixture",[dict(ID="Food", ResultID="ITM_FOOD_001")])]), [])

def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", type=Path, default=ROOT/"Data/ID_Registry.csv")
    parser.add_argument("--legacy", type=Path, default=ROOT/"Data/Legacy_ID_Allowlist.csv")
    parser.add_argument("--baseline", type=Path, help="Historical registry; default is git HEAD when present")
    parser.add_argument("--tables", nargs="*", default=[], help="Normalized CSV paths or globs; see Docs/DT_Schema.md")
    parser.add_argument("--self-test", action="store_true")
    args=parser.parse_args()
    try:
        baseline=read_rows(args.baseline) if args.baseline else head_baseline()
        table_paths=[]
        for pattern in args.tables:
            matches=glob.glob(pattern, recursive=True)
            if not matches:
                raise ValueError(f"No CSV files match: {pattern}")
            table_paths.extend(Path(p) for p in matches)
        tables=[(str(p),read_rows(p)) for p in dict.fromkeys(table_paths)]
        rows,legacy=read_rows(args.registry, REQUIRED | {"RangeStart", "RangeEnd"}),read_rows(args.legacy, REQUIRED)
        errors=check_registry(rows,legacy,baseline,tables)
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr); return 1
    if errors:
        for error in errors:print("ERROR: "+error, file=sys.stderr)
        return 1
    print(f"IDs passed: {len(rows)} registry + {len(legacy)} legacy; {len(tables)} table CSV(s); history {'checked' if baseline else 'not yet committed'}.")
    if args.self_test:
        suite=unittest.defaultTestLoader.loadTestsFromTestCase(ValidatorTests)
        if not unittest.TextTestRunner(verbosity=1).run(suite).wasSuccessful():return 1
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
