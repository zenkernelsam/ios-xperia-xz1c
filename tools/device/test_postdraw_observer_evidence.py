"""Reject mixed, truncated or unbalanced timing; keep original pending calls."""
from postdraw_observer_evidence import inspect, INSTALLED, APIS, METHOD_APIS, METHOD_ARGUMENTS

TOKEN = '123456789abcdef0'


def fixture():
    originals = '\n'.join(f'SWEET POSTDRAW ORIGINAL pid=285 api={api} image=ImageIO offset=0x10 resolved=1' for api in APIS)
    return (f'SWEET LOOP LIVE proto=1 token={TOKEN}\n{INSTALLED}\n{originals}\n'
            'SWEET POSTDRAW BEGIN pid=285 tid=33 call=1 root=1 depth=1 api=png at_ns=100 original_forwarded=1\n'
            'SWEET POSTDRAW BEGIN pid=285 tid=33 call=2 root=1 depth=2 api=finalize at_ns=110 original_forwarded=1\n'
            'SWEET POSTDRAW END pid=285 tid=33 call=2 api=finalize wall_ns=50 cpu_ns=40 cpu_valid=1 outcome=true bytes=0 returned_nil=0 original_behavior=1\n'
            'SWEET POSTDRAW END pid=285 tid=33 call=1 api=png wall_ns=80 cpu_ns=60 cpu_valid=1 outcome=return bytes=200 returned_nil=0 original_behavior=1\n'
            f'SWEET LOOP ACK FASTBOOT token={TOKEN}\n'
            'SWEET POSTDRAW BEGIN pid=285 tid=33 call=3 root=3 depth=1 api=png at_ns=500 original_forwarded=1\n')


def rejects(text, token=TOKEN):
    try:
        inspect(text, token)
    except (AssertionError, KeyError):
        return
    raise AssertionError('Malformed evidence was accepted')


text = fixture()
result = inspect(text, TOKEN)
assert result['complete_samples'] == 2 and not result['pending_calls']
assert result['longest_completed_inclusive_calls'][0]['wall_ns'] == 80
assert all(record['call'] != 3 for record in result['records'])
pending = text.replace('SWEET POSTDRAW END pid=285 tid=33 call=1 api=png wall_ns=80 cpu_ns=60 cpu_valid=1 outcome=return bytes=200 returned_nil=0 original_behavior=1\n', '')
result = inspect(pending, TOKEN)
assert result['complete_samples'] == 1 and result['incomplete_samples'] == 1
assert result['pending_calls'][0]['api'] == 'png'
rejects(text, 'fedcba9876543210')
rejects(text.replace('root=1 depth=2', 'root=2 depth=2'))
rejects(text.replace('END pid=285 tid=33 call=2', 'END pid=285 tid=34 call=2'))
rejects(text.replace('original_behavior=1\nSWEET POSTDRAW END', 'original_behavior=\nSWEET POSTDRAW END'))
rejects(text.replace('cpu_valid=1 outcome=true', 'cpu_valid=0 outcome=true'))
rejects(text.replace('outcome=true bytes=0', 'outcome=true bytes=1'))
rejects(text.replace('api=colorsync image=ImageIO', 'api=png image=ImageIO'))
print('postdraw evidence PASS: nesting/CPU state/token/truncation/identity rejection, inclusive durations and pending/exit separation')

metadata=('SWEET SNAPSHOT INIT pid=285 at_ns=90 expected=8 metadata_only=1\n'+
          '\n'.join(f'SWEET SNAPSHOT METHOD pid=285 api={api} present=1 owned=1 installed=1 arguments={METHOD_ARGUMENTS[api]} original_forwarded=1' for api in METHOD_APIS)+
          '\nSWEET SNAPSHOT observer pid=285 installed=8 expected=8 limit=64 metadata_only=1 original_arguments_results=1\n')
timed=text.replace('SWEET POSTDRAW BEGIN pid=285 tid=33 call=1',metadata+'SWEET POSTDRAW BEGIN pid=285 tid=33 call=1',1)
timed=timed.replace('depth=1 api=png at_ns=100','depth=1 api=snapshot at_ns=100')
timed=timed.replace('call=1 api=png wall_ns=80','call=1 api=snapshot wall_ns=80').replace('outcome=return bytes=200','outcome=return bytes=0')
timed=timed.replace('api=snapshot at_ns=100 original_forwarded=1\n','api=snapshot at_ns=100 original_forwarded=1\nSWEET SNAPSHOT OBJECT pid=285 call=1 api=snapshot object=0x1000 metadata_only=1\n')
result=inspect(timed,TOKEN,method_observer=True)
assert result['snapshot_method_coverage']=={285:8} and result['records'][0]['object_tag']=='0x1000'
assert result['records'][0]['since_snapshot_observer_init_ns']==10
rejects(timed)
for broken in (timed.replace('api=snapshot present=1 owned=1 installed=1 arguments=5','api=snapshot present=1 owned=1 installed=1 arguments=4'),
               timed.replace('observer pid=285 installed=8','observer pid=285 installed=7'),
               timed.replace('SWEET SNAPSHOT OBJECT pid=285 call=1 api=snapshot object=0x1000 metadata_only=1\n','')):
    try:
        inspect(broken,TOKEN,method_observer=True)
    except (AssertionError,KeyError):
        continue
    raise AssertionError('Malformed method timing evidence was accepted')
print('snapshot method evidence PASS: actual coverage/ABI/object/init binding and partial/mixed rejection')
