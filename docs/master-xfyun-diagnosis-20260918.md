# master 在 `.7` 运行讯飞 TTS 的正确性诊断（2026-09-18）

## 范围和身份

本记录只验证 master 预构建包在 LoongArch 测试机 `.7` 的实际行为，没有修改 master 源码或测试输入。

- master 源码：`/home/xzy/work/lat-master-20260917`，分支 `master`，提交 `97429a33e8`。
- 被测二进制：`/home/loongson/xzy/xfyun-master-97429a33-20260917/latx-master`，静态 LoongArch ELF，SHA-256 为 `891ae44cf14f3604d540ff18b4c9d6647881c5ce54e985547a5eb07f88ed7639`。包内 `identity.txt` 也标记为 `lat-x86_64 97429a3`。
- 被测 x86-64 程序：`$base/default/work/tts`；使用真实 `libmsc.so`、`msc/res/tts/*.jet` 与绝对输出 WAV 路径。
- 测试机：`loongson@192.168.8.7`。原始输出和 GDB 日志位于 `$base/validation-codex-20260918/`；其中 `$base` 为 `/home/loongson/xzy/xfyun-master-97429a33-20260917`。

执行命令的核心形式如下，避免依赖 binfmt 选择到其他 LAT 包：

```sh
cd "$base/default/work"
LATX_SOFTFPU=2 LD_LIBRARY_PATH=:$PWD \
  timeout 1800 "$base/latx-master" -L "$runtime" ./tts \
  'appid=5ce519e0,work_dir =.' "$absolute_wav" '需要帮助，请拨打12122' \
  'engine_type = local, voice_name = xiaoyan, text_encoding = UTF8, tts_res_path = fo|res/tts/xiaoyan.jet;fo|res/tts/common.jet, sample_rate = 16000, speed = 30, volume = 100, pitch = 50, rdn = 2'
```

## 已复现的结果

`latx-master` 加默认 glibc 2.17 guest runtime 在约 0.35 秒收到宿主 `SIGSEGV`（shell 返回码 139；Python 记录为 `-11`），没有 stdout/stderr，也没有生成 WAV。此前包内 `default/timing-3x.txt` 的三轮也都是 139；本次独立目录再次复现，故不是旧输出残留。

GDB 对这次独立复现的停止位置为：

```text
Program received signal SIGSEGV
0x000000ffd706f5a0 in code_gen_buffer ()
#0 code_gen_buffer ()
#1 0x190 in ?? ()
Badvaddr = 0x0
```

这说明故障发生于执行翻译出的 LoongArch 代码缓冲区，而不是 TTS 自身报告错误；它尚不能单独确定是哪一个 guest 基本块或哪条翻译指令造成错误。

## runtime 与 softfpu 对照

以下全部使用同一个 `latx-master`、同一个 TTS 输入和独立 WAV 输出位置：

| 编号 | guest runtime | `LATX_SOFTFPU` | 结果 |
| --- | --- | --- | --- |
| 1 | master 包内 glibc 2.17 | 2 | `SIGSEGV`（`-11`），约 0.29 秒 |
| 2 | no-LBT 包内 guest runtime | 2 | `SIGBUS`（`-7`），约 0.27 秒 |
| 3 | 另一 master glibc 2.28 runtime | 2 | `SIGSEGV`（`-11`），约 0.29 秒 |
| 4 | master 包内 glibc 2.17 | 0 | `SIGSEGV`（`-11`），约 0.01 秒 |
| 5 | master 包内 glibc 2.17 | 1 | `SIGSEGV`（`-11`），约 0.29 秒 |
| 6 | master 包内 glibc 2.17 | 2 | `SIGSEGV`（`-11`），约 0.28 秒 |

矩阵原始数据：`$base/validation-codex-20260918/master-runtime-softfpu-matrix.json`。

结论：更换 glibc 2.17 与 glibc 2.28 runtime 都不能避免 master 的 `SIGSEGV`，所以“仅仅是 master guest runtime 不匹配”不能解释该故障。no-LBT runtime 得到不同的 `SIGBUS`，表明它的装载器/库布局确实不同，但不能据此把 master 的崩溃归因于 runtime。

## 独立正确性对照

使用 `/home/loongson/xzy/lat-xfyun-board-env-20260917/xfyun-debug/bin/qemu-x86_64`（QEMU 7.2.22，SHA-256 `7585cb475f755f33394617f5a5150264abc1648a0103a9d796f1763b3af5d1f8`）搭配**同一 master glibc 2.17 runtime**运行相同 TTS 命令，返回 0，并生成 139170 字节的有效 16 kHz 单声道 PCM WAV，SHA-256 `e0f9aeafc08a9eece2cf1373eecea7102d2e7c0f219ba3c20845b7f374036afb`。

因此已证明 TTS 程序、资源、参数与 master 包内 glibc 2.17 runtime 在 x86-64 用户态语义下可以正常完成。它不能证明 LAT 的翻译结果正确。

## 当前判断与下一步

目前证据把问题限制在 master 的翻译器/生成代码执行路径，或该路径与 `.7` 的宿主状态交互；尚未定位到具体 guest PC、x86 指令或 LoongArch 指令。下一步应在不向实际 LAT 运行加入探针的前提下，用 GDB 记录崩溃前最后分派的 `TranslationBlock`（guest PC、生成代码地址和长度），再从该 guest PC 反查装载对象、指令与对应的翻译路径。随后才考虑最小化复现或临时插桩；插桩程序必须隔离，不能用于宣称 LAT 实际结果。
