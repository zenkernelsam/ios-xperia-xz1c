"""Build Mesa and the canonical GLES frontend without destructive clean commands."""
import os,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
venv=root/'build/python'
if not (venv/'bin/python3').exists():
    subprocess.run(['python3','-m','venv',str(venv)],check=True)
subprocess.run([str(venv/'bin/python3'),'-m','pip','install','-r',str(root/'tools/build/requirements.txt')],check=True)
env=os.environ.copy();env['PATH']=str(venv/'bin')+os.pathsep+env.get('PATH','');env['NINJA']=str(venv/'bin/ninja')
tool_dirs=[Path(p) for p in ('/opt/homebrew/opt/bison/bin','/usr/local/opt/bison/bin','/opt/homebrew/opt/flex/bin','/usr/local/opt/flex/bin')]
env['PATH']=os.pathsep.join(str(p) for p in tool_dirs if p.exists())+os.pathsep+env['PATH']
subprocess.run(['python3',str(root/'tools/build/prepare_sources.py'),'mesa'],check=True)
source=root/'deps/mesa';build=root/'build/mesa-ios';build.parent.mkdir(exist_ok=True)
def sdk(*args):return subprocess.check_output(['xcrun','--sdk','iphoneos',*args],text=True).strip()
cross=root/'build/ios-cross.ini'
cc=[sdk('--find','clang'),'-arch','arm64','-isysroot',sdk('--show-sdk-path'),'-miphoneos-version-min=16.0'];cxx=[sdk('--find','clang++'),*cc[1:]]
cross.write_text("[binaries]\nc = "+repr(cc)+"\ncpp = "+repr(cxx)+"\nobjc = "+repr(cc)+"\nar = "+repr(sdk('--find','ar'))+"\nstrip = "+repr(sdk('--find','strip'))+"\npkg-config = 'false'\n\n[host_machine]\nsystem = 'darwin'\nsubsystem = 'ios'\ncpu_family = 'aarch64'\ncpu = 'arm64'\nendian = 'little'\n")
opts=['--buildtype=release','-Db_ndebug=true','-Ddefault_library=static','-Dplatforms=','-Degl=disabled','-Dglx=disabled','-Dgbm=disabled',
      '-Dgallium-drivers=freedreno','-Dvulkan-drivers=','-Dfreedreno-kmds=msm','-Dllvm=disabled','-Dgles1=disabled','-Dgles2=enabled','-Dopengl=true',
      '-Dzstd=disabled','-Dzlib=disabled','-Dexpat=disabled','-Dxmlconfig=disabled','-Dvalgrind=disabled','-Dlibunwind=disabled','-Dlmsensors=disabled',
      '-Dbuild-tests=false','-Dshader-cache=disabled','-Dperfetto=false','-Dgallium-va=disabled','-Dtools=','-Dhtml-docs=disabled']
includes=['-I'+str(root/'gpu/sweetgl/include'),'-I'+str(source/'include/drm-uapi')]
if not (build/'build.ninja').exists():
    subprocess.run([str(venv/'bin/meson'),'setup',str(build),str(source),'--cross-file',str(cross),'-Dc_args='+repr(includes),'-Dcpp_args='+repr(includes),*opts],env=env,check=True)
else:
    subprocess.run([str(venv/'bin/meson'),'setup','--reconfigure','--clearcache',str(build),str(source)],env=env,check=True)
frontend=(root/'gpu/sweetgl/build.sh').read_text()
libraries=re.search(r'LIBS="([^"]+)"',frontend).group(1).split()
# Compile the actual linked dependency closure, not unrelated xmllint/bsdtar tools.
targets=libraries+['src/mapi/shared-glapi/libglapi.0.dylib']
subprocess.run([str(venv/'bin/ninja'),'-C',str(build),'-j8',*targets],env=env,check=True)
env['SWEETGL_MESA_SOURCE']=str(source);env['SWEETGL_MESA_BUILD']=str(build)
env['SWEETGL_OUTPUT']=str(root/'build/sweetgl')
subprocess.run(['bash',str(root/'gpu/sweetgl/build.sh')],env=env,check=True)
print('CANONICAL_GLES_BUILD_COMPLETE')
