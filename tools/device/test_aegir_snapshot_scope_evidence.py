import unittest
from aegir_snapshot_scope_evidence import inspect


class SnapshotScopeEvidence(unittest.TestCase):
    def test_exact_original_namespace_and_errors(self):
        token = "123456789abcdef0"
        spec = dict(identity="a"*64, component="sweet-aegir-"+"a"*32)
        text = (
            "SWEET LOOP LIVE proto=1 token=" + token + "\n"
            "SWEET ASCOPE ID pid=7 value=" + spec["identity"] + "\n"
            "SWEET ASCOPE CONFIG pid=7 revision=1 installed=1 caller=00000000000063b0 old_files_preserved=1 other_calls_original=1\n"
            "SWEET ASCOPE CALL pid=7 call=1 result=0 original_caller=1 old_files_preserved=1\n"
            "SWEET ASCOPE ROOT pid=7 call=1 path=/var/mobile/Library/Caches/" + spec["component"] + " mode=0700\n"
            "SWEET LOOP ACK FASTBOOT token=" + token)
        self.assertEqual(inspect(text, token, spec)["complete_calls"], 1)
        for changed in (text.replace("result=0", "result=-1"), text.replace("mode=0700", "mode=0777"),
                        text.replace("call=1 path=", "call=2 path="),
                        text.replace(spec["identity"], "b"*64), text.replace("/var/mobile/", "/foreign/"),
                        text.replace("caller=00000000000063b0", "caller=00000000000063b4")):
            with self.assertRaises(ValueError):
                inspect(changed, token, spec)


if __name__ == "__main__":
    unittest.main()
