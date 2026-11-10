"""Counter mutations must fail without rewriting archived device evidence."""

import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(TOOLS))

from verification.frame_accounting import stop_accounting, session_pace_accounting
import verify_p12_all as aggregate
import verify_p12c_sync as sync
import verify_p13d_drift as drift

TOTAL = "frames_discarded_on_stop"
SUBSET = "frames_discarded_on_stop_undelivered"


class StopContractTests(unittest.TestCase):
    def test_legacy_absence_is_explicit(self):
        total, subset, notes = stop_accounting({}, False)
        self.assertEqual((total, subset), (0, 0))
        self.assertEqual(len(notes), 2)
        self.assertTrue(all("旧证据" in note for note in notes))

    def test_both_fields_required(self):
        for holder in ({}, {TOTAL: 0}, {SUBSET: 0}):
            with self.subTest(holder=holder), self.assertRaises(ValueError):
                stop_accounting(holder, True)

    def test_invalid_types_and_subset_are_rejected(self):
        for key in (TOTAL, SUBSET):
            for value in (-1, True, False, 1.0, "1", None):
                with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                    stop_accounting({TOTAL: 2, SUBSET: 1, key: value}, True)
        for holder in (None, [], {TOTAL: 1}, {TOTAL: 1, SUBSET: 2}):
            with self.subTest(holder=holder), self.assertRaises(ValueError):
                stop_accounting(holder, False)


class PaceAccountingTests(unittest.TestCase):
    def report(self, total=3, subset=1, missing=1, schema="kivo.p12c.sync/44"):
        holder_key = "counters" if "decode" in schema else "video"
        doc = {"pace": {"frames_decided": 10, "presented": 10 - missing,
                        "dropped": 0, "undelivered_frames": missing},
               holder_key: {"frames_discarded_no_surface": 0, TOTAL: total, SUBSET: subset}}
        report = drift.Report(Path("mutation.json"))
        drift.check_frame_accounting(report, doc, schema)
        return report

    def test_only_undelivered_subset_explains_pace_loss(self):
        for schema in ("kivo.p12c.sync/44", "kivo.p12c.decode/20"):
            self.assertFalse(self.report(schema=schema).failed)
            self.assertTrue(any(row[0].startswith("S2 ") for row in
                                self.report(missing=3, schema=schema).failed))

    def test_eos_drop_and_undecided_slots_cannot_hide_missing_picture(self):
        self.assertFalse(self.report(total=3, subset=0, missing=0).failed)
        self.assertTrue(self.report(total=3, subset=0, missing=3).failed)

    def test_invalid_subset_and_negative_counters_fail(self):
        self.assertTrue(any(row[0].startswith("S7 ") for row in self.report(subset=4).failed))
        report = drift.Report(Path("negative.json"))
        drift.check_frame_accounting(report, {"pace": {
            "frames_decided": -1, "presented": -1, "dropped": 0,
            "undelivered_frames": 0}, "video": {
                "frames_discarded_no_surface": 0, TOTAL: 0, SUBSET: 0}}, "kivo.p12c.sync/44")
        self.assertTrue(any(row[0].startswith("S8 ") for row in report.failed))


class SyncAccountingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.baseline = json.loads((TOOLS.parent / "docs/evidence/p15a/p15a-back5-a1-sync.json")
                                  .read_text(encoding="utf-8"))

    def verify(self, data):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "mutation-sync.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            return sync.verify(path)

    def upgraded(self):
        doc = copy.deepcopy(self.baseline)
        doc["schema"] = "kivo.p12c.sync/44"
        doc["video"].update({TOTAL: 0, SUBSET: 0})
        return doc

    def test_legacy_and_upgrade_verify(self):
        _, notes, _ = self.verify(self.baseline)
        self.assertTrue(any("旧证据缺 frames_discarded_on_stop" in note for note in notes))
        self.verify(self.upgraded())

    def test_returned_slots_close_output_account_without_inflating_span(self):
        doc = self.upgraded()
        doc["video"][TOTAL] = 3
        for key in ("frames_received", "frames_released"):
            doc["video"][key] += 3
        self.verify(doc)
        doc["video"][TOTAL] -= 1
        with self.assertRaises(sync.Failure):
            self.verify(doc)

    def test_new_schema_missing_fields_rejected(self):
        for key in (TOTAL, SUBSET):
            doc = self.upgraded()
            del doc["video"][key]
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.verify(doc)

    def test_aggregate_registers_both_versions_and_requires_decode_fields(self):
        self.assertIn("kivo.p12c.sync/44", aggregate.SYNC_QUANTILE_BLOCK_SCHEMAS)
        for schema in ("kivo.p12c.decode/20", "kivo.p12c.sync/44"):
            self.assertIn(schema, aggregate.KNOWN_SCHEMAS)
        self.assertIn("kivo.p12c.sync/44", aggregate.SCHEMAS_WITH_SYNC_END_REASON)
        self.assertIn("kivo.p12c.decode/20", aggregate.SCHEMAS_WITH_DECODE_END_REASON)
        report = aggregate.Report()
        aggregate.check_frames_closed({"schema": "kivo.p12c.decode/20", "counters": {}},
                                      "missing.json", report)
        self.assertTrue(report.findings)


