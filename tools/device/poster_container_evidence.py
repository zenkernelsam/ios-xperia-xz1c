"""Strictly decode MCM identities; container identity is not wallpaper selection."""
import hashlib
import plistlib
import re
from pathlib import PurePosixPath

ROOTS=('/private/var/mobile/Containers/Data/Application',
       '/private/var/mobile/Containers/Shared/AppGroup',
       '/private/var/containers/Data/System',
       '/private/var/containers/Shared/SystemGroup')
PATTERNS=(
 re.compile(r'SWEET PCON BEGIN revision=2 roots=4 max_entries=256 max_files=128 max_bytes=262144 read_only=1'),
 re.compile(r'SWEET PCON ROOT index=(\d+) path_bytes=(\d+) result=(-?\d+)'),
 re.compile(r'SWEET PCON (ROOTPATH|PATH|DATA) id=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)'),
 re.compile(r'SWEET PCON ENTRY id=(\d+) root=(\d+) kind=([DFLOE]) path_bytes=(\d+) result=(-?\d+)'),
 re.compile(r'SWEET PCON FILE id=(\d+) bytes=(\d+) sha256=([0-9a-f]{64}) stable=1'),
 re.compile(r'SWEET PCON ITEMEND id=(\d+) result=(-?\d+) captured=([01]) bytes=(\d+)'),
 re.compile(r'SWEET PCON WALK root=(\d+) result=(-\d+)'),
 re.compile(r'SWEET PCON END result=(-?\d+) roots=(\d+) opened=(\d+) examined=(\d+) files=(\d+) bytes=(\d+) skipped=(\d+) errors=(\d+) read_only=1'))


def path(raw):
    value=raw.decode('utf8','strict');normalized=PurePosixPath(value)
    if not value.startswith('/') or '\0' in value or '..' in normalized.parts or normalized.as_posix()!=value or not 0<len(raw)<512:
        raise ValueError('Invalid container path')
    return value


