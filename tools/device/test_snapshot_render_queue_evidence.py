import unittest

from snapshot_render_queue_evidence import inspect


class OriginalRenderQueueEvidence(unittest.TestCase):
    def test_complete_error_and_strict_rejection(self):
        token = "123456789abcdef0"
        text = "\n".join((
            "SWEET LOOP LIVE proto=1 token=" + token,
            "SWEET RQUEUE CONFIG pid=7 revision=1 installed=1 limit=8 original_arguments_results=1 extra_GPU_work=0",
            "SWEET RQUEUE DRAW pid=7 call=1 primitive=4 first=0 count=4 instances=1",
            "SWEET RQUEUE BIND pid=7 call=1 vertex=3 fragment=1 valid=1 original_state=1",
            "SWEET RQUEUE PIPE pid=7 call=1 vertex=author_vertex fragment=author_fragment",
            "SWEET RQUEUE DRAW_END pid=7 call=1 outcome=return original_behavior=1",
            "SWEET RQUEUE RESULT pid=7 call=1 status=5 error_nil=0 code=9 terminal_getters_only=1",
            "SWEET RQUEUE ERROR pid=7 call=1 text=AIR_render_failed_result=-22",
            "SWEET LOOP ACK FASTBOOT token=" + token,
        ))
        report = inspect(text, token)
        self.assertEqual(report["failed_commands"], 1)
        self.assertEqual(report["draws"][0]["fragment_mask"], 1)
        for changed in (text.replace("BIND pid=7", "BIND pid=8"), text.replace("valid=1", "valid=0"),
                        text.replace("status=5", "status=3"), text.replace("ERROR pid=7", "ERROR pid=8"),
                        text.replace("error_nil=0", "error_nil=1"),
                        text.replace("SWEET RQUEUE PIPE", "BROKEN PIPE"),
                        text.replace("extra_GPU_work=0", "extra_GPU_work=1"),
                        text.replace("vertex=3 fragment=1", "vertex=256 fragment=1")):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                inspect(changed, token)
        complete=text.replace("status=5 error_nil=0 code=9","status=4 error_nil=1 code=0")
        complete=complete.replace("text=AIR_render_failed_result=-22",
            "text=none\nSWEET RQUEUE RETIRED pid=7 call=1 fence=123 operations=5 complete=1 original_command=1")
        self.assertEqual(inspect(complete,token,require_retirement=True)["results"][0]["operations"],5)
        for changed in (complete.replace("fence=123","fence=0"),complete.replace("operations=5","operations=0"),
                        complete.replace("RETIRED pid=7","RETIRED pid=8"),complete.replace("complete=1","complete=0")):
            with self.assertRaises(ValueError):inspect(changed,token,require_retirement=True)


if __name__ == "__main__":
    unittest.main()
