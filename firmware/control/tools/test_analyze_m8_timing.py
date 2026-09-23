import unittest

from analyze_m8_timing import analyze_lines, summarize


class AnalyzeM8TimingTest(unittest.TestCase):
    def test_pairs_touch_toggle_and_transport_response(self):
        lines = [
            "I sc_trace: ts_us=100000 task=ui_touch evt=evt_emit v=1\n",
            "I sc_trace: ts_us=100900 task=app_core evt=evt_rx v=1\n",
            "I audio_link: response=PLAY seq=7 latency_us=3300 rx_ts_us=112000\n",
            "I sc_trace: ts_us=200000 task=ui_touch evt=evt_emit v=1\n",
            "I sc_trace: ts_us=201100 task=app_core evt=evt_rx v=1\n",
            "I audio_link: response=STOP seq=8 latency_us=4100 rx_ts_us=216000\n",
        ]
        result = analyze_lines(lines)
        self.assertEqual(result.touch_accept_us, (900, 1100))
        self.assertEqual(result.command_ack_us, (12000, 16000))
        self.assertEqual(result.transport_rtt_us, (3300, 4100))
        self.assertEqual(result.command_names, ("PLAY", "STOP"))

    def test_uses_latest_coalesced_volume_event(self):
        lines = [
            "ts_us=300000 task=ui_touch evt=evt_emit v=4\n",
            "ts_us=300500 task=app_core evt=evt_rx v=4\n",
            "ts_us=302000 task=ui_touch evt=evt_emit v=4\n",
            "ts_us=302600 task=app_core evt=evt_rx v=4\n",
            "response=SET_VOLUME seq=9 latency_us=2500 rx_ts_us=309000\n",
        ]
        result = analyze_lines(lines)
        self.assertEqual(result.touch_accept_us, (500, 600))
        self.assertEqual(result.command_ack_us, (7000,))
        self.assertEqual(result.transport_rtt_us, (2500,))

    def test_ignores_ansi_and_reports_summary(self):
        result = analyze_lines([
            "\x1b[0;32mts_us=1 task=ui_touch evt=evt_emit v=1\x1b[0m\n",
            "ts_us=11 task=app_core evt=evt_rx v=1\n",
        ])
        self.assertEqual(summarize(result.touch_accept_us)["max_us"], 10)


if __name__ == "__main__":
    unittest.main()
