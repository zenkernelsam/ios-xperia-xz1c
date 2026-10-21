#!/bin/bash
# Reset the verify tree to the state right after patch 0014, so a new
# 0015-lilac patch can be regenerated via git diff.
set -u
SRC=/root/xnu-verify/xnu-xnu-8796.141.3
PATCHES=/root/xz1c/xnu/patches
cd "$SRC"
git reset -q --hard staged && git clean -fdxq
for p in "$PATCHES"/0*.patch; do
	case "$(basename "$p")" in
		0015-*|001[6-9]*|00[2-9]*) continue ;;
	esac
	git apply "$p" || { echo "apply failed: $p"; exit 1; }
done
git add -A .
git -c user.email=v@v -c user.name=v commit -qm thru0014
git tag -f thru-0014
echo "tree at post-0014 state"
