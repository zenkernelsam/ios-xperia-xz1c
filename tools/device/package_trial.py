"""Build a one-shot trial from exact tested baseline inputs, never from a legacy Patch path."""
import argparse,hashlib,json,re,secrets,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'));import adt,mkpayload
from diagnostic_candidate import WLED_VERSION, LOG_VERSION, LOG_KERNEL_SHA, configuration, kernel_path, verify_record, verify_wled_kernel
from ram_library import load as load_ram
from native_proof import validate as validate_native,digest as native_digest
from controller import Artifact,SHIM_SHA,SCREEN_CAPTURE_TIMES,checked_screen_capture_after,swipe_once_binding,console_status_binding
from numeric_boot_args import add_sampler
from serial_worker import capture_binding
from display_geometry_binding import CommandTooLong,transform as display_transform,arguments as display_arguments,compact as display_compact
from native_grid_control import apply as apply_grid_control,compact_fitted,validate as validate_grid_control
def sha(data):return hashlib.sha256(data).hexdigest()
def observer_environment(word,target):
    assert word.startswith('sweetenv=') and target in ('AegirPoster','backboardd')
    word=word.replace(',SWEETGL_TRACE=1','')
    if target=='AegirPoster':return word+';AegirPoster:DYLD_INSERT_LIBRARIES=/dev/sglram/e'
    original='backboardd:DYLD_INSERT_LIBRARIES=/dev/sglram/m'
    assert word.count(original)==1 and 'AegirPoster:' not in word
    return word.replace(original,original+':/dev/sglram/e')

def observer_log_binding(package):
    observer=package.get('exception_observer')
    if not observer:return None
    target=observer['target_process'];assert target in ('backboardd','AegirPoster')
    processes=['backboardd']
    if target=='AegirPoster':processes.append('AegirPoster')
    if package.get('geocompat_build_binding'):processes.append('SpringBoard')
    return {'revision':1,'observer_target':target,'processes':processes,'geometry_producer_required':bool(package.get('geocompat_build_binding'))}
BASE_HASHES={'private':'b958bd29a457cae61be27a53e1237ccdc4c1247d4cbcff238afbe34d88615162',
             'ready':'2eb112563f920669e2e932799c56b6af03e79e09a1c8e73833855a4ac1f93af1',
             'gpu':'ee284994e2ac7e280f0a3b727ce1ebb4796e3813bbd98b524fa16e6c3d5f9082'}
BASE_KERNEL='da6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a'

def validate_callee_compiler_binding(frontend):
    overlay=frontend['Mesa_shader_compiler_overlay']
    expected=json.loads((ROOT/'reproducibility/nir-callee-local-ssa-source-binding-20261128.json').read_bytes())
    assert overlay['revision']==1 and overlay['link_input_overlay_only'] and overlay['device_operations']==0
    assert overlay['patch']=='gpu/nir-callee-local-ssa.patch' and overlay['patch_sha256']==sha((ROOT/overlay['patch']).read_bytes())
    assert overlay['source_binding_sha256']==sha((ROOT/'reproducibility/nir-callee-local-ssa-source-binding-20261128.json').read_bytes())
    assert overlay['sources']==expected['sources']
    assert overlay['overlay_archives']==frontend['mesa_dependencies']
    assert {p:v['after_sha256'] for p,v in overlay['updated_archives'].items()}=={
        'src/compiler/glsl/libglsl.a':'1c4b7b108a4083662d97b6a3d8916b81e7b149f04095e56dcd7f2adef5ed724b',
        'src/compiler/nir/libnir.a':'9e9014305de600628cc35590aac7c23f4729d09cb70a364a9a908e217f7df2df'}
    assert len(overlay['compiled_objects'])==3 and all(v['architecture']=='arm64' for v in overlay['compiled_objects'].values())

def validate_ca_current_slots_binding(frontend):
    assert frontend['CA_matrix_observation_revision']==2
    assert frontend['CA_matrix_current_slots_read_only_revalidation'] is True
    sources=frontend['source_files']
    assert sources['eagl.c']=='e70950925e9f041cf99ad403ddb5fa30d57c70a058d80be06e948ac3b9524fd4'
    assert sources['QuartzCoreMatrixLayout.h']=='1d1d9e01a6c9c533f6f7d81b7246a574a1d05eeb0947f846e8289d6474f39117'

def validate_wled_gles_ram(package, binding):
    """Admit only the reviewed GLES/geometry payloads on v731."""
    assert 'exception_observer' not in package and 'metal_resources' not in package, 'v731 GLES9 does not admit Metal or an exception observer'
    frontend=package['frontend_build_binding']
    assert frontend.get('CA_matrix_revision')==1 and frontend.get('GLES_suite_tests')==9
    assert (binding['library_sha256'],binding['blob_sha256'],binding['manifest_sha256']) in (
        ('0766d5c713a883426e801e6d162bbdc46df79c5bc094ca77a7b6d6462599d607',
         '4277931a612742a2eb2e02f67af8c2083116a13686c7594e7d0a3b5e87f0cf27',
         '7c5da6c1025885e8e2c4c988475c138ec2a43d135987b88a316020f26c27cf46'),
        ('6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325',
         '25a417018c2febc28f1c72fc14cb0d0a9ab0c4a3ec28536cbcb13406f01b6543',
         '60680d669f037146a85b0a50e3353c0a2f991ff8be9ff9eedf4f734ef4191281'),
        ('d1f5075c0f35eac56f3d1976775700a36da49bc1c6cb84c7493377592aa31a0b',
         '1ee9fc1220c12cc69791e38fb89729dce84787983fd698423f82e77b69d7da22',
         '9c1c199f3b4aa8c8290e8d2a82f80826a758335e6a3f5c80abae17034b1b4ddb'),
        ('3665466730c20fbd2bde2409addd7f9d3bce47888714be37cc3be44a88452970',
         '50028fb9d6052b29282011b444dccf2026666eef8abd0d7678de155aa5995208',
         '5d5821127ef942bd3c411303f307df1b5e535f87a1e0262075b0ac289b775af3'),
        ('467b24f613e75bbe675d8003e47ddca7e1cf649e91340ebb64cc04b0183f86d4',
         '7273c98fb29cb050fc6d7272de971c33c7e4f5f44a9dc4e89740e87063fd509a',
         'c7d80ea1cdfddb7918a6102abc3041d1035ea2934178e5097574d168e90b1d98'),
        ('76f4c32c70aec93954795ed70f48fbc22ef7fd5dd1bc3b0c85eed357b6165e43',
         '1408d3012e9449662bcaa66b35bd0cabe0951d86d39c5dfcde8f76dab1b76c30',
         '9c370eec387015f6c32738cea034395e38de72e5a9a2f7675bd3af308a07d299'),
        ('dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308',
         '31b8ab8196b7bfa84987253b91f8bb7c4b8a3855b37e1eee000dd0f4488cf074',
         'ad6365ab02e69674f54e35b7f70895afb63b39b72a848b1bceed028d2f7a94e9'),
        ('dd3ffb371577fc1f158a59c722e5750db24c41511b2f2ac6732e35846bbc8887',
         '30a5bf63ddfa56fb9587c4308c6eb92525616c69ac4d062ada88ce6cb1ff00b7',
         '1073f71fe836322ef7357dd2276305b26f3dc6e16b73db96105f9c78956284d2'),
        ('e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e',
         '49b6bd0547405cf3632104f8dcaf4c9fe3a29a4df4f09b870414c515c44e8c47',
         'e290bb172be6b0840af1c41ce650b027416dcc24a1f562d8cef743051b898265'),
    ), 'v731 GLES9 RAM identity is not admitted'
    if binding['library_sha256']=='d1f5075c0f35eac56f3d1976775700a36da49bc1c6cb84c7493377592aa31a0b':
        assert frontend['ordered_XR_image_barrier_revision']==1
        assert frontend['ordered_XR_per_primitive_rounding_and_final_fences_preserved']
        assert frontend['ordered_XR_barrier_waits_shader_and_CP_writes_and_invalidates_UCHE']
    if binding['library_sha256']=='3665466730c20fbd2bde2409addd7f9d3bce47888714be37cc3be44a88452970':
        assert frontend['ordered_XR_image_barrier_revision']==2
        assert frontend['ordered_XR_monotonic_timestamp_wait_and_separate_scratch_cell']
        assert frontend['ordered_XR_per_primitive_rounding_and_final_fences_preserved']
    if binding['library_sha256']=='467b24f613e75bbe675d8003e47ddca7e1cf649e91340ebb64cc04b0183f86d4':
        assert frontend['ordered_XR_image_barrier_revision']==2 and frontend['ordered_XR_requires_synchronous_GL_frontend']
        assert frontend['ordered_XR_monotonic_timestamp_wait_and_separate_scratch_cell']
    if binding['library_sha256']=='76f4c32c70aec93954795ed70f48fbc22ef7fd5dd1bc3b0c85eed357b6165e43':
        assert frontend['ordered_XR_image_barrier_revision']==3 and frontend['ordered_XR_requires_synchronous_GL_frontend']
        assert frontend['ordered_XR_monotonic_timestamp_wait_and_separate_scratch_cell']
        assert frontend['ordered_XR_first_timestamp_scratch_initialized_by_GPU']
    if binding['library_sha256'] in ('dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308','dd3ffb371577fc1f158a59c722e5750db24c41511b2f2ac6732e35846bbc8887','e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e'):
        assert frontend['ordered_XR_image_barrier_revision']==4 and frontend['ordered_XR_requires_synchronous_GL_frontend']
        assert frontend['ordered_XR_monotonic_timestamp_wait_and_separate_scratch_cell']
        assert frontend['ordered_XR_first_timestamp_scratch_initialized_by_GPU']
        assert frontend['ordered_XR_dense_driver_images_resolved_from_linked_GL_program']
        assert frontend['ordered_XR_unknown_images_SSBO_writes_or_atomics_use_original_barrier']
    if binding['library_sha256'] in ('dd3ffb371577fc1f158a59c722e5750db24c41511b2f2ac6732e35846bbc8887','e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e'):
        validate_callee_compiler_binding(frontend)
    if binding['library_sha256']=='e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e':
        validate_ca_current_slots_binding(frontend)

