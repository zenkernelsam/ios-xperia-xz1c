# AIR frontend for iOS16

The retained reference/ sources are BSD-2-Clause/MIT code from ios_sagit at 898998cf1b74ea103e0c16d6dc0016822d8d644b. Original bytes and notices are preserved; [air-reference.json](../../../reproducibility/air-reference.json) binds each file. LLVM declarations are a fixed local dependency with Apache-2.0 WITH LLVM-exception notices.

Compiler.h defines the frontend Entry/reflection contract. materialize_air_sources.py creates an isolated build copy and applies the canonical local conversions. The translator uses audited exported LLVM C entry points and preserves unsupported-semantics errors. AIR/MTLB stages, function constants, narrow integers, binary16 storage/arithmetic, MRT, sampler and MSAA fixtures are covered by their original native probes.

The actual free/open-source execution backend is fixed Mesa/Freedreno GLES3.1 on FD540. Native queues retain original buffer/IOSurface pages through submit, retirement and synchronization. [BUILD](../../../docs/BUILD.md) describes reconstruction and independent admission; [STATUS](../../../docs/STATUS.md) alone maintains the current kernel/GLES/Metal/source-map and complete native21/PNG6 results. General original UI, wallpaper, FPS, sleep and peripherals remain separate evidence.

Apple SDK, source shader/container bytes and original scanout frames stay local. Author fixtures, local changes, build tools, licenses and source locks are committed here. Historical compute-only progress and the original README bytes are retained in the [entry archive](../../../docs/archive/20261123/project-entry-supplement/original-document-manifest.json).
