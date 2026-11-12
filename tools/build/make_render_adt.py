"""Convert the supplied board ADT to the exact four-property 750x1624 display contract."""
import argparse,hashlib,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]));import adt
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('output');a=p.parse_args()
source=Path(a.source);output=Path(a.output);assert not output.exists()
data=source.read_bytes();tree,end=adt.parse(data);assert end==len(data)
def child(node,name):
    found=[c for c in node.children if c.name==name];assert len(found)==1,name;return found[0]
display=child(child(tree,'arm-io'),'disp0')
for node,key,before,after in [(child(tree,'chosen'),'display-scale',3,2),(child(tree,'product'),'artwork-scale-factor',3,2),
                             (display,'canvas-width',1125,750),(display,'canvas-height',2436,1624)]:
    assert node.prop(key)==struct.pack('<I',before),key
    node.props=[(n,f,struct.pack('<I',after) if n==key else v) for n,f,v in node.props]
changed=adt.serialize(tree);assert len(changed)==len(data);output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(changed)
print('render_adt_sha256='+hashlib.sha256(changed).hexdigest())
