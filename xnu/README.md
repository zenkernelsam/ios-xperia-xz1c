# XNU for Xperia XZ1 Compact

Current kernel inputs, hashes and device acceptance are maintained in [STATUS](../docs/STATUS.md). The canonical build and reconstruction instructions are in [BUILD](../docs/BUILD.md); device operations use [OPERATIONS](../docs/OPERATIONS.md).

The source is Apple OSS xnu-8796.141.3 at the pinned source-lock revision, built with the pinned darwin-xnu-build and the canonical lilac-current.patch, followed by the v725 through v730 diagnostic patches. The frozen v730 kernel is audited against 83 actual compiled source files. Daily userspace GPU work reuses that accepted kernel; kernel GPU, allocator, SCM, PON, RPC and VM remain frozen.

Build outputs are local build artifacts and require their own device evidence. The former ios-sweet GitHub artifact instructions are preserved in the [original entry archive](../docs/archive/20261123/project-entry-supplement/original-document-manifest.json). Signed inputs and recovery files remain local and protected.
