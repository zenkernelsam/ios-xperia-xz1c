"""Inspect bounded original Metal draws and completed command results."""

import re

CONFIG = re.compile(r"SWEET RQUEUE CONFIG pid=(\d+) revision=1 installed=1 limit=8 original_arguments_results=1 extra_GPU_work=0")
DRAW = re.compile(r"SWEET RQUEUE DRAW pid=(\d+) call=(\d+) primitive=(\d+) first=(\d+) count=(\d+) instances=(\d+)")
BIND = re.compile(r"SWEET RQUEUE BIND pid=(\d+) call=(\d+) vertex=(\d+) fragment=(\d+) valid=1 original_state=1")
PIPE = re.compile(r"SWEET RQUEUE PIPE pid=(\d+) call=(\d+) vertex=([A-Za-z0-9_]{1,48}) fragment=([A-Za-z0-9_]{1,48})")
DRAW_END = re.compile(r"SWEET RQUEUE DRAW_END pid=(\d+) call=(\d+) outcome=(return|throw) original_behavior=1")
RESULT = re.compile(r"SWEET RQUEUE RESULT pid=(\d+) call=(\d+) status=([45]) error_nil=([01]) code=(-?\d+) terminal_getters_only=1")
ERROR = re.compile(r"SWEET RQUEUE ERROR pid=(\d+) call=(\d+) text=([\x21-\x7e]{1,128})")
RETIRED = re.compile(r"SWEET RQUEUE RETIRED pid=(\d+) call=(\d+) fence=(\d+) operations=(\d+) complete=1 original_command=1")


def inspect(text, token,require_retirement=False):
    live, ack = "SWEET LOOP LIVE proto=1 token=" + token, "SWEET LOOP ACK FASTBOOT token=" + token
    if not re.fullmatch("[0-9a-f]{16}", token) or text.count(live) != 1 or text.count(ack) != 1 or text.index(live) >= text.index(ack):
        raise ValueError("Need one original render queue interval")
    scope = text[text.index(live):text.index(ack)]
    configs = list(CONFIG.finditer(scope))
    pids = {m[1] for m in configs}
    if not configs or len(configs) != len(pids):
        raise ValueError("Original render queue installation differs")
    groups = [[*pattern.finditer(scope)] for pattern in (DRAW, BIND, PIPE, DRAW_END, RESULT, ERROR)]
    retired=list(RETIRED.finditer(scope))
    if scope.count("SWEET RQUEUE ") != len(configs) + sum(map(len, groups))+len(retired):
        raise ValueError("Unknown, refused or truncated render queue records")
    draws, bindings, pipelines, ends, results, errors = groups
    if not draws or not results or len({len(draws), len(bindings), len(pipelines), len(ends)}) != 1 or len(results) != len(errors):
        raise ValueError("Original render queue records incomplete")
    draw_rows, result_rows, keys = [], [], set()
    for draw, bind, pipe, end in zip(draws, bindings, pipelines, ends):
        key = draw.groups()[:2]
        if key in keys or key[0] not in pids or not 1 <= int(key[1]) <= 8 or any(m.groups()[:2] != key for m in (bind, pipe, end)):
            raise ValueError("Render queue draw identity differs")
        if not draw.end() < bind.start() < bind.end() < pipe.start() < pipe.end() < end.start():
            raise ValueError("Render queue draw ordering differs")
        if int(bind[3]) > 255 or int(bind[4]) > 255 or int(draw[3]) > 4:
            raise ValueError("Render queue draw exceeds observed ABI")
        keys.add(key)
        draw_rows.append(dict(pid=int(key[0]), call=int(key[1]), primitive=int(draw[3]),
                              first=int(draw[4]), count=int(draw[5]), instances=int(draw[6]),
                              vertex_mask=int(bind[3]), fragment_mask=int(bind[4]),
                              vertex_function=pipe[3], fragment_function=pipe[4], outcome=end[3]))
    keys.clear()
    for result, error in zip(results, errors):
        key = result.groups()[:2]
        if key in keys or key[0] not in pids or not 1 <= int(key[1]) <= 8 or error.groups()[:2] != key or result.end() >= error.start():
            raise ValueError("Render queue result identity/order differs")
        keys.add(key)
        status, nil, code = int(result[3]), int(result[4]), int(result[5])
        if (status == 4 and (not nil or code or error[3] != "none")) or (status == 5 and nil):
            raise ValueError("Original terminal command error differs")
        result_rows.append(dict(pid=int(key[0]), call=int(key[1]), status=status,
                                error_nil=bool(nil), code=code, description=error[3]))
        matching=[r for r in retired if r.groups()[:2]==key]
        if status==4 and require_retirement:
            if len(matching)!=1 or matching[0].start()<=error.end() or int(matching[0][3])<=0 or int(matching[0][4])<=0:
                raise ValueError("Original completed queue has no valid retirement")
            result_rows[-1].update(fence=int(matching[0][3]),operations=int(matching[0][4]))
        elif matching:
            raise ValueError("Unexpected original queue retirement")
    if len(retired)!=(sum(r["status"]==4 for r in result_rows) if require_retirement else 0):
        raise ValueError("Foreign, duplicate or incomplete original queue retirement")
    return dict(token=token, draws=draw_rows, results=result_rows,
                failed_commands=sum(r["status"] == 5 for r in result_rows),
                original_arguments_and_errors_preserved=True, extra_GPU_work=0,
                wallpaper_pixels_or_optical_not_proven=True)
