"""Decode bounded original filesystem metadata; never infer selected wallpaper from execs."""
import hashlib
import json
import plistlib
import re
from pathlib import PurePosixPath
from xml.parsers.expat import ExpatError

ROOTS=('/private/var/mobile/Library/PosterBoard',
       '/private/var/mobile/Library/Application Support/PosterBoard',
       '/private/var/mobile/Library/SpringBoard','/private/var/mobile/Library/Preferences',
       '/private/var/mobile/Library/Wallpaper')
PATTERNS=(
 re.compile(r'SWEET PCFG BEGIN revision=1 roots=(\d+) max_entries=256 max_files=16 max_bytes=131072 max_depth=4 read_only=1'),
 re.compile(r'SWEET PCFG ROOT index=(\d+) path_bytes=(\d+) result=(-?\d+)'),
 re.compile(r'SWEET PCFG (ROOTPATH|PATH|DATA) id=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)'),
 re.compile(r'SWEET PCFG ENTRY id=(\d+) root=(\d+) depth=(\d+) kind=([DFLOE]) path_bytes=(\d+) size=(-?\d+) result=(-?\d+)'),
 re.compile(r'SWEET PCFG FILE id=(\d+) bytes=(\d+) sha256=([0-9a-f]{64}) stable=1'),
 re.compile(r'SWEET PCFG ITEMEND id=(\d+) result=(-?\d+) captured=([01]) bytes=(\d+)'),
 re.compile(r'SWEET PCFG WALK root=(\d+) result=(-\d+)'),
 re.compile(r'SWEET PCFG END result=(-?\d+) roots=(\d+) opened=(\d+) entries=(\d+) files=(\d+) bytes=(\d+) skipped=(\d+) errors=(\d+) read_only=1'))

def _path(raw):
    value=raw.decode('utf-8','strict');path=PurePosixPath(value)
    if not value.startswith('/') or '\0' in value or '..' in path.parts or len(raw)>=512:
        raise ValueError('Invalid captured absolute path')
    return value

def _metadata(data):
    try:obj=plistlib.loads(data)
    except (ValueError,plistlib.InvalidFileException,ExpatError):
        try:obj=json.loads(data)
        except (ValueError,UnicodeError):return dict(parsed=False,references=[])
    # Preserve evidence paths rather than treating a provider reference as selection.
    stack=[([],obj,0)];seen=0;refs=[]
    while stack:
        path,value,depth=stack.pop();seen+=1
        if seen>8192 or depth>64:raise ValueError('Metadata graph exceeds bounds')
        if isinstance(value,dict):
            for key,child in value.items():stack.append((path+[str(key)],child,depth+1))
        elif isinstance(value,list):
            for index,child in enumerate(value):stack.append((path+[str(index)],child,depth+1))
        elif isinstance(value,str):
            key='.'.join(path).lower()
            if (value.startswith('com.apple.') and ('poster' in value.lower() or 'aegir' in value.lower())) or (
                any(word in key for word in ('configurationidentifier','collectionidentifier','wallpaperidentifier','selected','current','activeconfiguration'))
                and len(value)<=128 and re.fullmatch(r'[a-zA-Z0-9_.:-]+',value)):
                refs.append(dict(key_path=path,value=value))
        elif isinstance(value,int) and path and 'wallpaperidentifier' in path[-1].lower():
            refs.append(dict(key_path=path,value=value))
    return dict(parsed=True,references=refs,provider_references_are_not_selection_proof=True)

