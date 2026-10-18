import unittest

from tools.summarize_log import summarize


BANNER = "dlssnr_amd v0.2.15 (build f47996ae) loaded"


class WarningRecordTests(unittest.TestCase):
    def test_known_warning_records_emit_fixed_labels(self):
        segment = summarize(
            f"{BANNER}\n"
            "job 1 GPU errors: invalid kernel file img:invalid kernel file ...\n"
            "unsupported colour format 67\n"
            "inline: too many frames in flight, skipping one\n"
            "inline: 2 GPU wait timeouts so far (...)\n"
            "WARNING: 100.00% of the pre-block output is zero (healthy is ~0.1%).\n"
        )["segments"][0]

        self.assertEqual(
            segment["warnings"],
            [
                "invalid kernel file",
                "unsupported colour format",
                "too many frames in flight",
                "GPU wait timeout",
                "zero-output warning",
            ],
        )

    def test_healthy_pre_block_zero_record_is_not_a_warning(self):
        segment = summarize(
            f"{BANNER}\nself-check: pre-block zero bytes 0.033% (healthy)\n"
        )["segments"][0]

        self.assertEqual(segment["warnings"], [])

    def test_quoted_or_negated_warning_prose_is_ignored(self):
        lines = (
            "no GPU wait timeouts were observed",
            'the quoted text "invalid kernel file" is documentation',
            "message says: unsupported colour format",
            "not too many frames in flight; this is a negation",
            'quoted: "GPU wait timeout"',
            'the warning text says "zero-output" but no event occurred',
            "no zero-output warning was emitted",
        )

        for line in lines:
            with self.subTest(line=line):
                segment = summarize(f"{BANNER}\n{line}\n")["segments"][0]
                self.assertEqual(segment["warnings"], [])

    def test_negated_fault_record_is_not_reported(self):
        segment = summarize(
            f"{BANNER}\n"
            "not FAULT: exception 0xc0000005 at 7FFAB123 "
            "(C:\\Game\\version.dll + 0x20) thread 7, last job -1\n"
        )["segments"][0]

        self.assertEqual(segment["faults"], [])


if __name__ == "__main__":
    unittest.main()
