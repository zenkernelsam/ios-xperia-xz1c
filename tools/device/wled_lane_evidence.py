"""Decode current-LIVE lane observations; no epoch, IO lease or optical proof."""
import re

RECORDS=(
 re.compile(r'SWEET WLED LANES BEGIN token=([0-9a-f]{16}) id=1 revision=1 roots=2 read_only=1(?=\r?\n|$)'),
 re.compile(r'SWEET WLED LANES STATE token=([0-9a-f]{16}) id=1 root=(\d+) ppid=([0-9a-f]{3}) result=(-?\d+) matches=(\d+) selected=(\d+)(?=\r?\n|$)'),
 re.compile(r'SWEET WLED LANES ITEM token=([0-9a-f]{16}) id=1 root=(\d+) index=(\d+) apid=(\d+) map=([0-9a-f]{8}) owner=([0-9a-f]{8}) obs=([01]) wr=([01]) poison=([01])(?=\r?\n|$)'),
 re.compile(r'SWEET WLED LANES END token=([0-9a-f]{16}) id=1 result=([01]) roots=2 epoch=0 write_lease=0(?=\r?\n|$)'))


def inspect_scope(scope,token):
    events=sorted((m.start(),kind,m) for kind,p in enumerate(RECORDS) for m in p.finditer(scope))
    if len(events)!=scope.count('SWEET WLED LANES '):raise ValueError('Unknown or truncated lane record')
    begun=ended=False;roots={};current=None
    for _,kind,m in events:
        if m[1]!=token:raise ValueError('Foreign token')
        if ended:raise ValueError('Record after END')
        if kind==0:
            if begun:raise ValueError('Repeated BEGIN')
            begun=True;continue
        if not begun:raise ValueError('Record before BEGIN')
        if kind==1:
            if current is not None and len(roots[current]['lanes'])!=roots[current]['matches']:raise ValueError('Incomplete prior root')
            index,ppid,result,count,selected=int(m[2]),int(m[3],16),int(m[4]),int(m[5]),int(m[6])
            if index!=len(roots) or index>=2 or ppid!=(0x3d8 if index==0 else 0x3d9) or not 0<=count<=4:
                raise ValueError('Root identity/order/bounds changed')
            if result not in (0,-1) or (result and (count or selected!=65535)):raise ValueError('Failed state has data')
            if not count and selected!=65535:raise ValueError('Empty state has selected lane')
            roots[index]=dict(ppid=ppid,result=result,matches=count,selected=selected,lanes=[]);current=index
        elif kind==2:
            root,index,apid=int(m[2]),int(m[3]),int(m[4]);mapping,owner=int(m[5],16),int(m[6],16)
            if root!=current or root not in roots:raise ValueError('ITEM before current STATE')
            state=roots[root];lanes=state['lanes']
            if state['result'] or index!=len(lanes) or index>=state['matches'] or apid>=512 or not mapping or ((mapping>>8)&0xfff)!=state['ppid']:
                raise ValueError('Lane identity/order/bounds changed')
            if lanes and apid<=lanes[-1]['apid']:raise ValueError('APID order changed')
            lanes.append(dict(apid=apid,map_word=mapping,owner_word=owner,owner_EE=owner&7,
                              observer_mapped=bool(int(m[7])),writer_mapped=bool(int(m[8])),poisoned=bool(int(m[9]))))
        else:
            if len(roots)!=2 or any(len(r['lanes'])!=r['matches'] for r in roots.values()):raise ValueError('Missing roots/lanes at END')
            failed=any(r['result'] for r in roots.values())
            if int(m[2])!=int(failed):raise ValueError('END result changed')
            for r in roots.values():
                if r['lanes']:
                    selected=r['lanes'][0]['apid']
                    for lane in r['lanes']:
                        if lane['owner_EE']==0:selected=lane['apid']
                    if r['selected']!=selected:raise ValueError('Original selector result changed')
            ended=True
    if not ended:raise ValueError('Missing END')
    return dict(roots=list(roots.values()),both_queries_succeeded=not any(r['result'] for r in roots.values()),
                complete_record_framing_checked=True,no_epoch_or_retained_write_lease=True,
                does_not_prove_backlight_writes_or_optics=True)


def inspect(text,token):
    if not isinstance(token,str) or not re.fullmatch('[0-9a-f]{16}',token):raise ValueError('Invalid current token')
    live=list(re.finditer(re.escape('SWEET LOOP LIVE proto=1 token='+token)+r'(?=\s|$)',text))
    ack=list(re.finditer(re.escape('SWEET LOOP ACK FASTBOOT token='+token)+r'(?=\s|$)',text))
    if len(live)!=1 or len(ack)!=1 or live[0].start()>=ack[0].start():raise ValueError('Need one exact LIVE before ACK')
    scope=text[live[0].start():ack[0].start()]
    result=inspect_scope(scope,token)
    frames=list(re.finditer(re.escape('SWEET SCREEN END token='+token)+r' id=1 encoded=(\d+) blocks=(\d+) raw_fnv=([0-9a-f]{16}) enc_crc=([0-9a-f]{8}) complete=1',scope))
    if len(frames)!=1 or not int(frames[0][1]) or not int(frames[0][2]) or frames[0].start()>=scope.index('SWEET WLED LANES BEGIN '):
        raise ValueError('Need completed first-frame boundary before lane observations')
    result.update(frame_end_boundary_checked=True,frame_bytes_not_verified_here=True,post_ACK_excluded=True)
    return result
