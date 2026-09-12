import unittest

from tools.summarize_log import summarize


class BannerFormatTests(unittest.TestCase):
    def test_crlf_and_blank_lines_keep_each_banner_scoped(self):
        text = (
            "startup preamble\r\n\r\n"
            "dlssnr_amd v0.2.17 (build crlf-a) loaded\r\n\r\n"
            "engine init ok\r\n"
            "network job 1 done in 1 ms\r\n\r\n"
            "dlssnr_amd loaded\r\n\r\n"
            "first ffxDispatch type 0x10001\r\n"
            "network job 2 done in 2 ms\r\n"
        )

        segments = summarize(text)["segments"]

        self.assertEqual(len(segments), 2)
        self.assertTrue(segments[0]["engine_initialized"])
        self.assertEqual(segments[0]["completed_job_samples"], 1)
        self.assertTrue(segments[1]["fsr_dispatch_observed"])
        self.assertEqual(segments[1]["completed_job_samples"], 1)

    def test_v0218_and_legacy_headers_keep_version_and_build_separate(self):
        text = (
            "dlssnr_amd v0.2.18 (build 20260912-rc1) loaded\n"
            "network job 1 done in 1 ms\n"
            "dlssnr_amd loaded\n"
            "network job 2 done in 2 ms\n"
        )

        segments = summarize(text)["segments"]

        self.assertEqual(len(segments), 2)
        self.assertEqual(segments[0]["version"], "v0.2.18")
        self.assertEqual(segments[0]["build"], "20260912-rc1")
        self.assertIsNone(segments[1]["version"])
        self.assertIsNone(segments[1]["build"])

    def test_recognized_preamble_diagnostics_before_first_banner_are_ignored(self):
        text = (
            "engine init ok\n"
            "network job 99 done in 99 ms\n"
            "WARNING: invalid kernel file\n"
            "FAULT: exception 0xc0000005 at 0x1234 "
            "(C:\\Windows\\amdhip64.dll + 0x40) thread 1, last job 99\n"
            "dlssnr_amd v0.2.18 (build post-preamble) loaded\n"
            "network job 1 done in 1 ms\n"
        )

        segment = summarize(text)["segments"][0]

        self.assertFalse(segment["engine_initialized"])
        self.assertEqual(segment["completed_job_samples"], 1)
        self.assertEqual(segment["warnings"], [])
        self.assertEqual(segment["faults"], [])

    def test_unknown_banner_metadata_still_starts_its_own_segment(self):
        unknown_banners = (
            "dlssnr_amd v0.2.18 loaded",
            "dlssnr_amd v0.2.18 build broken loaded",
            "dlssnr_amd v0.2.18 (build) loaded",
            "dlssnr_amd v0.2.18 (build bad value!) loaded",
        )

        for unknown_banner in unknown_banners:
            with self.subTest(unknown_banner=unknown_banner):
                text = (
                    "dlssnr_amd v0.2.17 (build old) loaded\n"
                    "network job 1 done in 10 ms\n"
                    f"{unknown_banner}\n"
                    "network job 2 done in 20 ms\n"
                    "dlssnr_amd v0.2.19 (build new) loaded\n"
                    "network job 3 done in 30 ms\n"
                )

                segments = summarize(text)["segments"]

                self.assertEqual(len(segments), 3)
                self.assertEqual(
                    [segment["completed_job_samples"] for segment in segments],
                    [1, 1, 1],
                )
                self.assertIsNone(segments[1]["version"])
                self.assertIsNone(segments[1]["build"])


if __name__ == "__main__":
    unittest.main()
