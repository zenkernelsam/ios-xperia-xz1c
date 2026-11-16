import unittest
from original_prime_batch_evidence import compile_pairs


class CompilePairs(unittest.TestCase):
    def vertex(self, n, hash):
        return f'SWEET AIR RENDER compile_begin stage=vertex bytes={n} hash={hash}\n'

    def fragment(self, n, hash):
        return f'SWEET AIR RENDER compile_begin stage=fragment bytes={n} hash={hash}\n'

    def test_complete_actual_pair(self):
        text = self.vertex(4879, 'a'*16)+'SWEET AIR RENDER compile_end stage=vertex status=1\n'+self.fragment(42349, 'b'*16)
        self.assertEqual(compile_pairs(text), {(4879, 'a'*16, 42349, 'b'*16)})

    def test_intervening_vertex_refuses_false_pair(self):
        text = self.vertex(4840, 'c'*16)+self.vertex(4879, 'a'*16)+self.fragment(42349, 'b'*16)
        self.assertNotIn((4840, 'c'*16, 42349, 'b'*16), compile_pairs(text))

    def test_missing_fragment_is_incomplete(self):
        self.assertEqual(compile_pairs(self.vertex(4879, 'a'*16)), set())


if __name__ == '__main__':
    unittest.main(verbosity=2)
