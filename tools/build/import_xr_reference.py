"""Retain pinned OSS XR colour tables/notices; no Vulkan driver is built."""
import argparse,hashlib,json,re,subprocess,urllib.request
from pathlib import Path
root=Path(__file__).resolve().parents[2]
revision='219f01365dc4c8eb346816c405b5b17403793764'
upstream='https://github.com/steelbrain/metal2vulkan'
paths={'extended_srgb.rs':'crates/metal2vulkan/src/metal/format/extended_srgb.rs',
    'format.rs':'crates/metal2vulkan/src/metal/format/mod.rs',
    'LICENSE':'LICENSE','LICENSE-LGPL-3.0':'LICENSE-LGPL-3.0'}
def generate(source):
    result='/* SPDX-License-Identifier: LGPL-3.0-or-later\n * Tables from metal2vulkan '+revision+'.\n * Original source/notices: gpu/metal/xr/reference; generation/import manifest retained.\n */\n#pragma once\n#include <stdint.h>\n'
    for original,target in (('SRGB_DECODE_STEPS','sweet_xr_srgb_decode_steps'),('SRGB_ENCODE_FIRST','sweet_xr_srgb_encode_first')):
        match=re.search(r'const '+original+r': \[u16; 640\] = \[(.*?)\];',source,re.S);assert match
        values=[int(value.strip()) for value in match[1].split(',') if value.strip()];assert len(values)==640 and values==sorted(values)
        result+='static const uint16_t '+target+'[640] = {\n'
        for index in range(0,640,16):result+='    '+','.join(map(str,values[index:index+16]))+',\n'
        result+='};\n'
    return result.encode()
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--from-repository');args=parser.parse_args()
    reference=root/'gpu/metal/xr/reference';reference.mkdir(parents=True,exist_ok=True);rows={}
    for name,source in paths.items():
        if args.from_repository:
            repo=Path(args.from_repository);assert subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD']).decode().strip()==revision
            data=subprocess.check_output(['git','-C',str(repo),'show',revision+':'+source])
        else:
            with urllib.request.urlopen(upstream.replace('github.com','raw.githubusercontent.com')+'/'+revision+'/'+source,timeout=30) as response:data=response.read(256*1024)
        target=reference/name
        if target.exists():assert target.read_bytes()==data
        else:target.write_bytes(data)
        rows[source]={'destination':target.relative_to(root).as_posix(),'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data)}
    generated=generate((reference/'extended_srgb.rs').read_text());target=root/'gpu/sweetgl/XRColourTables.h'
    if target.exists():assert target.read_bytes()==generated
    else:target.write_bytes(generated)
    manifest=dict(upstream=upstream,revision=revision,sources=rows,adapted_table_license='LGPL-3.0-or-later',
        generated_header=target.relative_to(root).as_posix(),generated_sha256=hashlib.sha256(generated).hexdigest(),Vulkan_runtime_or_driver_imported=False,device_operations=0)
    (root/'reproducibility/xr-reference.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());print(json.dumps(manifest))
if __name__=='__main__':main()
