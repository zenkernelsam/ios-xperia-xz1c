# 2026-11-12 整理与正式源码迁移

唯一正式源码仓库为 ios-xperia-xz1c。Patch/Xperia只保留参考Git仓库、受保护输入/备份和历史资料。research只参考Git仓库：Patch86个、Mac3个；索引在REFERENCE-REPOSITORIES.md。

## 实际回收

| 范围 | 文件数 | 逻辑字节 |
| --- | ---: | ---: |
| Patch历史生成物 | 45,821 | 40,557,619,646 |
| Windows正式目录历史生成物 | 31 | 94,032,693 |
| 已结束Windows TEMP运行 | 45,585 | 34,762,893,461 |
| Mac历史生成物 | 1,596 | 4,253,552,628 |
| WSL旧输出 | 1,176 | 2,066,838,900 |

另回收旧xnu-builds/former-artifacts、源补丁残片副本、老bundle/tar、空破损文件与可再生成缓存。最后补漏72个旧输出（441,837,728字节）也已进入Windows回收站。Windows/WSL移入时均验证原生回收站入口，Mac用NSWorkspace原生Trash。代理未执行清空回收站；移入时数据仍占磁盘，不将回收称为释放全部空间。

最终复核时，Windows 488条历史回收记录中仅9条仍在原回收路径，479条现在缺失，原因未证明；不能保证这些旧条目现在仍可恢复。Mac已核对的8条相关回收记录均仍在垃圾桶。原移入记录保留organization本地清单，当前可恢复性另记reproducibility/organization-results.json。

flash/fw、unlock的XFL、本机SD/TA/校准、现有gl5与根镜像/救砖固件没有删除。当前核对基线和卡点所需少量raw日志/三张面板照片保持；其余一次性副本回收。每轮原结果、token、SHA和原文件计数保留reproducibility/evidence-cleanup-index.json，归档manifest不改写。

WSL两个Ubuntu垃圾桶本来为空；未对其它项目做清理。旧指向Windows C:/Users/Cisco He/Desktop的链接已换为D盘正式仓库，原链接tar恢复包在Windows回收站。Linux遗留socket目录留作历史，不跨平台破坏特殊文件。

安装Bison/Flex时有一次遗漏Homebrew的禁止自动清理变量，Homebrew自动清掉自己的旧下载/索引缓存；Sony目录、源、镜像和备份不受影响。已记录这个例外，随后禁用其自动更新/清理。所有项目清理仍仅归档或原生回收站。

## 正式源码与复现

当前XNU完整源码补丁、Mesa修补、GLES/CA、shim、几何辅助库、RAM HFS打包器、ADT显示调整器和设备辅助工具进入正式仓库。旧编号patch/handover保存docs/archive。

从固定上游重新应用补丁验证404个XNU源码变更路径、78个实机审计源、7个Mesa变更源；41个旧reject/original/Finder元数据从当前patch排除。新固定源码完整编译出XNU、GLES、shim，几何辅助库SHA与原实机e65bdc7b完全相同，新的32MiBRAM HFS只读/硬链接/压缩blob主机验证通过。记录见reproducibility/。

新编译产物不替换实机v724/da6ddc44和GLES9a59820c基线，没有在整理时发送手机。已验证原件保留assets/baselines，恢复镜像在Patch/Xperia/assets/recovery；两边archive现在没有编译过的.img镜像。当前使用的Mac符号缓存迁入assets/symbols，重复缓存及旧Mesa编译树回收。

正式源码主线已推送，Windows/Mac同步并通过干净检出的源码锁及20项故障回放；Patch的Xperia待提交项已归零，整理分支仅含Xperia变更，其它项目未动。全新gl5首次启动与迁移后的controller实机运行仍待验证，见ROOT-GL5.md和STATUS.md。目标保持完整GPU/显示/声音/无线移植，当前锁屏时间/日期/壁纸仍缺。
