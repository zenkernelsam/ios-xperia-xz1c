"""Check that compact fitted mode keeps its original runtime meaning."""
from copy import deepcopy
import unittest

from native_grid_control import (
    FITTED_DEFAULT_LIBRARY_SHA, FITTED_DEFAULT_SOURCE_SHA, HELPER_SOURCE,
    apply, compact_fitted, validate, verify_default_runtime,
)


ENV = ('sweetenv=backboardd:SWEETGL_TEST=1;'
       'SpringBoard:DYLD_INSERT_LIBRARIES=/dev/sglram/g,GEO_FIX=1;'
       'AegirPoster:DYLD_INSERT_LIBRARIES=/dev/sglram/e')
EXPLICIT_R21_SOURCE = '783c8393c0eadb0a831471b8b17ebd46b49aee4c0e010054085d5269a01f29a3'


class NativeGridControl(unittest.TestCase):
    def setUp(self):
        self.package = {
            'geocompat_build_binding': {
                'revision': 4, 'native_root_grid_mode_control_revision': 1,
                'library_sha256': FITTED_DEFAULT_LIBRARY_SHA,
                'source_files': {HELPER_SOURCE: FITTED_DEFAULT_SOURCE_SHA},
            },
            'files': {'georead.dylib': {'sha256': FITTED_DEFAULT_LIBRARY_SHA}},
        }
        self.explicit, self.binding = apply([ENV], 'fitted')
        self.compacted, self.default = compact_fitted(self.explicit, self.binding, self.package)

    def record(self, words, binding):
        return {'trial_mode': 'gpu', 'display_geometry_binding': {'profile': 'xz1c-native-2x'},
                'native_grid_control': binding, 'cmdline': ' '.join(words)}

    def test_only_pinned_helper_can_omit_fitted_env(self):
        self.assertEqual(self.compacted, [ENV])
        self.assertEqual(validate(self.record(self.compacted, self.default), self.package), self.default)
        for group, key in (('geocompat_build_binding', 'library_sha256'),
                           ('geocompat_build_binding', 'source_files'),
                           ('files', 'georead.dylib')):
            with self.subTest(group=group, key=key):
                broken = deepcopy(self.package)
                broken[group].pop(key)
                self.assertIsNone(compact_fitted(self.explicit, self.binding, broken))
                with self.assertRaises(ValueError):
                    validate(self.record(self.compacted, self.default), broken)

    def test_explicit_legacy_values_still_verified(self):
        for mode in ('original', 'fitted'):
            words, binding = apply([ENV], mode)
            old = dict(binding, source_sha256=EXPLICIT_R21_SOURCE)
            self.assertEqual(validate(self.record(words, old), self.package), old)
            with self.assertRaises(ValueError):
                validate(self.record([ENV], old), self.package)
        words, binding = apply([ENV], 'original')
        self.assertIsNone(compact_fitted(words, binding, self.package))

    def test_refuses_overridden_default_or_wrong_helper_injection(self):
        invalid_envs = (self.explicit, [ENV.replace('GEO_FIX=1', 'GEO_FIX=1,GEO_GRID=0')],
                        [ENV.replace('GEO_FIX=1', 'GEO_FIX=1,GEO_GRID')],
                        [ENV.replace('/dev/sglram/g', '/dev/sglram/e')],
                        [ENV + ';SpringBoard:GEO_GRID=0'], [ENV, ENV])
        for words in invalid_envs:
            with self.subTest(words=words), self.assertRaises(ValueError):
                validate(self.record(words, self.default), self.package)

    def test_default_must_be_confirmed_by_original_springboard_marker(self):
        marker = '|SpringBoard| SWEET GEOGRID MODE value='
        verify_default_runtime(marker + '1\n')
        for raw in ('', '|backboardd| SWEET GEOGRID MODE value=1\n', marker + '0\n',
                    marker + '1\n' + marker + '0\n'):
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                verify_default_runtime(raw)


if __name__ == '__main__':
    unittest.main()
