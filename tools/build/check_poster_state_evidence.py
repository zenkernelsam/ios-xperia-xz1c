"""Check original state-stream completeness and foreign/corrupt record rejection."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/device'))
from poster_state_evidence import inspect

TOKEN='6666666666666666'


def fixture():
    lines=['SWEET LOOP LIVE proto=1 token='+TOKEN,
           'SWEET PST CONFIG pid=9 revision=1 apis=11 slots=256 max=512 original_returns=1 getter_calls=0 GPU_work=0']
    samples=[('preview','B',0x100,1,0),('backlight','D',0x100,0x3ff0000000000000,0),
             ('unlock','D',0x100,0,0),('unlock','D',0x200,0x3ff0000000000000,0),
             ('env_update','E',0x200,0x300,0x400),('renderer_env','I',0x300,0x200,0),
             ('min_size','S',0x200,0x4076800000000000,0x4084000000000000),
             ('linear_backlight','D',0x200,0x7ff8000000001234,0)]
    for n,(api,kind,receiver,a,b) in enumerate(samples,1):
        lines+=[f'SWEET PST VALUE pid=9 sample={n} api={api} receiver={receiver:016x} kind={kind} outcome=R ticks={n:016x}',
                f'SWEET PST BITS pid=9 sample={n} first={a:016x} second={b:016x}']
    lines+=['SWEET LOOP ACK FASTBOOT token='+TOKEN,
            'SWEET PST VALUE pid=9 sample=99 api=preview receiver=0000000000000999 kind=B outcome=R ticks=0000000000000999']
    return '\n'.join(lines)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path);args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    raw=fixture();result=inspect(raw,TOKEN)
    assert result['complete_samples']==8 and not result['limits']
    assert result['records'][3]['double_value']==1 and result['records'][3]['receiver_identity']=='0000000000000200'
    assert result['records'][4]['transition_identity']=='0000000000000400'
    assert result['records'][5]['return_identity']=='0000000000000200'
    assert result['records'][6]['size_values']==[360,640]
    assert result['records'][7]['double_value'] is None and result['records'][7]['first_bits']=='7ff8000000001234'
    # Preserve nil/throw and raw negative-zero bits without a guessed default.
    thrown=inspect(raw.replace('sample=1 api=preview receiver=0000000000000100 kind=B outcome=R',
                              'sample=1 api=preview receiver=0000000000000100 kind=B outcome=T'),TOKEN)
    assert 'boolean_result' not in thrown['records'][0]
    faults={}
    for name,change in {
        'missing_bits':lambda s:s.replace('SWEET PST BITS pid=9 sample=1 first=0000000000000001 second=0000000000000000\n',''),
        'duplicate_header':lambda s:s.replace('SWEET PST VALUE pid=9 sample=2','SWEET PST VALUE pid=9 sample=1'),
        'foreign_pid':lambda s:s.replace('SWEET PST BITS pid=9 sample=1','SWEET PST BITS pid=10 sample=1'),
        'wrong_kind':lambda s:s.replace('api=preview receiver=0000000000000100 kind=B','api=preview receiver=0000000000000100 kind=D'),
        'missing_sample':lambda s:s.replace('sample=4','sample=99'),
        'invalid_bool':lambda s:s.replace('sample=1 first=0000000000000001','sample=1 first=0000000000000100'),
        'unknown_record':lambda s:s.replace('SWEET PST BITS pid=9 sample=1','SWEET PST BAD pid=9 sample=1'),
        'scalar_padding':lambda s:s.replace('sample=1 first=0000000000000001 second=0000000000000000','sample=1 first=0000000000000001 second=0000000000000001'),
        'wrong_token':lambda s:s.replace('LIVE proto=1 token='+TOKEN,'LIVE proto=1 token=7777777777777777'),
    }.items():
        try:inspect(change(raw),TOKEN)
        except ValueError:faults[name]=True
        else:raise AssertionError('Invalid original state evidence accepted: '+name)
    paths=[Path(__file__),ROOT/'tools/device/poster_state_evidence.py']
    proof={'source_files':{p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
           'same_original_receiver_and_changed_environment_values_checked':True,'raw_scalar_NaN_and_size_bits_preserved':True,
           'post_ACK_excluded':True,'faults_refused':faults,'synthetic_fixture_not_device_environment_or_UI':True,'device_operations':0}
    with (args.output/'manifest.json').open('x',encoding='utf-8',newline='\n') as f:json.dump(proof,f,indent=2);f.write('\n')
    print(json.dumps(proof))


if __name__=='__main__':main()
