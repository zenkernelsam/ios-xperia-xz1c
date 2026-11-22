"""Sampler command space is reclaimed only from verified numeric-one options."""
import unittest
from numeric_boot_args import add_sampler,numeric_options,SAMPLER_ONES,PARSER_SHA,native_zero_binding,NATIVE_DEFAULT_KERNEL


class NumericBootArgs(unittest.TestCase):
    def test_only_reviewed_numeric_spelling_changes(self):
        original=['serial=3','shimponly=1','launchd_unsecure_cache=1','sweetfptimeout=2000']+[n+'=1' for n in SAMPLER_ONES]
        compact=[];words,binding=add_sampler(original,compact)
        self.assertEqual(len(' '.join(words)),len(' '.join(original)))
        record=dict(cmdline=' '.join(words),numeric_one_argument_compaction=compact,thread_sample_observation=binding)
        result=numeric_options(record)
        self.assertTrue(all(result[n]=='1' for n in SAMPLER_ONES))
        self.assertEqual(result['shimponly'],'1');self.assertEqual(result['sweetfptimeout'],'2000')
        self.assertEqual(result['sweetsample'],'AegirPoster')
    def test_missing_wrong_duplicate_and_unbound_compaction_refused(self):
        original=[n+'=1' for n in SAMPLER_ONES]
        words,binding=add_sampler(original,[])
        record=dict(cmdline=' '.join(words),numeric_one_argument_compaction=list(SAMPLER_ONES),thread_sample_observation=binding)
        for broken in [dict(record,thread_sample_observation=None),
                       dict(record,numeric_one_argument_compaction=['shimponly']),
                       dict(record,cmdline=record['cmdline']+' rootrw=1'),
                       dict(record,cmdline=record['cmdline'].replace('sweetsample=AegirPoster','sweetsample=SpringBoard'))]:
            with self.assertRaises(AssertionError):numeric_options(broken)
        with self.assertRaises(AssertionError):add_sampler(original[:-1],[])
    def test_idle_trigger_keeps_the_same_numeric_spellings_and_single_command(self):
        compact=[];original=[n+'=1' for n in SAMPLER_ONES]
        words,binding=add_sampler(original,compact,'after_library_idle')
        record=dict(cmdline=' '.join(words),numeric_one_argument_compaction=compact,thread_sample_observation=binding)
        self.assertEqual(numeric_options(record)['sweetsample'],'AegirPoster')
        self.assertEqual(binding['after_seconds'],180);self.assertEqual(binding['max_commands'],1)
        for bad in (dict(binding,after_seconds=179),dict(binding,max_commands=2),dict(binding,process='SpringBoard')):
            with self.assertRaises(AssertionError):numeric_options(dict(record,thread_sample_observation=bad))
    def test_native_geometry_numeric_one_scope_has_exact_profile_and_flags(self):
        geometry={'profile':'xz1c-native-2x','parser_sha256':PARSER_SHA,'required_boot_arguments':{'sweetfb_native':1,'sf2x':0}}
        value={'cmdline':'rootrw sweetfb_native sf2x=0 shimponly=1','numeric_one_argument_compaction':['rootrw','sweetfb_native'],'display_geometry_binding':geometry}
        result=numeric_options(value);self.assertEqual(result['rootrw'],'1');self.assertEqual(result['sweetfb_native'],'1');self.assertEqual(result['sf2x'],'0')
        for bad in (dict(value,display_geometry_binding=None),dict(value,cmdline=value['cmdline'].replace('sf2x=0','sf2x=1')),
                    dict(value,display_geometry_binding=dict(geometry,parser_sha256='0'*64)),dict(value,numeric_one_argument_compaction=['shimponly'])):
            with self.assertRaises(AssertionError):numeric_options(bad)
    def test_native_zero_omission_requires_exact_v730_consumer_and_absent_argument(self):
        geometry={'profile':'xz1c-native-2x','parser_sha256':PARSER_SHA,'required_boot_arguments':{'sweetfb_native':1,'sf2x':0}}
        record={'cmdline':'sweetfb_native shimponly=1','numeric_one_argument_compaction':['sweetfb_native'],
                'display_geometry_binding':geometry,'version':730,'kernel_sha256':NATIVE_DEFAULT_KERNEL,
                'numeric_zero_argument_omission':native_zero_binding()}
        self.assertEqual(numeric_options(record)['sf2x'],'0')
        for bad in [dict(record,numeric_zero_argument_omission=None),dict(record,kernel_sha256='0'*64),
                    dict(record,version=729),dict(record,cmdline=record['cmdline']+' sf2x=1'),
                    dict(record,cmdline=record['cmdline']+' sf2x=0'),dict(record,cmdline=record['cmdline']+' sf2x'),
                    dict(record,numeric_zero_argument_omission=dict(native_zero_binding(),consumer_sha256='0'*64)),
                    dict(record,numeric_zero_argument_omission=dict(native_zero_binding(),values={'sf2x':1}))]:
            with self.assertRaises(AssertionError):numeric_options(bad)


if __name__=='__main__':unittest.main()
