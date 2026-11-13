# 本轮证据与回收

新增工作只在正式仓库ios-xperia-xz1c；Patch/research和WSL没有新增实验文件。关键r1/r2原始结果、单发wire报告、完整串口、健康观测命令记录与每靴一张关键照片已逐字节/SHA复制到本地archive/evidence/aegir-20261113，共661文件/2818648B；preservation.json记录原路径、归档路径与SHA。

Windows原生回收站已实际接收71项/97558852B：冗余MJPEG、重复帧、可由原始日志复算的checkpoint副本，以及r2临时glram.raw/glram.dmg。每项都有原生回收条目验证，windows-recycle.jsonl保留记录。只读压缩blob、manifest、测试库、最新内核、gl5和备份保留。

Mac已正常卸载本轮只读root，原始镜像未写入。两份可重新生成的符号缓存与r1/r2四份临时HFS镜像，共6项，使用NSWorkspace送进macOS Trash并验证目标存在；macos-trash.json保留原路径与Trash位置。未清空用户回收站。

源码与验收摘要进入Git；镜像、照片、原始日志和私有设备资料保持本地。原r1 STOP/STOP_ERROR没有改写成PASS，r2新的CPU/GPU结果独立保存。后续可从保留压缩blob重新解包RAM内容，从原始日志重新推导checkpoint或符号化；一次性工作继续放Windows TEMP或Mac build。

后续Metal／v729迭代的11靴关键结果、原始串口、wire报告、来源审计和每靴关键照片保存到archive/evidence/metal-20261113，215文件/5788125B逐SHA核对。Windows原生回收站新增73项/220956859B，包括重复视频／帧、checkpoint副本和已被v729取代或未投递的v728一次性镜像；原路径与实际回收位置在windows-recycle.jsonl。当前v729全部准入镜像、两版基线内核／库、gl5和备份保留。

Mac新增6份临时HFS镜像通过NSWorkspace送入Trash并核对，macos-trash.json记录位置。最新RAM压缩blob、manifest、实际测试库和构建源保留；未清空任何Windows／Mac回收站。这轮没有新增WSL文件。原未通过的v728结果不改写，恢复旧一次性镜像可查原生回收记录。
