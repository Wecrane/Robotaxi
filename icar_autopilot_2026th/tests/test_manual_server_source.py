import pathlib
import unittest


ROOT = pathlib.Path(__file__).parents[1]
CPP = (ROOT / "src" / "fsm" / "manualControl.cpp").read_text(encoding="utf-8")
HEADER = (ROOT / "include" / "fsm" / "manualControl.hpp").read_text(encoding="utf-8")


class ManualServerSafetyTests(unittest.TestCase):
    def test_transport_uses_complete_writes(self):
        self.assertIn("bool ManualControlThread::sendAll", CPP)

    def test_connection_threads_are_joined_not_detached(self):
        self.assertNotIn(".detach()", CPP)
        self.assertIn("cmdThread.join()", CPP)

    def test_command_watchdog_is_subsecond(self):
        self.assertIn("elapsed > 300)", CPP)
        self.assertNotIn("elapsed > 30000", CPP)

    def test_disconnect_clears_actual_manual_controls(self):
        self.assertIn("void ManualControlThread::clearManualControl", CPP)
        self.assertIn("clearManualControl(true)", CPP)

    def test_watchdog_timestamp_is_atomic(self):
        self.assertIn("std::atomic<int64_t> lastContactMs", HEADER)

    def test_manual_control_is_not_logged_every_frame(self):
        self.assertNotIn("[Manual] Applying manual control", CPP)

    def test_manual_steering_uses_calibrated_extremes(self):
        self.assertIn("*steering = PWMSERVOMAX", CPP)
        self.assertIn("*steering = PWMSERVOMIN", CPP)
        self.assertNotIn("PWMSERVOMID - 300", CPP)
        self.assertNotIn("PWMSERVOMID + 300", CPP)

    def test_server_has_no_manual_camera_transport(self):
        self.assertNotIn("IMAGE:", CPP)
        self.assertNotIn("imencode", CPP)
        self.assertNotIn("sendImage", HEADER)


if __name__ == "__main__":
    unittest.main()
