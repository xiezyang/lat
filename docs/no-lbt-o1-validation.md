# no-LBT O1验证记录（2026-09-18）

候选源码737b13e；分支codex/no-lbt-o1-performance。用户现已授权测试，最终目标不支持128位非对齐访问。
保护条件：latx_load/store_v128仍只在地址模16为0时发vld/vst；新增路径仅地址模8为0时发两次ld.d/st.d，其余保持字节回退。不能把.7通过当成最终严格对齐目标已通过。

## 原版TTS复现

目标192.168.8.7:2024，Loongson，内核4.19.190-comp+。运行目录为用户指定lat/work，LATX_SOFTFPU=2，LD_LIBRARY_PATH=.，timeout 1800 ../latx-release -L ../guest-runtime ./tts，参数来自test_tts1.sh；输出单独保存，不覆盖1.wav。
原二进制SHA256：4c7d563203fd1a6fe634243fa9cbd6b88dbdbae8652bbd51f73c5de8374b222e。
结果：rc=0，1.467秒，打印“合成完毕”；stderr为LATX: software state mode enabled (LSX=1 LASX=0 LBT_X86=0)。WAV 139170字节，SHA256 e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a，与既有记录一致。
原始命令、环境、输出、JSON、WAV保存在目标lat/validation-20260918/baseline.*。

## 构建进行中

la-dev GCC14.2，独立build64-validation，O1/static/no-KZT，加--enable-tests；不覆盖build64和已有latx-release。
第一次configure被clock_adjtime探测向nonnull参数传NULL的-Werror阻挡，属于旧探测代码与新工具链警告兼容问题。添加--disable-werror继续（保留脚本既有CFLAGS）。系统无meson，指定已有/home/xzy/work/lat-master-20260917/meson/meson.py，版本0.60.1。完整日志见build64-validation/configure-output.log与config.log。
尚未生成候选二进制，尚不能报告候选运行成功或性能改善。后续所有阶段继续追加记录。

## 回归用例补充

更新既有配置断言：LASX保留检测结果，instptn仅保留相邻CMP/TEST两个选项位，并验证用户关闭后不被重新开启。扩展既有unaligned-v128到256种源/目标地址余数组合，检查搬运数据与写入范围外哨兵；跨页数据改为跨64KB边界，兼容4KB/16KB/64KB宿主页尺寸。测试已登记在原有integration入口，未加入产品构建。正在编译，尚未报告通过。

## 构建问题与处理

la-dev产品源文件已编译，最终链接在Binutils 2.44的elfnn-loongarch.c:2710断言并SIGABRT，不能据此归因产品运行错误。转到原版构建机192.168.8.2:22522（GCC8.3），使用独立lat-no-lbt-o1-validation-20260918目录构建相同产品源码，保留原发布构建。
另发现既有latx-config-regression目标只链接string-utils/runtime对象，但用例调用options_init和host策略函数，缺少真实实现导致未定义引用。测试目标补入latx-options.c与have_am所在tr-opnd-process.c，用函数节回收排除无关翻译器代码；不更改产品链接，不用stub替换被测实现。待构建验证。

配置测试补齐qemu_strtou64所属qemuutil依赖。GCC8.3独立候选构建已成功，产物对应0988c23（后续仅测试链接/文档变化）；已复制到目标validation目录的latx-candidate-0988c23，不覆盖原版。正在执行真实目标focused对照。

## 最新检查点：用户要求先总结并保证可续接

候选0988c23产品二进制SHA256 8e0333b8f48dc2b3170f8f4276ff1acbe5fef2ac5ab15b91f5d1c4283e2e711a，GCC8.3构建，O1/static/no-KZT。
目标validation-20260918/focused.json：candidate与旧baseline均通过unaligned-v128、signal-xmm-no-lbt、lock-cmpxchg-no-lbt、lua-number-conversion-no-lbt、latx-tso-ordering-no-lbt（全部rc0）。环境LATX_SOFTFPU=2，-latx-host-hwcap 0x10。测试guest先在xzy86用GCC -nostdlib -static -no-pie构建并原生运行rc0；源文件和guest保存在xzy86:/home/xzy86/work/no-lbt-validation-20260918，以及本地validation备份目录。
目标tts-paired.json：候选3次1.257915、1.245445、1.272196秒；旧版3次1.441850、1.417163、1.468884秒。全部rc0，输出“合成完毕”，WAV均139170字节且SHA256 e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a。stderr只有既有软件状态提示（LSX=1 LASX=0 LBT_X86=0）。这是O1候选对旧O2发布版，不能作单变量性能归因。
la-dev配置回归已构建并13/13通过，日志build64-validation/config-test-output.log。
完整lat-pr-fast使用--no-rebuild运行时因test-exclusive-timeout、test-kzt-callback-fpr等尚未生成而FileNotFoundError；不能称套件通过，也不是产品运行失败。下一步只构建已登记fast套件的目标再执行，避开la-dev产品链接器故障。
同配置基线：d3defa8已归档到原版构建机/home/yuerengan/lala/xzy/lat-no-lbt-o1-baseline-20260918，./latxbuild/build64.sh -c构建中，日志build-output.log；需检查完成状态，再复制到目标独立validation目录与候选交错比较。原旧latx-release和1.wav未覆盖。
剩余：同O1基线对照、完整fast套件、严格对齐最终板卡验证；LASX在本次softfpu2运行环境下关闭，故此轮不证明LASX优化生效。没有新的产品源码修复，测试链接依赖修复已独立提交。


