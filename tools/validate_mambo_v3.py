#!/usr/bin/env python3
"""Fast, dependency-free integration checks before the PlatformIO build."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class MamboIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main = (ROOT / "src/main.cpp").read_text()
        cls.radio = (ROOT / "include/mambo_radio_lab.h").read_text()
        cls.workflow = (ROOT / ".github/workflows/mambo-v3-validation.yml").read_text()

    def test_radio_routes_registered(self):
        for route in ("/api/mambo/radio", "/api/mambo/sub/inspect"):
            self.assertIn('server.on("'+route+'"', self.main)

    def test_radio_page_is_present(self):
        self.assertIn("RADIO LAB", self.main)
        self.assertIn('mambo-mode', self.main)

    def test_one_radio_at_a_time(self):
        self.assertIn('Module::None', self.radio)
        self.assertIn('Module::CC1101', self.radio)
        self.assertIn('Module::NRF24', self.radio)
        self.assertIn('digitalWrite(p.control,LOW)', self.radio)

    def test_transmission_disabled_in_radio_lab(self):
        self.assertIn('d["transmission_enabled"]=false', self.main)
        self.assertNotIn('esp_wifi_80211_tx', self.radio)

    def test_ci_build(self):
        self.assertIn("pio run -e m5stick-cplus2", self.workflow)
        self.assertIn("actions/upload-artifact", self.workflow)

if __name__ == "__main__":
    unittest.main(verbosity=2)
