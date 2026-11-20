# 原生 Metal 队列 — 2026-11-13

v729/774b5efd 与r7 GLES/db47937b已实际完整准入。自有SweetMTLCommandQueue / SweetMTLCommandBuffer / SweetMTLBlitEncoder接入现有SweetScatter，内核GPU allocator、SCM、PON、RPC、VM或时钟没有改变。显式工厂提供真实Metal对象；system MetalPluginName、AIR library/pipeline和texture/render尚未接入，原Aegir系统device仍nil，时间/日期/壁纸未恢复。

队列保留enqueue的FIFO位置，后发commit不越过未提交的前序command buffer；队列限制实际未完成command buffer数量。encoder必须结束才能提交，commit只能一次。recorded operations强持有资源直到issued fence退休，完成后先释放backing引用，再发布terminal status；scheduled/completed callback在状态改变后调用。copy/fill当前支持DWORD对齐逻辑区间，拒绝越界、重叠复制和别的device的buffer。字节边缘、其他encoder和GPU shader/render继续待实现，不能将局部API当完整Metal协议。

实际工作通过旧GEM分配、submit和fence路径。A5xx CP_MEM_WRITE做重复字节填充，CP_MEM_TO_MEM做正DWORD源复制；字段来自固定Mesa adreno_pm4.xml，不依赖Vulkan。每16KiB分块，一个被提交的命令BO，所有关联源/目标BO跟踪同一hardware fence。发表失败不重发；命令BO使用原释放路径退休，完成后按既有cache路径同步。packet builder、真实C bridge和队列对象ASan/UBSan覆盖包奇偶/计数/地址/容量、chunk、发布失败、native errno、FIFO、容量、回调、持有与错误状态，iOS arm64编译通过。

第一靴队列r2有正确33,024B全区域结果与回调，并38.219秒完整PON06回程，但独立验收要求的kernel提交记录缺失。实际旧源码只打印前32次submit，Aegir队列从global fence424之后运行，故未作为完整队列通过；v729-metal-queue-r2-incomplete.json保留原controller PASS与独立未通过。没有改写或重发原靴。

r7只为实际C blit接口增加有界记录：真实返回的sequence、chunk字节/PM4 hash、等待结果、命令BO释放和CPU同步结果。独立工具按source/destination实际IOVA重建包字节并计算FNV，确认当前probe的3操作/4提交、两个command buffer顺序、完整destination/保护字节和两次scheduled/两次completed callback。全局fence允许其他客户端插入，不能要求sequence连续，也不借用另一个进程的kernel行。

实际队列r3 token23538081cbe0c6c2取得20份完整、0份缺项：每份填充64B、复制64B、跨块复制16396B，完整33024B含未写区域hash704d3d2c161d15e9零差异。20份共80次真实chunk提交/等待、40个command buffers和660480B全区域检查。原GLES全部测试和960000只读像素仍通过，最后503资源全部released/unmapped、orphan/active/root0；四标记/连续健康30秒和38.454秒回程通过，原worker退出。logSHA45a8dc484b7b95b376ab2cd824e3db6e85570f3b985b83a36b45cbbaadf57800。

GLES r6 CPU37.125/GPU38.781、r7 CPU37.047/GPU38.422秒完整自动返回；队列r2 CPU37.187、r3 CPU37.187秒完整返回。最终库GLES db47937b3bb21ba21cf68de1d775ca95ab473ed375854dcd26a4f83565ea7c0d、Metal a2b11a468c037a46b4d60929b9bd0d38eca8c2ac65081138b8a98a9717574cea、observer1a7930b15a1c1da1fb74b4a1fe9898dc6d1dc40c5e1e169e1f8b595913cbb37f。manifest分别校验源/库/blob，不用不同比例或库的前序证据作准入。

复现入口build_frontend_trial.py、build_metal_library.py、build_exception_observer.py --metal-resources、package_glram.py --presigned --observer --metal-resources、package_trial.py --version729和run_trial.py的同kernel/private/owner/ready/CPU/完整GLES门槛。工具与实际报告都在正式仓库，镜像和原始媒体留本地。下一实现AIR→Mesa着色器和原生texture/render，让资源/队列服务实际壁纸和系统UI；声音/无线/睡眠唤醒目标保持active。
