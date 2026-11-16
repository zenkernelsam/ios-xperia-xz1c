"""Read an immutable ipsw A2S v2 symbol map without private cache extraction."""
import mmap
import struct
from pathlib import Path


class A2S:
    def __init__(self, path):
        self.file = Path(path).open('rb')
        self.map = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        assert self.map[:4] == b'A2S\x02'
        self.count, strings = struct.unpack_from('<II', self.map, 4)
        self.strings_at = 12 + self.count*12
        assert self.count and len(self.map) == self.strings_at+strings

    def lookup(self, address):
        lo, hi = 0, self.count
        while lo < hi:
            mid = (lo+hi)//2
            value = struct.unpack_from('<Q', self.map, 12+mid*12)[0]
            if value <= address:
                lo = mid+1
            else:
                hi = mid
        if not lo:
            return None
        start, offset = struct.unpack_from('<QI', self.map, 12+(lo-1)*12)
        position = self.strings_at+offset
        assert self.strings_at <= position < len(self.map)
        end = self.map.find(b'\0', position)
        assert position < end < len(self.map)
        return dict(symbol_address=hex(start), symbol_offset=hex(address-start),
                    symbol=self.map[position:end].decode('utf-8', errors='replace'))

    def close(self):
        self.map.close()
        self.file.close()


if __name__ == '__main__':
    import argparse
    import json
    parser = argparse.ArgumentParser()
    parser.add_argument('symbols')
    parser.add_argument('addresses', nargs='+', type=lambda value:int(value, 16))
    args = parser.parse_args()
    symbols = A2S(args.symbols)
    try:
        print(json.dumps([{**(symbols.lookup(address) or {}), 'unslid_pc':hex(address)} for address in args.addresses]))
    finally:
        symbols.close()
