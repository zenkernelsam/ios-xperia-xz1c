"""Preserve deferred replies while refusing malformed original gate evidence."""

import unittest
from poster_snapshot_gate_evidence import inspect

TOKEN = "123456789abcdef0"


def fixture():
    apis = ("cutout", "max_cutout", "background", "floating", "vista")
    lines = ["SWEET LOOP LIVE proto=1 token=" + TOKEN]
    for api in apis:
        argc = 4 if api in ("cutout", "max_cutout", "vista") else 2
        lines.append(f"SWEET PSG METHOD pid=7 api={api} present=1 owner=1 installed=1 argc={argc} exact_ABI=1")
    lines += [
        "SWEET PSG CONFIG pid=7 revision=1 installed=5 expected=5 per_api=8 max_calls=40 max_replies=64 extra_getters=0 GPU_work=0",
        "SWEET PSG BEGIN pid=7 call=1 api=cutout receiver=0000000000000100 argument=0000000000000200 original_forwarded=1",
        "SWEET PSG REQUEST pid=7 call=1 orientation=0000000100000001 callback_nil=0",
        "SWEET PSG END pid=7 call=1 api=cutout result=0000000000000000 outcome=return original_behavior=1",
        "SWEET PSG REPLY pid=7 call=1 reply=1 original_forwarded=1",
        "SWEET PSG RECT0 pid=7 call=1 reply=1 x=8000000000000000 y=3ff0000000000000",
        "SWEET PSG RECT1 pid=7 call=1 reply=1 w=4076800000000000 h=4084000000000000",
        "SWEET PSG REPLY_END pid=7 call=1 reply=1 outcome=return original_behavior=1",
        "SWEET PSG BEGIN pid=7 call=2 api=background receiver=0000000000000300 argument=0000000000000000 original_forwarded=1",
        "SWEET PSG END pid=7 call=2 api=background result=0000000000000000 outcome=return original_behavior=1",
        "SWEET LOOP ACK FASTBOOT token=" + TOKEN,
        "SWEET PSG REPLY pid=7 call=1 reply=2 original_forwarded=1",
    ]
    return "\n".join(lines)


class PosterGateEvidence(unittest.TestCase):
    def test_deferred_reply_preserves_original_bits_and_exit_period(self):
        report = inspect(fixture(), TOKEN)
        self.assertEqual((report["complete_calls"], report["complete_replies"]), (2, 1))
        self.assertEqual(report["replies"][0]["rectangle_bits"]["x"], "8000000000000000")
        self.assertFalse(report["pending_original_cutout_requests"])

    def test_absent_reply_is_preserved_not_fabricated(self):
        text = "\n".join(line for line in fixture().splitlines() if not line.startswith((
            "SWEET PSG REPLY ", "SWEET PSG REPLY_END ", "SWEET PSG RECT")))
        report = inspect(text, TOKEN)
        self.assertEqual(report["complete_replies"], 0)
        self.assertEqual(len(report["pending_original_cutout_requests"]), 1)

    def test_truncation_foreign_call_and_identity_changes_are_refused(self):
        text = fixture()
        changes = [
            text.replace("SWEET PSG RECT1 pid=7 call=1 reply=1 w=4076800000000000 h=4084000000000000\n", ""),
            text.replace("reply=1 outcome=return", "reply=3 outcome=return"),
            text.replace("pid=7 call=1 reply=1", "pid=8 call=1 reply=1"),
            text.replace("callback_nil=0", "callback_nil=1"),
            text.replace("installed=5 expected=5", "installed=4 expected=5"),
            text.replace("api=cutout present=1 owner=1 installed=1 argc=4", "api=cutout present=1 owner=1 installed=1 argc=3"),
            text.replace("receiver=0000000000000100", "receiver=0000000000000000"),
            text.replace("call=2 api=background", "call=3 api=background"),
        ]
        for changed in changes:
            with self.subTest(change=changed), self.assertRaises(ValueError):
                inspect(changed, TOKEN)

    def test_original_image_path_bytes_and_void_setter_are_bound(self):
        text = fixture()
        config = "SWEET PSG CONFIG pid=7 revision=1 installed=5 expected=5 per_api=8 max_calls=40 max_replies=64 extra_getters=0 GPU_work=0"
        text = text.replace(config,
            "SWEET PSG METHOD pid=7 api=image_load present=1 owner=1 installed=1 argc=3 exact_ABI=1\n"
            "SWEET PSG METHOD pid=7 api=image_set present=1 owner=1 installed=1 argc=3 exact_ABI=1\n"
            "SWEET PSG CONFIG pid=7 revision=2 installed=7 expected=7 per_api=8 max_calls=56 max_replies=64 scene_getters=0 path_metadata=1 GPU_work=0")
        original = (
            "SWEET PSG BEGIN pid=7 call=3 api=image_load receiver=0000000000000400 argument=0000000000000500 original_forwarded=1\n"
            "SWEET PSG PATH pid=7 call=3 bytes=3 truncated=0 getter_error=0 pixels_read=0\n"
            "SWEET PSG PATH_DATA pid=7 call=3 offset=0 bytes=3 hex=2f782f\n"
            "SWEET PSG END pid=7 call=3 api=image_load result=0000000000000600 outcome=return original_behavior=1\n")
        text = text.replace("SWEET LOOP ACK FASTBOOT", original + "SWEET LOOP ACK FASTBOOT", 1)
        report = inspect(text, TOKEN)
        self.assertEqual(report["revision"], 2)
        self.assertEqual(report["calls"][-1]["path"]["utf8"], "/x/")
        for changed in (text.replace("offset=0 bytes=3 hex=2f782f", "offset=1 bytes=3 hex=2f782f"),
                        text.replace("bytes=3 truncated=0", "bytes=4 truncated=0"),
                        text.replace("bytes=3 hex=2f782f", "bytes=3 hex=2f78")):
            with self.assertRaises(ValueError):
                inspect(changed, TOKEN)


if __name__ == "__main__":
    unittest.main()
