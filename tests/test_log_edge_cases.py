import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from tools.summarize_log import summarize


ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "tools" / "summarize_log.py"
BANNER = "dlssnr_amd v0.2.15 (build f47996ae) loaded"


class LogEdgeCaseTests(unittest.TestCase):
    def test_headerless_input_consumes_every_record(self):
        result = summarize(
            "env: HIP: 1 device(s), driver 70260201, runtime 70260201\n"
            "engine init ok\n"
            "network job 1 done in 78 ms\n"
            "network job 2 done in 82 ms\n"
        )

        segment = result["segments"][0]
        self.assertEqual(segment["hip_runtime"], 70260201)
        self.assertTrue(segment["engine_initialized"])
        self.assertEqual(segment["completed_job_samples"], 2)
        self.assertEqual(segment["job_time_ms"], {"min": 78, "median": 80, "max": 82})

    def test_counters_allow_negative_ready_value(self):
        segment = summarize(
            f"{BANNER}\nframes 12 dispatches 4 submitted 4 ready -1 route fsr\n"
        )["segments"][0]

        self.assertEqual(
            segment["last_counters"],
            {"frames": 12, "dispatches": 4, "submitted": 4, "ready": -1, "route": "fsr"},
        )

    def test_fault_keeps_unprefixed_address_and_unicode_free_basename(self):
        segment = summarize(
            f"{BANNER}\n"
            "FAULT: exception 0xc0000005 at 7FFAB123 "
            "(C:\\Program Files (x86)\\Game\\version.dll + 0x20) "
            "thread 7, last job -1\n"
        )["segments"][0]

        self.assertEqual(len(segment["faults"]), 1)
        self.assertEqual(segment["faults"][0]["module"], "version.dll")
        self.assertEqual(segment["faults"][0]["last_job"], -1)

    def test_only_actual_zero_output_warning_is_reported(self):
        cases = (
            ("warning", "WARNING: 100.00% of the pre-block output is zero (healthy is ~0.1%).\n", ["zero-output warning"]),
            ("healthy self-check", "self-check: pre-block zero bytes 0.033% (healthy)\n", []),
        )
        for name, line, expected in cases:
            with self.subTest(name=name):
                segment = summarize(f"{BANNER}\n{line}")["segments"][0]
                self.assertEqual(segment["warnings"], expected)

    def test_negated_network_job_sentence_is_not_a_sample(self):
        segment = summarize(f"{BANNER}\nnot network job 1 done in 78 ms\n")["segments"][0]

        self.assertEqual(segment["completed_job_samples"], 0)
        self.assertIsNone(segment["job_time_ms"])

    def test_malformed_duration_values_are_ignored_without_crashing(self):
        segment = summarize(
            f"{BANNER}\n"
            "network job 1 done in 999999999999999999999999 ms\n"
            "network job 2 done in NaN ms\n"
            "network job 3 done in Inf ms\n"
            "network job 4 done in -1 ms\n"
        )["segments"][0]

        self.assertEqual(segment["completed_job_samples"], 0)
        self.assertIsNone(segment["job_time_ms"])

    def run_cli(self, log_path, *, env=None):
        return subprocess.run(
            [sys.executable, "-B", str(CLI), str(log_path)],
            cwd=ROOT,
            text=True,
            capture_output=True,
            env=env,
        )

    def test_cli_accepts_utf8_bom_before_banner(self):
        with tempfile.TemporaryDirectory() as directory:
            log_path = Path(directory) / "bom.log"
            log_path.write_text(f"{BANNER}\nengine init ok\n", encoding="utf-8-sig")

            completed = self.run_cli(log_path)

            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertTrue(json.loads(completed.stdout)["segments"][0]["engine_initialized"])

    def test_cli_ascii_stdio_json_escapes_unicode_module_basename(self):
        with tempfile.TemporaryDirectory() as directory:
            log_path = Path(directory) / "unicode.log"
            log_path.write_text(
                f"{BANNER}\n"
                "FAULT: exception 0xc0000005 at 7FFAB123 "
                "(C:\\Games\\测试\\版本.dll + 0x20) thread 7, last job -1\n",
                encoding="utf-8",
            )
            child_env = os.environ.copy()
            child_env["PYTHONIOENCODING"] = "ascii"

            completed = self.run_cli(log_path, env=child_env)

            self.assertEqual(completed.returncode, 0, completed.stderr)
            self.assertEqual(
                json.loads(completed.stdout)["segments"][0]["faults"][0]["module"],
                "版本.dll",
            )


if __name__ == "__main__":
    unittest.main()
