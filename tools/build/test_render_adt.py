"""Geometry changes must preserve all unrelated boot-tree bytes."""
import struct
import unittest
from make_render_adt import convert,child
import adt

def node(name,values=(),children=()):
    item=adt.Node();item.props=[('name',0,name.encode()+b'\0')]+[(key,0,struct.pack('<I',value)) for key,value in values]
    item.children=list(children);return item

class DisplayADT(unittest.TestCase):
    def source(self):
        chosen=node('chosen',[('display-scale',3)],children=[node('memory-map')])
        chosen.props.append(('protected-fixture',0,b'\xde\xad\xbe\xef'*7))
        root=node('device-tree',children=[chosen,node('product',[('artwork-scale-factor',3)]),
            node('arm-io',children=[node('disp0',[('canvas-width',1125),('canvas-height',2436),('untouched-stride',2160)])])])
        return adt.serialize(root)
    def test_default_exact_four_property_compatibility(self):
        raw=self.source();changed,proof=convert(raw)
        self.assertEqual(proof['canvas_pixels'],[750,1624]);self.assertTrue(proof['other_properties_byte_exact'])
        tree,_=adt.parse(changed);self.assertEqual(child(child(tree,'arm-io'),'disp0').prop('untouched-stride'),struct.pack('<I',2160))
        self.assertEqual(child(tree,'chosen').prop('protected-fixture'),b'\xde\xad\xbe\xef'*7)
    def test_native_points_ratio_and_boot_arguments(self):
        initial=self.source();scaled,_=convert(initial);native,proof=convert(scaled,'xz1c-native-2x')
        self.assertEqual(proof['canvas_pixels'],[720,1280]);self.assertEqual(proof['UIKit_points'],[360,640])
        self.assertTrue(proof['aspect_ratio_preserved']);self.assertEqual(proof['required_boot_arguments']['sf2x'],0)
        self.assertEqual(proof['required_boot_arguments']['sweetfb_native'],1)
        again,_=convert(native,'xz1c-native-2x');self.assertEqual(native,again)
        self.assertEqual(len(initial),len(native));self.assertEqual(proof['device_operations'],0)
    def test_unknown_mixed_missing_duplicate_or_trailing_tree_refused(self):
        raw=self.source()
        for mutate in ('mixed','missing','duplicate','size'):
            tree,_=adt.parse(raw);display=child(child(tree,'arm-io'),'disp0')
            if mutate=='mixed':display.props=[(n,f,struct.pack('<I',750) if n=='canvas-width' else v) for n,f,v in display.props]
            elif mutate=='missing':display.props=[row for row in display.props if row[0]!='canvas-width']
            elif mutate=='duplicate':display.props.append(('canvas-width',0,struct.pack('<I',1125)))
            else:display.props=[(n,f,b'\x01' if n=='canvas-width' else v) for n,f,v in display.props]
            with self.subTest(mutate=mutate),self.assertRaises(AssertionError):convert(adt.serialize(tree),'xz1c-native-2x')
        with self.assertRaises(AssertionError):convert(raw+b'\0'*4)

if __name__=='__main__':unittest.main()
