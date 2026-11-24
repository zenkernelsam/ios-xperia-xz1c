import unittest

from adsp_prepare_evidence import BEGIN, END, inspect


TOKEN = "0123456789abcdef"
LIVE = b"SWEET LOOP LIVE proto=1 token=0123456789abcdef mode=graceful pon=0x06"
ACK = b"SWEET LOOP ACK FASTBOOT token=0123456789abcdef mode=graceful pon=0x06"
FW = (b"SWEET ADSP FW verified=1 inflate=0 packed=5761449 raw=27262976 "
      b"fnv=5917c1c2c0cb472b mdt=7260 mdt_fnv=97d7a2020d3de26e "
      b"target=0x8b200000 loaded=0 authenticated=0")
APIS = [f"SWEET ADSP API cmd={command} result=0 available=1".encode() for command in (1, 2, 5)]
SETUP = (b"SWEET ADSP AP_SETUP result=0 out=0x1cf030 desc=0x1cf198 tx=0x1cf1c8 "
         b"global_free=0x6d78 private_free=0x41c8 created=1/1/1 lock_released=1 dsp_started=0 unowned_writes=0")
ROWS = [BEGIN, FW, *APIS, SETUP, END]


class AdspPrepareEvidence(unittest.TestCase):
    def test_verifies_preparation_without_promoting_audio(self):
        proof = inspect(b"\r\n".join([LIVE, *ROWS, ACK, b"SWEET ADSP APR ACCEPT response_verified=1"]), TOKEN)
        self.assertTrue(proof["ap_setup_verified"])
        self.assertFalse(proof["adsp_started"])
        self.assertFalse(proof["apr_transaction_verified"])
        self.assertFalse(proof["audio_output_verified"])

    def test_refuses_missing_duplicate_reordered_or_contradictory_evidence(self):
        cases = (
            [LIVE, *ROWS[:-1], ACK, END],
            [LIVE, *ROWS, BEGIN, ACK],
            [LIVE, BEGIN, FW, APIS[1], APIS[0], APIS[2], SETUP, END, ACK],
            [LIVE, *ROWS[:-2], SETUP.replace(b"unowned_writes=0", b"unowned_writes=1"), END, ACK],
            [LIVE, *ROWS[:-2], SETUP.replace(b"tx=0x1cf1c8", b"tx=0x1cf198"), END, ACK],
            [LIVE, *ROWS, b"SWEET ADSP COLD AUTH result=0 value=0", ACK],
            [ACK, *ROWS, LIVE],
        )
        for rows in cases:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                inspect(b"\n".join(rows), TOKEN)


if __name__ == "__main__":
    unittest.main()
