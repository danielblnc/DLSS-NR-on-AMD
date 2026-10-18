import unittest

from tools.summarize_log import summarize


BANNER = "dlssnr_amd v0.2.15 (build f47996ae) loaded"


class NumericObservationTests(unittest.TestCase):
    def test_odd_timing_samples_include_zero_duration(self):
        segment = summarize(
            f"{BANNER}\n"
            "network job 1 done in 0 ms\n"
            "network job 2 done in 3.5 ms\n"
            "network job 3 done in 8 ms\n"
        )["segments"][0]

        self.assertEqual(segment["completed_job_samples"], 3)
        self.assertEqual(
            segment["job_time_ms"], {"min": 0, "median": 3.5, "max": 8}
        )

    def test_repeated_cumulative_counter_records_keep_last_observation(self):
        segment = summarize(
            f"{BANNER}\n"
            "frames 10 dispatches 2 submitted 2 ready 1 route fsr\n"
            "frames 25 dispatches 7 submitted 7 ready 6 route fsr\n"
        )["segments"][0]

        self.assertEqual(
            segment["last_counters"],
            {"frames": 25, "dispatches": 7, "submitted": 7, "ready": 6, "route": "fsr"},
        )

    def test_counter_reset_is_kept_as_last_observation(self):
        segment = summarize(
            f"{BANNER}\n"
            "frames 100 dispatches 40 submitted 40 ready 39 route fsr\n"
            "frames 4 dispatches 1 submitted 1 ready 0 route fsr\n"
        )["segments"][0]

        self.assertEqual(
            segment["last_counters"],
            {"frames": 4, "dispatches": 1, "submitted": 1, "ready": 0, "route": "fsr"},
        )

    def test_repeated_identical_timing_lines_count_as_observed_samples(self):
        segment = summarize(
            f"{BANNER}\n"
            "network job 1 done in 5 ms\n"
            "network job 1 done in 5 ms\n"
        )["segments"][0]

        self.assertEqual(segment["completed_job_samples"], 2)
        self.assertEqual(segment["job_time_ms"], {"min": 5, "median": 5, "max": 5})

    def test_oversized_numeric_tokens_are_not_truncated_into_matches(self):
        segment = summarize(
            f"{BANNER}\n"
            "env: HIP: 1 device(s), driver 70260201, runtime 1234567890123\n"
            "network job 1 done in 12.1234567890123 ms\n"
        )["segments"][0]

        self.assertIsNone(segment["hip_runtime"])
        self.assertEqual(segment["completed_job_samples"], 0)
        self.assertIsNone(segment["job_time_ms"])


if __name__ == "__main__":
    unittest.main()
