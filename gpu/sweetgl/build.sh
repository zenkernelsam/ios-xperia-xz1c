#!/bin/sh
# Build sweetgl.dylib (Mesa freedreno + libdrm/msm emulation + EAGL shim)
# for iOS 16 arm64 on lilac. Mesa static libs come from research/mesa-ios.
set -e
H=$(cd "$(dirname "$0")" && pwd)
R=$(cd "$H/.." && pwd)
M=${SWEETGL_MESA_SOURCE:?}
B=${SWEETGL_MESA_BUILD:?}
CC="xcrun -sdk iphoneos clang -arch arm64 -miphoneos-version-min=16.0"
LIBS="src/mesa/libmesa.a src/gallium/auxiliary/libgallium.a
	src/gallium/drivers/freedreno/libfreedreno.a
	src/gallium/winsys/freedreno/drm/libfreedrenowinsys.a
	src/freedreno/drm/libfreedreno_drm.a src/freedreno/ir3/libfreedreno_ir3.a
	src/freedreno/ir2/libfreedreno_ir2.a src/freedreno/fdl/libfreedreno_layout.a
	src/freedreno/common/libfreedreno_common.a src/freedreno/isa/libir3encode.a
	src/freedreno/isa/libir3decode.a src/compiler/isaspec/libisaspec.a
	src/freedreno/perfcntrs/libfreedreno_perfcntrs.a
	src/compiler/glsl/libglsl.a src/compiler/glsl/glcpp/libglcpp.a
	src/compiler/nir/libnir.a src/compiler/spirv/libvtn.a
	src/compiler/libcompiler.a src/mapi/glapi/libglapi_bridge.a
	src/util/libxmlconfig.a src/util/libmesa_util.a
	src/util/blake3/libblake3.a src/c11/impl/libmesa_util_c11.a"
INC="-I$H/include -I$M/include -I$M/src -I$M/src/mesa -I$M/src/mapi
	-I$M/src/gallium/include -I$M/src/gallium/auxiliary
	-I$M/src/gallium/drivers -I$M/src/gallium/winsys -I$M/src/freedreno -I$M/src/freedreno/common -I$M/src/freedreno/drm -I$B/src -I$B/src/mesa
	-I$B/src/mapi -I$B/src/freedreno/registers/adreno -I$M/src/gallium/drivers/freedreno -I$M/include/drm-uapi"
O=${SWEETGL_OUTPUT:-$H/out}
mkdir -p "$O"
cp "$B/src/mapi/shared-glapi/libglapi.0.dylib" "$O/libglapi.0.dylib"
install_name_tool -id /usr/lib/libglapi.0.dylib "$O/libglapi.0.dylib"
python3 -I -c '
import json, shlex, sys
cc = json.load(open(sys.argv[1]))
e = [x for x in cc if x["file"].endswith("state_tracker/st_manager.c")][0]
print("\n".join(shlex.quote(a) for a in shlex.split(e["command"]) if a.startswith("-D")))
' "$B/compile_commands.json" > "$O/mesa-defines.rsp"
nm -g "$B/src/mapi/glapi/libglapi_bridge.a" > "$O/bridge.nm"
python3 -I "$H/gen-dispatch.py" "$H/ios-dispatch.txt" "$M/include" "$O/bridge.nm" "$O/dispatch.c"
$CC -O2 -Wall -fvisibility=hidden -c "$H/compat.c" -o "$O/compat.o"
for f in sweetgl sweet_drm metal_compute metal_render eagl layerdump surface_probe surface_observe surface_region_observe surface_initial_copy caller_probe readonly_probe copy_refresh_probe copy_context_probe bridge_state_probe program_lifecycle_probe; do
	$CC -O2 -Wall -fvisibility=hidden @"$O/mesa-defines.rsp" $INC -I"$H" -c "$H/$f.c" -o "$O/$f.o"
done
$CC -O2 -Wall -fvisibility=hidden @"$O/mesa-defines.rsp" $INC -I"$H" -c "$O/dispatch.c" -o "$O/dispatch.o"
printf '_sweetgl_init\n_sweetgl_create\n_sweetgl_make_current\n_sweetgl_destroy\n_sweetgl_selftest\n' > "$O/exports.txt"
printf '_sweetgl_selftest_state\n' >> "$O/exports.txt"
printf '_sweetgl_metal_buffer_allocate\n_sweetgl_metal_buffer_release\n_sweetgl_metal_buffer_synchronize\n' >> "$O/exports.txt"
printf '_sweetgl_metal_surface_import\n' >> "$O/exports.txt"
printf '_sweetgl_metal_buffer_blit\n' >> "$O/exports.txt"
printf '_sweetgl_metal_compute_create\n_sweetgl_metal_compute_dispatch\n_sweetgl_metal_program_destroy\n' >> "$O/exports.txt"
printf '_sweetgl_metal_render_create\n_sweetgl_metal_render_draw\n_sweetgl_metal_render_clear\n' >> "$O/exports.txt"
printf '_sweetgl_metal_texture_mipmaps\n' >> "$O/exports.txt"
xcrun -sdk iphoneos clang++ -arch arm64 -miphoneos-version-min=16.0 \
	-dynamiclib -dead_strip -Wl,-undefined,dynamic_lookup -Wl,-not_for_dyld_shared_cache \
	-install_name /usr/lib/sweetgl.dylib -exported_symbols_list "$O/exports.txt" \
	-o "$O/sweetgl.dylib" "$O/sweetgl.o" "$O/sweet_drm.o" "$O/metal_compute.o" "$O/metal_render.o" "$O/eagl.o" "$O/layerdump.o" \
	"$O/dispatch.o" "$O/compat.o" "$O/surface_probe.o" "$O/surface_observe.o" "$O/surface_region_observe.o" "$O/surface_initial_copy.o" "$O/caller_probe.o" "$O/readonly_probe.o" "$O/copy_refresh_probe.o" "$O/copy_context_probe.o" "$O/bridge_state_probe.o" "$O/program_lifecycle_probe.o" \
	$(for l in $LIBS; do echo "$B/$l"; done) \
	"$O/libglapi.0.dylib" \
	-framework IOKit -framework CoreFoundation -framework IOSurface -framework OpenGLES \
	-framework CoreGraphics -lobjc -lc++ -lz
codesign -f -s - "$O/sweetgl.dylib"
ls -la "$O/sweetgl.dylib"