def validate_wled_gpu_ram(package, binding):
    assert not package.get('exception_observer',{}).get('poster_container_snapshot_revision'), 'Container RAM has no reviewed signed candidate'
    if 'exception_observer' not in package and 'metal_resources' not in package:
        validate_wled_gles_ram(package,binding)
        return 'gles'
    if binding['blob_sha256']=='2234fdec29ad5d8b8cc86728eba858988ecc26329d6cb70e5bfac648633ac79c':
        assert binding['manifest_sha256']=='f5dc6132c3714c2385061af2da231c440fe6908838ce49cd39a6852ca9ebdc2d'
        gles='e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e'
        resources=package['metal_resources'];o=package['exception_observer'];frontend=package['frontend_build_binding']
        assert binding['library_sha256']==resources['gles_library_sha256']==gles
        assert resources['library_sha256']=='19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49'
        assert native_digest(resources['source_files'])=='41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee'
        assert resources['native_ordered_XR_image_barrier'] and resources['ordered_XR_image_barrier_revision']==4
        assert frontend['ordered_XR_dense_driver_images_resolved_from_linked_GL_program']
        validate_callee_compiler_binding(frontend);validate_ca_current_slots_binding(frontend)
        assert o['library_sha256']=='3d9d3d07a438ca597c6c6499cb5dc1e966ecc439a490739ac06bfbb196b20789'
        assert o['target_process']=='AegirPoster' and o['registration_only'] and o['snapshot_method_observer'] and o['png_filter_encoder']
        assert o['snapshot_readback_observer'] and o['original_CG_input_observer'] and o['original_queue_retirement_observer']
        native=validate_native(o['prior_native_hardware_proof'],resources);png=o['png_encoder_prior_proof']
        assert native['version']==LOG_VERSION and native['kernel_sha256']==LOG_KERNEL_SHA and len(native['native_tests'])==21
        assert native['native_run_token']==png['reference_token']=='66a0459acc20c392'
        assert native['native_run_log_sha256']==png['reference_raw_sha256']=='ddf4b1b96b4a6949e0cb3179c8d7e701cef5c719afaaf267bb088f70b3551731'
        assert png['kernel_sha256']==LOG_KERNEL_SHA and png['GLES_sha256']==gles and png['reference_cases']==6
        assert o['aegir_snapshot_cache_scope']['identity']=='fe2b9fcebe1b7f76256555f6b0e29d40416ab3356b76d17568ee4bebacd25f53'
        assert o['program_cache_scope']['identity']=='e8bc5ec11770891598a0eff837ff303567977da68a81af6d1cf89b271a66a972'
        assert not any(o.get(k) for k in ('poster_snapshot_gate_observer','quad_route_observer','loaded_snapshot_PNG_export','program_cache_prime'))
        return 'poster'
    if binding['blob_sha256']=='c6aab7f471247d3e8da7bebf3ac6a1885b9c9625ab9b02ee95413478d6ce2959':
        assert binding['manifest_sha256']=='48ca18e9d25762d0e75a61230f49f50a6443f419d383d00ed66cdcf6a584955d'
        gles='dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308'
        resources=package['metal_resources'];o=package['exception_observer'];assert binding['library_sha256']==resources['gles_library_sha256']==gles
        assert resources['library_sha256']=='19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49'
        assert native_digest(resources['source_files'])=='41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee'
        assert resources['native_ordered_XR_image_barrier'] and resources['ordered_XR_image_barrier_revision']==4
        assert package['frontend_build_binding']['ordered_XR_dense_driver_images_resolved_from_linked_GL_program']
        assert o['library_sha256']=='2ec05f92e6aba51cc39bfeb389447516793327a0f1ee06a5d147033e9d65132b'
        assert o['target_process']=='AegirPoster' and o['registration_only'] and o['snapshot_method_observer'] and o['png_filter_encoder']
        assert o['snapshot_readback_observer'] and o['original_CG_input_observer'] and o['original_queue_retirement_observer']
        native=validate_native(o['prior_native_hardware_proof'],resources);png=o['png_encoder_prior_proof']
        assert native['version']==LOG_VERSION and native['kernel_sha256']==LOG_KERNEL_SHA and len(native['native_tests'])==21
        assert native['native_run_token']==png['reference_token']=='1cfd34d92fe4f08d'
        assert native['native_run_log_sha256']==png['reference_raw_sha256']=='697d3124f5888bb8bf53292c41c4fcf83b0fa47b9fe06c6f7d72954f27755025'
        assert png['kernel_sha256']==LOG_KERNEL_SHA and png['GLES_sha256']==gles and png['reference_cases']==6
        assert o['aegir_snapshot_cache_scope']['identity']=='4db7d15042df9a74be613cbeb8fa38e93e65ded8545860f6de51a194a8168fd5'
        assert o['program_cache_scope']['identity']=='dc22fd53f7ba1b6a70e12d50753e19f37cf0a0da97f7a6140afd2bc304b2a60d'
        assert not any(o.get(k) for k in ('poster_snapshot_gate_observer','quad_route_observer','loaded_snapshot_PNG_export','program_cache_prime'))
        return 'poster'
    barrier_native={
        '4f07040cacbd5f1cc332e8de71ca9d4a7c2c35dafefbf2d13d662ce5a590d348':
            ('c51a15f7361922b0c47b182cf850897d07be77acd39f8c33ff368120b84a67d9','d1f5075c0f35eac56f3d1976775700a36da49bc1c6cb84c7493377592aa31a0b','96b9807eb034bf2b98b4689e1e4615024b86f311a24f292b2fbe0788be44dd44','3335d4d3393145edb8d92ffd7f0c27679cb75a91f611ea9963b8d1a73c722ca6',1),
        '40f3244b0af94151e58f236e125f0e81a4ffd0734b465c57745781eb8bd14db8':
            ('755b9fa36b95a0118d7405da354d0ed1eb0a2bfcd8c859684a0070294833bc09','3665466730c20fbd2bde2409addd7f9d3bce47888714be37cc3be44a88452970','3e48e7dc5e8147e7a179b8ab6c0306535804afbf559677e21d0d9c8a529482e6','2130525c5693841d9e1dd956ab2d0d50042247a2e7a688c759147470b0f41f2a',2),
        'd08f7734bf5a187fc21c1460bb0685dd00dcb4a4bf95be0bd6e47847cb317c20':
            ('fde24836e930224b4d71a596307703e5e1d4d6bf2950f926f4c218279dc21f55','467b24f613e75bbe675d8003e47ddca7e1cf649e91340ebb64cc04b0183f86d4','c693131677c34afee7915c8564fb11949318c9e4537eddecebe75330e8bae19b','c3a25c362cfe419ae8871db5718852386568171cb0c57aa67084b86cb6a75811',2),
        'e88507f2743fc2f96479f3890e9cc31812fe7b4b413f36c27d743fa3bf854a1f':
            ('6b3e5c922530389e815942d2013607c1eb5064525f3861e5a6cb5ced36214735','76f4c32c70aec93954795ed70f48fbc22ef7fd5dd1bc3b0c85eed357b6165e43','3354ed72082e6ab5cb87b06919e33882ae73481fe3c47a0427d23d7ea24fcee6','a36b8f40ac78d0a3d5fbe7393915c61e0a75e2cfd51d2f5e2ca38f8da6d0c999',3),
        '75b8f08201a972f9c67ca1a63110131e05286116bfbf47bb1a8fb19548b354f0':
            ('ae8acd03c0d7e2ef8a0bec81c01a133c73a3e09228f5beb58b534802a8419e08','dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308','2f100d56d9ec22bdaca3d01cd8f8b369a24c76ee6d252fd671ffb16da4c561db','dc22fd53f7ba1b6a70e12d50753e19f37cf0a0da97f7a6140afd2bc304b2a60d',4),
        'eee6db388a4e3adfb04c27bd7e4a433a4649efd2d216f2d0cb139a210874d01b':
            ('656f254be2bd6adad3742b83839d61e3633ad9461da009400f4339a37354951f','dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308','8e7951086923e0886bcfe836cee8a2268591cda8bab4f4de4eb2eb26139093c9','dc22fd53f7ba1b6a70e12d50753e19f37cf0a0da97f7a6140afd2bc304b2a60d',4),
        '10b9da760c42fe3524d3f6ff59b119b9771d0e43d2c0f5193af9cb9f9b7be74e':
            ('cb9ea3e6b4152a3ee41e10d35888dadda18ec0419b8080371510e8847f1847cf','dc7113a10542c4b8de2036def8373a4574ab2092d17efd1a39e1c224f1bec308','e30dba715e7e57dd97416e913946a29a916efbd7c055251f078c0f27c0b6edbe','dc22fd53f7ba1b6a70e12d50753e19f37cf0a0da97f7a6140afd2bc304b2a60d',4,True),
        '6bfe39ba8aa094fc9235181cd7ba633ea8ac7629a69bd5f8f3ab947b0d62f2e3':
            ('45629ea392f03d38878a51d8fb810b472bf3a092a887c5fd8ba1f56e642771e7','dd3ffb371577fc1f158a59c722e5750db24c41511b2f2ac6732e35846bbc8887','2d0c846af5036af3817f967f0e2eda5d5748ab7ed3c9a0718e3d15bb3c424a93','a12b24b9d1d82720b769f70d55dec41fc797109ad371d50c8b235008dff27010',4),
        'd4b7185fb1aff42b7ed09e686be0d1e454974c08bad6a999d6cbcfe420560b0b':
            ('c35f8eed58e8de3e600129dd1859b70ec45a87db7f3e01e940314191928aaffe','e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e','6628cee594a5f869765b32e914f8773ea9d292b4337fd8ed239a704d4de5b607','e8bc5ec11770891598a0eff837ff303567977da68a81af6d1cf89b271a66a972',4),
        '6da9e8801d0adfd883b35969d87a3bddaa7e982e56742940b24ee9ee0aa3a2e0':
            ('50ad033ed84a53df5f77b190b10019c69468820e2d6acfcd3edf196dc8576504','e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e','bc977b0cf67b442e30ca6e1b7ec2c4e530cdf2f62e5de106f28ea9463246f4c0','e8bc5ec11770891598a0eff837ff303567977da68a81af6d1cf89b271a66a972',4),
    }
    if binding['blob_sha256'] in barrier_native:
        reviewed=barrier_native[binding['blob_sha256']]
        assert len(reviewed) in (5,6)
        manifest_sha,gles,observer_sha,scope,revision=reviewed[:5]
        conditional_prime=len(reviewed)==6 and reviewed[5] is True
        assert binding['manifest_sha256']==manifest_sha
        assert binding['library_sha256']==gles
        resources=package['metal_resources'];observer=package['exception_observer']
        assert resources['gles_library_sha256']==binding['library_sha256']
        assert resources['library_sha256']=='19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49'
        assert native_digest(resources['source_files'])=='41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee'
        assert resources['native_ordered_XR_image_barrier'] and resources['ordered_XR_image_barrier_revision']==revision
        assert resources['original_XR_per_primitive_rounding_and_final_fences_preserved']
        assert observer['target_process']=='backboardd' and observer['background_full_suite'] and not observer.get('registration_only')
        assert observer['library_sha256']==observer_sha
        assert observer['render_unused_buffer_binding_probe_revision']==2 and observer['render_pipeline_switch_retains_unused_fragment_slot0']
        assert observer['presentation_probe'] and observer['cif10_reference_cases']==6
        assert bool(observer.get('program_cache_prime')) is conditional_prime
        assert observer['program_cache_inventory']['read_only'] and observer['program_cache_inventory']['conditional_prime'] is conditional_prime
        assert observer['program_cache_scope']['identity']==scope
        if gles in ('dd3ffb371577fc1f158a59c722e5750db24c41511b2f2ac6732e35846bbc8887','e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e'):
            validate_callee_compiler_binding(package['frontend_build_binding'])
        if gles=='e7cf76d676f9211e3f87bf8ae8c420dc590488cd51e8f711a7c0b66ec01c0c2e':
            validate_ca_current_slots_binding(package['frontend_build_binding'])
        if binding['blob_sha256']=='6da9e8801d0adfd883b35969d87a3bddaa7e982e56742940b24ee9ee0aa3a2e0':
            assert observer['poster_config_snapshot_revision']==1 and observer['poster_config_snapshot_roots']==5
            assert observer['poster_config_snapshot_max_bytes']==131072 and observer['poster_config_snapshot_readonly']
            assert observer['poster_config_snapshot_after_native21_PNG6'] and observer['poster_config_snapshot_changes_UI_policy'] is False
            assert observer['poster_config_snapshot_host_FS_and_decoder_checked']
        if conditional_prime:
            inventory=observer['program_cache_inventory']
            assert inventory['context_identity']=='b2d3872d86da13a0ec1db5c198f56ffd1fcf1fa2905314824a1cf33ea41ca9c1'
            assert inventory['build_identity']=='19fd0e76cce2eb272390f11d3880c31b1599a7cb0bda7b1192d0e79d6f0a573c'
            assert inventory['scope_identity']==scope and inventory['directory']=='/private/var/tmp/sgm-dc22fd53f7ba1b6a70e12d50753e19f3'
            assert inventory['max_prime_entries']==8 and inventory['max_files']==64 and inventory['max_bytes']==33554432
            assert observer['empty_primer_not_dispatched'] and observer['cache_inventory_after_complete_native21_PNG6']
        if gles=='467b24f613e75bbe675d8003e47ddca7e1cf649e91340ebb64cc04b0183f86d4':
            assert package['frontend_build_binding']['ordered_XR_requires_synchronous_GL_frontend']
        if revision>=3:
            assert package['frontend_build_binding']['ordered_XR_requires_synchronous_GL_frontend']
            assert package['frontend_build_binding']['ordered_XR_first_timestamp_scratch_initialized_by_GPU']
        if revision==4:
            assert package['frontend_build_binding']['ordered_XR_image_barrier_revision']==4
            assert package['frontend_build_binding']['ordered_XR_dense_driver_images_resolved_from_linked_GL_program']
            assert package['frontend_build_binding']['ordered_XR_unknown_images_SSBO_writes_or_atomics_use_original_barrier']
        if binding['blob_sha256'] in ('eee6db388a4e3adfb04c27bd7e4a433a4649efd2d216f2d0cb139a210874d01b','10b9da760c42fe3524d3f6ff59b119b9771d0e43d2c0f5193af9cb9f9b7be74e','6bfe39ba8aa094fc9235181cd7ba633ea8ac7629a69bd5f8f3ab947b0d62f2e3','d4b7185fb1aff42b7ed09e686be0d1e454974c08bad6a999d6cbcfe420560b0b'):
            assert observer['cif10_export_pacing_revision']==3 and observer['cif10_export_pacing_microseconds']==15000
            assert observer['cif10_export_ranges_emitted_once'] and observer['cif10_export_remaining_time_EINTR_and_failure_host_passed']
            assert observer['observer_log_write_failures_checked'] is True and observer['observer_log_summary_revision']==1
            assert observer['cif10_reference_shared_encoder'] and observer['presentation_probe_phases']==5
            assert observer['program_cache_inventory']['native_groups_before_inventory']==21 and observer['program_cache_inventory']['PNG_cases_before_inventory']==6
        return 'native'
    reflected_posters={
        '0ff63e76417f8b821c374fb66f2150e3514e530cdcbbeec5f86e70ef7be332ca':
            ('6d7c9ea8b7ce2716bf9f643b6da468fd8954b373e3ac97d159d7cb5c30a33ecf','fed5a7552b3a7272079b0f9842e43cc590dbb9cecf5123e71da67b37563f981a','ea632529cb6c9f5153de8de787c4463889bd766fde5ecc4f654a7a5ea3a58d96',False),
        'f2ac192e04c4ad62c24a6cc06d8b5709c2efa5c999946bbae47bb60cf076e926':
            ('6888250730a4a432a4a241a5d90d0a03963076452b86aa4809d56a0b8037c517','785c9873e82c5167bc7fcc516f22a28499a353155ba08b718b748fff9e83bcb4','b60041f325e4d01039e7990aef7f526595ed36c9948193e4ffdec017a88312c0',True),
        '1b44a30dd5a3ca167ed22cb1dad43cf256434a0120265296b95b62b90f832741':
            ('d7ca6afa7cf0ce35a89203fd7cf7aad9336ac553550c15a6bceebbce4d80efe3','7d9065b88a1abd038ec6670faa6b5eaef987f77cb1927ebe6c9ace387fdae013','e0e19e4f8bda3e7de2d17178eb60e31ac9fcc97c4420e889eebc50ff68dccedd',True),
        '358a2121bbc6a2fb0ca41f3cb550d2430b414bab91e3d8af844e7fed817187e6':
            ('546eb6ec657780a33fd2fbad2b4a1f5bd05a847c7f9d34e0da742aaa65140900','bed4e03dfbfb7018f2b1dbfcdde3bbbaae510d8dd020af3cfd8ce86cbcd92f6e','e0e19e4f8bda3e7de2d17178eb60e31ac9fcc97c4420e889eebc50ff68dccedd',True),
        '61cbcf2c67cf63fb6eaf76c4114aeadb2d55d6b8f50a23bbdd3cf4f81798d1fd':
            ('789ee2be30427a13747477b31aa9b6e8b69f0114edbdc88cb39c98953ca3d7ab','e536e79531c9ff36bc19cc019100b841ca86281f160ec74e7658c684769e5d80','6929e249be79e2ae51bbc5f6e3612bb14ef16b29e1b59b86d72f451ccc56af72',True),
    }
    if binding['blob_sha256'] in reflected_posters:
        manifest_sha,observer_sha,scope_id,readback=reflected_posters[binding['blob_sha256']]
        assert binding['manifest_sha256']==manifest_sha
        resources=package['metal_resources'];observer=package['exception_observer']
        gles='6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325'
        assert binding['library_sha256']==resources['gles_library_sha256']==gles
        assert resources['library_sha256']=='19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49'
        assert native_digest(resources['source_files'])=='41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee'
        assert resources['native_reflected_render_buffer_selection'] and resources['render_buffer_selection_revision']==1
        assert observer['target_process']=='AegirPoster' and observer['registration_only'] and observer['snapshot_method_observer']
        assert observer['library_sha256']==observer_sha
        assert observer['observer_log_write_revision']==1 and observer['observer_log_partial_EINTR_errno_host_passed']
        assert observer['snapshot_render_queue_revision']==1 and observer['snapshot_render_queue_no_extra_GPU_work']
        assert observer['snapshot_render_queue_draws_max']==8 and observer['snapshot_render_queue_results_max']==8
        assert observer['snapshot_render_queue_actual_host_decoder_passed']
        assert not any(observer.get(k) for k in ('poster_snapshot_gate_observer','original_request_observer','quad_route_observer','loaded_snapshot_PNG_export'))
        native=validate_native(observer['prior_native_hardware_proof'],resources)
        assert native['version']==WLED_VERSION and len(native['native_tests'])==21 and native['native_run_token']=='1e1bf33f91081ea2'
        png=observer['png_encoder_prior_proof']
        assert observer['png_filter_encoder'] and png['GLES_sha256']==gles and png['Metal_sha256']==resources['library_sha256']
        assert png['reference_token']==native['native_run_token'] and png['reference_cases']==6
        scope=observer['aegir_snapshot_cache_scope']
        assert scope['identity']==scope_id
        assert scope['observation_source_sha256']==observer['source_files']['gpu/diagnostics/SnapshotRenderQueueObserve.h']
        assert scope['original_caches_root_preserved'] and scope['directory_mode']==0o700 and scope['original_caller_offset']==0x63b0
        assert bool(observer.get('snapshot_readback_observer')) is readback
        if readback:
            assert observer['snapshot_readback_revision']==1 and observer['snapshot_readback_max_pixels']==1048576
            assert observer['snapshot_readback_once_per_process'] and observer['snapshot_readback_extra_GPU_work'] is False
            assert observer['snapshot_readback_original_returned_bytes_not_changed'] and observer['snapshot_readback_actual_host_decoder_passed']
            assert scope['readback_source_sha256']==observer['source_files']['gpu/diagnostics/SnapshotReadbackObserve.h']
        retirement=binding['blob_sha256']=='61cbcf2c67cf63fb6eaf76c4114aeadb2d55d6b8f50a23bbdd3cf4f81798d1fd'
        png_export=retirement or binding['blob_sha256']=='358a2121bbc6a2fb0ca41f3cb550d2430b414bab91e3d8af844e7fed817187e6'
        cg_input=png_export or binding['blob_sha256']=='1b44a30dd5a3ca167ed22cb1dad43cf256434a0120265296b95b62b90f832741'
        assert bool(observer.get('original_CG_input_observer')) is cg_input
        if cg_input:
            assert observer['original_CG_input_revision']==1 and observer['original_CG_input_provider_bytes_max']==4194304
            assert observer['original_CG_input_once_per_process'] and observer['original_CG_input_no_pixel_conversion_or_extra_GPU_work']
            assert observer['original_CG_input_host_ASan_UBSan_actual_provider_passed']
            assert scope['cg_input_source_sha256']==observer['source_files']['gpu/diagnostics/OriginalCGInputObserve.h']
        assert bool(observer.get('original_snapshot_PNG_export')) is png_export
        if png_export:
            assert observer['original_snapshot_PNG_export_revision']==2 and observer['original_snapshot_PNG_export_codec']=='ZLIB1'
            assert observer['original_snapshot_PNG_export_max_bytes']==65536 and observer['original_snapshot_PNG_export_once']
            assert observer['original_snapshot_PNG_export_background_no_extra_draw'] and observer['original_snapshot_PNG_export_actual_bytes_host_ASan_UBSan_passed']
        assert bool(observer.get('original_queue_retirement_observer')) is retirement
        if retirement:
            assert observer['original_queue_retirement_revision']==1 and observer['original_queue_retirement_completed_fence_and_operation_count_required']
            assert observer['original_queue_retirement_same_original_command_no_extra_GPU_work']
        return 'poster'
    poster_packages={
        'ab1990dfdaef042c06588d21c78fea999195efeff62b62916d95060879e78f3d':
            '1e346d62937f85b21ae807f0164aad7e97c9bb2d94cb79f7e8b05cfc3cd6a4a1',
        'e98ed2cd62595b2b414ef2d04ce1cfa93af34041a99fd6b38691f004314eb83b':
            '55eee7cb15985a1e27c939ae4601ceb104d9d43ae760edb7c6191f450899ec98',
        '0d4076fd78bc5f9ceae6f3386fcefc16b3f7b5bc20bf93680e8cf8d207f29a04':
            '82ea4046016e6b136712d905508c4628376acca41bf238ec1ddf967069688d14',
        'a6e40c574453772f993cb76570f749289281d00e03d45ec57eb2d1bcdcfd7d91':
            '21ca40cf281038c892a12f852735dd35c054cb1bec37d90874ddd6170374e374',
        '0c1f8ccc8f602a939cbf80e1416abc4de709d63f0b1e29bf627b6a554b88c8cf':
            'd8c15bdcc462a60d09c13e653f4bf6d38e32b8e13d70dbfaa0c92b11bdab1f07',
        '84a03a910b5fefb6632bf529203ac5ff2ca56646fb10d92a736e45603339d0cf':
            '0a43524123de0b6fd1959bd1a2d6868dc7c19f6f18eaf4b8db811c5242794593',
        '82311b6c4ed92cbdc1e3ccd00a7019759998bcbdc35512c190b1f41ce32ec672':
            '8013c5e5da5a3798b5475b3a6dc7b9ab7684971bd5cf012cc8a0aeacf13542c2',
        'cf2dae517c3f4c6b2b77ccb480f00069dcc5d366eecc30a8f499df76642c5d13':
            '1feaf85d33002097259260b1cb801a66fc13895f4b665a6037cf15e1c26558e3',
        '006333a4c8ec135e23893a7fc07d770f02fdced6b86ab41ff23aed054f27c54e':
            '96877d4070d120246e7667a49ea8c9c5ff0859a670e1d4800348f0ceb5b66994',
        '83b36fae6c095bea77e0b4ded6b2de4dea4670974c89dec0f4cff96ce39dca15':
            '7681e9059ca71d9e405a3322c74a1e81d165139fe9108fca01cff911cfa78347',
        'bd0ccca991d32c7db3e7ee8c393883b971262ec61970e583ed33696d41b34ddf':
            '64a29b78488ee81854ff1d12c5a3454736fa353f1cdedb2d0ff06193385b846e',
        '1b7504a47a614d63a7c89818fee9f2af76570a44ecfe75505c6a53d962d21458':
            '4d1644260d04ccebc3f18d10697b97e693e172d902a2cd32e7632ae455020b0a',
    }
    if binding['blob_sha256'] in poster_packages:
        assert binding['manifest_sha256']==poster_packages[binding['blob_sha256']]
        observer=package['exception_observer']
        assert observer['target_process']=='AegirPoster' and observer['registration_only']
        assert observer['prior_native_hardware_proof']['version']==WLED_VERSION
        assert observer['png_filter_encoder'] and observer['snapshot_method_observer']
        if binding['blob_sha256']!='ab1990dfdaef042c06588d21c78fea999195efeff62b62916d95060879e78f3d':
            gles='6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325'
            assert binding['library_sha256']==gles and package['metal_resources']['gles_library_sha256']==gles
            compressed_export=binding['blob_sha256']=='bd0ccca991d32c7db3e7ee8c393883b971262ec61970e583ed33696d41b34ddf'
            queue_observer=binding['blob_sha256']=='1b7504a47a614d63a7c89818fee9f2af76570a44ecfe75505c6a53d962d21458'
            loaded_export=compressed_export or binding['blob_sha256']=='83b36fae6c095bea77e0b4ded6b2de4dea4670974c89dec0f4cff96ce39dca15'
            png_export=binding['blob_sha256']=='006333a4c8ec135e23893a7fc07d770f02fdced6b86ab41ff23aed054f27c54e'
            logwrite=queue_observer or png_export or binding['blob_sha256']=='cf2dae517c3f4c6b2b77ccb480f00069dcc5d366eecc30a8f499df76642c5d13'
            warm=logwrite or binding['blob_sha256']=='82311b6c4ed92cbdc1e3ccd00a7019759998bcbdc35512c190b1f41ce32ec672'
            namespace=loaded_export or warm or binding['blob_sha256']=='84a03a910b5fefb6632bf529203ac5ff2ca56646fb10d92a736e45603339d0cf'
            cache_gate=namespace and not warm or binding['blob_sha256']=='0c1f8ccc8f602a939cbf80e1416abc4de709d63f0b1e29bf627b6a554b88c8cf'
            gate=cache_gate or binding['blob_sha256']=='a6e40c574453772f993cb76570f749289281d00e03d45ec57eb2d1bcdcfd7d91'
            minimal=warm or gate or binding['blob_sha256']=='0d4076fd78bc5f9ceae6f3386fcefc16b3f7b5bc20bf93680e8cf8d207f29a04'
            expected_observer='a73e18addf69c8e8dd017529f46e0a8b4728bd0cb3e0ac054f99123504352536' if queue_observer else 'cd0a2b65ab2abf960ab8a6d08dd734447b3d9494adc540726a893c24c78bc079' if compressed_export else '3e11244af2844e304dd22395cfc68abaff1b8d232d7e7045ab32caf1809d2db1' if loaded_export else 'c930622d2903483daa5dd994ee4c6a46951bdd9947f05f20a330896180b05a2a' if png_export else '503d734bef26021ded302ed44d7c1de51174b258bfd67febf9cf2373b810746e' if logwrite else 'b7b2dc04b3e2551b821e9c13a3d22f35dbd9fe913f72cfc48597c0a999f10045' if warm else '2a8003ea068a5584f110add834c740b42c37cd0bc612440c6502452169a96ee3' if namespace else '894d04bd46a159a55af93c6b046e9d1b5f6ee0f825a149ce6bc102c251f0393b' if cache_gate else 'a9641074b83100310d5f455972bb5f9aac0db874049a54986919c52210c4e80f' if gate else '3c442f3085c1106d9de233d56fdbed801ea87345a0ec1ea536d0df355ec5ca22' if minimal else 'af1c42dc51fbccab4f6d42461044c14c9913ffa7eaa4b1a480c1b213b24cddd2'
            assert observer['library_sha256']==expected_observer
            if queue_observer:
                assert observer['snapshot_render_queue_observer'] and observer['snapshot_render_queue_revision']==1
                assert observer['snapshot_render_queue_draws_max']==8 and observer['snapshot_render_queue_results_max']==8
                assert observer['snapshot_render_queue_original_arguments_errno_throws_and_terminal_getters_host_passed']
                assert observer['snapshot_render_queue_no_extra_GPU_work'] and observer['snapshot_render_queue_actual_host_decoder_passed']
            else:assert not observer.get('snapshot_render_queue_observer')
            if logwrite:assert observer['observer_log_write_revision']==1 and observer['observer_log_partial_EINTR_errno_host_passed']
            if png_export:
                assert observer['original_snapshot_PNG_export_revision']==1 and observer['original_snapshot_PNG_export_once']
                assert observer['original_snapshot_PNG_export_max_bytes']==65536 and observer['original_snapshot_PNG_export_chunk_bytes']==48
            else:assert not observer.get('original_snapshot_PNG_export')
            if loaded_export:
                assert observer['loaded_snapshot_PNG_export_revision']==(2 if compressed_export else 1) and observer['loaded_snapshot_PNG_export_once']
                assert observer['loaded_snapshot_PNG_export_max_bytes']==65536 and observer['loaded_snapshot_PNG_export_read_only']
                if compressed_export:assert observer['loaded_snapshot_PNG_export_codec']=='ZLIB1'
            else:assert not observer.get('loaded_snapshot_PNG_export')
            prior=validate_native(observer['prior_native_hardware_proof'],package['metal_resources'])
            assert len(prior['native_tests'])==21 and prior['native_run_token']=='2d1794b225c1a6c1'
            png=observer['png_encoder_prior_proof']
            assert png['GLES_sha256']==gles and png['reference_token']==prior['native_run_token'] and png['reference_cases']==6
            if minimal:
                assert not any(observer.get(key) for key in ('original_request_observer','quad_route_observer','poster_state_observer_revision'))
            else:
                assert observer['original_request_observer'] and observer['quad_route_observer'] and observer['poster_state_observer_revision']==1
            if gate:
                assert observer['poster_snapshot_gate_observer'] and observer['poster_snapshot_gate_revision']==(2 if cache_gate else 1)
                assert observer['poster_snapshot_gate_methods']==(7 if cache_gate else 5) and observer['poster_snapshot_gate_extra_getters_or_GPU_work'] is False
                assert observer['poster_snapshot_gate_max_calls']==(56 if cache_gate else 40) and observer['poster_snapshot_gate_max_replies']==64
                if cache_gate:assert observer['poster_snapshot_gate_bounded_original_path_metadata'] and observer['poster_snapshot_gate_path_bytes_max']==256
            else:
                assert not observer.get('poster_snapshot_gate_observer')
            if namespace:
                scope=observer['aegir_snapshot_cache_scope']
                assert scope['identity']==('9aaef9058e9778c32535c3ab473fd5417268e1a21258cf277954a400afa78767' if queue_observer else '31e2a8bef37463f45aa31d45f7b1fdcf9fb502ab2d83686b885b39c244329962')
                if queue_observer:assert scope['observation_source_sha256']==observer['source_files']['gpu/diagnostics/SnapshotRenderQueueObserve.h']
                assert scope['GLES_sha256']==gles and scope['original_caches_root_preserved']
                assert scope['original_caller_offset']==0x63b0 and scope['directory_mode']==0o700
            else:
                assert not observer.get('aegir_snapshot_cache_scope')
        return 'poster'
    budget_native={
        'a8821cead75c078861eed21733c6366b594c8de4a631c3848b8bfd55d3cb66bf':
            ('f86dd5f209f54996f393a7071526eecd7db2daf33d27fb788056c5562b6c61cd',
             'f4c0f0329f0d746063ac6123e5e81b7cb3e3c84fe251e6e5b0545ad8ae2f2e56',False),
        'be99855aa1663151815501cb9e5dfac5c960f3457a4611d9d055c4dbb80ae010':
            ('b0c57520ca89071822ac73be41008bc8b78023c898abf0cce882e61e8d486245',
             'c59fa4c83325f4029e99e799e3edb358a8e9fd897f2231071c284c4aae505bb9',True),
        'd7ad09c89481202c17f8d8e9fb33deb9409616ae6ba2e5b3c54f8cd031ae12c4':
            ('56b2087210b4eec26d6a02e8ce60c5d9f0e5891dfc969732333212565892cf98',
             'c59fa4c83325f4029e99e799e3edb358a8e9fd897f2231071c284c4aae505bb9',True),
        'ce892920c933fdd976a3f35a0a5447f4ef60b5511a6d544bacf34c2d1a825b3a':
            ('d71cb6cb010d78fe949eab9f16d288193a128c6697e5daecef24a7ac5063ee1b',
             'c59fa4c83325f4029e99e799e3edb358a8e9fd897f2231071c284c4aae505bb9',True),
    }
    reflected_packages={
        '166d4c7db03d9a99a4acb9e7f75fed22f3f06344b401e13159005d5ad30aaf81':
            ('ea2506260c87455b2019feec088e86928a6a18da6dc1f37218870f7cb69f8f47','ced47032290f2fe28f3ec437ae70ee52371aee15bf68ad4737231e622024dbae'),
        '63a755ece059bd869be9ae8f86aeb883fdad71ca9da10a08600aa4ee5734a44c':
            ('52aab45bc2ad022eed980eafccbfad71d179c42be384bfb02ac7cef5e3baff7d','a6e27aae2d6402b102b08552d424b99edcffa2828683210337be0a5bf1f53d2c'),
        '91ec02d744ce8e645a138005da39a015bfc351f4159a9f74454ca169710e7eaf':
            ('ffd427114c90a76cc16ad496fe56ad28b5024ff58ab3bea0d3e454fae67ec8d6','819a0ad96666006eff966b7342f388aa317a143de9d1dfa56b501c5c0a90299c'),
        'c3bf3bcd5f27773dc533ad25aded2d91c18336b575a93298fcfd9437f738ae97':
            ('fc9e7580a47c6dadd7b8cf8eac275fd7d7f481f59cf797f759886b2dc03616d4','2b9d922d06b91299bba513d83b70f71c91362aaa69a8fe08fbbb420d9cdc4ff1'),
    }
    reflected_native=binding['blob_sha256'] in reflected_packages
    if reflected_native:
        manifest_sha,observer_sha=reflected_packages[binding['blob_sha256']]
        assert binding['manifest_sha256']==manifest_sha
        assert binding['library_sha256']=='6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325'
        resources=package['metal_resources'];observer=package['exception_observer']
        assert resources['gles_library_sha256']==binding['library_sha256']
        assert resources['library_sha256']=='19a08083833851d5a13016473ec3c3db98629d6ce6fc928b70947af114abee49'
        assert native_digest(resources['source_files'])=='41bdf3e49258c8cf6d57a69beb99d15dec0e1404aec2509725c386941e0f25ee'
        assert resources['render_buffer_selection_revision']==1 and resources['unused_encoder_bindings_retained_and_not_consumed']
        assert resources['missing_required_render_buffers_rejected']
        primer=binding['blob_sha256']=='c3bf3bcd5f27773dc533ad25aded2d91c18336b575a93298fcfd9437f738ae97'
        expected_revision=2 if primer or binding['blob_sha256']=='91ec02d744ce8e645a138005da39a015bfc351f4159a9f74454ca169710e7eaf' else 1
        assert observer['render_unused_buffer_binding_probe_revision']==expected_revision
        if expected_revision==2:assert observer['render_pipeline_switch_retains_unused_fragment_slot0']
        assert observer['render_unused_buffer_binding_probe_vertex_slot']==7 and observer['render_unused_buffer_binding_probe_fragment_slot']==7
        assert observer['library_sha256']==observer_sha
        assert bool(observer.get('program_cache_prime')) is primer
        assert observer['program_cache_inventory']['read_only'] and observer['program_cache_inventory']['conditional_prime'] is primer
        assert observer['program_cache_inventory']['native_groups_before_inventory']==21 and observer['program_cache_inventory']['PNG_cases_before_inventory']==6
        assert observer['program_cache_scope']['identity']=='b1aa0e0a92644521ddc4f06ff078c45a04bd8bfe0a73114d584ba75743ee2458'
    elif binding['blob_sha256'] in budget_native:
        manifest_sha,observer_sha,conditional_prime=budget_native[binding['blob_sha256']]
        assert binding['manifest_sha256']==manifest_sha, 'Cache-budget native RAM manifest differs'
        gles='6efabfc0627a8818a8c4c45665459cd33d8ee814f2e50a3f3cdc4813bb254325'
        identity='16722b126d14e64954fc349d20626f9e639cc726ed1c24856a889b2249efd236'
        assert binding['library_sha256']==gles, 'Cache-budget native GLES differs'
        resources=package['metal_resources']
        assert resources['gles_library_sha256']==gles
        assert resources['library_sha256']=='8feea4593116ffe9187204f5dd4cf4dd48d3027816d4763ecef20da200f86a27'
        assert native_digest(resources['source_files'])=='db1605ef8dae8f8ed0a29999dcb2cf7537457aba7ad428677c21e0e914771b78'
        assert resources['immutable_program_cache_identity']==identity
        assert package['frontend_build_binding']['program_cache_build_identity']['build_identity']==identity
        observer=package['exception_observer']
        assert observer['library_sha256']==observer_sha
        inventory=observer['program_cache_inventory']
        assert inventory['build_identity']==identity and inventory['read_only'] and inventory['conditional_prime'] is conditional_prime
        assert bool(observer.get('program_cache_prime')) is conditional_prime
        assert inventory['native_groups_before_inventory']==21 and inventory['PNG_cases_before_inventory']==6
    else:
        assert binding['blob_sha256']=='cb1960218b50b7d908eda964fad198a9394941c0db5b1af9cee7e84471837b0a', 'v731 native RAM identity is not admitted'
        assert binding['manifest_sha256']=='27c39aa515bbfd247012b887a163558421f21bbe38ffce54ce2182be422ad459'
    observer=package['exception_observer']
    assert observer['target_process']=='backboardd' and observer['background_full_suite']
    assert not observer.get('registration_only') and observer['presentation_probe']
    assert observer['cif10_reference_cases']==6 and observer['cif10_reference_shared_encoder']
    assert package['metal_resources']['native_presentation_required_tests']==21
    return 'native'

