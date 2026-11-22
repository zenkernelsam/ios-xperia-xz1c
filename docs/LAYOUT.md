# 目录与恢复

唯一正式源码D:/Desktop/Git/ios-xperia-xz1c，Mac为~/Desktop/ios-xperia-xz1c。Patch/Xperia只作参考和历史入口。现行设备与准入看[STATUS](STATUS.md)，构建看[BUILD](BUILD.md)，专题与历史由[INDEX](INDEX.md)串联。

| 目录 | 唯一用途 |
| --- | --- |
| gpu/sweetgl、gpu/metal、gpu/geocompat、xnu | 前端、固定引用及完整内核修补 |
| tools、reproducibility | 构建、单发、核验、回收工具、源码锁与原结果摘要 |
| docs | 现行手册及技术专题，当前状态只在STATUS |
| docs/archive | 过时handover／handout、工作日志和手册原文／SHA |
| deps | 固定上游构建源 |
| build | 新输出、当轮产物；完成后的重复容器、host与object及时回收 |
| assets | 用户提供原镜像、固件、救砖／校准／备份、符号及基线 |
| archive/experiments、archive/evidence | 历史实验与少量必要原证据 |
| archive/artifacts | 尚需恢复／复现的少量产物；废弃镜像进回收站 |
| organization | 完整本地盘点、迁移、原生回收journal |
| Patch/research | 实查89个参考Git仓库，只放repo |
| Windows TEMP | 一次性run、staging／传输，不在桌面堆放 |

## 回收与受保护输入

Windows原生Recycle Bin，Mac NSWorkspace／Finder Trash；WSL废弃内容先移至Windows TEMP再进Windows原生回收站。禁止永久删除或清空Windows／Mac回收站。Ubuntu垃圾桶非空先转Windows回收站，再恢复空目录；本轮两套Ubuntu均无内容。

回收前核对绝对范围、符号链接、重复／父子路径、全部文件SHA与保护指纹；回收后独立核对真实原生目标和内容。原路径、Trash／Recycle Bin路径及最新空间快照只在[ORGANIZATION](ORGANIZATION.md)和各原receipt维护。历史移入记录与当前目标复核不同，旧条目是否后来消失按原报告保留原因未知。

gl4／gl5、IPSW、Sony固件、SD／TA／XFL、校准、救砖／flash/fw／unlock及备份不去重或回收。Mac保持准确A2S、v730内核、dxb实际83源、固定deps／Mesa归档、AIR输入、全部当前signed库／observer、精确RAM blob／manifest、失败包／raw／proof和必要页面；~/xnu-autobuild.pause保持。

Windows assets/baselines的v724 private／ready／tested-panel、kernel／shim及恢复资料作为完整payload输入保持。本轮Root／Dock／Folder原模型、UI的REVISE、三靴原raw和原扫描像素全部保留。回收后恢复旧产物不会自动取得新库准入或授权重发旧失败。

## 从保留blob恢复RAM

重复.dmg／.raw已进垃圾桶，glram.zblob和manifest保留。SGLZ0001是40B头及zlib流，原raw32MiB／HFS+；manifest锁raw／blob SHA、文件raw_offset／bytes／SHA。输出须新build或TEMP目录，原blob／manifest保持。

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

设备gl5不重刷。保留sparse的精确SHA、原17179869184B HFSX及恢复工具见[ROOT-GL5](ROOT-GL5.md)。静态范围研究使用hfs_sparse_reader.py直接读稀疏extents，不为分析展开16GiB raw或挂载输入；全新gl5首次启动仍未独立验收。

原照片不再次旋转；原扫描图可由对应token／artifact／raw离线解到新目录，日期与来源明确。host／object按正式源重编，新二进制仍需自己的实机证据。

## 文档原文恢复

本轮50份整合前原字节、原路径和SHA见[manifest](archive/20261122/native-full-model-r19-close-out/original-document-manifest.json)。每份originals按原目录保存；恢复到其原路径后使用相对链接，历史“当前／in-flight”不成为执行状态。现行构建与操作以BUILD／OPERATIONS为准。
