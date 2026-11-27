"""Build/test the nonzero property adapter and audit fixed Sony duty contracts."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]
COMMIT='33dc678e7692f04ecc1eeddeb1cca8a809bc1727'


def main():
    p=argparse.ArgumentParser();p.add_argument('--reference',required=True);p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    ref=Path(a.reference).resolve();assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=ref,text=True).strip()==COMMIT
    source=subprocess.check_output(['git','show',COMMIT+':drivers/leds/leds-qpnp-wled.c'],cwd=ref)
    text=source.decode();assert 'low_limit = WLED_MAX_LEVEL_4095 * 4 / 1000' in text
    assert 'level_255 = DIV_ROUND_CLOSEST(level, 16)' in text
    paths=['xnu/support/LilacBacklightDutyBackend.h','xnu/support/LilacBacklightProperties.h','xnu/support/LilacWledDutyTransaction.h','xnu/support/LilacWledDutyPlan.h','xnu/support/LilacWledSnapshot.h','tools/build/test_lilac_backlight_duty_backend.cpp','tools/build/check_lilac_backlight_duty_backend.py']
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={name:sha(ROOT/name) for name in paths}
    def linux(path):
        path=Path(path).resolve();return '/mnt/'+path.drive[0].lower()+'/'+path.as_posix()[3:]
    prefix=['wsl.exe','-d',a.distribution,'--exec'] if os.name=='nt' else []
    def host(path):return linux(path) if os.name=='nt' else str(path)
    def run(cmd,name):
        r=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=60);(out/(name+'.stdout')).write_bytes(r.stdout);(out/(name+'.stderr')).write_bytes(r.stderr)
        assert r.returncode==0,r.stderr.decode(errors='replace');return r
    binary=out/'host-test'
    run([*prefix,'c++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-sanitize-recover=all',*(['-no-pie'] if os.name=='nt' else []),host(ROOT/'tools/build/test_lilac_backlight_duty_backend.cpp'),'-o',host(binary)],'compile')
    r=run([*prefix,host(binary)],'host-test');assert b'property_values=65535' in r.stdout and b'epoch_reentry_faults=1' in r.stdout
    dtb=ROOT/'assets/device-tree/dtb.img';assert sha(dtb)=='06f4c3493aad91d2240508b0c5788d4799c9c2a58357462507b3bc5e28bf6669'
    node='/soc/qcom,spmi@800f000/qcom,pmi8998@3/qcom,leds@d800'
    props=run([*prefix,'fdtget','-p',host(dtb),node],'dtb-properties').stdout.decode().split()
    assert 'qcom,wled-brightness-map' not in props and 'qcom,wled-stepper-en' not in props
    delay=run([*prefix,'fdtget','-t','u',host(dtb),node,'qcom,cons-sync-write-delay-us'],'dtb-sync-delay').stdout.strip();assert delay==b'1000'
    assert before=={name:sha(ROOT/name) for name in paths}
    report=dict(Sony_commit=COMMIT,Sony_driver_sha256=hashlib.sha256(source).hexdigest(),stock_dtb_sha256=sha(dtb),stock_optional_256_entry_map_absent=True,stock_stepper_absent=True,stock_sync_delay_us=1000,source_files=before,host_ASan_UBSan=True,property_values_checked=65535,epoch_reentry_write_fault_checks_passed=True,unsupported_power_fade_level_do_not_write=True,no_control_power_writes=True,physical_brightness_mapping_and_IO_callbacks_NOT_verified=True,device_operations=0)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))


if __name__=='__main__':main()
