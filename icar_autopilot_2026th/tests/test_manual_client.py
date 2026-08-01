import importlib.util
import pathlib
import threading
import time
import unittest


MODULE_PATH = pathlib.Path(__file__).parents[1] / "src" / "tool" / "manual_client.py"
SPEC = importlib.util.spec_from_file_location("manual_client", MODULE_PATH)
manual_client = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manual_client)
CLIENT_SOURCE = MODULE_PATH.read_text(encoding="utf-8")


class FakeSocket:
    def __init__(self):
        self.sent = []
        self.lock = threading.Lock()

    def sendall(self, data):
        with self.lock:
            self.sent.append((time.monotonic(), data))


class CommandMailboxTests(unittest.TestCase):
    def test_priority_stop_is_delivered_before_latest_movement(self):
        mailbox = manual_client.CommandMailbox()
        mailbox.set_movement(b"W\n")
        mailbox.put_priority(b"STOP\n")

        self.assertEqual(mailbox.next_command(0), b"STOP\n")
        self.assertEqual(mailbox.next_command(0), b"W\n")

    def test_latest_movement_replaces_stale_movement(self):
        mailbox = manual_client.CommandMailbox()
        mailbox.set_movement(b"W\n")
        mailbox.set_movement(b"WD\n")

        self.assertEqual(mailbox.next_command(0), b"WD\n")
        self.assertIsNone(mailbox.next_command(0))


class PeriodicSenderTests(unittest.TestCase):
    def test_control_channel_sends_stop_immediately_on_startup(self):
        sock = FakeSocket()
        client = manual_client.ManualClient()
        client.sock = sock
        client.running = True

        client._start_sender()
        deadline = time.monotonic() + 0.2
        while not sock.sent and time.monotonic() < deadline:
            time.sleep(0.005)
        client._stop_sender()

        self.assertTrue(sock.sent)
        self.assertEqual(sock.sent[0][1], b"STOP\n")

    def test_held_key_is_resent_as_heartbeat(self):
        sock = FakeSocket()
        mailbox = manual_client.CommandMailbox()
        stop_event = threading.Event()
        sender = threading.Thread(
            target=manual_client.run_command_sender,
            args=(sock, mailbox, stop_event),
            kwargs={"heartbeat_interval": 0.03},
        )
        mailbox.set_movement(b"WA\n")
        sender.start()
        time.sleep(0.085)
        stop_event.set()
        mailbox.wake()
        sender.join(0.5)

        sent = [data for _, data in sock.sent]
        self.assertGreaterEqual(sent.count(b"WA\n"), 2)


class HeadlessClientTests(unittest.TestCase):
    def test_client_has_no_camera_rotation_or_opencv_window(self):
        self.assertNotIn("cv2.rotate", CLIENT_SOURCE)
        self.assertNotIn("cv2.namedWindow", CLIENT_SOURCE)
        self.assertNotIn("cv2.imshow", CLIENT_SOURCE)
        self.assertNotIn("cv2.waitKey", CLIENT_SOURCE)

    def test_client_has_no_image_protocol(self):
        self.assertNotIn("IMAGE_PREFIX", CLIENT_SOURCE)
        self.assertNotIn("_handle_image", CLIENT_SOURCE)


if __name__ == "__main__":
    unittest.main()
