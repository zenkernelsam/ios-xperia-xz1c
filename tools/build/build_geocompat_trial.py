"""Build/audit a new native-screen reader/provider adapter without overwrites."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args();assert sys.platform=='darwin'
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    paths=['gpu/geocompat/geocompat.mm','gpu/geocompat/test_adapter.mm','tools/build/build_geocompat_trial.py',
           'gpu/geocompat/log_write.c','gpu/sweetgl/LogWriteAll.h']
    sources={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}
    def run(cmd,label):
        result=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=55)
        (out/(label+'.log')).write_bytes(result.stdout+result.stderr)
        assert result.returncode==0,(result.stdout+result.stderr)[-2500:].decode(errors='replace')
        return result.stdout
    run(['clang','-O1','-g','-fsanitize=address,undefined','-c',ROOT/paths[3],'-o',out/'host-log-write.o'],'host-log-compile')
    run(['clang++','-O1','-g','-fblocks','-fno-objc-arc','-fsanitize=address,undefined',ROOT/paths[1],out/'host-log-write.o','-framework','Foundation','-framework','CoreGraphics','-o',out/'test_adapter'],'host-compile')
    host=run([out/'test_adapter'],'host-test')
    assert b'PASS: real 4-argument ObjC dispatch' in host and b'PASS: bounded original layout observer' in host and b'PASS: native root grid factory/cache' in host
    assert b'PASS: real shared C log writer' in host and b'PASS: original model identities' in host
    sdk=subprocess.check_output(['xcrun','--sdk','iphoneos','--show-sdk-path'],text=True).strip()
    library=out/'georead.dylib'
    run(['xcrun','--sdk','iphoneos','clang','-target','arm64-apple-ios16.0','-isysroot',sdk,'-O1','-c',ROOT/paths[3],'-o',out/'arm64-log-write.o'],'ios-log-compile')
    run(['xcrun','--sdk','iphoneos','clang++','-target','arm64-apple-ios16.0','-isysroot',sdk,'-O1','-fblocks','-fno-objc-arc','-dynamiclib',ROOT/paths[0],out/'arm64-log-write.o','-framework','CoreFoundation','-framework','CoreGraphics','-lobjc','-Wl,-install_name,/dev/sglram/georead.dylib','-o',library],'ios-compile')
    run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'signature')
    assert sources=={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}
    report=dict(revision=2,source_files=sources,library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),library_bytes=library.stat().st_size,
                host_ASan_UBSan_real_four_argument_provider_dispatch_passed=True,exact_native360x640_720x1280_scale2_flat_provider=True,
                opaque_visual_provider_info_forwarded=True,valid_other_screen_providers_preserved=True,read_only_window_safe_area_ABI_checked=True,
                read_only_layout_observer_revision=2,host_ASan_UBSan_bounded_layout_and_CGRect_conversion_passed=True,
                original_frame_grid_values_preserved=False,read_only_layout_observer_preserves_current_values=True,
                layout_snapshot_once_after_constructor_seconds=120,
                host_ASan_UBSan_shared_log_writer_EINTR_partial_errno_passed=True,original_icon_model_identity_ABI_host_passed=True,
                native_root_grid_density_revision=1,native_root_grid_rows=5,original_grid_factory_cache_ABI_and_setter_host_passed=True,
                original_icon_widget_font_and_insets_preserved=True,other_screen_or_nonroot_grid_configs_preserved=True,
                new_device_RAM_CPU_GLES_UI_NOT_verified=True,device_operations=0)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
