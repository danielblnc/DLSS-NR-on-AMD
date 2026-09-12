import json
import unittest

from tools.summarize_log import summarize


BANNER = "dlssnr_amd v0.2.15 (build f47996ae) loaded"


class OutputBoundaryTests(unittest.TestCase):
    def test_repeated_fault_signature_is_emitted_once(self):
        text = (
            f"{BANNER}\n"
            "FAULT: exception 0xc0000005 at 0x111 (C:\\Games\\amdhip64.dll + 0x20) "
            "thread 7, last job -1\n"
            "FAULT: exception 0xc0000005 at 0x222 (C:\\Games\\amdhip64.dll + 0x20) "
            "thread 99, last job -1\n"
            "FAULT: exception 0xc0000005 at 0x333 (C:\\Games\\amdhip64.dll + 0x20) "
            "thread 7, last job -1\n"
        )

        faults = summarize(text)["segments"][0]["faults"]

        self.assertEqual(faults, [{"code": "0xc0000005", "module": "amdhip64.dll", "offset": "0x20", "last_job": -1}])

    def test_faults_are_bounded_to_64_distinct_signatures_per_segment(self):
        lines = [
            f"FAULT: exception 0x{index:x} at 0xabc (C:\\Games\\module_{index}.dll + 0x{index + 1:x}) "
            f"thread 1, last job {index}"
            for index in range(70)
        ]

        faults = summarize(f"{BANNER}\n" + "\n".join(lines))["segments"][0]["faults"]

        self.assertEqual(len(faults), 64)
        self.assertEqual([fault["last_job"] for fault in faults], list(range(64)))

    def test_unrecognized_lines_with_sensitive_data_are_not_echoed(self):
        text = (
            f"{BANNER}\n"
            "unrelated note: C:\\Users\\synthetic.person\\Documents\\private\\notes.txt "
            "person@example.invalid sk-synthetic-secret-token-123\n"
            "another ignored record with no recognized diagnostic fields\n"
        )

        output = json.dumps(summarize(text))

        self.assertNotIn("synthetic.person", output)
        self.assertNotIn("person@example.invalid", output)
        self.assertNotIn("sk-synthetic-secret-token-123", output)

    def test_fault_module_is_only_the_unicode_parenthesized_basename(self):
        segment = summarize(
            f"{BANNER}\n"
            "FAULT: exception 0xc0000005 at 0xabc "
            "(C:\\Games\\测试 (nightly)\\版本 (测试).dll + 0x20) thread 7, last job -1\n"
        )["segments"][0]

        self.assertEqual(segment["faults"][0]["module"], "版本 (测试).dll")
        self.assertNotIn("\\", segment["faults"][0]["module"])
        self.assertNotIn("nightly", json.dumps(segment["faults"][0]))

    def test_repeated_warnings_are_fixed_labels_without_raw_tails(self):
        warning_lines = []
        for label in (
            "invalid kernel file",
            "unsupported colour format",
            "too many frames in flight",
            "GPU wait timeout",
            "output is zero",
        ):
            warning_lines.extend(
                f"WARNING: {label}; detail=C:\\Users\\synthetic\\private.txt "
                "email=person@example.invalid token=synthetic-token"
                for _ in range(40)
            )

        warnings = summarize(f"{BANNER}\n" + "\n".join(warning_lines))["segments"][0]["warnings"]

        self.assertEqual(
            warnings,
            [
                "invalid kernel file",
                "unsupported colour format",
                "too many frames in flight",
                "GPU wait timeout",
                "zero-output warning",
            ],
        )
        warning_json = json.dumps(warnings)
        self.assertNotIn("synthetic", warning_json)
        self.assertNotIn("person@example.invalid", warning_json)


if __name__ == "__main__":
    unittest.main()
