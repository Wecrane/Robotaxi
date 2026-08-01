import pathlib
import unittest


ROOT = pathlib.Path(__file__).parents[1]
ICAR = (ROOT / "include" / "icar.hpp").read_text(encoding="utf-8")
SHOW = (ROOT / "include" / "utils" / "show.hpp").read_text(encoding="utf-8")


class LivePreviewSourceTests(unittest.TestCase):
    def test_live_preview_uses_four_panel_compositor(self):
        self.assertIn('make_shared<Show>(4, "ICAR Live", true)', ICAR)
        self.assertNotIn('cv::imshow("Camera", img)', ICAR)

    def test_live_preview_contains_recommended_panels(self):
        for panel in ('"Original"', '"Binary"', '"Track"', '"Control"'):
            self.assertIn(panel, ICAR)

    def test_live_compositor_has_no_debug_frame_controls(self):
        self.assertIn("bool liveMode", SHOW)
        self.assertIn("if (!liveMode)", SHOW)

    def test_live_composite_has_no_orientation_rotation(self):
        self.assertNotIn("ROTATE_90_", SHOW)
        self.assertIn("predeal->correction(img)", ICAR)

    def test_camera_uses_v4l2_instead_of_failed_gstreamer_pipeline(self):
        self.assertIn("cv::CAP_V4L2", ICAR)

    def test_per_frame_manual_logs_are_removed(self):
        self.assertNotIn('cout << "[Icar] Manual takeover active."', ICAR)

    def test_stall_warning_is_rate_limited(self):
        self.assertIn("lastStallWarn", ICAR)

    def test_manual_takeover_does_not_publish_camera_frames(self):
        self.assertNotIn("sendImage", ICAR)


if __name__ == "__main__":
    unittest.main()
