import unittest

from peripheral_log_evidence import summarize


TOKEN = "0123456789abcdef"
LIVE = b"SWEET LOOP LIVE proto=1 token=" + TOKEN.encode() + b" mode=graceful pon=0x06"
ACK = b"SWEET LOOP ACK FASTBOOT token=" + TOKEN.encode() + b" mode=graceful pon=0x06"
BACKLIGHT = (b'IOK backboardd: matching1 <dict ID="0"><key>IOPropertyMatch</key><dict ID="1">'
             b'<key>backlight-control</key><true/></dict></dict> -> none')
WLAN = b'IOK SpringBoard: matching <dict><key>IONameMatch</key><string>wlan</string></dict> -> 0 found'


class PeripheralLogEvidence(unittest.TestCase):
    def test_current_boot_only(self):
        raw = b"\r\n".join([
            b"exec bluetoothd", LIVE, b"SWEET FB: display off", BACKLIGHT,
            b"IOK backboardd: path IODeviceTree:/backlight -> none", WLAN,
            b"exec mediaserverd", b"exec bluetoothd", b"exec bluetoothuserd",
            ACK, b"SWEET FB: display on", b"exec wifid",
        ])
        report = summarize(raw, TOKEN)
        self.assertEqual(report["display_power_requests"], [{"line": 3, "requested": "off"}])
        self.assertEqual([row["line"] for row in report["backlight_lookup_misses"]], [4, 5])
        self.assertEqual(report["wlan_lookup_misses"], [
            {"line": 6, "caller": "SpringBoard", "query": "service_match", "result": "0 found"},
        ])
        self.assertEqual(report["daemon_exec_lines"], {"mediaserverd": [7], "bluetoothd": [8], "wifid": []})

    def test_does_not_infer_from_unrelated_matching_or_daemon_names(self):
        raw = b"\n".join([
            LIVE, BACKLIGHT.replace(b"-> none", b"-> 1 found"),
            BACKLIGHT.replace(b"<true/>", b"<false/>"), WLAN.replace(b"wlan", b"wlansim"),
            b"prefix exec bluetoothd", b"exec bluetoothd extra", ACK,
        ])
        report = summarize(raw, TOKEN)
        self.assertEqual(report["backlight_lookup_misses"], [])
        self.assertEqual(report["wlan_lookup_misses"], [])
        self.assertEqual(report["daemon_exec_lines"]["bluetoothd"], [])

    def test_bad_current_boot_boundaries(self):
        for lines in ([LIVE], [ACK, LIVE], [LIVE, LIVE, ACK], [LIVE, ACK, ACK],
                      [LIVE.replace(TOKEN.encode(), b"1000000000000000"), ACK]):
            with self.subTest(lines=lines), self.assertRaises(ValueError):
                summarize(b"\n".join(lines), TOKEN)


if __name__ == "__main__":
    unittest.main()