def validate_wled_gpu_record(record, package, binding):
    stage=validate_wled_gpu_ram(package,binding)
    if binding['blob_sha256'] in ('c6aab7f471247d3e8da7bebf3ac6a1885b9c9625ab9b02ee95413478d6ce2959','10b9da760c42fe3524d3f6ff59b119b9771d0e43d2c0f5193af9cb9f9b7be74e','30a5bf63ddfa56fb9587c4308c6eb92525616c69ac4d062ada88ce6cb1ff00b7','6bfe39ba8aa094fc9235181cd7ba633ea8ac7629a69bd5f8f3ab947b0d62f2e3','49b6bd0547405cf3632104f8dcaf4c9fe3a29a4df4f09b870414c515c44e8c47','d4b7185fb1aff42b7ed09e686be0d1e454974c08bad6a999d6cbcfe420560b0b','2234fdec29ad5d8b8cc86728eba858988ecc26329d6cb70e5bfac648633ac79c','6da9e8801d0adfd883b35969d87a3bddaa7e982e56742940b24ee9ee0aa3a2e0'):
        assert record['version']==LOG_VERSION and record['kernel_sha256']==LOG_KERNEL_SHA
    assert 'thread_sample_observation' not in record, 'v731 GPU trials exclude thread sampling'
    if 'swipe_observation' in record:
        assert stage=='poster' and 'screen_snapshot_binding' in record, 'One swipe is restricted to the reviewed original-poster scanout'
        assert record['swipe_observation']==swipe_once_binding()
    if stage!='gles':
        assert record['exception_observer']==package['exception_observer'] and record['metal_resources']==package['metal_resources']
        assert record['process_log_binding']==observer_log_binding(package)
        backboard='DYLD_INSERT_LIBRARIES=/dev/sglram/m:/dev/sglram/e,SWEETGL_TEST=1'
        if stage=='poster':
            backboard='DYLD_INSERT_LIBRARIES=/dev/sglram/m,SWEETGL_TEST=1'
    else:
        assert not {'exception_observer','metal_resources','process_log_binding'} & record.keys()
        backboard='DYLD_INSERT_LIBRARIES=/dev/sglram/m,SWEETGL_TRACE=1,SWEETGL_TEST=1'
    assert record.get('display_geometry_binding',{}).get('profile')=='xz1c-native-2x'
    assert record.get('native_grid_control',{}).get('mode')=='fitted'
    if stage=='poster':
        if 'screen_snapshot_binding' in record:
            assert binding['blob_sha256'] in ('e98ed2cd62595b2b414ef2d04ce1cfa93af34041a99fd6b38691f004314eb83b','0d4076fd78bc5f9ceae6f3386fcefc16b3f7b5bc20bf93680e8cf8d207f29a04','a6e40c574453772f993cb76570f749289281d00e03d45ec57eb2d1bcdcfd7d91','0c1f8ccc8f602a939cbf80e1416abc4de709d63f0b1e29bf627b6a554b88c8cf','84a03a910b5fefb6632bf529203ac5ff2ca56646fb10d92a736e45603339d0cf','82311b6c4ed92cbdc1e3ccd00a7019759998bcbdc35512c190b1f41ce32ec672','cf2dae517c3f4c6b2b77ccb480f00069dcc5d366eecc30a8f499df76642c5d13','006333a4c8ec135e23893a7fc07d770f02fdced6b86ab41ff23aed054f27c54e','83b36fae6c095bea77e0b4ded6b2de4dea4670974c89dec0f4cff96ce39dca15','bd0ccca991d32c7db3e7ee8c393883b971262ec61970e583ed33696d41b34ddf','0ff63e76417f8b821c374fb66f2150e3514e530cdcbbeec5f86e70ef7be332ca','f2ac192e04c4ad62c24a6cc06d8b5709c2efa5c999946bbae47bb60cf076e926','1b44a30dd5a3ca167ed22cb1dad43cf256434a0120265296b95b62b90f832741','358a2121bbc6a2fb0ca41f3cb550d2430b414bab91e3d8af844e7fed817187e6','61cbcf2c67cf63fb6eaf76c4114aeadb2d55d6b8f50a23bbdd3cf4f81798d1fd','c6aab7f471247d3e8da7bebf3ac6a1885b9c9625ab9b02ee95413478d6ce2959','2234fdec29ad5d8b8cc86728eba858988ecc26329d6cb70e5bfac648633ac79c'), 'Poster scanout requires the reviewed new-library RAM'
            assert 'poster_startup_serial_only' not in record
            from scanout_snapshot import binding as screen_binding
            assert record['screen_snapshot_binding']==screen_binding(record) and record.get('screen_capture_after_seconds')==150
        else:
            assert record.get('poster_startup_serial_only') is True
            assert 'screen_capture_after_seconds' not in record
            assert not any(word.split('=',1)[0]=='sfscreen' for word in record['cmdline'].split())
    else:
        assert 'poster_startup_serial_only' not in record
        from scanout_snapshot import binding as screen_binding
        assert record.get('screen_snapshot_binding')==screen_binding(record) and record.get('screen_capture_after_seconds')==150
    env=[w for w in record['cmdline'].split() if w.startswith('sweetenv=')]
    grid=record['native_grid_control']
    spring=';SpringBoard:DYLD_INSERT_LIBRARIES=/dev/sglram/g,GEO_FIX=1'
    spring+=',GEO_GRID=1' if grid.get('revision')!=2 else ''
    aegir=';AegirPoster:DYLD_INSERT_LIBRARIES=/dev/sglram/e' if stage=='poster' else ''
    assert env==['sweetenv=backboardd:'+backboard+spring+aegir], 'v731 GPU needs exact probe and geometry environment'

