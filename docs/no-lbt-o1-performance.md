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