## 同配置 O1 对照结果

两份二进制均由192.168.8.2:22522的GCC 8.3在相同O1/static/no-KZT条件下构建：基线为d3defa8，SHA256 88b47688377e1f462b74995c25f41fa7ea6706c989947925fd2f4c52a3850e56；候选包含至0988c23的产品改动，SHA256 8e0333b8f48dc2b3170f8f4276ff1acbe5fef2ac5ab15b91f5d1c4283e2e711a。两者均独立存放于目标validation-20260918，不覆盖发布二进制。

在192.168.8.7的用户指定TTS环境中，以baseline/candidate交错顺序各运行5次。基线秒数：1.455097、1.451112、1.468804、1.453749、1.485766，中位数1.455097；候选秒数：1.244301、1.281782、1.248579、1.275734、1.271796，中位数1.271796。按中位数计算候选快14.41%。10次均rc=0、打印“合成完毕”，WAV均为139170字节，SHA256均为e0f9aeaf8619a87fa510ba8891138aa0bc19e1dd1d2d10c72b9c428cdb21348a。原始命令、每轮stdout/stderr、WAV与tts-o1-paired.json均保留在目标validation目录。

同配置专项对照也已完成：baseline与candidate均通过unaligned-v128、signal-xmm-no-lbt、lock-cmpxchg-no-lbt、lua-number-conversion-no-lbt、latx-tso-ordering-no-lbt，全部rc=0。unaligned-v128已覆盖16种源地址余数乘16种目标地址余数，含跨64KB边界；它验证新增8字节对齐双64位路径与其余逐字节回退在.7工作，但.7不是最终不支持128位非对齐访问的板卡，不能替代该板卡验证。所有专项运行显式LATX_SOFTFPU=2、LD_LIBRARY_PATH=.、-latx-host-hwcap 0x10；因此LASX运行时为0，LASX保留策略尚未在本轮实际执行。

结论：该候选已在.7上完成机制相关的专项正确性和单一TTS应用性能验证，继续调查；它尚不是最终板卡或完整回归意义上的产品接受。


## 快速回归完成

在192.168.8.2:22522的独立候选树以O1/static/no-KZT及--enable-tests完整构建后，lat-pr-fast为24/24通过。首次24项中仅x86_64-linux-user-latx-config-regression失败；原因是该静态源码审计测试没有收到G_TEST_SRCDIR，无法打开flag-lbt.h，不是候选二进制或运行语义失败。提交a889d61向该测试传入meson.project_source_root()，重新配置、构建和运行后24/24通过。完整日志：/home/yuerengan/lala/xzy/lat-no-lbt-o1-validation-20260918/build64/fast-test-output.log。


## 分项 TTS 测量

为区分各产品提交，分别由192.168.8.2:22522的GCC8.3构建4个相同O1/static/no-KZT二进制：d3defa8基线；07db9d5（LASX策略加相邻CMP/TEST+Jcc）；688467c（前者加8字节对齐向量搬运）；0988c23候选（前者加LEA屏障排除，0988c23后续仅有测试代码）。在192.168.8.7用户指定环境中，以5种轮换顺序各运行5次TTS，全部rc=0、WAV哈希一致。

基线中位数1.427194秒（原始：1.425467、1.424093、1.451073、1.427194、1.437033）。至07db9d5中位数1.310792秒，较基线快8.88%；本轮softfpu2显示LASX=0，故这一步可归于受限相邻CMP/TEST分支组合，而非LASX。至688467c中位数1.294435秒，较上一步快1.26%、较基线快10.26%。完整候选中位数1.261689秒，较上一步快2.60%、较基线快13.12%。每项原始时间、命令、输出和WAV保存在目标validation-20260918/tts-components.json及同名前缀文件。

该测量证明提交序列在该应用上的端到端增益，不能直接量化每条生成指令的执行次数；当前没有无扰动计数器证明TTS实际命中了每一种局部模式。LASX仍未在softfpu2路径下执行。最终严格对齐板卡尚未测，不能据此宣布产品接受。


## 最终严格对齐板卡验证

经xzy86临时登录root@192.168.8.200，在/tmp/codex-no-lbt-validation-20260918运行；未改动板卡现有LAT、配置或用户目录。该板卡为Loongson-2K1001，内核5.10.0.lsgd+，CPU特性仅含lsx和lbt_mips，不含lasx和lbt_x86。候选latx-candidate来自.7验证目录，SHA256 8e0333b8f48dc2b3170f8f4276ff1acbe5fef2ac5ab15b91f5d1c4283e2e711a；guest unaligned-v128 SHA256 71981ec75f3639f5d2adc72667a220749a060140c2b78e7f6c969e398fa190cf。

命令：LATX_SOFTFPU=2 timeout 60 ./latx-candidate ./unaligned-v128。结果打印“LATX: software state mode enabled (LSX=1 LASX=0 LBT_X86=0)”，rc=0。guest覆盖源地址余数0..15乘目标地址余数0..15和跨64KB边界，验证16字节对齐vld/vst、8字节对齐双64位访问、其余地址的逐字节回退均可完成。该项关闭了“最终硬件不支持128位非对齐访问”的关键正确性缺口；仍未在该板卡运行TTS或完整lat-pr-fast，性能结论仍来自.7。
