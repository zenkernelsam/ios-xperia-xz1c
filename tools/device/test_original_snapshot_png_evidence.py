import struct
import unittest
import zlib
from original_snapshot_png_evidence import inspect, fnv, validate_png


def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)


class OriginalPNGTests(unittest.TestCase):
    def test_actual_return_identity_complete_bytes_and_CRC(self):
        token = "123456789abcdef0"
        data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
        data += chunk(b"IDAT", zlib.compress(b"\x00\x01\x02\x03")) + chunk(b"IEND", b"")
        text = "SWEET LOOP LIVE proto=1 token=" + token + "\n"
        text += f"SWEET POSTDRAW END pid=7 tid=8 call=9 api=png wall_ns=10 cpu_ns=5 cpu_valid=1 outcome=return bytes={len(data)} returned_nil=0 original_behavior=0\n"
        text += f"SWEET OPNG BEGIN pid=7 call=9 bytes={len(data)} fnv={fnv(data)} chunks=48 original_return_bytes=1\n"
        for offset in range(0, len(data), 48):
            part = data[offset:offset + 48]
            text += f"SWEET OPNG DATA pid=7 call=9 offset={offset} bytes={len(part)} hex={part.hex()}\n"
        text += f"SWEET OPNG END pid=7 call=9 bytes={len(data)} fnv={fnv(data)} unchanged=1 original_return_bytes=1\n"
        text += "SWEET LOOP ACK FASTBOOT token=" + token
        output, report = inspect(text, token)
        self.assertEqual(output, data)
        self.assertEqual(report["PNG"]["width"], 1)
        for changed in (text.replace("offset=48", "offset=49"), text.replace("unchanged=1", "unchanged=0"),
                        text.replace("pid=7 call=9 offset=", "pid=8 call=9 offset=")):
            with self.assertRaises(ValueError):
                inspect(changed, token)
        with self.assertRaises(ValueError):
            validate_png(data[:-5])
        with self.assertRaises(ValueError):
            validate_png(data[:33] + bytes([data[33] ^ 1]) + data[34:])

    def test_readonly_loaded_snapshot_is_bound_to_original_image(self):
        token="123456789abcdef0"
        data=b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",1,1,8,2,0,0,0))
        data+=chunk(b"IDAT",zlib.compress(b"\x00\x01\x02\x03"))+chunk(b"IEND",b"")
        path=b"/var/mobile/Library/Caches/aegir-fixture.png"
        text="SWEET LOOP LIVE proto=1 token="+token+"\n"
        text+=f"SWEET PSG PATH pid=7 call=9 bytes={len(path)} truncated=0 getter_error=0 pixels_read=0\n"
        for offset in range(0,len(path),24):
            part=path[offset:offset+24]
            text+=f"SWEET PSG PATH_DATA pid=7 call=9 offset={offset} bytes={len(part)} hex={part.hex()}\n"
        text+="SWEET PSG END pid=7 call=9 api=image_load result=0000000000000100 outcome=return original_behavior=1\n"
        text+="SWEET LPNG SOURCE pid=7 call=9 result=0 original_image=0000000000000100 file_read_only=1\n"
        text+=f"SWEET OPNG BEGIN pid=7 call=9 bytes={len(data)} fnv={fnv(data)} chunks=48 original_return_bytes=1\n"
        for offset in range(0,len(data),48):
            part=data[offset:offset+48]
            text+=f"SWEET OPNG DATA pid=7 call=9 offset={offset} bytes={len(part)} hex={part.hex()}\n"
        text+=f"SWEET OPNG END pid=7 call=9 bytes={len(data)} fnv={fnv(data)} unchanged=1 original_return_bytes=1\n"
        text+="SWEET LOOP ACK FASTBOOT token="+token
        actual,report=inspect(text,token,source="loaded")
        self.assertEqual(actual,data)
        self.assertTrue(report["original_loaded_file_verified"])
        with self.assertRaises(ValueError):
            inspect(text.replace("original_image=0000000000000100","original_image=0000000000000200"),token,source="loaded")
        packed=zlib.compress(data)
        start=text.index("SWEET OPNG BEGIN")
        finish=text.index("SWEET OPNG END")
        encoded=f"SWEET OPNG BEGIN pid=7 call=9 revision=2 bytes={len(data)} fnv={fnv(data)} codec=ZLIB1 original_return_bytes=1\n"
        encoded+=f"SWEET OPNG PACKED pid=7 call=9 bytes={len(packed)} fnv={fnv(packed)} chunks=48\n"
        for offset in range(0,len(packed),48):
            part=packed[offset:offset+48]
            encoded+=f"SWEET OPNG DATA pid=7 call=9 offset={offset} bytes={len(part)} hex={part.hex()}\n"
        compressed=text[:start]+encoded+text[finish:]
        actual,report=inspect(compressed,token,source="loaded")
        self.assertEqual(actual,data)
        self.assertEqual(report["compression"],"ZLIB1")
        with self.assertRaises(ValueError):
            inspect(compressed.replace(f"fnv={fnv(packed)}", "fnv="+"0"*16),token,source="loaded")


if __name__ == "__main__":
    unittest.main()
