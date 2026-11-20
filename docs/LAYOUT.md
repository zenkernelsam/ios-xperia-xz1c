# 目录与恢复

唯一正式源码D:/Desktop/Git/ios-xperia-xz1c，Mac~/Desktop/ios-xperia-xz1c；Patch/Xperia只作参考和历史入口。源码／工具／锁／复现进入正式Git，镜像／备份／原private帧不提交。当前设备与准入见[STATUS](STATUS.md)，文档入口见[INDEX](INDEX.md)。

| 目录 | 唯一用途 |
| --- | --- |
| gpu/sweetgl、gpu/metal、gpu/geocompat、xnu | 前端／固定引用／完整内核修补 |
| tools、reproducibility | 构建／单发／核验／回收工具、源码锁、原结果摘要 |
| docs | 现行手册和技术专题；当前状态只在STATUS |
| docs/archive | 过时handover／handout／日志手册原文及SHA |
| deps | 固定上游构建源码，不混作实验 |
| build | 新输出和当轮输入；可重建重复容器／程序及时回收 |
| assets | 用户提供原镜像、固件、备份／救砖、符号表及当前基线 |
| archive/experiments、archive/evidence | 历史实验、小量必要原证据，原结果不可覆盖 |
| archive/artifacts | 少量受保护恢复参考或尚需复现的产物；废弃镜像进回收站 |
| organization | 完整本地盘点／迁移／原生回收journal |
| Patch/research | 89个参考Git仓库，只有repo；实验不放这里 |
| Windows TEMP | 一次性运行、staging／传输，不在桌面堆放 |

## 回收与保护

禁止永久删除。Windows原生Recycle Bin；WSL废弃内容转Windows回收站；Mac NSWorkspace／Finder Trash。不清空Windows／Mac回收站；Ubuntu垃圾桶非空先转Windows回收站再恢复空目录。所有递归移动／回收先核对绝对范围、符号链接和父子路径，重复或嵌套整批拒绝。flash/fw、unlock、XFL、救砖、SD／TA／校准和备份受保护，备份不去重。

本周Mac1713项／34883文件／11.04GiB已原生垃圾桶，build约14→3.6GiB；垃圾桶仍占空间，系统仍约94%。原路径与Trash位置及内容SHA见[ORGANIZATION](ORGANIZATION.md)。之前Windows旧回收条目有部分在最终复核时已缺失，原因未证明；保留原移入记录，不保证它们仍可恢复。每次原生移入和最终目标复核分别记录。

当前Mac保留gl4／gl5和IPSW，符号表assets/symbols/ios16-20H392-root.a2s，v730 build/surface-diag-v730/kernel.v730.macho、dxb实际源码、deps固定源／Mesa IR3归档、AIR输入及CA／当前tiled／原失败包精确blob／manifest／必要signed库。Windowsassets/baselines的v724 private／ready／tested-panel、kernel／shim和恢复资料保持，作为完整payload重建输入。

## 恢复可重建RAM与raw

过时RAM的builder `.dmg`、重复`.raw`和部分signed库展开副本已进垃圾桶，保留`glram.zblob`与manifest。SGLZ0001为40B头加zlib流，raw32MiB／HFS+；manifest锁raw／blob SHA和每个文件raw_offset／length／SHA。需要时输出新build或TEMP目录，逐SHA恢复，原blob／manifest不改。

```python
import hashlib, json, struct, zlib
from pathlib import Path
package = Path("/path/to/retained/ram-package")
output = Path("/path/to/new/output")
assert not output.exists()
m = json.loads((package / "manifest.json").read_bytes())
blob = (package / "glram.zblob").read_bytes()
assert hashlib.sha256(blob).hexdigest() == m["blob_sha256"]
magic, revision, header, length, packed, fnv = struct.unpack_from("<8sIIQQQ", blob)
assert (magic, revision, header) == (b"SGLZ0001", 1, 40)
assert length == 33554432 and packed == len(blob) - 40
raw = zlib.decompress(blob[40:])
assert len(raw) == length and hashlib.sha256(raw).hexdigest() == m["raw_sha256"]
output.mkdir(parents=True)
(output / "glram.raw").write_bytes(raw)
for name, info in m["files"].items():
    assert Path(name).name == name
    data = raw[info["raw_offset"]:info["raw_offset"] + info["bytes"]]
    assert len(data) == info["bytes"] and hashlib.sha256(data).hexdigest() == info["sha256"]
    (output / name).write_bytes(data)
```

原设备gl5不重刷。其sparse SHA e9bf1547、raw17179869184B／4c90d65a，完整指纹及`restore_gl5_raw.py`恢复见[ROOT-GL5](ROOT-GL5.md)。原raw可用回收站恢复，或从保留sparse重建新文件；任何重建不自动发送设备。全新gl5首次启动仍未独立验收。

原日志保存证据本身，截图可由本token raw和artifact在新目录离线复解。原照片不再次旋转，capture日期和source明确。废弃host测试可由正式源重编，新库仍需自己的实机准入，不能用恢复旧二进制跳过源码锁。
