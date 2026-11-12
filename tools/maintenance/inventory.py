"""Read-only Lilac inventory. Never follow links or dump credential contents."""
import argparse,collections,json,os,stat,subprocess,time
from pathlib import Path
from urllib.parse import urlsplit,urlunsplit

def git_info(root):
    if not (root/'.git').exists() and not ((root/'HEAD').is_file() and (root/'objects').is_dir()):return None
    def command(args):
        r=subprocess.run(['git','-C',str(root),*args],capture_output=True,text=True,timeout=10)
        return r.stdout.strip() if r.returncode==0 else None
    url=command(['config','--get','remote.origin.url'])
    if url:
        parts=urlsplit(url)
        if parts.scheme:url=urlunsplit((parts.scheme,parts.netloc.split('@')[-1],parts.path,'',''))
    return {'head':command(['rev-parse','HEAD']),'origin':url,'toplevel':command(['rev-parse','--show-toplevel'])}

def walk(root,files,errors):
    stack=[root];size=count=0
    while stack:
        folder=stack.pop()
        try:
            entries=list(os.scandir(folder))
        except OSError as error:errors.append({'path':str(folder),'error':str(error)});continue
        for entry in entries:
            p=Path(entry.path)
            try:
                s=entry.stat(follow_symlinks=False)
                reparse=bool(getattr(s,'st_file_attributes',0)&getattr(stat,'FILE_ATTRIBUTE_REPARSE_POINT',0x400))
                if entry.is_symlink() or reparse:
                    files.append({'path':str(p),'kind':'link','target':os.readlink(p) if entry.is_symlink() else None,'bytes':0})
                elif stat.S_ISDIR(s.st_mode):stack.append(p)
                elif stat.S_ISREG(s.st_mode):
                    files.append({'path':str(p),'kind':'file','bytes':s.st_size,'mtime_ns':s.st_mtime_ns,'inode':s.st_ino})
                    size+=s.st_size;count+=1
                else:files.append({'path':str(p),'kind':'special','bytes':s.st_size})
            except OSError as error:errors.append({'path':str(p),'error':str(error)})
    return size,count

def inventory(root):
    root=Path(root).absolute();files=[];errors=[];entries=[]
    for p in sorted(root.iterdir()):
        s=p.lstat()
        if p.is_symlink() or bool(getattr(s,'st_file_attributes',0)&0x400):
            row={'name':p.name,'kind':'link','bytes':0,'target':os.readlink(p) if p.is_symlink() else None}
        elif p.is_dir():
            repo=git_info(p);size,count=walk(p,files,errors)
            row={'name':p.name,'kind':'directory','git':repo,'bytes':size,'files':count}
        else:
            row={'name':p.name,'kind':'file','bytes':s.st_size}
            files.append({'path':str(p),'kind':'file','bytes':s.st_size,'mtime_ns':s.st_mtime_ns,'inode':s.st_ino})
        entries.append(row)
    return {'root':str(root),'generated_unix':time.time(),'entries':entries,'files':files,'errors':errors}

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('root');parser.add_argument('--output',required=True)
    args=parser.parse_args();report=inventory(args.root)
    Path(args.output).parent.mkdir(parents=True,exist_ok=True)
    Path(args.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'root':report['root'],'entries':len(report['entries']),'file_records':len(report['files']),
        'logical_bytes':sum(x['bytes'] for x in report['entries']),'errors':len(report['errors']),
        'largest':sorted(report['entries'],key=lambda x:x['bytes'],reverse=True)[:12]},ensure_ascii=False,indent=2))
