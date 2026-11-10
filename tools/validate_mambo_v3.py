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
        cls.diagnostics = (ROOT / "include/mambo_radio_diagnostics.h").read_text()
        cls.android = (ROOT / "android/app/src/main/java/uk/skb/mambo/MainActivity.java").read_text()
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

    def test_preferences_declared_before_radio_handlers(self):
        self.assertLess(self.main.index("static Preferences skbPrefs;"), self.main.index("static void mamboRadioSelect()"))
        self.assertIn('skbPrefs.putString("mambo_radio",mode)', self.main)

    def test_shared_usb_and_http_radio_diagnostics(self):
        self.assertIn('MamboRadio::fillDiagnostics(d,mamboRadioMode)', self.main)
        self.assertIn('MamboRadio::fillDiagnostics(response,mamboRadioMode)', self.main)
        self.assertIn('server.on("/api/mambo/diagnostics", HTTP_GET', self.main)
        for marker in ('doc["hardware_detected"]=false', 'doc["transmission_enabled"]=false', 'doc["swap_requires_power_off"]=true'):
            self.assertIn(marker, self.diagnostics)

    def test_esp32_input_only_pins_not_driven(self):
        self.assertIn('if(p.sck>=34 || p.mosi>=34 || p.csn>=34) return false;', self.radio)
        self.assertIn('if(p.control>=34) return false;', self.radio)

    def test_android_usb_and_no_voice(self):
        for marker in ('requestPermission(', 'reconnectCheck', 'handleResponse(', 'radio_select', 'UsbSerialProber'):
            self.assertIn(marker,self.android)
        self.assertNotIn('TextToSpeech',self.android)
        self.assertNotIn('voice.speak',self.android)

    def test_ci_build(self):
        self.assertIn("pio run -e m5stick-cplus2", self.workflow)
        self.assertIn("actions/upload-artifact", self.workflow)

if __name__ == "__main__":
    unittest.main(verbosity=2)
