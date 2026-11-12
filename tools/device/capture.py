"""DroidCam: override, 1s, AF, 2s, MJPEG stream, every fifth JPEG, rotate 90."""
import argparse
from pathlib import Path
import json
import time
import urllib.request
from io import BytesIO

CAMERA = "http://192.168.68.60:4747"

class MJPEG:
    def __init__(self): self.buffer = bytearray(); self.frames = 0
    def feed(self, chunk):
        self.buffer.extend(chunk)
        result = []
        while True:
            start = self.buffer.find(b"\xff\xd8")
            if start < 0:
                self.buffer[:] = self.buffer[-1:]; break
            end = self.buffer.find(b"\xff\xd9", start + 2)
            if end < 0:
                if start: del self.buffer[:start]
                if len(self.buffer) > 4 * 1024 * 1024: raise ValueError("JPEG frame exceeds 4MiB")
                break
            frame = bytes(self.buffer[start:end + 2]); del self.buffer[:end + 2]
            self.frames += 1
            if self.frames % 5 == 0: result.append((self.frames, frame))
        return result

def capture(directory, duration=8, max_stream_bytes=32*1024*1024):
    from PIL import Image, ImageFilter, ImageStat
    out = Path(directory); out.mkdir(exist_ok=True)
    for endpoint, pause in (("/override", 1), ("/cam/1/af", 2)):
        with urllib.request.urlopen(CAMERA + endpoint, timeout=5) as response: response.read(4096)
        time.sleep(pause)
    parser, saved = MJPEG(), []
    fallback = False
    def save(n, jpg):
        image = Image.open(BytesIO(jpg)); image.load()
        image = image.rotate(90, expand=True)
        target = out / f"frame-{n:04d}.jpg"
        image.save(target, quality=93)
        sharpness = ImageStat.Stat(image.convert("L").filter(ImageFilter.FIND_EDGES)).var[0]
        saved.append({"path": str(target), "sharpness": sharpness})
    try:
        start = time.monotonic()
        with urllib.request.urlopen(CAMERA + "/video?1280x720", timeout=5) as stream, (out / "stream.mjpeg").open("wb") as raw:
            while time.monotonic() - start < duration:
                chunk = stream.read(16384)
                if not chunk: break
                raw.write(chunk)
                if raw.tell() > max_stream_bytes: break
                for n, jpeg in parser.feed(chunk): save(n, jpeg)
    except Exception as e:
        (out / "stream-error.txt").write_text(str(e))
    if not saved:
        # Protocol permits exactly one still fallback after the video stream fails.
        with urllib.request.urlopen(CAMERA + "/cam/1/frame.jpg", timeout=5) as response:
            save(1, response.read(4 * 1024 * 1024))
        fallback = True
    result = {"protocol": "override-af-mjpeg", "rotation": 90, "fallback_once": fallback,
              "stream_frames": parser.frames, "frames": saved, "best": max(saved, key=lambda s: s["sharpness"])["path"]}
    (out / "capture.json").write_text(json.dumps(result, indent=2) + "\n")
    return result

if __name__ == "__main__":
    ap = argparse.ArgumentParser(); ap.add_argument("directory"); ap.add_argument("--duration", type=float, default=8)
    ap.add_argument("--max-stream-mib", type=int, default=32)
    args = ap.parse_args()
    if not 1 <= args.max_stream_mib <= 512: ap.error("max-stream-mib must be 1..512")
    print(json.dumps(capture(args.directory, args.duration, args.max_stream_mib*1024*1024)))