def assemble_image(header,shim,payload):
    result=bytearray(header);struct.pack_into('<I',result,16,len(payload))
    result+=shim+b'\0'*(-len(shim)%4096);result+=payload+b'\0'*(-len(payload)%4096)
    return bytes(result)

def prove_payload(record):
    """Bind the entire embedded image to the compiled kernel and exact tested input blobs."""
    mode=record['trial_mode'];version=record['version']
    if version in (WLED_VERSION,LOG_VERSION):
        verify_record(record)
        if mode=='gpu':
            assert 'ram_library_binding' in record, 'v731 GPU requires an exact RAM binding'
    kernel=kernel_path(version)
    raw_kernel=kernel.read_bytes()
    verify_wled_kernel(version,raw_kernel)
    assert sha(raw_kernel)==record['kernel_sha256'], 'Compiled kernel differs from image record'
    selected='gpu' if mode=='gpu' else 'ready' if mode in ('owner','ready') else 'private'
    raw,_,reference,blobs,_,shim=baseline(selected,ROOT/'assets/baselines')
    if mode=='ramcpu':blobs.append(baseline('gpu',ROOT/'assets/baselines')[3][-1])
    if 'ram_library_binding' in record:
        assert mode in ('ramcpu','gpu')
        blob,ram_binding,ram_package=load_ram(record['ram_package'],record['ram_library_binding'])
        if version in (WLED_VERSION,LOG_VERSION) and mode=='gpu':
            validate_wled_gpu_record(record,ram_package,ram_binding)
        if record.get('exception_observer',{}).get('program_cache_inventory') or ram_package.get('exception_observer',{}).get('program_cache_inventory'):
            assert record.get('exception_observer')==ram_package.get('exception_observer'), 'Inventory observer metadata differs from the exact signed RAM manifest'
        if 'process_log_binding' in record:
            binding=observer_log_binding(ram_package)
            assert record['trial_mode']=='gpu' and binding and record['process_log_binding']==binding,'Process log binding differs from actual RAM producers'
            tee=[word.split('=',1)[1] for word in record['cmdline'].split() if word.startswith('sweetnull=')]
            assert tee==[','.join(binding['processes'])],'Required process records are excluded from the console tee'
        grid=validate_grid_control(record,ram_package)
        if grid and grid['revision']==2:
            assert record.get('process_log_binding')==observer_log_binding(ram_package) and record.get('process_log_binding'), 'Default grid mode needs current SpringBoard evidence'
        blobs=[(n,blob if n=='GL-RAM-Z' else data) for n,data in blobs]
    elif 'native_grid_control' in record:
        raise ValueError('Native grid control requires its exact RAM package')
    expected_blobs={name:{'bytes':len(data),'sha256':sha(data)} for name,data in blobs}
    assert expected_blobs==record['blobs'], 'Trial firmware/RAM inputs differ from tested baseline'
    if 'display_geometry_binding' in record:
        assert version>=730 and mode in ('ramcpu','gpu') and 'ram_library_binding' in record
        reference,binding=display_transform(reference,record['display_geometry_binding']['profile'])
        assert binding==record['display_geometry_binding'],'Native geometry ADT/binding differs from exact input'
        from numeric_boot_args import numeric_options
        options=numeric_options(record);assert options.get('sf2x')=='0' and options.get('sweetfb_native')=='1'
    payload,_=mkpayload.build(raw_kernel,reference,record['cmdline'],blobs=blobs)
    image=assemble_image(raw[:4096],shim,payload)
    actual=Path(record['image']).read_bytes()
    assert image==actual and sha(image)==record['sha256'], 'Embedded kernel/payload differs from audited inputs'
    return {'image_sha256':sha(image),'kernel_sha256':sha(raw_kernel),'blob_count':len(blobs),
            'complete_payload_reconstructed_from_compiled_kernel':True}
