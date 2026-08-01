import pathlib
import unittest


ROOT = pathlib.Path(__file__).parents[1]
TRACK_CPP = (ROOT / "src" / "ctrl" / "track.cpp").read_text(encoding="utf-8")


class TrackSourceTests(unittest.TestCase):
    def test_start_block_selection_is_center_biased(self):
        self.assertIn("centerImage", TRACK_CPP)
        self.assertIn("distanceToCenter", TRACK_CPP)
        self.assertIn("indexStartBlock", TRACK_CPP)

    def test_start_block_threshold_accepts_fragmented_curve_or_crosswalk(self):
        self.assertIn("minStartBlockWidth", TRACK_CPP)
        self.assertNotIn("* 0.65", TRACK_CPP)
        self.assertIn("COLSIMAGE / 8", TRACK_CPP)


if __name__ == "__main__":
    unittest.main()
