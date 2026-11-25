"""Verify bounded statistics from the bytes returned by original XR getBytes."""

import re

BEGIN=re.compile(r"SWEET RBYTES BEGIN pid=(\d+) call=(\d+) revision=1 width=(\d+) height=(\d+) row=(\d+) format=(554|555) original_getBytes=1")
COUNT=re.compile(r"SWEET RBYTES COUNT pid=(\d+) call=(\d+) pixels=(\d+) black=(\d+) nonblack=(\d+) zero_words=(\d+)")
RANGE=re.compile(r"SWEET RBYTES RANGE pid=(\d+) call=(\d+) rmin=(\d+) rmax=(\d+) gmin=(\d+) gmax=(\d+) bmin=(\d+) bmax=(\d+)")
PIXEL=re.compile(r"SWEET RBYTES PIXEL pid=(\d+) call=(\d+) index=([0-2]) x=(\d+) y=(\d+) word=([0-9a-f]{8})")
END=re.compile(r"SWEET RBYTES END pid=(\d+) call=(\d+) logical_bytes=(\d+) fnv=([0-9a-f]{16}) original_bytes_not_changed=1")


def inspect(text,token,source="readback"):
    if source not in ("readback","cgimage"):raise ValueError("Unknown original pixel source")
    tag="RBYTES" if source=="readback" else "CGBYTES"
    patterns=(BEGIN,COUNT,RANGE,PIXEL,END)
    if source=="cgimage":
        patterns=tuple(re.compile(p.pattern.replace("RBYTES","CGBYTES").replace("original_getBytes","original_CGImage")) for p in patterns)
    live="SWEET LOOP LIVE proto=1 token="+token
    ack="SWEET LOOP ACK FASTBOOT token="+token
    if not re.fullmatch("[0-9a-f]{16}",token) or text.count(live)!=1 or text.count(ack)!=1 or text.index(live)>=text.index(ack):
        raise ValueError("Need one original getBytes interval")
    scope=text[text.index(live):text.index(ack)]
    records=[(m.start(),kind,m) for kind,pattern in enumerate(patterns) for m in pattern.finditer(scope)]
    if not records or len(records)!=scope.count("SWEET "+tag+" "):
        raise ValueError("Unknown, truncated or refused original readback record")
    records.sort();rows=[];active=None;seen=set()
    for at,kind,m in records:
        key=m.groups()[:2]
        if kind==0:
            if active or key[0] in seen:raise ValueError("Repeated original readback")
            width,height,row,format_=map(int,m.groups()[2:])
            if not 0<width<=16384 or not 0<height<=16384 or width*height>1048576 or not width*4<=row<=65536:
                raise ValueError("Original readback bound differs")
            returned=re.compile(
                r"SWEET POSTDRAW END pid="+key[0]+r" tid=\d+ call="+key[1]+r" api=read(?:_full)? wall_ns=\d+ cpu_ns=\d+ cpu_valid=[01] outcome=return bytes=0 returned_nil=0 original_behavior=1"
                if source=="readback" else
                r"SWEET POSTDRAW BEGIN pid="+key[0]+r" tid=\d+ call="+key[1]+r" root=\d+ depth=\d+ api=png at_ns=\d+ original_forwarded=[01]")
            calls=list(returned.finditer(scope))
            if len(calls)!=1 or calls[0].end()>=at:raise ValueError("Readback does not follow an original successful getBytes return")
            active=dict(pid=int(key[0]),call=int(key[1]),width=width,height=height,row=row,format=format_,pixels=[],ranges=None,count=None)
            seen.add(key[0])
        elif not active or (str(active["pid"]),str(active["call"]))!=key:
            raise ValueError("Original readback identity differs")
        elif kind==1:
            if active["count"] is not None:raise ValueError("Repeated original counts")
            pixels,black,nonblack,zero=map(int,m.groups()[2:])
            if pixels!=active["width"]*active["height"] or black+nonblack!=pixels or zero>nonblack:
                raise ValueError("Original readback counts differ")
            active["count"]=dict(pixels=pixels,black=black,nonblack=nonblack,zero_words=zero)
        elif kind==2:
            if active["count"] is None or active["ranges"] is not None:raise ValueError("Original ranges out of order")
            values=list(map(int,m.groups()[2:]))
            if any(not 0<=lo<=hi<=1023 for lo,hi in zip(values[::2],values[1::2])):
                raise ValueError("Original XR code range differs")
            active["ranges"]=values
        elif kind==3:
            if active["ranges"] is None or int(m[3])!=len(active["pixels"]):raise ValueError("Original sample order differs")
            coordinates=[(0,0),(active["width"]//2,active["height"]//2),(active["width"]-1,active["height"]-1)]
            if (int(m[4]),int(m[5]))!=coordinates[int(m[3])]:raise ValueError("Original sampled coordinate differs")
            active["pixels"].append(dict(x=int(m[4]),y=int(m[5]),word=m[6]))
        else:
            if active["count"] is None or active["ranges"] is None or len(active["pixels"])!=3 or int(m[3])!=active["count"]["pixels"]*4:
                raise ValueError("Original readback incomplete")
            active["logical_bytes"]=int(m[3]);active["fnv"]=m[4]
            rows.append(active);active=None
    if active:raise ValueError("Original readback has no terminal record")
    return dict(token=token,records=rows,source=source,original_returned_bytes_observed=True,
                extra_GPU_work=0,pixels_not_repaired=True,full_image_or_optical_not_proven=True)