def baseline(name,folder):
    image=folder/({'private':'v724-private.img','ready':'v724-ready.img','gpu':'v724-tested-panel.img'}[name])
    raw=image.read_bytes();assert sha(raw)==BASE_HASHES[name],image
    ks,rs=struct.unpack_from('<I',raw,8)[0],struct.unpack_from('<I',raw,16)[0]
    off=4096+(ks+4095)//4096*4096;payload=raw[off:off+rs]
    assert raw[:8]==b'ANDROID!' and sha(raw[4096:4096+ks])==SHIM_SHA
    assert payload[:8]==b'SWEETXNU' and len(payload)==rs
    vb,entry,ao,an,end,pb,mem=struct.unpack_from('<7Q',payload,16)
    count=struct.unpack_from('<I',payload,12)[0]
    segments=[struct.unpack_from('<4Q',payload,680+i*32) for i in range(count)]
    original_tree=payload[ao:ao+an];tree,parsed=adt.parse(original_tree);assert parsed==an
    chosen=next(c for c in tree.children if c.name=='chosen')
    mmap=next(c for c in chosen.children if c.name=='memory-map')
    names=['GPU-PM4','GPU-PFP','GPU-ZAP-MDT','GPU-ZAP-SEG','GPU-GPMU','ADSP-MDT','ADSP-IMAGE-Z']
    if name=='gpu':names+=['GL-RAM-Z']
    blob_data=[]
    for key in names:
        value=mmap.prop(key);assert value and len(value)==16,key
        pa,bytes=struct.unpack('<2Q',value)
        match=[s for s in segments if s[0]-vb+pb==pa and s[3]==bytes];assert len(match)==1,key
        s=match[0];data=payload[s[2]:s[2]+s[3]];assert len(data)==bytes
        blob_data.append((key,data))
    mmap.props=[v for v in mmap.props if v[0] not in names]
    reference=adt.serialize(tree)
    assert sha(reference)=='257076987411b50f037f70bf97524fae56ac2965e773c1bb8678d05d45480ba6'
    return raw,payload,reference,blob_data,payload[72:680].split(b'\0',1)[0].decode(),raw[4096:4096+ks]
