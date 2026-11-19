"""Numeric-one spellings verified against pinned XNU PE_parse_boot_argn."""
SAMPLER_ONES=('ufsrw','rootrw','cs_enforcement_disable','amfi_get_out_of_my_way',
              'sweetfb_noconsole','lilac_usbdbg','sweetrmi4','sweetfb_bootscan',
              'sweetgpuplan','sweetloopdrain','ssca','sfscreen')
PARSER_SHA='1431f1f8aacae240b87bd29cd91905784c968f8014784201b3ed862bdb1b4ed9'


def numeric_options(record):
    words=record['cmdline'].split();options={w.split('=',1)[0]:w.split('=',1)[1] for w in words if '=' in w}
    compact=record.get('numeric_one_argument_compaction',[])
    assert len(compact)==len(set(compact)) and set(compact)<=set(SAMPLER_ONES)|{'sweetbattery','sf2x'}
    for name in compact:
        assert words.count(name)==1 and name not in options
        options[name]='1'
    if set(compact)&set(SAMPLER_ONES):
        assert record.get('thread_sample_observation')==dict(process='AegirPoster',command='sample',max_commands=1,trigger='first_target_clear',parser_sha256=PARSER_SHA)
        assert options.get('sweetsample')=='AegirPoster'
    return options


def add_sampler(words, compact):
    assert not any(w.startswith('sweetsample') for w in words)
    # These twelve options all use numeric PE_parse_boot_argn consumers in
    # the audited kernel. String-valued/user-space and shim options stay intact.
    for name in SAMPLER_ONES:
        assert words.count(name+'=1')==1
        words=[name if w==name+'=1' else w for w in words]
        compact.append(name)
    words.append('sweetsample=AegirPoster')
    return words,dict(process='AegirPoster',command='sample',max_commands=1,trigger='first_target_clear',parser_sha256=PARSER_SHA)
