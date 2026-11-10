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
        cls.probe = (ROOT / "include/mambo_radio_probe.h").read_text()
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

    def test_offline_radio_observation_import(self):
        for marker in ('mambo-observations', 'mambo-analyse', 'frequency_mhz',
                       'power_dbm', 'row.detected===true', 'rows.length>512',
                       'OFFLINE IMPORT ONLY'):
            self.assertIn(marker,self.main)

    def test_probe_is_explicitly_disabled_by_default(self):
        self.assertIn('#ifndef MAMBO_RADIO_HW_VERIFIED', self.probe)
        self.assertIn('out["error"]="hardware_probe_disabled"',self.probe)
        self.assertIn('out["transmission_enabled"]=false',self.probe)
        self.assertIn('MamboRadio::probeReadOnly(response,mamboRadioMode)',self.main)
        self.assertIn('server.on("/api/mambo/probe", HTTP_POST',self.main)
        self.assertIn('mambo-probe',self.main)
        self.assertIn('radio_probe',self.android)

    def test_receive_only_monitor_paths(self):
        self.assertIn('server.on("/api/mambo/observe", HTTP_POST',self.main)
        self.assertIn('server.on("/api/mambo/cc1101/rssi", HTTP_GET',self.main)
        self.assertIn('MamboRadio::observeChannels(response,mamboRadioMode',self.main)
        self.assertIn('MamboRadio::observeCc1101(response,mamboRadioMode)',self.main)
        self.assertIn('radio_observe',self.android)
        self.assertIn('cc1101_rssi',self.android)
        nrf=(ROOT / "include/mambo_radio_observe.h").read_text()
        cc=(ROOT / "include/mambo_cc1101_observe.h").read_text()
        for module in (nrf,cc):
            self.assertIn('#ifndef MAMBO_RADIO_HW_VERIFIED',module)
            self.assertIn('out["transmission_enabled"]=false',module)
            self.assertNotIn('0xA0',module) # no nRF TX payload command
        self.assertIn('firstChannel+count>126',nrf)
        self.assertIn('strobe(0x34)',cc) # SRX only
        self.assertNotIn('strobe(0x35)',cc) # no STX strobe

    def test_radio_gpio_flash_pins_excluded(self):
        self.assertIn('pin>=6 && pin<=11',self.radio)

    def test_owner_pinmap_and_boot_guard(self):
        pins=(ROOT / "include/mambo_radio_pinmap.h").read_text()
        probe=(ROOT / "include/mambo_radio_probe.h").read_text()
        expected={"CSN":26,"SCK":0,"MOSI":32,"MISO":33,"CONTROL":25}
        for name,value in expected.items():
            self.assertIn(f"#define MAMBO_RADIO_PIN_{name} {value}",pins)
        self.assertIn("MAMBO_GPIO0_BOOT_VERIFIED",pins)
        self.assertIn('#include "mambo_radio_pinmap.h"',probe)
        self.assertNotIn("-D MAMBO_RADIO_HW_VERIFIED", (ROOT / "platformio.ini").read_text())

    def test_ci_build(self):
        self.assertIn("pio run -e m5stick-cplus2", self.workflow)
        self.assertIn("actions/upload-artifact", self.workflow)

if __name__ == "__main__":
    unittest.main(verbosity=2)