def prepare(mode,kernel,folder,output,version=725,ram_package=None,owner_interval=None,screen_evidence=False,thread_sample=False,screen_after=None,idle_sample=False,swipe_once=False,display_profile=None,native_grid_mode=None):
    assert not (thread_sample and idle_sample), 'Only one reviewed sample trigger can be selected'
    if swipe_once:
        assert mode=='gpu' and screen_evidence and version>=730 and ram_package and screen_after==150
        assert not thread_sample and not idle_sample,'Single-swipe observation stays separate from sampling'
    if screen_after is not None:
        assert mode=='gpu' and screen_evidence, 'Screenshot timing is only for a GPU screen-evidence trial'
        checked_screen_capture_after(screen_after)
    capture_after=(45 if screen_after is None else screen_after) if mode=='gpu' and screen_evidence else None
    config=configuration(version)
    verify_wled_kernel(version,kernel.read_bytes())
    if version in (WLED_VERSION,LOG_VERSION):
        if mode!='gpu':
            assert screen_evidence == (mode=='private'), 'v731 private requires a scanout'
        if mode=='gpu':
            assert ram_package and display_profile=='xz1c-native-2x'
            assert native_grid_mode in (None,'fitted') and not (thread_sample or idle_sample)
    selected='gpu' if mode=='gpu' else 'ready' if mode in ('owner','ready') else 'private'
    raw,payload,reference,blobs,cmd,shim=baseline(selected,folder)
    files=list(blobs)
    # Prove extraction covers the complete tested baseline payload, not just the header.
    reproduced,_=mkpayload.build(str(folder/'kernel-v724-tested.macho'),reference,cmd,blobs=files)
    assert reproduced==payload,'Tested baseline reconstruction mismatch'
    geometry=None
    if display_profile is not None:
        assert display_profile=='xz1c-native-2x' and version>=730 and mode in ('ramcpu','gpu') and ram_package
        assert not thread_sample and not idle_sample,'Geometry and thread sampling are independent trials'
        reference,geometry=display_transform(reference,display_profile)
    token=secrets.token_hex(8);words=cmd.split()
    words=[('sweetloop='+token if w.startswith('sweetloop=') else 'sweetutc='+str(int(time.time())) if w.startswith('sweetutc=') else w) for w in words]
    if mode=='owner':
        words=[w.replace('sweetsample=-','sweetsample=bootdiag').replace('svmro=7','svmro=4') for w in words]
    watchdog=300;observe=240
    if owner_interval is not None:
        assert mode=='owner' and owner_interval==90, 'Only the reviewed owner timing comparison is supported'
        words=[('sweetsample_iv=90' if w.startswith('sweetsample_iv=') else 'sweetar=420' if w.startswith('sweetar=') else w) for w in words]
        watchdog=420;observe=300
    if mode=='ramcpu':
        _,_,_,ramblobs,_,_=baseline('gpu',folder)
        files.append(ramblobs[-1])
    library_binding=None
    serial_only=False
    if ram_package:
        assert mode in ('ramcpu','gpu')
        blob,library_binding,package=load_ram(ram_package)
        if version in (WLED_VERSION,LOG_VERSION) and mode=='gpu':
            stage=validate_wled_gpu_ram(package,library_binding)
            serial_only=stage=='poster' and not screen_evidence
            if stage=='poster' and screen_evidence:
                assert library_binding['blob_sha256'] in ('e98ed2cd62595b2b414ef2d04ce1cfa93af34041a99fd6b38691f004314eb83b','0d4076fd78bc5f9ceae6f3386fcefc16b3f7b5bc20bf93680e8cf8d207f29a04','a6e40c574453772f993cb76570f749289281d00e03d45ec57eb2d1bcdcfd7d91','0c1f8ccc8f602a939cbf80e1416abc4de709d63f0b1e29bf627b6a554b88c8cf','84a03a910b5fefb6632bf529203ac5ff2ca56646fb10d92a736e45603339d0cf','82311b6c4ed92cbdc1e3ccd00a7019759998bcbdc35512c190b1f41ce32ec672','cf2dae517c3f4c6b2b77ccb480f00069dcc5d366eecc30a8f499df76642c5d13','006333a4c8ec135e23893a7fc07d770f02fdced6b86ab41ff23aed054f27c54e','83b36fae6c095bea77e0b4ded6b2de4dea4670974c89dec0f4cff96ce39dca15','bd0ccca991d32c7db3e7ee8c393883b971262ec61970e583ed33696d41b34ddf','0ff63e76417f8b821c374fb66f2150e3514e530cdcbbeec5f86e70ef7be332ca','f2ac192e04c4ad62c24a6cc06d8b5709c2efa5c999946bbae47bb60cf076e926','1b44a30dd5a3ca167ed22cb1dad43cf256434a0120265296b95b62b90f832741','358a2121bbc6a2fb0ca41f3cb550d2430b414bab91e3d8af844e7fed817187e6','61cbcf2c67cf63fb6eaf76c4114aeadb2d55d6b8f50a23bbdd3cf4f81798d1fd','c6aab7f471247d3e8da7bebf3ac6a1885b9c9625ab9b02ee95413478d6ce2959','2234fdec29ad5d8b8cc86728eba858988ecc26329d6cb70e5bfac648633ac79c')
                assert capture_after==150, 'Poster scanout requires one frame at 150 seconds'
            if serial_only:
                assert not screen_evidence
            else:
                assert screen_evidence and capture_after==150, 'v731 graphics suites require the 150-second frame'
        if version>=727:assert 'readonly_probe.c' in package['frontend_build_binding']['source_files']
        files=[(n,blob if n=='GL-RAM-Z' else data) for n,data in files]
        if mode=='gpu' and 'exception_observer' in package:
            # The removed zero-valued scan options have default zero in the
            # audited framebuffer source; TRACE is logging only. Keep cadence.
            words=[w for w in words if w not in ('sweetfb_direct=0','sweetfb_kick=0')]
            words=[observer_environment(w,package['exception_observer']['target_process']) if w.startswith('sweetenv=') else w for w in words]
            # svmro7 owns/defer-starts GPU independently of sweetgpuasync.
            # Keep runtime/display/touch settings; make room for explicit tee.
            words=[w for w in words if w not in ('sweetgpuasync=1','-noprogress','keepsyms=1')]
            # Include the process that produces required geometry records.
            # A background native probe does not observe original Aegir calls.
            words.append('sweetnull='+','.join(observer_log_binding(package)['processes']))
    if screen_evidence:
        assert version>=730, 'Full screenshots require the separately audited v730 reader'
        # Replace periodic diagnostic dumps with an explicit full snapshot; keep
        # display/GPU/input operation and the existing PON06 return untouched.
        words=[w for w in words if not w.startswith(('sweetfbshot=','sweetfbshot_surface=','sweetfbcolor='))]
        # These zero options have explicit zero defaults in the audited source.
        # Reclaim the fixed payload command area without changing scanout mode.
        words=[w for w in words if w not in ('sweetfb_direct=0','sweetfb_kick=0')]
        words=[('sweetfb_noconsole=1' if w.startswith('sweetfb_noconsole=') else w) for w in words]
        words=[('sweetar=420' if w.startswith('sweetar=') else w) for w in words]
        words.append('sfscreen=1');watchdog=420
    grid_control=None
    if native_grid_mode is not None:
        assert mode=='gpu' and geometry and package['geocompat_build_binding']['revision'] in (3,4) and screen_evidence
    if mode=='gpu' and geometry and package.get('geocompat_build_binding',{}).get('revision') in (3,4):
        words,grid_control=apply_grid_control(words,native_grid_mode or 'fitted')
    compact_numeric_ones=[]
    if geometry:words=display_arguments(words)
    if screen_evidence and len(' '.join(words).encode())>607:
        # The pinned XNU PE_parse_boot_argn/getval returns numeric 1 for a
        # present argument without '='. Keep both enabled features identical.
        for name in ('sweetbattery','sf2x'):
            explicit=name+'=1'
            if explicit in words:
                words=[name if w==explicit else w for w in words]
                compact_numeric_ones.append(name)
    zero_omissions=[]
    if geometry:
        original_ones=compact_numeric_ones.copy()
        if serial_only:
            fitted=compact_fitted(words,grid_control,package)
            assert fitted is not None
            words,grid_control=fitted
        try:
            words=display_compact(words,compact_numeric_ones,zero_omissions=zero_omissions)
        except CommandTooLong:
            fitted=compact_fitted(words,grid_control,package) if grid_control and observer_log_binding(package) else None
            if fitted is None:raise
            words,grid_control=fitted
            compact_numeric_ones=original_ones;zero_omissions=[]
            words=display_compact(words,compact_numeric_ones,zero_omissions=zero_omissions)
    zero_binding=None
    if zero_omissions:
        from numeric_boot_args import native_zero_binding
        zero_binding=native_zero_binding(version)
        assert zero_omissions==['sf2x'] and sha(kernel.read_bytes())==zero_binding['kernel_sha256']
    sample_binding=None
    if thread_sample or idle_sample:
        assert mode=='gpu' and screen_evidence and version>=730 and ram_package
        assert package['exception_observer'].get('registration_only') and package['metal_resources'].get('native_Clear_shader_cache')
        words,sample_binding=add_sampler(words,compact_numeric_ones,'after_library_idle' if idle_sample else 'first_target_clear')
    cmd=' '.join(words);assert len(cmd.encode())<=607
    candidate,info=mkpayload.build(str(kernel),reference,cmd,blobs=files)
    assert b'SWEET SURF CALLER DIAG phase=' in candidate
    if version>=726:
        assert b'SWEET SURF MEMREF ERROR address=' in candidate and b'SWEET SURF MEMREF INIT length=' in candidate
    result=assemble_image(raw[:4096],shim,candidate)
    image=output/'trial.img';assert not image.exists();image.write_bytes(result)
    m={'id':'surface-v'+str(version)+'-'+mode,'version':version,'image':str(image),'sha256':sha(result),
       'kernel_sha256':sha(kernel.read_bytes()),'shim_sha256':SHIM_SHA,'cmdline':cmd,'cmdline_bytes':len(cmd.encode()),
       'token':token,'watchdog_seconds':watchdog,'source_revision':config['binding']['manifest_sha256'],
       'diagnostic_binding':config['binding'],
       'boot_sender':'sony-boot-once','boot_sender_sha256':sha((ROOT/'tools/device/boot_once.py').read_bytes()),
       'return_evidence_sha256':sha((ROOT/'tools/device/return_evidence.py').read_bytes()),
       'trial_mode':mode,'baseline_full_payload_reconstructed':True,
        'serial_capture_binding':capture_binding(token),
       'blobs':{name:{'bytes':len(data),'sha256':sha(data)} for name,data in files}}
    if compact_numeric_ones:m['numeric_one_argument_compaction']=compact_numeric_ones
    if zero_binding:m['numeric_zero_argument_omission']=zero_binding
    if geometry:m['display_geometry_binding']=geometry
    if grid_control:m['native_grid_control']=grid_control
    if sample_binding:m['thread_sample_observation']=sample_binding
    if swipe_once:m['swipe_observation']=swipe_once_binding()
    if version==LOG_VERSION:m['console_status_observation']=console_status_binding()
    Artifact(m).validate()
    if library_binding:
        m.update(ram_package=str(Path(ram_package).resolve()),ram_library_binding=library_binding)
        if 'exception_observer' in package:m['exception_observer']=package['exception_observer']
        if 'metal_resources' in package:m['metal_resources']=package['metal_resources']
        if mode=='gpu' and package.get('exception_observer'):m['process_log_binding']=observer_log_binding(package)
    if owner_interval is not None:
        m['owner_observation_interval_seconds']=owner_interval
    if compact_numeric_ones:m['numeric_one_argument_compaction']=compact_numeric_ones
    if zero_binding:m['numeric_zero_argument_omission']=zero_binding
    if screen_evidence:
        from scanout_snapshot import binding as screen_binding
        m['screen_snapshot_binding']=screen_binding(m)
        if mode=='gpu':m['screen_capture_after_seconds']=capture_after
    if serial_only:m['poster_startup_serial_only']=True
    if version in (WLED_VERSION,LOG_VERSION) and mode=='gpu':
        validate_wled_gpu_record(m,package,library_binding)
    if version==LOG_VERSION:m['console_status_observation']=console_status_binding()
    (output/'artifacts.json').write_text(json.dumps({'target_serial':'BH905ND69G','pon_route':6,'artifacts':[m]},indent=2)+'\n')
    common=['SWEET LOOP LIVE proto=1 token='+token,'SWEET RMI4: F12 page=']
    if grid_control:
        common+=['SWEET GEOGRID MODE value='+str(grid_control['value'])]
        if grid_control['value']:common+=['SWEET GEOGRID APPLY native=360x640 old=6 rows=5 cols=4']
    if swipe_once:common+=['SWEET RMI4: swipe up injected']
    if mode in ('private','ramcpu'):
        common+=['SWEET VM RO END snap=2 valid=1 .*writes=0 complete=1',
                 'SWEET SMEM STATE END snap=2 mode=2 attempted=9 .*writes=0 unowned_reads=0 complete=1']
        if mode=='ramcpu':
            library=library_binding or {'library_bytes':14675424,'library_fnv64':'bb763317000ee0d7'}
            common+=['SWEET GLRAM: library read=0 bytes='+str(library['library_bytes'])+' fnv='+library['library_fnv64']+' ro=1']
    elif mode=='owner':
        common+=['SWEET SG OWN END snap=1 result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1',
                 'SWEET SG OWN END snap=2 result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1']
    else:
        common+=['SWEET SCATTER READY END result=0 context=1 identity=1 idle=1 input_bits=36 output_bits=36 native_probe=0']
        if mode=='gpu':
            if package.get('geocompat_build_binding',{}).get('revision')==4:
                common+=['SWEET GLAYOUT END rev=4 visited=[0-9]+ recorded=[0-9]+ grids=[0-9]+ refused=0 truncated=0 write_failures=0 tree_nodes=[0-9]+ owners=[0-9]+']
            common+=['sweetgl: full selftest -> PASS','sweetgl: full surface selftest -> PASS',
                     'sweetgl: caller selftest -> PASS','sweetgl: copy refresh selftest -> PASS',
                     'sweetgl: copy context selftest -> PASS',
                     'sweetgl: bridge state selftest -> PASS','sweetgl: program life selftest -> PASS',
                     'SWEET SURF CALLER DIAG phase=.*status=(0x[0-9a-f]+|0)']
            if package['frontend_build_binding'].get('CA_matrix_revision'):
                assert package['frontend_build_binding']['CA_matrix_revision']==1 and package['frontend_build_binding']['GLES_suite_tests']==9
                common+=['SWEET CA MATRIX END result=0 phases=6 checked_bytes=1536 guard_bytes=192 context_disposed=1 production_GLI=1 matrix_slots=2']
            if version>=727:
                assert library_binding,'Read-only trial needs the tested-probe frontend'
                common+=['sweetgl: readonly selftest -> PASS phase=held_sampler']
            if 'exception_observer' in m:
                if m['exception_observer']['target_process']=='backboardd':
                    if m['exception_observer'].get('observer_log_write_failures_checked'):
                        common+=['SWEET OBSERVER LOG SUMMARY revision=1 failures=0 first_error=0 original_bytes_once=1']
                    common+=['SWEET EXCEPTION observer installed process=backboardd limit=16 original_chain=1',
                        'SWEET GLES SUITE END result=0 tests='+str(package['frontend_build_binding'].get('GLES_suite_tests',8))+' state=2 original_tests=1',
                        'SWEET METAL PROBES begin process=backboardd GLES_state=2 full_suite=1 background=1',
                        'SWEET METAL PROBES END process=backboardd result=0 prerequisite=0 full_suite=1 background=1']
                    if m['exception_observer'].get('AIR_sampler_probe'):
                        common+=['SWEET AIR SAMPLER END result=0 checked_bytes=147456 phases=3 final_allocated=0 GPU_only=1']
                    if m['exception_observer'].get('program_cache_probe'):
                        common+=['SWEET PROGRAM CACHE roundtrip phase=2 first_state=[12] restored_state=1 fresh_pipeline=1 valid=1']
                    if m['exception_observer'].get('program_cache_prime'):
                        if not m['exception_observer'].get('program_cache_inventory'):
                            common+=['SWEET PROGRAM CACHE PRIME END result=0 compiled=[0-9]+ failed=0 max_entries=8 original_requests=1']
                else:
                    common+=['SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1']
                    if not m['exception_observer'].get('registration_only'):
                        common+=['SWEET EXCEPTION begin call=1 class=.*name=',
                                 'SWEET EXCEPTION end call=1 frames=[1-9].*original_chain=1']
                if 'renderer_observations_max' in m['exception_observer']:
                    common+=['SWEET METAL observer installed renderer=NUNIAegirRenderer limit=32 index_type=[Qq] device_object=1 original_forwarded=1']
                    if not m['exception_observer'].get('original_Metal_registry_probe'):
                        common+=['SWEET METAL uniform call=1 index=[0-2] device_nil=[01].*metadata_only=1 original_called=1']
                if m['exception_observer'].get('original_postdraw_observer'):
                    common+=['SWEET POSTDRAW observer installed=1 interpose_pairs=4 limit=64 metadata_only='+('0 original_arguments_results=0' if m['exception_observer'].get('png_filter_encoder') else '1 original_arguments_results=1')]
                if m['exception_observer'].get('png_filter_encoder'):
                    p=m['exception_observer']['png_encoder_prior_proof']
                    common+=['SWEET PNG ENCODER installed pid=[0-9]+ backend=ImageIO_UP filter=32 source_sha='+p['encoder_source_sha256']+' proof_token='+p['reference_token']+' fallbacks_original=1']
                if m['exception_observer'].get('postdraw_png_input_metadata'):
                    common+=['SWEET POSTDRAW PNG_CONFIG pid=[0-9]+ revision=1 getters_only=1 pixels_read=0 profile_read=0']
                if m['exception_observer'].get('snapshot_method_observer'):
                    common+=['SWEET SNAPSHOT observer pid=[0-9]+ installed=[1-8] expected=8 limit=64 metadata_only=1 original_arguments_results=1']
                if m['exception_observer'].get('poster_snapshot_gate_observer'):
                    q=m['exception_observer'];revision=q['poster_snapshot_gate_revision']
                    common+=[f"SWEET PSG CONFIG pid=[0-9]+ revision={revision} installed={q['poster_snapshot_gate_methods']} expected={q['poster_snapshot_gate_methods']} per_api=8 max_calls={q['poster_snapshot_gate_max_calls']} max_replies=64 "+('extra_getters=0' if revision==1 else 'scene_getters=0 path_metadata=1')+' GPU_work=0',
                             'SWEET PSG BEGIN pid=[0-9]+ call=[0-9]+ api=(background|vista)']
                if m['exception_observer'].get('aegir_snapshot_cache_scope'):
                    scope=m['exception_observer']['aegir_snapshot_cache_scope']
                    common+=['SWEET ASCOPE ID pid=[0-9]+ value='+scope['identity'],
                             'SWEET ASCOPE CONFIG pid=[0-9]+ revision=1 installed=1 caller=00000000000063b0 old_files_preserved=1 other_calls_original=1',
                             'SWEET ASCOPE CALL pid=[0-9]+ call=[0-9]+ result=0 original_caller=1 old_files_preserved=1']
                if m['exception_observer'].get('original_snapshot_PNG_export') or m['exception_observer'].get('loaded_snapshot_PNG_export'):
                    common+=['SWEET OPNG END pid=[0-9]+ call=[0-9]+ bytes=[0-9]+ fnv=[0-9a-f]+ unchanged=1 original_return_bytes=1']
                if m['exception_observer'].get('snapshot_render_queue_observer'):
                    common+=['SWEET RQUEUE CONFIG pid=[0-9]+ revision=1 installed=1 limit=8 original_arguments_results=1 extra_GPU_work=0',
                             'SWEET RQUEUE DRAW_END pid=[0-9]+ call=[0-9]+ outcome=return original_behavior=1',
                             'SWEET RQUEUE RESULT pid=[0-9]+ call=[0-9]+ status=4 error_nil=1 code=0 terminal_getters_only=1']
                if m['exception_observer'].get('snapshot_readback_observer'):
                    common+=['SWEET RBYTES END pid=[0-9]+ call=[0-9]+ logical_bytes=[0-9]+ fnv=[0-9a-f]{16} original_bytes_not_changed=1']
                if m['exception_observer'].get('original_CG_input_observer'):
                    common+=['SWEET CGBYTES END pid=[0-9]+ call=[0-9]+ logical_bytes=[0-9]+ fnv=[0-9a-f]{16} original_bytes_not_changed=1']
                if m['exception_observer'].get('original_queue_retirement_observer'):
                    common+=['SWEET RQUEUE RETIRED pid=[0-9]+ call=[0-9]+ fence=[1-9][0-9]* operations=[1-9][0-9]* complete=1 original_command=1']
                if m['exception_observer'].get('original_request_observer'):
                    common+=['SWEET METAL REQUEST CONFIG pid=[0-9]+ installed=13 expected=13 limit=64 revision=2 original_behavior=1 no_extra_GPU_work=1' if m['exception_observer']['request_observer_revision']==2 else 'SWEET METAL REQUEST CONFIG pid=[0-9]+ installed=6 expected=6 limit=64 arguments_returns_errors_errno=1 no_GPU_work=1']
                if m['exception_observer'].get('quad_route_observer'):
                    q=m['exception_observer'];common+=[f"SWEET QUAD ROUTE CONFIG pid=[0-9]+ installed={q['quad_route_methods']} expected={q['quad_route_methods']} per_api=8 max={q['quad_route_max_records']} revision={q['quad_route_revision']} original_behavior=1 no_extra_GPU_work=1"]
                    if q['quad_route_revision']==5:
                        common+=['SWEET PST CONFIG pid=[0-9]+ revision=1 apis=11 slots=256 max=512 original_returns=1 getter_calls=0 GPU_work=0',
                                 'SWEET PST VALUE pid=[0-9]+ sample=[0-9]+ api=(preview|snapshot|backlight|unlock|env_init|env_update)']
                if 'runtime_Metal_ABI_methods' in m['exception_observer']:
                    common+=['SWEET METAL ABI begin class=_MTLDevice present=1 limits_bytes=264 limits_alignment=8 limits_bounded=1',
                        'SWEET METAL ABI end methods=22 metadata_only=1 instances_created=0 gpu_operations=0']
                if m['exception_observer'].get('metal_resource_probe'):
                    common+=['SWEET METAL RESOURCE END result=0 buffers=3 bytes=33201 span=65536 final_allocated=0 gpu_submit=0']
                if m['exception_observer'].get('metal_queue_probe'):
                    common+=['SWEET METAL QUEUE END result=0 final_allocated=0 copy_chunks=2 fill_ops=1 copy_ops=2 native_queue=1']
                if m['exception_observer'].get('AIR_compute_probe'):
                    common+=['SWEET AIR COMPUTE END result=0 phases=2 kernel_invocations=128 checked_words=160 guard_checks=32 final_allocated=0 standard_Metal=1']
                if m['exception_observer'].get('AIR_render_probe'):
                    common+=['SWEET AIR RENDER END result=0 phases=4 draw_ops=3 clear_ops=1 checked_target_bytes=15104 checked_sample_bytes=2048 guard_bytes=6560 final_allocated=0 standard_Metal=1']
                if m['exception_observer'].get('AIR_constants_probe'):
                    common+=['SWEET AIR CONSTANTS END result=0 phases=3 invocations=192 checked_words=240 guard_checks=48 final_allocated=0 standard_Metal=1']
                if m['exception_observer'].get('original_Metal_registry_probe'):
                    common+=['SWEET METAL DISCOVERY END result=0 same_original_default=1 original_API=1 GPU_work_preverified=1']
                if m['exception_observer'].get('registration_only'):
                    prior=validate_native(m['exception_observer']['prior_native_hardware_proof'],m['metal_resources'])
                    assert prior['version']==version and prior['kernel_sha256']==m['kernel_sha256']
                    common+=['SWEET METAL REGISTRY proof_sha='+native_digest(prior)+' long_probes_in_constructor=0']
                if m['exception_observer'].get('original_pipeline_observer'):
                    common+=['SWEET METAL PIPELINE observer installed=1 methods=3 metadata_only=1 original_returns_errors=1',
                        'SWEET METAL PIPELINE END call=1 returned_nil=[01] error_pointer=[01] error_code=-?[0-9]+ original_return=1']
                if m['exception_observer'].get('AIR_XR_probe'):
                    common+=['SWEET AIR XR END result=0 formats=4 phases=12 checked_raw_output_bytes=34944 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('Metal_IOSurface_probe'):
                    common+=['SWEET METAL SURFACE END result=0 cases=2 phases=6 checked_bytes=14976 row_padding=6480 final_allocated=0 original_surface_GPU=1']
                if m['exception_observer'].get('Metal_mipmap_probe'):
                    common+=['SWEET METAL MIPS END result=0 formats=4 levels=6 generations=8 samples=24 checked_bytes=917504 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('AIR_narrow_probe'):
                    common+=['SWEET AIR NARROW END result=0 phases=2 checked_bytes=98304 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('AIR_half_probe'):
                    common+=['SWEET AIR HALF END result=0 checked_bytes=131072 phases=2 graphics=1 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('AIR_MRT_probe'):
                    common+=['SWEET METAL MRT END result=0 XR_formats=4 phases=16 eight_targets=8 ordered_formats=4 checked_bytes=1245184 final_allocated=0 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('AIR_static_half_probe'):
        common+=['SWEET AIR STATIC HALF END result=0 cases=12 paths=4 checked_bytes=196608 final_allocated=0 original_constants=1 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('Clear_cache_probe'):
        common+=['SWEET CLEAR CACHE END result=0 phases=2 targets=8 checked_bytes=262144 final_allocated=0 original_clear=1 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('IR3_cache_probe'):
        common+=['SWEET IR3 CACHE PROBE END result=0 phases=3 checked_bytes=960 final_allocated=0 cold_and_restore=1 original_pages=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('depth32_probe'):
        common+=['SWEET DEPTH32 END result=0 phases=29 checked_bytes=74240 guard_bytes=38744 read_bytes=26100 final_allocated=0 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('stencil8_probe'):
        common+=['SWEET STENCIL8 END result=0 phases=99 checked_bytes=329472 guard_bytes=193149 read_bytes=118800 final_allocated=0 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('memoryless_probe'):
        common+=['SWEET MEMORYLESS END result=0 phases=8 checked_bytes=19456 guard_bytes=10888 final_allocated=0 GPU_only=1 transient_pages=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('multisample_probe'):
        common+=['SWEET MULTISAMPLE END result=0 phases=18 checked_bytes=151488 guard_bytes=55098 final_allocated=0 GPU_only=1 original_samples=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('presentation_probe'):
        common+=['SWEET PRESENT PROBE END result=0 phases=5 checked_pixels=765 final_allocated=0 original_drawables=1 display_completion_unverified=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('program_request_export'):
        common+=['SWEET PROGRAM REQUEST EXPORT END result=0 records=2 bytes=[0-9]+ limit=262144 read_only=1 no_compile=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('program_cache_inventory'):
        common+=['SWEET PCINV BEGIN pid=[0-9]+ phase=B revision=1 max_files=64 max_bytes=33554432 read_only=1 no_compile=1',
                 'SWEET PCINV COMPLETE pid=[0-9]+ result=0 verified_before_after=1']
        if m['exception_observer']['program_cache_inventory']['conditional_prime']:
            common+=['SWEET PCINV ACTION pid=[0-9]+ action=(prime|skip) missing=[0-9]+ max_entries=8 actual_files_checked=1',
                     'SWEET PCINV END pid=[0-9]+ phase=A result=0 files=[0-9]+ records=[0-9]+ requests=[0-9]+ bins=[0-9]+ present=[0-9]+ missing=0']
    if mode=='gpu' and m.get('exception_observer',{}).get('cif10_PNG_reference'):
        ref=m['exception_observer'];cases=ref['cif10_reference_cases']
        common+=['SWEET CIF10 REFERENCE END result=0 cases='+str(cases)+' original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0']
        if m['exception_observer']['cif10_reference_revision']==2:
            common+=['SWEET CIF10 CONFIG revision=2 PNG_exports=1 vImage_modes=2 original_options=1 converter_installed=0']
        if ref['cif10_reference_revision']==3:
            common+=['SWEET CIF10 CONFIG revision=3 PNG_exports=1 vImage_modes=2 filter_policies=3 original_options=1 converter_installed=0']
    if mode=='gpu' and m.get('exception_observer',{}).get('program_cache_scope'):
        scope=m['exception_observer']['program_cache_scope']
        common+=['SWEET PROGRAM CACHE SCOPE identity='+scope['identity'],
                 'SWEET PROGRAM CACHE SCOPE result=0 inherited=[01] directory='+re.escape(scope['directory'])+' mode=1777 old_files_preserved=1']
    plan={'target_serial':'BH905ND69G','pon_route':6,'max_cycles':1,'budget_seconds':600 if screen_evidence else 480,'camera_required':mode=='gpu' and not screen_evidence and not serial_only,
          'steps':[{'artifact_id':m['id'],'observe_seconds':observe,'stable_seconds':30,'return_seconds':120,'goals':common,
                    'return_evidence':'final-drain','on_pass':'stop','on_failure':'stop'}]}
    if screen_evidence:plan.update(screen_required=True,screen_timeout_seconds=100)
    if mode=='gpu' and screen_evidence:plan['screen_capture_after_seconds']=capture_after
    if grid_control:plan['native_grid_control']=grid_control
    if version==LOG_VERSION:plan['console_status_observation']=console_status_binding()
    (output/'plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    (output/'package-proof.json').write_text(json.dumps({'baseline':selected,'baseline_sha256':BASE_HASHES[selected],
       'baseline_payload_exact':True,'kernel_sha256':m['kernel_sha256'],'image_sha256':m['sha256'],'cmdline_bytes':len(cmd),
       'blob_count':len(files),'device_operations':0},indent=2)+'\n')
    return m
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--mode',choices=['private','owner','ready','ramcpu','gpu'],required=True)
    p.add_argument('--kernel',required=True);p.add_argument('--baselines',default=str(ROOT/'assets/baselines'));p.add_argument('--output',required=True)
    p.add_argument('--version',type=int,choices=[725,726,727,728,729,730,WLED_VERSION,LOG_VERSION],default=725)
    p.add_argument('--ram-package')
    p.add_argument('--owner-interval',type=int,choices=[90])
    p.add_argument('--screen-evidence',action='store_true')
    p.add_argument('--screen-after',type=int,choices=SCREEN_CAPTURE_TIMES,help='GPU single-frame observation time; default 45 seconds')
    p.add_argument('--thread-sample',action='store_true')
    p.add_argument('--idle-sample',action='store_true',help='Single original Aegir sample after a returned library at observation 180 seconds')
    p.add_argument('--swipe-once',action='store_true',help='One upward swipe after observed F12/LIVE at 120 seconds, then one 150-second scanout')
    p.add_argument('--display-profile',choices=['xz1c-native-2x'],help='Exact four-property native ADT plus existing native1/sf2x0 framebuffer flags')
    p.add_argument('--native-grid-mode',choices=['original','fitted'],help='Explicit original six-row or fitted native grid, using the exact revision3/4 helper')
    a=p.parse_args();out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    m=prepare(a.mode,Path(a.kernel),Path(a.baselines),out,a.version,a.ram_package,a.owner_interval,a.screen_evidence,a.thread_sample,a.screen_after,a.idle_sample,a.swipe_once,a.display_profile,a.native_grid_mode)
    print(json.dumps({k:m[k] for k in ('trial_mode','token','kernel_sha256','sha256','cmdline_bytes')}))
