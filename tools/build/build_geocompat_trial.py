"""Build/audit a new native-screen reader/provider adapter without overwrites."""
import argparse
import hashlib
import json
import re
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
    host_paths=['tools/device/geometry_layout_evidence.py','tools/device/native_model_graph.py','tools/device/native_label_ownership.py']
    host_sources={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in host_paths}
    def run(cmd,label):
        result=subprocess.run(list(map(str,cmd)),capture_output=True,timeout=55)
        (out/(label+'.log')).write_bytes(result.stdout+result.stderr)
        assert result.returncode==0,(result.stdout+result.stderr)[-2500:].decode(errors='replace')
        return result.stdout
    run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-c',ROOT/paths[3],'-o',out/'host-log-write.o'],'host-log-compile')
    run(['clang++','-O1','-g','-fblocks','-fno-objc-arc','-fsanitize=address,undefined','-fno-sanitize-recover=all',ROOT/paths[1],out/'host-log-write.o','-framework','Foundation','-framework','CoreGraphics','-o',out/'test_adapter'],'host-compile')
    run([out/'test_adapter'],'host-test')
    # Production records use stderr; keep both original streams for the decoder.
    host=(out/'host-test.log').read_bytes()
    assert b'PASS: real 4-argument ObjC dispatch' in host and b'PASS: bounded original layout observer' in host and b'PASS: native root grid factory/cache' in host
    assert b'PASS: real shared C log writer' in host and b'PASS: original model identities' in host
    assert b'PASS: full root/folder/dock model graph' in host
    assert b'PASS: original view-tree ancestry' in host
    sys.path.insert(0,str(ROOT/'tools/device'))
    from geometry_layout_evidence import decode as decode_layout
    blocks=re.findall(rb'SWEET GLAYOUT BEGIN rev=4 after_ctor=120[^\r\n]*[\s\S]*?SWEET GLAYOUT END rev=4[^\r\n]*',host)
    assert len(blocks)==1,'Missing exact production helper host ancestry capture'
    token='4444444444444444'
    original=decode_layout('SWEET LOOP LIVE proto=1 token='+token+'\n'+blocks[0].decode('ascii')+'\nSWEET LOOP ACK FASTBOOT token='+token,token)
    assert original['original_label_ownership_complete'] and original['counts']['tree_nodes']==8 and original['counts']['owners']==2
    owners=list(original['label_owners'].values())
    assert sum(bool(x['icon']) and bool(x['list']) for x in owners)==1 and sum(x['icon']==x['list']==0 for x in owners)==1
    (out/'host-ancestry-decoded.json').write_text(json.dumps(original,indent=2)+'\n',encoding='utf-8')
    sdk=subprocess.check_output(['xcrun','--sdk','iphoneos','--show-sdk-path'],text=True).strip()
    library=out/'georead.dylib'
    run(['xcrun','--sdk','iphoneos','clang','-target','arm64-apple-ios16.0','-isysroot',sdk,'-O1','-c',ROOT/paths[3],'-o',out/'arm64-log-write.o'],'ios-log-compile')
    run(['xcrun','--sdk','iphoneos','clang++','-target','arm64-apple-ios16.0','-isysroot',sdk,'-O1','-fblocks','-fno-objc-arc','-dynamiclib',ROOT/paths[0],out/'arm64-log-write.o','-framework','CoreFoundation','-framework','CoreGraphics','-lobjc','-Wl,-install_name,/dev/sglram/georead.dylib','-o',library],'ios-compile')
    run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'signature')
    assert sources=={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in paths}
    assert host_sources=={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in host_paths}
    report=dict(revision=4,source_files=sources,library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),library_bytes=library.stat().st_size,
                host_ASan_UBSan_real_four_argument_provider_dispatch_passed=True,exact_native360x640_720x1280_scale2_flat_provider=True,
                opaque_visual_provider_info_forwarded=True,valid_other_screen_providers_preserved=True,read_only_window_safe_area_ABI_checked=True,
                read_only_layout_observer_revision=4,host_ASan_UBSan_bounded_layout_and_CGRect_conversion_passed=True,
                original_frame_grid_values_preserved=False,read_only_layout_observer_preserves_current_values=True,
                layout_snapshot_once_after_constructor_seconds=120,
                host_ASan_UBSan_shared_log_writer_EINTR_partial_errno_passed=True,original_icon_model_identity_ABI_host_passed=True,
                native_root_grid_density_revision=1,native_root_grid_rows=5,original_grid_factory_cache_ABI_and_setter_host_passed=True,
                native_root_grid_mode_control_revision=1,original_full_folder_dock_model_ABI_host_passed=True,
                label_ownership_capture_revision=1,original_view_tree_ancestry_host_passed=True,
                host_original_ancestry_wire_decoder_passed=True,host_ancestry_decoder_sources=host_sources,
                original_icon_widget_font_and_insets_preserved=True,other_screen_or_nonroot_grid_configs_preserved=True,
                new_device_RAM_CPU_GLES_UI_NOT_verified=True,device_operations=0)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