def inspect_scope(scope,expected_roots=None,keep_data=False):
    matches=sorted((m.start(),kind,m) for kind,p in enumerate(PATTERNS) for m in p.finditer(scope))
    if len(matches)!=scope.count('SWEET PCFG '):raise ValueError('Unknown or truncated PCFG record')
    started=ended=False;root_count=0;roots={};items={};active=None;active_root=None;captured=[];walk_errors=[]
    for _,kind,m in matches:
        if ended:raise ValueError('Metadata after END')
        if kind==0:
            if started:raise ValueError('Repeated BEGIN')
            root_count=int(m[1]);started=True
            if not 0<root_count<=8:raise ValueError('Invalid root count')
            continue
        if not started:raise ValueError('Record before BEGIN')
        if kind==1:
            index,length,result=map(int,m.groups())
            if active is not None or index!=len(roots) or index>=root_count or not 0<length<512:raise ValueError('Root order/bounds changed')
            roots[index]=dict(length=length,result=result,path=bytearray());active_root=index
        elif kind==2:
            label,id_text,offset_text,n_text,hex_value=m.groups();ident,offset,n=map(int,(id_text,offset_text,n_text));data=bytes.fromhex(hex_value)
            if not 0<n<=64 or len(data)!=n:raise ValueError('Invalid chunk length')
            if label=='ROOTPATH':
                if ident!=active_root or active is not None:raise ValueError('Wrong root chunk')
                record=roots[ident];buffer=record['path'];limit=record['length']
            else:
                if ident!=active:raise ValueError('Foreign or late item chunk')
                record=items[ident];buffer=record['path' if label=='PATH' else 'data'];limit=record['length' if label=='PATH' else 'file_bytes']
                if label=='DATA' and record.get('sha256') is None:raise ValueError('DATA before FILE')
            if offset!=len(buffer) or offset+n>limit:raise ValueError('Missing/repeated/out-of-order chunk')
            buffer.extend(data)
        elif kind==3:
            ident,root,depth=int(m[1]),int(m[2]),int(m[3]);length,size,result=int(m[5]),int(m[6]),int(m[7])
            if active is not None or ident!=len(items)+1 or ident>256 or root!=active_root or depth>4 or not 0<length<512:
                raise ValueError('Invalid item order or bounds')
            if len(roots[root]['path'])!=roots[root]['length']:raise ValueError('Root path incomplete')
            items[ident]=dict(root=root,depth=depth,kind=m[4],length=length,size=size,result=result,path=bytearray(),data=bytearray(),file_bytes=0,sha256=None)
            active=ident
        elif kind==4:
            ident,length=int(m[1]),int(m[2])
            if ident!=active:raise ValueError('Foreign FILE')
            item=items[ident]
            if item['sha256'] is not None or item['kind']!='F' or item['result'] or length!=item['size'] or not 0<length<=65536:
                raise ValueError('FILE metadata mismatch')
            if len(item['path'])!=item['length']:raise ValueError('Incomplete item path')
            item['file_bytes']=length;item['sha256']=m[3]
        elif kind==5:
            ident,result,capture,length=map(int,m.groups())
            if ident!=active:raise ValueError('ITEMEND without current item')
            item=items[ident]
            if len(item['path'])!=item['length']:raise ValueError('Missing item path')
            path=_path(bytes(item['path']));root_path=_path(bytes(roots[item['root']]['path']))
            if not path.startswith(root_path+'/'):raise ValueError('Item outside root')
            item['end_result']=result
            if item['result'] and result!=item['result']:raise ValueError('Catalog error changed at ITEMEND')
            if capture:
                data=bytes(item['data'])
                if result or length!=item['file_bytes'] or len(data)!=length or hashlib.sha256(data).hexdigest()!=item['sha256']:
                    raise ValueError('Captured data length/SHA/result mismatch')
                record=dict(path=path,bytes=length,sha256=item['sha256'],metadata=_metadata(data))
                if keep_data:record['data']=data
                captured.append(record)
            elif length or item['data'] or item['sha256'] is not None:raise ValueError('Uncaptured item has data')
            active=None
        elif kind==6:
            if int(m[1]) not in roots:raise ValueError('WALK error before root')
            walk_errors.append(dict(root=int(m[1]),result=int(m[2])))
        else:
            values=list(map(int,m.groups()));result,nroots,opened,entries,files,total,skipped,errors=values
            if active is not None or nroots!=root_count or len(roots)!=root_count or any(len(r['path'])!=r['length'] for r in roots.values()):
                raise ValueError('Incomplete roots/items at END')
            if opened!=sum(r['result']==0 for r in roots.values()) or entries!=len(items) or files!=len(captured) or total!=sum(x['bytes'] for x in captured):
                raise ValueError('END count mismatch')
            if files>16 or total>131072:raise ValueError('Capture limits exceeded')
            expected_errors=sum(r['result'] not in (0,-2) for r in roots.values())+len(walk_errors)+sum(i['end_result']<0 for i in items.values())
            if errors!=expected_errors:raise ValueError('Error count mismatch')
            summary=dict(result=result,roots=nroots,opened=opened,entries=entries,files=files,bytes=total,skipped=skipped,errors=errors)
            ended=True
    if not ended:raise ValueError('Missing END')
    decoded_roots=[dict(path=_path(bytes(r['path'])),result=r['result']) for r in roots.values()]
    if expected_roots is not None and [r['path'] for r in decoded_roots]!=list(expected_roots):raise ValueError('Root identity changed')
    return dict(summary=summary,roots=decoded_roots,files=captured,walk_errors=walk_errors,
                readonly_complete_stream_checked=True,selected_wallpaper_not_inferred=True)

def inspect(text,token):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    if text.count(live)!=1 or text.count(ack)!=1:raise ValueError('Need one current LIVE→ACK interval')
    scope=text[text.index(live):text.index(ack)]
    return inspect_scope(scope,ROOTS)
