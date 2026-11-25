import unittest

from snapshot_readback_evidence import inspect


class OriginalReadbackEvidence(unittest.TestCase):
    def test_counts_samples_and_original_return_binding(self):
        token="123456789abcdef0"
        text="\n".join((
            "SWEET LOOP LIVE proto=1 token="+token,
            "SWEET POSTDRAW END pid=7 tid=1 call=14 api=read wall_ns=1 cpu_ns=1 cpu_valid=1 outcome=return bytes=0 returned_nil=0 original_behavior=1",
            "SWEET RBYTES BEGIN pid=7 call=14 revision=1 width=3 height=2 row=16 format=555 original_getBytes=1",
            "SWEET RBYTES COUNT pid=7 call=14 pixels=6 black=4 nonblack=2 zero_words=1",
            "SWEET RBYTES RANGE pid=7 call=14 rmin=0 rmax=1023 gmin=0 gmax=1023 bmin=0 bmax=1023",
            "SWEET RBYTES PIXEL pid=7 call=14 index=0 x=0 y=0 word=d8060180",
            "SWEET RBYTES PIXEL pid=7 call=14 index=1 x=1 y=1 word=d8060180",
            "SWEET RBYTES PIXEL pid=7 call=14 index=2 x=2 y=1 word=d8060180",
            "SWEET RBYTES END pid=7 call=14 logical_bytes=24 fnv=123456789abcdef0 original_bytes_not_changed=1",
            "SWEET LOOP ACK FASTBOOT token="+token,
        ))
        self.assertEqual(inspect(text,token)["records"][0]["count"]["nonblack"],2)
        cg=text.replace("SWEET RBYTES ","SWEET CGBYTES ").replace("original_getBytes=1","original_CGImage=1")
        cg=cg.replace(
            "SWEET POSTDRAW END pid=7 tid=1 call=14 api=read wall_ns=1 cpu_ns=1 cpu_valid=1 outcome=return bytes=0 returned_nil=0 original_behavior=1",
            "SWEET POSTDRAW BEGIN pid=7 tid=1 call=14 root=14 depth=1 api=png at_ns=1 original_forwarded=0")
        self.assertEqual(inspect(cg,token,source="cgimage")["records"][0]["count"]["black"],4)
        with self.assertRaises(ValueError):inspect(text,token,source="cgimage")
        for changed in (text.replace("black=4","black=3"),text.replace("row=16","row=8"),
                        text.replace("call=14 revision","call=15 revision"),text.replace("gmax=1023","gmax=1024"),
                        text.replace("index=2 x=2","index=2 x=3"),text.replace("api=read ","api=wait "),
                        text.replace("outcome=return","outcome=throw"),text.replace("logical_bytes=24","logical_bytes=23"),
                        text.replace("SWEET RBYTES COUNT","BROKEN COUNT")):
            with self.subTest(changed=changed),self.assertRaises(ValueError):
                inspect(changed,token)


if __name__=="__main__":
    unittest.main()