def inspect_scope(text,expected_roots=ROOTS):
    matches=sorted((m.start(),kind,m) for kind,p in enumerate(PATTERNS) for m in p.finditer(text))
    if len(matches)!=text.count('SWEET PCON '):raise ValueError('Unknown or truncated PCON record')
    begun=ended=False;roots={};items={};active=None;current_root=None;files=[];walk=[]
    for _,kind,m in matches:
        if ended:raise ValueError('Record after END')
        if kind==0:
            if begun:raise ValueError('Repeated BEGIN')
            begun=True;continue
        if not begun:raise ValueError('Record before BEGIN')
        if kind==1:
            index,length,result=map(int,m.groups())
            if active is not None or index!=len(roots) or index>=4 or not 0<length<512:raise ValueError('Invalid root order/bounds')
            if current_root is not None and len(roots[current_root]['path'])!=roots[current_root]['length']:raise ValueError('Incomplete root path')
            roots[index]=dict(length=length,result=result,path=bytearray());current_root=index
        elif kind==2:
            label=m[1];ident,offset,n=map(int,(m[2],m[3],m[4]));raw=bytes.fromhex(m[5])
            if not 0<n<=64 or n!=len(raw):raise ValueError('Invalid range length')
            if label=='ROOTPATH':
                if ident!=current_root or active is not None:raise ValueError('Foreign root range')
                row=roots[ident];buffer=row['path'];limit=row['length']
            else:
                if ident!=active:raise ValueError('Foreign item range')
                row=items[ident];buffer=row['path' if label=='PATH' else 'data'];limit=row['length' if label=='PATH' else 'file_bytes']
                if label=='DATA' and row['sha256'] is None:raise ValueError('DATA before FILE')
            if offset!=len(buffer) or offset+n>limit:raise ValueError('Missing/repeated/out-of-order range')
            buffer.extend(raw)
        elif kind==3:
            ident,root,length,result=int(m[1]),int(m[2]),int(m[4]),int(m[5])
            if active is not None or ident!=len(items)+1 or ident>256 or root!=current_root or not 0<length<512:
                raise ValueError('Invalid item order/bounds')
            if roots[root]['result'] or len(roots[root]['path'])!=roots[root]['length']:raise ValueError('Item in unopened root')
            items[ident]=dict(root=root,kind=m[3],length=length,result=result,path=bytearray(),data=bytearray(),file_bytes=0,sha256=None)
            active=ident
        elif kind==4:
            ident,length=int(m[1]),int(m[2])
            if ident!=active:raise ValueError('Foreign FILE')
            row=items[ident]
            if row['kind']!='D' or row['result'] or row['sha256'] is not None or not 0<length<=65536 or len(row['path'])!=row['length']:
                raise ValueError('FILE identity/bounds changed')
            row.update(file_bytes=length,sha256=m[3])
        elif kind==5:
            ident,result,captured,length=map(int,m.groups())
            if ident!=active:raise ValueError('Foreign ITEMEND')
            row=items[ident]
            if len(row['path'])!=row['length']:raise ValueError('Missing container path')
            name=path(bytes(row['path']));root=path(bytes(roots[row['root']]['path']))
            if str(PurePosixPath(name).parent)!=root:raise ValueError('Container is not immediate root child')
            row['end_result']=result
            if row['result'] and result!=row['result']:raise ValueError('Catalog error changed')
            if captured:
                raw=bytes(row['data'])
                if result or len(raw)!=length or length!=row['file_bytes'] or hashlib.sha256(raw).hexdigest()!=row['sha256']:
                    raise ValueError('Metadata SHA/length/result mismatch')
                obj=plistlib.loads(raw)
                if not isinstance(obj,dict) or not isinstance(obj.get('MCMMetadataIdentifier'),str):raise ValueError('MCM identity absent')
                identifier=obj['MCMMetadataIdentifier']
                if not identifier or len(identifier)>256:raise ValueError('MCM identity bounds')
                files.append(dict(container_path=name,metadata_path=name+'/.com.apple.mobile_container_manager.metadata.plist',
                                  bytes=length,sha256=row['sha256'],identifier=identifier,identity_key='MCMMetadataIdentifier'))
            elif length or row['data'] or row['sha256'] is not None:raise ValueError('Uncaptured item has bytes')
            active=None
        elif kind==6:
            if int(m[1]) not in roots:raise ValueError('WALK before root')
            walk.append(dict(root=int(m[1]),result=int(m[2])))
        else:
            result,nroots,opened,examined,nfiles,total,skipped,errors=map(int,m.groups())
            if active is not None or nroots!=4 or len(roots)!=4 or any(len(r['path'])!=r['length'] for r in roots.values()):
                raise ValueError('Incomplete roots/items at END')
            if examined!=len(items) or nfiles!=len(files) or total!=sum(f['bytes'] for f in files) or nfiles>128 or total>262144:
                raise ValueError('END count/limits mismatch')
            expected_errors=sum(r['result'] not in (0,-2) for r in roots.values())+len(walk)+sum(r['end_result']<0 for r in items.values())
            expected_skipped=sum(r['end_result']>=0 and r['sha256'] is None for r in items.values())
            if errors!=expected_errors or skipped!=expected_skipped:raise ValueError('END errors/skipped mismatch')
            if not errors and opened!=sum(r['result']==0 for r in roots.values()):raise ValueError('END opened mismatch')
            summary=dict(result=result,roots=nroots,opened=opened,examined=examined,files=nfiles,bytes=total,skipped=skipped,errors=errors)
            ended=True
    if not ended:raise ValueError('Missing END')
    decoded_roots=[dict(path=path(bytes(r['path'])),result=r['result']) for r in roots.values()]
    if [r['path'] for r in decoded_roots]!=list(expected_roots):raise ValueError('Root identity changed')
    exact=[f for f in files if f['identifier']=='com.apple.PosterBoard']
    relevant=[f for f in files if any(w in f['identifier'].lower() for w in ('poster','wallpaper','aegir'))]
    complete=summary['result']==0 and summary['errors']==0
    return dict(summary=summary,roots=decoded_roots,containers=files,relevant_containers=relevant,
                exact_PosterBoard_containers=exact,exact_PosterBoard_identity_unique=complete and len(exact)==1,
                catalog_enumeration_complete=complete,
                readonly_complete_stream_checked=True,selected_wallpaper_not_inferred=True)


def inspect(text,token,expected_roots=ROOTS):
    if not isinstance(token,str) or not re.fullmatch('[0-9a-f]{16}',token):raise ValueError('Invalid current token')
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    live_records=list(re.finditer(re.escape(live)+r'(?=\s|$)',text))
    ack_records=list(re.finditer(re.escape(ack)+r'(?=\s|$)',text))
    if len(live_records)!=1 or len(ack_records)!=1 or live_records[0].start()>=ack_records[0].start():
        raise ValueError('Need one exact current LIVE before ACK')
    scope=text[live_records[0].start():ack_records[0].start()]
    result=inspect_scope(scope,expected_roots)
    png_end='SWEET CIF10 REFERENCE END result=0 cases=6 original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0'
    suite_end='SWEET METAL PROBES END process=backboardd result=0 prerequisite=0 full_suite=1 background=1'
    if scope.count(png_end)!=1 or scope.count(suite_end)!=1:raise ValueError('Need one PNG6 and full native suite end')
    if not scope.index(png_end)<scope.index('SWEET PCON BEGIN ')<scope.index('SWEET PCON END ')<scope.index(suite_end):
        raise ValueError('Native PNG6/metadata/suite boundary order changed')
    result['native_PNG6_then_metadata_then_suite_boundary_order_checked']=True
    return result
