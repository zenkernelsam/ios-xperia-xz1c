# 2026-11-15 MRT 轮整理

Windows原生回收站已核对90条移入记录，涵盖93个文件、142687976字节；Mac NSWorkspace原生垃圾桶已核对13条、129个文件、300480732字节。回收的是重复HFS .dmg、未部署/被替代的构建输出、一次性传输tar、主机测试程序和已结束轮次的多余相机帧。完整清单和当前原生回收路径在reproducibility/organization-mrt-20261115.json。

四个已结束GPU轮各保留两张相机帧，合计8张。原始串口raw、原结果、command/回程证据、当前与失败的signed RAM包/blob/raw、r1精确源输入保持。不是永久删除，没有清空Windows/Mac回收站。Ubuntu及Ubuntu-22.04用户垃圾桶复核为0文件；本轮WSL生成的可再生程序本来位于D盘，已走Windows原生回收站。

Patch/Xperia Git待提交项为0，research仍只有86个参考Git仓库。archive/artifacts复核103文件，无.img/.dmg/.raw/.macho镜像；剩余救砖/参考输入及assets、flash/fw、unlock保持受保护。没有操作设备gl5/userdata/SD/TA/XFL。

源码及失败证据已以bce747a提交并推送正式ios-xperia-xz1c，Windows/Mac正式主线对齐；后续r3绑定器修正继续在正式仓库维护。旧handover/AGENTS已在docs/archive，不恢复Patch源码维护。当前状态集中在STATUS.md，多附件实现/门槛在METAL-MRT.md；完整UI/FPS/睡眠/声音/无线仍active。

本轮发现并修正Mac回收工具对丰富清单字段与/private/tmp别名的处理：路径统一到实际目录，并在第一条回收操作前检查完整请求。最初字段格式拒绝时没有移动；第二次已有10条原生移入，保留原journal，仅单独继续剩余3条，没有覆盖原回收记录。
