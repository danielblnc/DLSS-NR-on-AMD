import unittest

from tools.summarize_log import summarize


BANNER = "dlssnr_amd v0.2.15 (build f47996ae) loaded"


class FsrObservationTests(unittest.TestCase):
    def test_generic_graphics_hooks_are_not_fsr_evidence(self):
        segment = summarize(
            f"{BANNER}\n"
            "hooked dxgi.dll!Present\n"
            "hooked d3d12.dll!ExecuteCommandLists\n"
            "hooked game.exe!Present\n"
        )["segments"][0]

        self.assertFalse(segment["fsr_hook_observed"])
        self.assertFalse(segment["fsr_dispatch_observed"])

    def test_known_fidelityfx_hook_variants_are_recognized(self):
        variants = (
            "amd_fidelityfx_dx12.dll!ffxDispatch",
            "amd_fidelityfx_upscaler_dx12.dll!ffxCreateContext",
            "ffx_fsr3upscaler_x64.dll!ffxDispatch",
        )

        for variant in variants:
            with self.subTest(variant=variant):
                segment = summarize(f"{BANNER}\nhooked {variant}\n")["segments"][0]

                self.assertTrue(segment["fsr_hook_observed"])
                self.assertFalse(segment["fsr_dispatch_observed"])

    def test_hook_presence_does_not_imply_dispatch_or_completed_job(self):
        segment = summarize(
            f"{BANNER}\n"
            "hooked amd_fidelityfx_dx12.dll!ffxDispatch\n"
        )["segments"][0]

        self.assertTrue(segment["fsr_hook_observed"])
        self.assertFalse(segment["fsr_dispatch_observed"])
        self.assertEqual(segment["completed_job_samples"], 0)
        self.assertIsNone(segment["job_time_ms"])

    def test_explicit_dispatch_record_is_separate_dispatch_evidence(self):
        segment = summarize(
            f"{BANNER}\n"
            "hooked amd_fidelityfx_upscaler_dx12.dll!ffxDispatch\n"
            "first ffxDispatch type 0x10001\n"
        )["segments"][0]

        self.assertTrue(segment["fsr_hook_observed"])
        self.assertTrue(segment["fsr_dispatch_observed"])
        self.assertEqual(segment["completed_job_samples"], 0)

    def test_counters_and_unknown_hook_names_do_not_infer_active_fsr(self):
        segment = summarize(
            f"{BANNER}\n"
            "hooked amd_fidelityfx_dx12.dllx!ffxDispatch\n"
            "hooked amd_fidelityfx_dx12.dll!ffxDispatchExtra\n"
            "frames 120 dispatches 8 submitted 8 ready 8 route backbuffer\n"
        )["segments"][0]

        self.assertFalse(segment["fsr_hook_observed"])
        self.assertFalse(segment["fsr_dispatch_observed"])
        self.assertEqual(
            segment["last_counters"],
            {"frames": 120, "dispatches": 8, "submitted": 8, "ready": 8, "route": "backbuffer"},
        )


if __name__ == "__main__":
    unittest.main()
