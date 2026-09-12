import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from tools.summarize_log import summarize


ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "tools" / "summarize_log.py"
BANNER = "dlssnr_amd v0.2.15 (build f47996ae) loaded"


class SummarizeLogTests(unittest.TestCase):
    def test_preamble_is_ignored_and_each_banner_starts_a_segment(self):
        text = "startup text before the helper\n" + (
            "dlssnr_amd v0.2.10 (build old) loaded\n"
            "env: HIP: 1 device(s), driver 70260201, runtime 70260201\n"
            "FAULT: exception 0xc0000005 at 0000 (C:\\Windows\\amdhip64.dll + 0x10) "
            "thread 1, last job -1\n"
            + BANNER
            + "\n"
            "env: HIP: 1 device(s), driver 71526312, runtime 71526312\n"
            "engine init ok\nnetwork job 1 done in 78 ms\n"
        )

        result = summarize(text)

        self.assertEqual(len(result["segments"]), 2)
        self.assertEqual(result["segments"][0]["build"], "old")
        self.assertEqual(result["segments"][0]["hip_runtime"], 70260201)
        self.assertFalse(result["segments"][0]["engine_initialized"])
        self.assertEqual(len(result["segments"][0]["faults"]), 1)
        self.assertEqual(result["segments"][0]["completed_job_samples"], 0)
        self.assertEqual(result["segments"][1]["version"], "v0.2.15")
        self.assertEqual(result["segments"][1]["build"], "f47996ae")
        self.assertEqual(result["segments"][1]["hip_runtime"], 71526312)
        self.assertEqual(result["segments"][1]["completed_job_samples"], 1)
        self.assertEqual(result["segments"][1]["faults"], [])

    def test_legacy_banner_without_version_or_build_is_unversioned(self):
        segment = summarize("dlssnr_amd legacy loaded\nengine init ok\n")["segments"][0]

        self.assertIsNone(segment["version"])
        self.assertIsNone(segment["build"])

    def test_duplicate_headers_are_not_collapsed(self):
        text = f"{BANNER}\nengine init ok\n{BANNER}\nfirst ffxDispatch type 0x10001\n"

        result = summarize(text)

        self.assertEqual(len(result["segments"]), 2)
        self.assertTrue(result["segments"][0]["engine_initialized"])
        self.assertFalse(result["segments"][0]["fsr_dispatch_observed"])
        self.assertTrue(result["segments"][1]["fsr_dispatch_observed"])

    def test_dispatch_counters_do_not_count_as_completed_jobs(self):
        text = (
            f"{BANNER}\nfirst ffxDispatch type 0x10001\n"
            "frames 43800 dispatches 2822 submitted 2822 ready 2822 route fsr\n"
        )

        segment = summarize(text)["segments"][0]

        self.assertTrue(segment["fsr_dispatch_observed"])
        self.assertEqual(segment["completed_job_samples"], 0)
        self.assertIsNone(segment["job_time_ms"])
        self.assertEqual(
            segment["last_counters"],
            {"frames": 43800, "dispatches": 2822, "submitted": 2822, "ready": 2822, "route": "fsr"},
        )

    def test_zero_job_segment_has_no_timing_summary(self):
        segment = summarize(
            f"{BANNER}\nenv: HIP: 1 device(s), driver 70260201, runtime 70260201\n"
            "engine init ok\nhooked amd_fidelityfx_upscaler_dx12.dll!ffxDispatch\n"
        )["segments"][0]

        self.assertEqual(segment["hip_runtime"], 70260201)
        self.assertTrue(segment["engine_initialized"])
        self.assertTrue(segment["fsr_hook_observed"])
        self.assertEqual(segment["completed_job_samples"], 0)
        self.assertIsNone(segment["job_time_ms"])

    def test_decimal_job_durations_have_numeric_summary(self):
        segment = summarize(
            f"{BANNER}\n"
            "network job 1 done in 1.25 ms\n"
            "network job 2 done in 2.5 ms (history off, zero-copy)\n"
            "network job 3 done in 3 ms\n"
            "network job 100 done in 4 ms\n"
        )["segments"][0]

        self.assertEqual(segment["completed_job_samples"], 4)
        self.assertEqual(segment["job_time_ms"]["min"], 1.25)
        self.assertAlmostEqual(segment["job_time_ms"]["median"], 2.75)
        self.assertEqual(segment["job_time_ms"]["max"], 4)

    def test_faults_keep_only_module_basename_for_drive_unc_and_unix_paths(self):
        text = (
            f"{BANNER}\n"
            "FAULT: exception 0xc0000005 at 00007FFE85285007 "
            "(C:\\WINDOWS\\SYSTEM32\\amdhip64_7.dll + 0x465007) thread 19924, last job -1\n"
            "FAULT: exception 0xdead at 0x123 (\\\\server\\share\\legacy.dll + 0x99) "
            "thread 2, last job 7\n"
            "FAULT: exception 0xbeef at 0xabc (/opt/rocm/lib/libamdhip64.so + 0x42) "
            "thread 3, last job 8\n"
        )

        faults = summarize(text)["segments"][0]["faults"]

        self.assertEqual([fault["module"] for fault in faults], [
            "amdhip64_7.dll", "legacy.dll", "libamdhip64.so"
        ])
        for fault in faults:
            self.assertNotRegex(json.dumps(fault), r"[\\/]")
            self.assertIn("code", fault)
            self.assertIn("offset", fault)
            self.assertIn("last_job", fault)

    def test_warnings_are_descriptive_labels_and_do_not_include_log_text(self):
        text = (
            f"{BANNER}\nWARNING: invalid kernel file\nWARNING: unsupported colour format\n"
            "WARNING: too many frames in flight\nWARNING: GPU wait timeout\n"
            "WARNING: 100.00% of the pre-block output is zero (healthy is ~0.1%).\n"
        )

        warnings = summarize(text)["segments"][0]["warnings"]

        self.assertGreaterEqual(len(warnings), 5)
        self.assertTrue(all(isinstance(warning, str) and warning for warning in warnings))
        self.assertNotIn("synthetic detail", json.dumps(warnings).lower())

    def test_recognized_lines_without_a_banner_make_an_unversioned_segment(self):
        segment = summarize("engine init ok\nnetwork job 1 done in 78 ms\n")["segments"][0]

        self.assertIsNone(segment["version"])
        self.assertIsNone(segment["build"])
        self.assertEqual(segment["completed_job_samples"], 1)

    def test_empty_and_unrecognized_input_raise_value_error(self):
        for text in ("", "just an unrelated line\n"):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    summarize(text)

    def run_cli(self, *args):
        return subprocess.run(
            [sys.executable, "-B", str(CLI), *map(str, args)],
            cwd=ROOT,
            text=True,
            capture_output=True,
        )

    def test_cli_emits_json_to_stdout_without_changing_input(self):
        with tempfile.TemporaryDirectory() as directory:
            log_path = Path(directory) / "dlssnr.log"
            log_path.write_text(f"{BANNER}\nnetwork job 1 done in 78 ms\n", encoding="utf-8")
            before = log_path.read_bytes()

            completed = self.run_cli(log_path)

            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertEqual(json.loads(completed.stdout)["segments"][0]["completed_job_samples"], 1)
            self.assertEqual(log_path.read_bytes(), before)
            self.assertFalse((Path(directory) / "summary.json").exists())

    def test_cli_bad_inputs_exit_two_without_traceback_or_path_echo(self):
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "missing log.txt"
            empty = Path(directory) / "empty.log"
            unrecognized = Path(directory) / "unrecognized.log"
            empty.write_text("", encoding="utf-8")
            unrecognized.write_text("not a dlssnr log\n", encoding="utf-8")
            for path in (missing, empty, unrecognized):
                with self.subTest(path=path):
                    completed = self.run_cli(path)
                    output = completed.stdout + completed.stderr
                    self.assertEqual(completed.returncode, 2)
                    self.assertNotIn("Traceback", output)
                    self.assertNotIn(str(path), output)


if __name__ == "__main__":
    unittest.main()
