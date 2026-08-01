import pathlib
import re
import unittest


WORKSPACE = pathlib.Path(__file__).resolve().parents[2]
PROJECT = WORKSPACE / "icar_autopilot_2026th"
MOTOR_C = WORKSPACE / "Code_GD32F1_CarDo" / "HARDWARE" / "Motor.c"
MOTOR_H = WORKSPACE / "Code_GD32F1_CarDo" / "HARDWARE" / "Motor.h"
ICAR_HPP = PROJECT / "include" / "icar.hpp"


class EncoderFaultPolicyTests(unittest.TestCase):
    def test_mcu_encoder_fault_waits_long_enough_for_high_friction_start(self):
        motor_h = MOTOR_H.read_text(encoding="utf-8")

        self.assertIn("MOTOR_ENCODER_FAULT_CYCLES", motor_h)
        match = re.search(r"#define\s+MOTOR_ENCODER_FAULT_CYCLES\s+(\d+)", motor_h)
        self.assertIsNotNone(match)
        self.assertGreaterEqual(int(match.group(1)), 150)

    def test_mcu_encoder_fault_uses_absolute_pwm_threshold(self):
        motor_c = MOTOR_C.read_text(encoding="utf-8")

        self.assertIn("pwmAbs", motor_c)
        self.assertIn("MOTOR_ENCODER_FAULT_PWM_THRESHOLD", motor_c)
        self.assertNotIn("PwmOutput > 100", motor_c)

    def test_edgeboard_encoder_fault_log_is_rate_limited_and_stops_command(self):
        icar = ICAR_HPP.read_text(encoding="utf-8")

        self.assertIn("lastEncoderFaultWarn", icar)
        self.assertIn("params->ctrl.speed = 0.0f", icar)
        self.assertNotIn('printf("[ERROR] Encoder fault detected! errorCode=0x%04X\\n",\n                       params->ctrl.errorCode);', icar)


if __name__ == "__main__":
    unittest.main()