class SessionPaceTests(SyncAccountingTests):
    def upgraded(self):
        doc = super().upgraded()
        doc["schema"] = "kivo.p12c.sync/45"
        doc["pace"].update(accounting_scope="session", discarded_on_stop=0,
                           discarded_no_surface=0)
        return doc

    def test_multi_seek_uses_session_dispositions(self):
        doc = self.upgraded()
        pace = doc["pace"]
        pace["discarded_on_stop"] = 3
        pace["discarded_no_surface"] = 2
        pace["frames_decided"] += 5
        pace["undelivered_frames"] += 5
        # The recent run has no discarded frames; previous runs still count.
        session_pace_accounting(pace, doc["video"])
        for schema, holder in (("kivo.p12c.sync/45", "video"),
                               ("kivo.p12c.decode/21", "counters")):
            report = drift.Report(Path("multi-seek.json"))
            drift.check_frame_accounting(report, {"pace": pace, holder: doc["video"]}, schema)
            self.assertFalse(report.failed, report.failed)

    def test_session_mutations_fail_both_verifiers(self):
        for key in ("accounting_scope", "discarded_on_stop", "discarded_no_surface",
                    "undelivered_frames", "frames_decided", "presented", "dropped"):
            values = (None, "run", True) if key == "accounting_scope" else (None, -1, True, 0.0, "0")
            for value in values:
                doc = self.upgraded()
                doc["pace"][key] = value
                with self.subTest(key=key, value=value):
                    with self.assertRaises((ValueError, sync.Failure)):
                        self.verify(doc)
                    report = drift.Report(Path("mutation.json"))
                    drift.check_frame_accounting(report, doc, doc["schema"])
                    self.assertTrue(report.failed)
            doc = self.upgraded()
            del doc["pace"][key]
            with self.subTest(missing=key), self.assertRaises((ValueError, sync.Failure, KeyError)):
                self.verify(doc)

    def test_wrong_totals_and_scope_cannot_hide_losses(self):
        for changed in ({"discarded_on_stop": 1}, {"discarded_no_surface": 1},
                        {"undelivered_frames": 1}, {"frames_decided": 1}):
            doc = self.upgraded()
            doc["pace"].update(changed)
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                session_pace_accounting(doc["pace"], doc["video"])
        for key in (SUBSET, "frames_discarded_no_surface"):
            doc = self.upgraded()
            doc["video"].update({TOTAL: 1, key: 1})
            with self.subTest(recent=key), self.assertRaises(ValueError):
                session_pace_accounting(doc["pace"], doc["video"])

    def test_aggregate_registers_session_versions(self):
        for schema in ("kivo.p12c.decode/21", "kivo.p12c.sync/45"):
            self.assertIn(schema, aggregate.KNOWN_SCHEMAS)
        self.assertIn("kivo.p12c.sync/45", aggregate.SYNC_QUANTILE_BLOCK_SCHEMAS)
        self.assertIn("kivo.p12c.sync/45", aggregate.SCHEMAS_WITH_SYNC_END_REASON)
        self.assertIn("kivo.p12c.decode/21", aggregate.SCHEMAS_WITH_DECODE_END_REASON)
        report = aggregate.Report()
        aggregate.check_frames_closed({"schema": "kivo.p12c.decode/21", "counters": {}},
                                      "missing.json", report)
        self.assertTrue(report.findings)


if __name__ == "__main__":
    unittest.main()
