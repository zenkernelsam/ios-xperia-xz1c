"""Numeric-one spellings verified against pinned XNU PE_parse_boot_argn."""
SAMPLER_ONES=('ufsrw','rootrw','cs_enforcement_disable','amfi_get_out_of_my_way',
              'sweetfb_noconsole','lilac_usbdbg','sweetrmi4','sweetfb_bootscan',
              'sweetgpuplan','sweetloopdrain','ssca','sfscreen')
PARSER_SHA='1431f1f8aacae240b87bd29cd91905784c968f8014784201b3ed862bdb1b4ed9'
NATIVE_DEFAULT_KERNEL='f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d'
NATIVE_DEFAULT_CONSUMER='e5b0a52ad5f6608589dbb12f890fa096845463187d78805d673caf7d97071ad2'

def native_zero_binding():
    # Exact compiled v730 SweetFramebuffer: int render2x=0, then the pinned
    # PE_parse_boot_argn("sf2x",...) leaves that value unchanged when absent.
    return dict(revision=1,values={'sf2x':0},kernel_sha256=NATIVE_DEFAULT_KERNEL,
                consumer='iokit/Kernel/arm/SweetFramebuffer.cpp',consumer_sha256=NATIVE_DEFAULT_CONSUMER,
                parser_sha256=PARSER_SHA)

def sampler_binding(trigger='first_target_clear'):
    assert trigger in ('first_target_clear','after_library_idle')
    value=dict(process='AegirPoster',command='sample',max_commands=1,trigger=trigger,parser_sha256=PARSER_SHA)
    if trigger=='after_library_idle':value['after_seconds']=180
    return value


def numeric_options(record):
    words=record['cmdline'].split();options={w.split('=',1)[0]:w.split('=',1)[1] for w in words if '=' in w}
    compact=record.get('numeric_one_argument_compaction',[])
    geometry=record.get('display_geometry_binding')
    native=geometry is not None
    if native:
        assert geometry['profile']=='xz1c-native-2x' and geometry['parser_sha256']==PARSER_SHA
        assert geometry['required_boot_arguments']=={'sweetfb_native':1,'sf2x':0}
    assert len(compact)==len(set(compact)) and set(compact)<=set(SAMPLER_ONES)|{'sweetbattery','sf2x'}|({'sweetfb_native'} if native else set())
    for name in compact:
        assert words.count(name)==1 and name not in options
        options[name]='1'
    defaults=record.get('numeric_zero_argument_omission')
    if defaults is not None:
        assert native and defaults==native_zero_binding()
        assert record.get('version')==730 and record.get('kernel_sha256')==NATIVE_DEFAULT_KERNEL
        assert 'sf2x' not in options and 'sf2x' not in words
        options['sf2x']='0'
    if set(compact)&set(SAMPLER_ONES) and not native:
        observed=record.get('thread_sample_observation')
        assert isinstance(observed,dict) and observed==sampler_binding(observed.get('trigger'))
        assert options.get('sweetsample')=='AegirPoster'
    if native:assert options.get('sf2x')=='0' and options.get('sweetfb_native')=='1'
    return options


def add_sampler(words, compact,trigger='first_target_clear'):
    assert not any(w.startswith('sweetsample') for w in words)
    # These twelve options all use numeric PE_parse_boot_argn consumers in
    # the audited kernel. String-valued/user-space and shim options stay intact.
    for name in SAMPLER_ONES:
        assert words.count(name+'=1')==1
        words=[name if w==name+'=1' else w for w in words]
        compact.append(name)
    words.append('sweetsample=AegirPoster')
    return words,sampler_binding(trigger)
