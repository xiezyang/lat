# no-LBT O1 性能修改记录

状态：待验证候选；用户明确要求本次不编译、不运行测试，所有运行正确性和性能结论均未验证。
源码：用户指定的发布副本，原目录没有Git；05af5aa为原样源码快照（现有latx-release二进制不纳入版本控制，也不更新）。原始来源见RELEASE_SOURCE_INFO.txt。分支codex/no-lbt-o1-performance，不使用master。
约束：O1、静态链接、关闭KZT；不改AVX和rounding策略；无探针，无部署，无新二进制。

## 01 构建配置

将build64.sh默认O1 configure改为--optimize-O1 --static，删除--enable-kzt（configure默认关闭KZT，且不支持--disable-kzt）；仍使用build64.sh -c，不需要切到O2。脚本本身不接受-static参数，静态链接由configure的--static落实。
这项只统一构建条件，不声称其本身有性能收益。
待补：在LoongArch构建机执行./latxbuild/build64.sh -c，检查生成配置为O1、KZT关闭；检查ELF为静态链接。确认O2专属JRRA_STACK、RADICAL_EFLAGS未启用。未执行。

## 通用后续验证要求

各优化逐提交对比，使用相同O1/static/no-KZT构建条件（比较优化时以01配置提交为基线）。先补lat-pr-fast及受影响的tests/latx测试，再在真实无LBT目标验证无非法指令、结果、信号与内存顺序；有LBT机器强制关闭不能代替真实无LBT验证。
性能采用代表性应用及SPEC ref，基线/候选交错多轮，保留每次输出校验与耗时。不加探针测175通过率，不修改既有LAT结果。若需要路径证明，将诊断与正式计时分开。无收益或正确性失败即回退相应独立提交。

## 02 LASX独立于LBT

删除no-LBT策略对option_enable_lasx的无条件清零，保留latx_apply_host_hwcap的检测结果。不强制启用LASX，不开启guest AVX，不放宽no-LBT非对齐内存回退。意图是在具有LASX的机器保留既有向量指令选择，避免仅因LBT关闭而退化。
待补：分别验证无LASX、有LASX无LBT、有LASX且强制no-LBT三种能力；SSE/SSSE3/SSE4结果、helper前后和信号前后向量状态、非对齐及跨页访问；检查不支持LASX时无xv指令。调整tests/latx/latx-config-regression.c旧的“no-LBT必关LASX”断言，覆盖独立能力组合。比较代表向量负载的指令数和总时间；未编译、未测试、收益未知。

## 03 相邻寄存器CMP/TEST + Jcc

no-LBT的inst pattern从全关改为仅保留CMP_JCC/TEST_JCC位，并与原有用户mask取交集。限定相邻、寄存器/立即数操作数、非LOCK、非单步；内存操作与中间夹其他指令的组合仍回退，避免跨内存访问移动屏障和进入含原始LBT指令的异常恢复。
为这两个组合增加软件flags专用生成路径：分支前只生成一份仍需保留的flags，分支直接比较操作数。跳过TU专用链接和EFLAGS_CACULATE的单指令补丁/备份，保持eflags_target_arg无效。原因：原有tb_eflag_eliminate/recover只替换4字节，不能处理软件flags指令序列。
预期减少从软件flags重新提取分支条件的指令，并避免分支两侧复制软件flags；不承诺能消除全部flags计算。
待补：8/16/32/64位（含高8位寄存器）、有符号/无符号边界、立即数符号扩展、各支持Jcc的taken/not-taken；后继块通过ADC/SBB/SETcc/LAHF/PUSHF消费flags；块链接/失效/重新链接、自修改代码、信号、单步；内存/非相邻/LOCK负例应保持普通翻译；检查生成代码没有LBT且未发布单指令flags补丁位置。更新配置回归对instptn全关的旧断言。未编译、未运行，实际收益待对比分支密集整数负载。

## 04 八字节对齐的128位访存

按用户追加要求优先低改动且可能有大收益的路径。原先no-LBT的latx_load/store_v128对非16字节对齐地址统一走16次字节访问与拼接/拆分；新增8字节对齐分支，用两次自然对齐ld.d/st.d搬运原16字节。16字节对齐仍走原vld/vst，更低对齐仍逐字节回退。无需新的CPU扩展，不用LBT，不读取范围外字节，不改变已有屏障策略。
预期：地址模16等于8时，访存从16次降到2次，同时减少移位拼接；其他非16字节对齐地址增加一次对齐判断，有退化可能。是否划算取决于实际对齐分布。
待补：0..15每种地址余数，随机/全零/全一数据，load/store，页尾与保护页边界，信号恢复，真实严格对齐无LBT宿主；检查地址模16为8确实命中ld.d/st.d而奇数地址仍用字节路径。比较8对齐密集与奇数地址密集两类负载及真实应用。未编译、未测试。
