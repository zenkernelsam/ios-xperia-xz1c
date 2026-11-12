# 当前状态 — 2026-11-12

BH905ND69G健康fastboot，无device/COM/camera worker。内核v724 SHAda6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a；GLES库SHA9a59820c7e54b1626cacd49acb70a7d460be3c01c415a17429a2464bb5b11571（14,675,424B）。

GPU核心、Surface双向1,218,000像素、caller创建后修改/GPU写回/原VM解除后持有、动态35,600像素、CA双context384像素、shader/program20,480像素与真正名字复用实机通过。

180秒面板实际182.823s，38.547s完整PON06自动回程含健康30秒。三批照片仍只见充电标志、相机按钮、导航条；时间/日期/壁纸缺。关闭前1129次400×400 Surface VMError，20次AegirPoster退出，符号栈为ObjC/C++terminate与FBSSerialQueue，原因尚未捕获。

原controller数据PASS与视觉INCOMPLETE分别保存，失败不改写。logSHA182d3e4b1e335565320c6e8fa78d6cad3d0d28b7cbd80e5de871a4d0d4224e1b；原包418文件/248,653,297B的结果与SHA清单保持，当前关键原日志和三张面板照片保留，其余一次性副本已进回收站。最后277对象全部释放/撤销，orphan0/active0/root0。

正式仓库当前源已重新编译XNU、GLES与shim，结果见reproducibility/build-results.json和shim-build-results.json。新内核e0eb264c/GLES f22dc3d0/shim77d425b0未上机，不代替此前实机通过的da6ddc44/9a59820c基线。

下一步在整理/复现完成后核对caller acquire/prepare/map与AegirPoster异常，不先猜改色。声音CORE APR/AVS2.8已过，实际播放录音/codec/iOS16前端未过，无线同样未完成；完整目标active。
