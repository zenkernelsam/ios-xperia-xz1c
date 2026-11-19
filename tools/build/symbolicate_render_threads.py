"""Resolve one original render-thread sample against its exact GLES call sites."""
import argparse
import bisect
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/device'))
from failed_trial_evidence import verify as verify_failure


def read_sample(text,token):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    scope=text[text.index(live):text.index(ack)]
    headers=list(re.finditer(r'^SMP AegirPoster\[(\d+)\] slide (0x[0-9a-f]+) mh (0x[0-9a-f]+)\r?$',scope,re.M))
    assert len(headers)==1,'Require one actual original process sample'
    header=headers[0];threads=[]
    pattern=re.compile(r'^SMP AegirPoster t(\d+) pc (0x[0-9a-f]+) lr (0x[0-9a-f]+) sp (0x[0-9a-f]+) bt([^\r\n]*)\r?$',re.M)
    for match in pattern.finditer(scope[header.end():]):
        index=int(match[1]);assert index==len(threads)
        frames=[int(pc,16) for pc in match[5].split()]
        tail=scope[header.end()+match.end():]
        continuation=re.match(r'\r?\nSMPBT AegirPoster t'+str(index)+r' bt([^\r\n]*)',tail)
        if continuation:frames.extend(int(pc,16) for pc in continuation[1].split())
        threads.append(dict(index=index,pc=int(match[2],16),lr=int(match[3],16),backtrace=frames))
    assert threads and scope.count('SMP AegirPoster t')==len(threads)
    assert scope.count('SMPBT AegirPoster t')==sum(bool(re.search(r'^SMPBT AegirPoster t'+str(t['index'])+r' bt',scope,re.M)) for t in threads)
    assert scope.count('VERMEER DEBUG: sample queued (coalesced; selected tasks only)')==1
    return dict(pid=int(header[1]),slide=int(header[2],16),main_header=int(header[3],16),threads=threads)


def main():
    p=argparse.ArgumentParser();p.add_argument('--directory',required=True);p.add_argument('--library',required=True)
    p.add_argument('--library-sha256',required=True);p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu-22.04')
    a=p.parse_args();folder=Path(a.directory);library=Path(a.library).resolve();out=Path(a.output).resolve()
    assert not out.exists() and hashlib.sha256(library.read_bytes()).hexdigest()==a.library_sha256
    actual=verify_failure(folder);row=json.loads((folder/'results.json').read_bytes())[-1]
    artifact=row['artifact'];assert artifact['ram_library_binding']['library_sha256']==a.library_sha256
    assert artifact.get('thread_sample_observation',{}).get('process')=='AegirPoster'
    commands=[json.loads(line)['command'] for line in (folder/'cycle-01/serial-commands.jsonl').read_text().splitlines()]
    assert commands==['frame','sample','fastboot '+artifact['token']]
    raw=(folder/'cycle-01/serial.raw.log').read_bytes();assert hashlib.sha256(raw).hexdigest()==actual['log_sha256']
    sample=read_sample(raw.decode('ascii','replace'),artifact['token'])
    def linux(path):
        assert re.fullmatch('[A-Za-z]:',path.drive);return '/mnt/'+path.drive[0].lower()+path.as_posix()[2:]
    def run(tool,*args):
        result=subprocess.run(['wsl.exe','-d',a.distribution,'--',tool,*args],capture_output=True,check=True)
        return result.stdout.decode('utf-8')
    symbols=[]
    for line in run('llvm-nm-14','--numeric-sort','--demangle',linux(library)).splitlines():
        m=re.match(r'([0-9a-f]{16})\s+[tT]\s+(.+)',line)
        if m:symbols.append((int(m[1],16),m[2]))
    def returns(name,target):
        disassembly=run('llvm-objdump-14','--disassemble','--disassemble-symbols='+name,linux(library))
        calls=re.findall(r'^\s*([0-9a-f]+):[^\n]*\bbl\s+[^\n]*<'+re.escape(target)+'>',disassembly,re.M)
        assert len(calls)==1,'Ambiguous production return anchor: '+name
        return int(calls[0],16)+4,disassembly
    clear_return,prepare_disassembly=returns('_prepare_targets','_glDrawArrays')
    targets_return,draw_disassembly=returns('_sweetgl_metal_render_draw','_prepare_targets')
    candidates=[]
    for thread in sample['threads']:
        for pc in thread['backtrace']:
            base=pc-clear_return
            if base>=0x100000000 and base%0x4000==0 and base+targets_return in thread['backtrace']:
                candidates.append((base,thread['index']))
    assert len(candidates)==1,'Need one thread with both actual native return PCs'
    base,render_thread=candidates[0];addresses=[offset for offset,name in symbols];rows=[]
    for thread in sample['threads']:
        frames=[]
        for index,pc in enumerate([thread['pc'],thread['lr']]+thread['backtrace']):
            if base<=pc<base+0x700000:
                offset=pc-base;position=bisect.bisect_right(addresses,offset)-1;assert position>=0
                symbol_offset,name=symbols[position]
                frames.append(dict(frame=index,relative_pc=hex(offset),symbol=name,symbol_offset=hex(offset-symbol_offset)))
        rows.append(dict(index=thread['index'],native_library_frames=frames))
    render=rows[render_thread];names=[frame['symbol'] for frame in render['native_library_frames']]
    assert all(name in names for name in ('_prepare_targets','_sweetgl_metal_render_draw','__mesa_DrawArrays','_fd5_draw_vbos','_ir3_cache_lookup','_ir3_get_shader','__util_queue_fence_wait'))
    workers=[t['index'] for t in rows if any('_ir3_compile_shader_nir'==f['symbol'] or '_ir3_nir_post_finalize'==f['symbol'] for f in t['native_library_frames'])]
    assert workers
    out.mkdir(parents=True);(out/'prepare-targets-callsite.txt').write_text(prepare_disassembly,encoding='utf-8');(out/'draw-targets-callsite.txt').write_text(draw_disassembly,encoding='utf-8')
    report=dict(token=artifact['token'],raw_sha256=actual['log_sha256'],GLES_sha256=a.library_sha256,
        pid=sample['pid'],actual_shared_cache_slide=hex(sample['slide']),sample_threads=len(rows),
        base_inferred_by_two_unique_actual_16K_aligned_return_PCs=True,
        clear_draw_return_offset=hex(clear_return),render_targets_return_offset=hex(targets_return),
        render_thread=render_thread,compiler_worker_threads=workers,threads=rows,
        original_result=row['result'],return_seconds_with_health=row['reset_to_fastboot_seconds'],
        compiler_worker_samples_and_original_draw_fence_wait_observed=True,
        entire_wait_duration_or_CPU_hardware_time_not_inferred=True,
        optical_UI_not_verified=True,post_ACK_exit_period_excluded=True,
        kernel_allocator_SCM_PON_RPC_VM_unchanged=True,tool_source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (out/'symbols.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({k:report[k] for k in ('token','pid','sample_threads','render_thread','compiler_worker_threads')}))


if __name__=='__main__':main()
