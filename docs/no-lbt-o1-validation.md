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
