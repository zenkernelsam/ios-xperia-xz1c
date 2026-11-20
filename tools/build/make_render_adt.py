"""Create a checked display ADT; native geometry remains a separate device trial."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]));import adt
PROFILES={'iphone-x-2x':(750,1624,2),'xz1c-native-2x':(720,1280,2)}

def child(node,name):
    found=[c for c in node.children if c.name==name];assert len(found)==1,name;return found[0]

def convert(data,profile='iphone-x-2x'):
    assert profile in PROFILES
    tree,end=adt.parse(data);assert end==len(data)
    chosen=child(tree,'chosen');product=child(tree,'product');display=child(child(tree,'arm-io'),'disp0')
    fields=[(chosen,'display-scale'),(product,'artwork-scale-factor'),(display,'canvas-width'),(display,'canvas-height')]
    original=[]
    for node,key in fields:
        records=[v for n,f,v in node.props if n==key]
        assert len(records)==1 and len(records[0])==4,key
        original.append(struct.unpack('<I',records[0])[0])
    assert tuple(original) in ((3,3,1125,2436),(2,2,750,1624),(2,2,720,1280)),'Unknown or inconsistent display contract'
    width,height,scale=PROFILES[profile];after=[scale,scale,width,height]
    for (node,key),value in zip(fields,after):
        node.props=[(n,f,struct.pack('<I',value) if n==key else v) for n,f,v in node.props]
    changed=adt.serialize(tree);assert len(changed)==len(data)
    # Remove only the four new values and reproduce every original byte.
    for (node,key),value in zip(fields,original):
        node.props=[(n,f,struct.pack('<I',value) if n==key else v) for n,f,v in node.props]
    assert adt.serialize(tree)==data,'Unrelated ADT properties or representation changed'
    manifest={'revision':1,'profile':profile,'canvas_pixels':[width,height],'UIKit_points':[width//scale,height//scale],
        'display_scale':scale,'panel_pixels':[720,1280],'aspect_ratio_preserved':width*1280==height*720,
        'source_sha256':hashlib.sha256(data).hexdigest(),'render_adt_sha256':hashlib.sha256(changed).hexdigest(),
        'four_properties_only':True,'other_properties_byte_exact':True,'device_operations':0,'device_geometry_acceptance_pending':True,
        'required_boot_arguments':{'sweetfb_native':1,'sf2x':0,'sweetfb_xf':8} if profile=='xz1c-native-2x' else {'sf2x':1},
        'MobileGestalt_and_UIScreen_must_be_verified_on_the_same_boot':True,
        'statusbar_safe_area_original_UI_touch_and_optical_acceptance_pending':True}
    return changed,manifest

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source');p.add_argument('output')
    p.add_argument('--profile',choices=PROFILES,default='iphone-x-2x');p.add_argument('--manifest')
    args=p.parse_args();source=Path(args.source);output=Path(args.output);assert not output.exists()
    manifest=Path(args.manifest) if args.manifest else None
    if manifest:assert not manifest.exists() and manifest.resolve()!=output.resolve()
    changed,proof=convert(source.read_bytes(),args.profile);output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(changed)
    if manifest:manifest.parent.mkdir(parents=True,exist_ok=True);manifest.write_bytes((json.dumps(proof,indent=2)+'\n').encode())
    print('render_adt_sha256='+proof['render_adt_sha256'])

if __name__=='__main__':main()
