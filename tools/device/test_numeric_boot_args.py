"""Sampler command space is reclaimed only from verified numeric-one options."""
import unittest
from numeric_boot_args import add_sampler,numeric_options,SAMPLER_ONES


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


if __name__=='__main__':unittest.main()
