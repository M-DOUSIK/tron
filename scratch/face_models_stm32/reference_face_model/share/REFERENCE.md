# Grove Vision AI V2 人脸识别 —— 数据参考

本文是配套正文（`ARTICLE.md`）的数据底稿。所有数字取自项目实际文件，每节标注来源路径。正文中引用的任何数字都能在这里找到出处。

## 模型来源与许可

本项目的工程贡献是**把这条流水线在 Ethos-U55 上跑起来**：量化方案、内存布局、导出链路、固件集成。

上游来源：人脸检测用 [InsightFace](https://github.com/deepinsight/insightface) 的 SCRFD；embedding 模型从 InsightFace 的 `glint360k_r100` 蒸馏而来。上游声明其模型仅限非商业研究用途。

仓库许可据此拆分：代码 Apache-2.0，模型权重沿用上游限制，逐文件血缘见 `MODEL_LICENSE.md`。本文档记录的平台侧结论与具体 teacher 无关。

## 本文与正文的范围差异

正文只收录**平台固有、对外部读者有价值**的内容 —— Grove Vision / Ethos-U55 / Vela 工具链的特性。本文保留全部记录，包括正文未采用的部分，供内部查阅。

| 分类 | 内容 | 是否进正文 |
|---|---|---|
| 平台固有 | arena 峰值机制、SRAM 台阶函数、per-tensor 量化、clamp 导出、Vela 参数边界、算子结构与 passes、量化验证工作流、看门狗/arena 重叠/缓存一致性 | ✅ |
| 通用 ML 方法论 | 共享阈值选型、LFW/CFP 跷跷板、ArcFace 小样本、加宽后重训配方 | 部分，压缩为叙述 |
| 本项目自身的实现缺陷 | CFP-FP 评估样本残缺（§5.3）、Glint360K 缓存 OOM（§5.6）、`stream_valid=0/200000`（§5.7）、in-training eval 静默失败（§5.8）、质量分 roll 项、WSL2 环境问题 | ❌ 仅存档 |

第三类是自身配置和代码错误，对读者没有迁移价值，仅在本文保留以备内部复盘。

---

## 0. 读数注意事项

写作和引用前必须知道的三条口径问题，混用会得出错误结论。

### 0.1 精度数字有两套口径

| 时期 | 评估方式 | 典型数字 |
|---|---|---|
| S2 探索期（2026-05-04 ~ 05-05） | `evaluate_embedding_models.py --max-pairs 120`，即 120 同人对 + 60 异人对 | LFW "77.8%"、"81.1%" 这类 |
| 定版期（2026-05-25 之后） | 标准 6000 对 LFW bin / 7000 对 CFP-FP bin | LFW 99.33% / CFP-FP 94.26% |

S2 期的百分比是小样本最优阈值准确率，**只在该批次内横向比较有意义**，不能与标准协议的 99.x% 直接相减。

### 0.2 Vela 的 cycles 与 inference_time 不能跨轮次比

同一架构在不同轮次的 CSV 里 cycles 差近一倍（`w1_pairft_thr_a` 3,565,157 cycles / 7.30 ms vs `w1_pairft_balanced` 6,590,083 cycles / 13.29 ms，两者 SRAM/flash/MMAC 几乎相同）。这是不同 Vela 版本或配置产出的。

**SRAM 和 flash 数值稳定可比，cycles 和 inference_time 只在同一批次内可比。**

### 0.3 CPU 算子数不在 Vela summary schema 里

CSV 没有该字段。CPU ops = 0 的依据是 `HISTORY.md` 的逐条记录，以及对定版模型和 SCRFD 的 vela tflite 做 custom op 扫描（各只有 1 个 `ethos-u` + 1 个 `ethos_u_command_stream`）。

---

## 1. 时间线

来源：`sscma-example-we2` git log（过滤 `model_zoo/tflm_face_embedding` 与 `app/scenario_app/sscma_face`）

| 日期 | 事件 |
|---|---|
| 2025-12-15 | 首个模型转换/量化工具；人脸对齐预处理修正 |
| 2026-02-10 | `sscma_face` scenario app 落地（SCRFD + MobileFaceNet） |
| 2026-05-02 | 引入 InsightFace 官方 MobileFaceNet（w600k）；为塞下 1176 KiB Vela SRAM，EL_ALLOC 从 1112 KB 扩到 1712 KB |
| 2026-05-04 ~ 05-05 | S2 学生蒸馏 + pair finetune + ArcFace + Glint360K，30+ 轮扫参 |
| 2026-05-25 | distill_v2：ResNet100 teacher → MobileFaceNet 512D，4×GPU DDP，4 epoch |
| 2026-05-29 | 拆分 SCRFD/MFN 独立 arena（220 KB + 620 KB）；修 SCRFD 双 sigmoid |
| 2026-07-01 ~ 07-02 | 固件三连坑：watchdog、placement-new、use-after-clobber |
| 2026-07-16 | 定版：QAT distill_v2 ReLU6 128D 上线；5 点最小二乘对齐；quality score 重写 |

---

## 2. 硬件约束

### 2.1 芯片内存地图

来源：`EPII_CM55M_APP_S/app/scenario_app/sscma_face/linker/grove.ld`

| 区域 | 地址范围 | 大小 |
|---|---|---|
| `CM55M_S_APP_ROM` | 0x10000000 – 0x10040000 | 256 KB |
| `CM55M_S_APP_DATA`（堆/栈） | 0x30000000 – 0x30040000 | 256 KB |
| `CM55M_S_SRAM`（系统 / FreeRTOS heap） | 0x34000000 – 0x34054000 | 336 KB |
| **`CM55M_S_EL_ALLOC`（模型 arena 区）** | **0x34054000 – 0x3416A000** | **1112 KB = 0x116000** |
| 摄像头 YUV422 DMA 帧缓冲 | 0x3416A000 起（SRAM1 尾部） | — |

1112 KB 是所有模型 tensor arena 的总预算，且与摄像头 DMA 争地址。

### 2.2 生产配置的预算推导

来源：`sscma_face/common_config.h:96-120`

```
FACE_SRAM_BEFORE_YUV422_BYTES  = 0x116000            = 1,138,688 B = 1112.00 KiB
FACE_FIXED_IMAGE_BUFFER_SIZE   = 160*160*3 + 112*112*3
                               = 76,800 + 37,632     =   114,432 B =  111.75 KiB
FACE_SAFE_RUNTIME_ARENA_BUDGET = 1,138,688 - 114,432 = 1,024,256 B = 1000.25 KiB
FACE_RUNTIME_ARENA_SIZE        = 220 KB + 620 KB     =   860,160 B =  840.00 KiB
headroom                                             =   164,096 B ≈  160.25 KiB
```

`FACE_STOP_STREAM_BEFORE_INFERENCE = (FACE_RUNTIME_ARENA_SIZE > FACE_SAFE_RUNTIME_ARENA_BUDGET)`
840 < 1000.25，不需要停流，摄像头可以持续开启推理。

**headroom 演进**（commit `7238554`，2026-05-29）：

| MFN arena | 合计 | headroom | 后果 |
|---|---|---|---|
| 760 KB | 980 KiB | ≈ 20 KB | 每次推理必须停流 |
| **620 KB** | **840 KiB** | **≈ 160 KB** | 流不用停 |

### 2.3 arena 预算的三次改口

| 时期 | `MOBILEFACENET_ARENA_SIZE` | 触发原因 | 来源 |
|---|---|---|---|
| 初版 | 700 KB | 目标 Vela ≤ 700 KB | `TASK_OFFICIAL_MOBILFACENET.md:119,136` |
| 2026-05-02 | 1300 KB | w600k Vela SRAM = 1176 KiB 塞不下，EL_ALLOC 同步 1304→1712 KB | commit `4d4972a` / `a72c6bb` |
| 2026-05-29 | 760 → **620 KB** | 蒸馏 128D 模型 Vela 599 KiB，620 = 599 + ~3.5% TFLM/对齐余量 | commit `7238554` |

`SCRFD_ARENA_SIZE` 全程 220 KB（Vela 报 200.83 KiB）。

### 2.4 Flash 布局

来源：`sscma_face/common_config.h:28-37`

| 区间 | 内容 | 预留 | 实测文件大小 |
|---|---|---|---|
| 0x000000 – 0x200000 | 固件 | 2 MB | — |
| **0x400000 – 0x4B4000** | SCRFD（ID=1） | 720 KiB | 663,152 B = **647.6 KiB** |
| **0x510000 – 0x64C000** | MobileFaceNet QAT distill_v2 ReLU6 128D（ID=2） | 1264 KiB | 1,044,752 B = **1020.3 KiB** |
| 0x700000 – 0x89B000 | Swift YOLO / 测试输入区 | — | — |

旧 app（`tflm_face_embedding`）：SCRFD @ 0x200000，MFN @ 0x400000。

### 2.5 定版模型的硬件指标

来源：`qat_distill_v2_relu6_128d/model_128d.int8_summary_Ethos_U55_High_End_Embedded.csv`

| 项 | 值 |
|---|---|
| 加速器 | `Ethos_U55_64`，system_config `Ethos_U55_High_End_Embedded`，memory_mode `Shared_Sram` |
| core_clock | 500 MHz |
| SRAM（arena 峰值） | **596.84 KiB**（620 KB 预留的 96.3%，1112 KB EL_ALLOC 的 52.4%） |
| Flash（on-chip） | **1007.86 KiB** |
| 推理时间 | **20.19 ms**（49.52 inferences/s） |
| NPU cycles | 10,046,560 / total 10,095,960（NPU 占 **99.51%**） |
| nn_macs | 221.0 M |
| passes | 129 |
| CPU 算子 | **0** |

SCRFD（`scrfd_500m_kps_int8`）：SRAM **200.83 KiB**、Flash **632.66 KiB**、**3.97 ms**、NPU cycles 1,986,788（100%）、45.9 MMAC、66 passes。

**端到端一帧推理**：3.97 + 20.19 ≈ **24.2 ms**（不含预处理、对齐、后处理）。

---

## 3. Vela 方案对比（95 份 CSV 提炼）

来源：`records/vela_csv/*.csv`

### 3.1 被 SRAM 预算否决的方案

| 方案 | SRAM KiB | Flash KiB | NPU cycles | 推理 ms | MMAC | passes | 否决原因 |
|---|---:|---:|---:|---:|---:|---:|---|
| `sface_int8` | **2352.00** | 8345.11 | 36.87 M | 136.98 | 574.9 | 116 | 超 EL_ALLOC 2 倍，flash 8 MB |
| `mfn128_512d_int16` | **2352.00** | 1190.81 | 19.61 M | 39.58 | 221.2 | 129 | int16 激活让 arena 精确 ×4 |
| `mobilefacenet_qat_int8` | 1176.08 | 2066.34 | 49.63 M | 133.62 | 442.8 | 361 | 逼得 EL_ALLOC 扩到 1712 KB |
| `w600k_dense128` | 1176.09 | 2067.38 | 29.06 M | 92.48 | 439.6 | 293 | 同上 |
| `w600k_projected_128d` | 1176.09 | 3199.31 | 29.22 M | 94.80 | 440.9 | 295 | 投影不省 SRAM 的证据 |
| `mfn_w1.2_probe_128d` | 754.12 | 1609.45 | 9.54 M | 19.24 | 344.2 | 64 | 需放宽预算到 <760 KiB |
| `w600k_stage82_dense128`（LinearAlloc） | **11711.75** | 1968.66 | 22.79 M | 68.77 | 323.7 | 244 | 分配器选错，arena 炸到 11.4 MiB |

### 3.2 落在预算内的候选

| 方案 | SRAM KiB | Flash KiB | NPU cycles | 推理 ms | MMAC | passes |
|---|---:|---:|---:|---:|---:|---:|
| `scrfd_500m_kps_int8`（生产检测） | 200.83 | 632.66 | 1.99 M | 3.97 | 45.9 | 66 |
| `ghostfacenet_fixed_int8` | 245.00 | 716.03 | 5.61 M | 14.79 | 11.4 | 370 |
| `mfn_w0.75_distill_128d` | 449.12 | 646.48 | 4.53 M | 9.15 | 134.4 | 64 |
| **`qat_distill_v2_relu6_128d`（定版）** | **596.84** | **1007.86** | **10.05 M** | **20.19** | **221.0** | **129** |
| `mfn128_relu_int8` | 599.03 | 1009.02 | 6.38 M | 12.85 | 221.0 | 62 |
| `model_distilled_qat_128d.int8`（旧生产） | 599.41 | 1252.03 | 6.41 M | 13.32 | 221.2 | 63 |
| S2 系列 `w1_pairft` / `iddistill_v2_lfw*` | 599.83–599.84 | 1055.91–1057.22 | 6.59 M | 13.29 | 233.8 | 64 |
| `mfn_w1.05_probe_128d` | 599.84 | 1263.44 | 7.14 M | 14.40 | 247.6 | 64 |
| `mfn_w1_distill_256d` | 599.84 | 1127.75 | 6.60 M | 13.43 | 233.9 | 64 |
| `mfn_w1.1_probe_128d` | 702.09 | 1400.61 | 8.68 M | 17.51 | 293.6 | 64 |
| `mfn_w1.15_pairft_128d` | 702.08 | 1394.30 | 8.69 M | 17.51 | 293.9 | 64 |

### 3.3 从表里读出的规律

**SRAM 是台阶函数。** width 1.00 / 1.05 同在 599.8 KiB 档，1.10 / 1.15 同在 702.1 KiB 档，1.20 在 754.1 KiB。通道数向上取整到同一档，加宽 5% 和加宽 15% 的内存开销相同。

**Flash 与 SRAM 解耦。** `w600k_projected_128d` flash 3199 KiB 是 `w600k_tail48_dense128` 的 1.6 倍，两者 SRAM 完全相同（1176.09 KiB）。

**passes 数是算子友好度的代理。** GhostFaceNet 370 passes / 11.4 MMAC 用掉 5.61 M cycles；MobileFaceNet 62 passes / 221 MMAC 用掉 6.38 M cycles。GhostFaceNet 的 MAC 效率约为 MFN 的 1/20，Ghost module 的算子结构不适合 U55。

**两份 CSV 全 0**（编译实质失败）：`mfn128_512d_hybrid`（int8/int16 混合精度）、`mobilefacenet_qat_int8_vela`（对已 Vela 过的模型再跑一次 Vela）。

### 3.4 Vela 参数扫描

同一基准模型 `w600k_stage82_dense128` 上的参数对比：

| 参数 | SRAM KiB | 说明 |
|---|---:|---|
| 默认（`--optimise Performance`） | 882.00 | 基准 |
| `--optimise Size` | **849.62** | 省 32.4 KiB（3.7%），passes 247→244 |
| Size + Greedy 分配器 | 849.62 | 与 Size 相同 |
| Size + HillClimb 分配器 | 849.62 | 与 Size 相同 |
| Size + **LinearAlloc** 分配器 | **11711.75** | 炸到 11.4 MiB，13 倍 |
| `--arena-cache-size 778240 / 786432 / 800000` | 849.62 | 三档均无效，仅 CSV 字段变化 |

编译器旋钮最多省 3.7%。从 1176 KiB 降到 597 KiB 靠的是换架构。

---

## 4. 精度演进

### 4.1 Score 定义

来源：`evaluate_embedding_models.py:243-261`

```python
for threshold in np.linspace(-1, 1, 800):
    accs  = [accuracy_at_threshold(stats[d], threshold) for d in datasets]
    hmean = len(accs) / sum(1/a for a in accs)   # 调和平均，惩罚短板
    gap   = max(accs) - min(accs)
    score = hmean - 0.25 * gap                   # 再罚数据集间差异
# 取全局最优的 (score, floor, macro, -gap)
```

### 4.2 S2 蒸馏路线全轨迹（小样本口径）

来源：`HISTORY.md`（318 行全文）

| # | 方案 | LFW sep / acc | CFP-FP sep / acc | 共享阈值 | Score | 结论 |
|---|---|---|---|---|---|---|
| 基线 | `w600k-512d`（教师） | 0.5560 / 97.8% | 0.1110 / 78.0% | — | — | SRAM 超标，只能当 teacher |
| 0 | 512D→128D 训练投影 | 0.6008 / 97.8% | 0.1593 / 79.7% | — | — | PC 指标好看，不降 SRAM，整轮作废 |
| 1 | `s2-w1-hardneg` | 0.1462 / 73.9% | 0.1104 / 78.0% | — | 0.713 | SRAM 达标，LFW 远逊教师 |
| 2 | `w1_pairft` | 0.3231 / 81.1% | 0.0754 / **74.6%↓** | — | — | 跷跷板首次出现 |
| 3 | `w1_pairft_balanced` | 0.2510 / 81.7% | 0.0813 / 76.3% | 0.1589 | **0.762** | 当时最优 |
| 4 | `distill2`（distill=2.0） | 0.1667 / 77.8% | 0.1038 / 79.7% | — | 0.753 | 提高教师保留度可避免 CFP 回退 |
| 5 | `distill3`（distill=3.0） | 0.1642 / 78.3% | 0.0977 / 81.4% | — | 0.751 | 同上 |
| 6 | `score_a/b/c` | best-thr 82.8% | — | — | 0.720–0.724 | best-threshold 涨、共享阈值 score 跌 |
| 7 | `cfp_score_a` | 82.8% | 78.0% | **0.0138** | 0.719 | 阈值塌到 0.0138，不可部署 |
| — | **启用 center-crop fallback** | — | — | — | — | CFP-FP 从 `44s/15d` → `120s/60d`，全部重算 |
| 8 | w600k（重测） | 81.1%@Thr | 67.8%@Thr | — | 0.705 | gap 13.3% |
| 9 | `balanced`（重测） | 68.9%@Thr | 69.4%@Thr | — | **0.690** | gap 0.6%，比重测前跌 0.072 |
| 10 | `cfp_fb_b` | 0.2621 / 81.7% | 0.0973 / 73.3% | 0.0138 | 0.700 | |
| 11 | `thr_a`（阈值 hinge loss） | 0.2821 / 83.3% | 0.0965 / 72.8% | 0.0463 | 0.714 | |
| 12 | `tpair_hn_a`（教师 pair + 1200 挖掘负） | 0.2066 / 81.1% | 0.0831 / 71.7% | 0.1064 | **0.725** | 长期最优 |
| 13 | `arc_a`（ArcFace w=0.05, m=0.25） | 0.2034 / 80.6% | 0.0735 / 70.6% | — | 0.681 | 1046 类，loss 正常降但指标全跌 |
| 14 | `arc_b`（w=0.10, m=0.35） | 0.2061 / 80.0% | 0.0728 / 72.2% | — | 0.684 | LFW EER 23.3%，CFP EER 40.0% |
| 15 | `glint_arc_c`（10k Glint，1562 类） | 0.2215 / 77.2% | 0.0719 / 70.6% | — | 0.712 | |
| 16 | `glint_arc_stream_a`（50k，8498 类） | 0.2259 / 79.4% | 0.0876 / 72.8% | — | 0.705 | LFW TAR@FAR1 29.2%→51.7% |
| 17 | `glint_arc_bal200k_a` | 0.2064 / 80.6% | 0.0734 / 72.2% | — | 0.699 | TAR@FAR5 52.5%→45.8%（倒退） |
| 18 | `glint_arc_bal200k_b`（修 bug 后） | 0.2016 / 78.9% | 0.0670 / 71.1% | — | 0.701 | |
| 19 | `thr_refine_a` | 0.1706 / 80.6% | 0.0730 / 70.0% | — | 0.700 | 过度正则化，弃用 |
| 20 | `thr_mild_a` | 0.2626 / 83.3% | 0.0919 / 73.9% | — | 0.715 | LFW TAR@FAR5 65.0% |
| 21 | `thr_mild_b` | — | — | — | 0.710 | LFW TAR@FAR5 68.3% |
| 22 | `cfp160_a` | — | — | — | 0.707 | 无改善 |
| 23 | `256d_mild_a` | 0.1343 / 71.7% | 0.0455 / 66.7% | — | **0.668** | 256D 明确劣于 128D，路线终止 |
| 24 | `topk_thr_a` | — | — | — | 0.712 | LFW TAR@FAR1 65.8% |
| 25 | `topk_thr_b` / `topk_tpair_a` | — | CFP 73.3% | — | 0.712 / 0.707 | |
| 26 | `iddistill_v2_lfw` | 66.7% / 50.8% | 67.3% / 41.3% | — | **0.803** | 身份蒸馏把 score 从 0.72 抬到 0.80 |
| 27 | `iddistill_v2_lfw2` | 65.0% / **57.5%** | 65.3% / 36.6% | — | 0.802 | 严格 LFW 最佳 |
| 28 | `iddistill_v2_lfw3`（5000 教师难负） | 65.8% / 45.8% | 63.4% / 34.0% | — | **0.815** | 全程最高，但 LFW FAR1 掉到 45.8% |
| 29 | `iddistill_v2_lfw4`（教师难负→1000） | 62.5% / 40.0% | 68.5% / 40.8% | — | 0.800 | |
| 30 | `iddistill_v2_lfw5`（1500 学生难负） | 61.7% / 55.8% | 66.8% / 38.2% | — | 0.805 | |
| 31 | `iddistill_v2_lfw6`（`--mine-hard-positives 800`） | 62.5% / **58.3%** | 65.4% / **40.7%** | — | 0.806 | 打破 57.5% 天花板，仅多 0.8pt |
| 32 | `iddistill_v2_w115_lfw1`（width 1.15） | 40.0% / **25.8%** | 6.8% / **1.4%** | — | **0.667** | 精度断崖 |

TAR@FAR5 / TAR@FAR1 列格式为 `TAR@FAR5% / TAR@FAR1%`。

**S2 路线合计 30+ 轮，score 在 0.667–0.815 之间横跳，跨度 0.15，无任何一次数量级提升。**

### 4.3 QAT distill_v2 的 float 蒸馏阶段

来源：`records/logs/distill_v2_eval_watcher.log`（4×GPU DDP，5,000,000 张 ms1m-retinaface-t1）

| epoch | LFW acc | LFW xnorm | CFP-FP acc | CFP xnorm | loss（arcface / distill） | 耗时 | 吞吐 |
|---|---|---|---|---|---|---|---|
| 1 | 99.37% (±0.48) | 10.75 | 95.13% (±1.34) | 9.13 | 15.7844（12.3581 / 0.685258） | 1745 s | 716 img/s |
| 2 | 99.43% (±0.29) | 10.98 | 95.39% (±0.85) | 9.38 | 11.3000（8.6192 / 0.536152） | 1769 s | 706 img/s |
| 3 | 99.33% (±0.40) | 10.72 | 95.94% (±0.61) | 9.15 | 9.7926（7.4714 / 0.464234） | 1759 s | 711 img/s |
| **4** | **99.48% (±0.38)** | 10.41 | **96.44% (±1.04)** | 8.93 | 8.5392（6.3985 / 0.428134） | 1729 s | 723 img/s |

验证 λ=5.0：8.5392 ≈ 6.3985 + 5.0 × 0.428134 = 8.5392 ✓

### 4.4 定版 INT8 模型 vs 旧生产模型

来源：`qat_distill_v2_relu6_128d/README.md:36-43`、commit `d9be1d5`

| 指标 | 旧模型 `qat_distilled_128d`（w600k-PCA） | 定版 QAT distill_v2 ReLU6 128D | 变化 |
|---|---|---|---|
| LFW acc | 98.93% | **99.33%** | +0.40 pt |
| CFP-FP acc | 91.54% | **94.26%** | +2.72 pt |
| genuine 相似度均值 | 0.699 | 0.608 | 下降 |
| **impostor 相似度均值** | 0.237 | **0.005** | **↓ ~50×** |
| genuine/impostor 分离度 | 0.46 | **0.60** | +0.14 |
| 端上 office4 陌生人相似度 | 0.25 | **0.045** | ↓ ~5.6× |
| 部署阈值 | 0.4 | **0.30** | |
| Vela SRAM | 599.41 KiB | 596.84 KiB | −2.57 KiB |
| Vela Flash | 1252.03 KiB | 1007.86 KiB | −244 KiB |

**阈值 0.30 的推导**（`qat_distill_v2_relu6_128d/README.md:45-52`、`face_db.py:54-57`）：

```
genuine  p5  = 0.385
impostor p99 = 0.231
→ 0.231 < 0.30 < 0.385，两侧余量 0.069 / 0.085
```

**float → int8 的代价**：LFW 99.48% → 99.33%（−0.15 pt），CFP-FP 96.44% → 94.26%（−2.18 pt）。侧脸掉幅是正脸的 14 倍。

端上 Vela 模型与 PC 前 Vela int8 模型 embedding 余弦一致性 **0.99**。

---

## 5. 坑的原始记录

### 5.1 压缩投影不省 SRAM

来源：`HISTORY.md:143-150`；`records/vela_csv/`

训练命令：`train_w600k_projection_128d.py --num-train 1200 --steps 1000`，1196 个 teacher embedding。产物 `outputs/w600k_projection_128d.npz`，float32 256 KiB / int8 64 KiB。

PC 侧指标：

| 数据集 | 512D 基线 | 128D 投影后 |
|---|---|---|
| LFW | sep 0.5560 / acc 97.8% | sep **0.6008** / acc 97.8% |
| CFP-FP | sep 0.1110 / acc 78.0% | sep **0.1593** / acc 79.7% |

分离度两个数据集都涨了。

原文结论：*"This projection is suitable for 128D matching experiments after w600k inference, but it does not reduce the w600k Ethos-U tensor arena peak SRAM."*

**Vela 侧的证据**：

| CSV | 输出维度 | SRAM KiB | Flash KiB | NPU cycles | MMAC |
|---|---|---:|---:|---:|---:|
| `w600k_dense128` | 128D（直接 dense 头） | **1176.09** | 2067.38 | 29,061,147 | 439.6 |
| `w600k_projected_128d` | 128D（512→128 投影层） | **1176.09** | 3199.31 | 29,223,557 | 440.9 |
| `w600k_tail48_dense128` | 128D（改 tail） | **1176.09** | 1979.39 | 29,036,813 | 439.1 |
| `mobilefacenet_no_bn_int8` | 512D（原始骨干） | **1176.09** | 2076.59 | 29,061,147 | 439.6 |

四者 SRAM 精确到小数点后两位完全相同。Flash 差 1.2 MB，cycles 差 0.6%，arena 峰值一个字节没动。

**根因**：arena 峰值由骨干中间张量的最大值决定。112×112 输入下的早期特征图已把峰值定死，末端 512×128 矩阵乘只有 0.065 MMAC（占总量 0.015%）。

**对照**：S2 学生 `mfn_w0.75_distill_128d` 把 SRAM 干到 **449.12 KiB**（−62%）。

**教训已沉淀进代码注释**：
- `archive/train_w600k_projection_128d.py:6` — *"It does not reduce Ethos-U intermediate tensor SRAM."*
- `archive/evaluate_w600k_compression.py:4` — *"This is a PC-side experiment. It does not reduce Ethos-U SRAM by itself"*
- `archive/export_w600k_projected_128d.py:7` — *"intermediate-tensor SRAM may remain close to the original w600k"*

### 5.2 LFW / CFP-FP 跷跷板

| 轮次 | 动作 | LFW | CFP-FP | 来源 |
|---|---|---|---|---|
| `w1_pairft` | 只用 LFW pair 监督 | sep 0.1462→**0.3231**（+121%），acc 73.9%→81.1% | sep 0.1104→**0.0754**（−32%），acc 78.0%→74.6% | `HISTORY.md:161` |
| `balanced` | 降正样本权重、加负样本权重 | sep 0.2510 / 81.7% | sep 0.0813 / 76.3% | `:162` |
| `distill2/3` | 教师蒸馏权重 2.0 / 3.0 | 回落到 77.8% / 78.3% | 回升到 79.7% / **81.4%** | `:168-169` |
| `score_a/b/c` | 继续加蒸馏追 LFW | best-thr **82.8%** | — | `:178` |
| `cfp_score_a` | 加 CFP 训练对 | 82.8% | 78.0% | `:179` |
| `iddistill_v2_lfw3` → `lfw4` | 教师难负 5000→1000 | FAR1 45.8%→40.0% | 63.4%→**68.5%** | `:266-267` |
| `iddistill_v2_lfw` → `lfw2` | 加强 LFW 难负 | FAR1 50.8%→**57.5%** | FAR1 41.3%→36.6% | `:260-261` |

`score_a/b/c` 是最好的反例：per-dataset best-threshold 下 LFW 从 81.7% 涨到 82.8%，共享阈值 score 从 0.762 跌到 0.720。

`cfp_score_a` 的共享阈值塌到 **0.0138**，贴着 0，genuine/impostor 分布几乎重叠。

原文（`HISTORY.md:139-140, 175`）：*"Use the balanced single-threshold table for deployment candidate selection. Use the per-dataset best-threshold tables only for diagnosis, because they hide threshold-transfer risk."*

### 5.3 CFP-FP 检测失败污染评估

来源：`HISTORY.md:182-184`

原文：*"Enabling cropped-face fallback changed CFP-FP evaluation from partial `44s/15d` to full `120s/60d`; each evaluated model used `72` fallback images and had `0` failures."*

| 项 | 值 |
|---|---|
| 期望 | 120 同人对 + 60 异人对 = 180 对 |
| 实际有效 | **44 同人对 + 15 异人对 = 59 对** |
| 有效率 | **32.8%** |
| 丢失样本性质 | SCRFD 检测失败的大姿态侧脸，即 CFP-FP 存在的意义本身 |

修复前后同一模型：

| 模型 | 修复前 | 修复后 | Score 变化 |
|---|---|---|---|
| `w600k`（教师） | acc 78.0% | CFP@Thr 67.8%，gap 13.3% | 0.705 |
| `balanced` | acc 76.3%，score 0.762 | 68.9% / 69.4%，gap 0.6% | **0.690**（跌 0.072） |

`balanced` 的 score 从 0.762 掉到 0.690，不是模型变差，是之前的分数是虚的。

**同期诊断线索**（`:179`）：*"Current SCRFD alignment also rejects many CFP profile images, so cross-pose training is partly bottlenecked by detection/alignment."* — 偏差同时污染训练与评估两端。

**修复**：`evaluate_embedding_models.py` 与 `train_mfn_student_pair_finetune.py` 加 opt-in center-crop fallback。`compute_embedding.py` 保持固件等价的 SCRFD 对齐不变（它要模拟设备）。

**同类问题再现**（`records/logs/iddistill_v2_lfw7.log`）：

```
Computed 2218 embeddings (failures=146, fallbacks=730)
# LFW
v2lfw6  128 FAILED
v2lfw7  128 FAILED
```

**弯路长度**：从第一个 CFP-FP 数字（`:157`）到修复（`:183`），中间至少 10 轮实验的 CFP-FP 结论建立在 32.8% 有效样本上。

### 5.4 width 1.0 → 1.15 精度断崖

**SRAM 侧探针**（`HISTORY.md:272` + CSV 交叉验证）：

| width | SRAM KiB | Flash KiB | NPU cycles | 推理 ms | MMAC | 原文 |
|---|---:|---:|---:|---:|---:|---|
| 1.00 | 599.84 | 1056.22 | 6,590,051 | 13.29 | 233.8 | 基准 |
| 1.05 | 599.84 | 1263.44 | 7,135,572 | 14.40 | 247.6 | *"stayed at about 599.84 KiB SRAM due to channel rounding"* |
| 1.10 | 702.09 | 1400.61 | 8,684,787 | 17.51 | 293.6 | *"compiled to about 702.09 KiB"* |
| 1.15 | 702.09 | 1410.91 | 8,696,739 | 17.54 | 293.9 | 与 1.10 同档 |
| 1.20 | 754.12 | 1609.45 | 9,538,743 | 19.24 | 344.2 | *"fits only if the budget is relaxed to `<760 KiB`"* |

**精度侧**（`HISTORY.md:273`）：

| 指标 | `v2_lfw6`（width 1.0） | `w115_lfw1`（width 1.15） | 相对变化 |
|---|---|---|---|
| LFW TAR@FAR5 | 62.5% | 40.0% | −36% |
| LFW TAR@FAR1 | **58.3%** | **25.8%** | **−56%** |
| CFP-FP TAR@FAR5 | 65.4% | 6.8% | **−90%** |
| CFP-FP TAR@FAR1 | 40.7% | **1.4%** | **−97%** |
| 共享阈值 Score | 0.806 | 0.667 | 退回全程最低区间 |
| Vela SRAM | 599.83 KiB | 702.08 KiB | +17% |
| Vela Flash | 1056.22 KiB | 1394.48 KiB | +32% |

CFP-FP TAR@FAR1 = 1.4%，等于随机。

**训练配方**（原文 `:273`）：*"trained a wider `width=1.15` student from scratch with 50-epoch projection distillation, then the same LFW hard-positive + Glint identity-distill fine-tune recipe."*

**根因**（原文 `:274`）：*"The `w1.15` run has much higher different-person similarity after distillation/fine-tune, so the next attempt should change the training recipe, not just run `width=1.20` with the same settings."*

反直觉之处：SRAM 有余量（702 < 750），算力有余量（17.5 ms < 20 ms），MAC 多 26%，参数更多，唯一没变的是训练配方。

### 5.5 小样本 ArcFace 帮倒忙

**规模**（`HISTORY.md:205`、`records/logs/logs_s2_w1_pairft_arc_a_e8.log:22-31`）：

```
Kept 5103 aligned images and 3640 valid pairs
ArcFace identities: classes=1046 valid_images=3636/5103 min_images=2
```

1046 类，3636 张参与，平均每类 **3.48 张**，`min_images=2` 意味大量类只有 2 张。

**训练 loss 完美下降**（8 epoch 全量）：

| epoch | total | distill | pos | neg | thr | tpair | arc |
|---|---|---|---|---|---|---|---|
| 1 | 3.57259 | 0.81275 | 0.56709 | 0.11142 | 0.13497 | 0.04662 | 18.54953 |
| 2 | 3.28702 | 0.81772 | 0.55955 | 0.08962 | 0.11103 | 0.04320 | 18.41471 |
| 3 | 3.08096 | 0.82330 | 0.55572 | 0.07361 | 0.09320 | 0.04242 | 18.30935 |
| 4 | 2.91705 | 0.82849 | 0.55493 | 0.06069 | 0.07854 | 0.04226 | 18.21324 |
| 5 | 2.79573 | 0.83281 | 0.55324 | 0.05143 | 0.06785 | 0.04281 | 18.07571 |
| 6 | 2.70200 | 0.83598 | 0.55218 | 0.04426 | 0.05950 | 0.04327 | 17.97033 |
| 7 | 2.62840 | 0.83865 | 0.55274 | 0.03881 | 0.05301 | 0.04359 | 17.81382 |
| 8 | **2.56432** | 0.84090 | 0.55234 | 0.03412 | 0.04744 | 0.04364 | **17.68457** |

total loss 单调降 28%，负样本项降 69%。训练曲线看不出任何问题。

**下游指标全面下滑**（`HISTORY.md:207-210`）：

| 模型 | LFW sep/acc | CFP-FP sep/acc | Score | LFW EER | CFP-FP EER |
|---|---|---|---|---|---|
| `tpair_hn_a`（基线） | 0.2066 / 81.1% | 0.0831 / 71.7% | **0.725** | 22.9% | 35.0% |
| `arc_a` | 0.2034 / 80.6% | 0.0735 / 70.6% | 0.681 | 23.3% | 41.7% |
| `arc_b` | 0.2061 / 80.0% | 0.0728 / 72.2% | 0.684 | 23.3% | 40.0% |

**换 Glint360K 后仍不稳定**：

| 变体 | Glint 图数 | 类数 | 有效图 | Score | 关键变化 |
|---|---|---|---|---|---|
| `glint_arc_c` | 10,000 | 1562 | 4710/15103 | 0.712 | id_arc 18.84→16.91 |
| `glint_arc_stream_a` | 50,000 | 8498 | 17878/50000 | 0.705 | LFW TAR@FAR1 29.2%→51.7%，阈值塌太低 |
| `glint_arc_bal200k_a` | 200,000 | — | — | 0.699 | TAR@FAR5 52.5%→45.8%（倒退） |
| `glint_arc_bal200k_b` | 200,000 | — | 186456/200000 | 0.701 | TAR@FAR5 52.5%→48.3%（倒退） |

没有一个变体超过 `tpair_hn_a` 的 0.725。类数从 1046 扩到 8498 都没解决。

**原文结论**（`:227`）：*"simply adding more ArcFace identity data can push the distribution in the wrong direction; continue from this code path by lowering ArcFace pressure and adding an explicit FAR/hard-negative penalty, not by reverting to larger in-memory caches."*

**对照组**：定版 distill_v2 用同样的 ArcFace（`--arcface-s 64.0 --arcface-m 0.5`），数据是 ms1m-retinaface-t1 的 **93431 类 / 500 万张**，平均每类 53.5 张。类数差 89 倍、每类图数差 15 倍，结果完全相反。

### 5.6 Glint360K 2.0 GiB 缓存 OOM

来源：`HISTORY.md:213-214`

下载命令：

```bash
uv run python download_glint360k_subset.py --output-dir datasets/glint360k_subset_112 \
  --start-shard 0 --num-shards 8 --max-images 50000 --max-images-per-id 20
```

产出 50000 张对齐图，覆盖 39574 个身份。

原文：*"The first 50k-image training attempt generated a `2.0 GiB` aligned cache and `108 MiB` teacher cache, then exited around GPU initialization."*

**现象特征**：不是训练中途 OOM，是缓存全部生成完毕后、在 GPU 初始化那一刻死。

**根因**：所有身份图片塞进内存常量张量 `x_tf` 全量驻留。修复描述反证了这点（`:216`）：*"This run validates that 50k identity data no longer enters the large `x_tf` image constant."*

**修复**（`:214`）：*"keeps only pair-loss images in the aligned/teacher caches and streams external identity images from file paths for ArcFace-only steps."*

**修复后**（`:216`）：

```
5103   张缓存的 pair 图（必须常驻）
50000  条流式 Glint 身份路径（不再进内存）
8498   个 ArcFace 类
17878/50000 张符合 min_images=2 的流式样本
loss 3.70730 → 2.36125；id_arc 20.53532 → 17.39129
```

内存常量从 50000 张降到 5103 张（−90%）。

规模再上一级（`:217`）：64 shard、`min_images_per_id=4`、`max_images_per_id=16`，训练+导出 729 s 完成。

同期环境坑：*"WSL2 needed `HTTP_PROXY/HTTPS_PROXY` after starting Clash on Windows."*

### 5.7 `stream_valid=0/200000` 静默空转

来源：`HISTORY.md:218`

原文：*"conservative 200k attempt after fixing Glint identity parsing for directories named `glint360k_*`. Before the fix, balanced external images had `stream_valid=0/200000`; after the fix, the same dataset had `stream_valid=186456/200000`."*

**现象**：训练正常启动、跑完、导出，Vela 正常编译（599.84 KiB SRAM、1056.23 KiB flash、CPU ops=0、NPU 100%）。无报错、无异常日志。20 万张身份图，实际参与训练 0 张。729 s 完成，产出一个"能用"的模型。

**根因**：身份目录命名格式 `glint360k_*` 未被解析逻辑匹配，全部样本在 `stream_valid` 检查里被跳过。

**有效率**：0.00% → 93.2%。

**指标对比**：

| 版本 | Score |
|---|---|
| `bal200k_a`（bug 版，0 张有效） | 0.699 |
| `bal200k_b`（修复版，186456 张有效） | 0.701 |

**差 0.002，在噪声范围内。** 光看指标发现不了 20 万张数据完全没进模型。唯一发现途径是主动打印 `stream_valid=N/M`。

### 5.8 distill_v2 训练全程 in-training eval 静默失败

来源：`records/logs/distill_v2_distill_ddp.log`

```
19820:  [eval e1 lfw]    FAIL: Input type (torch.FloatTensor) and weight type (torch.cuda.FloatTensor) should be the same
19822:  [eval e1 cfp_fp] FAIL: ...
39505:  [eval e2 lfw]    FAIL: ...
59190:  [eval e3 lfw]    FAIL: ...
78875:  [eval e4 lfw]    FAIL: ...
```

4 epoch × 2 数据集 = **8 次评估全部失败**（eval 时 tensor 没搬到 GPU）。训练本身正常（每 epoch ~1750 s，~715 img/s），整个 7002 秒训练没有任何验证信号。

同一 log 结尾第二个 bug：

```
UnboundLocalError: local variable 'final_path' referenced before assignment
torch.distributed.elastic.multiprocessing.errors.ChildFailedError
```

训练完成、ONNX 导出成功（`model_distill.onnx` 4.5 MB）之后，在打印"下一步命令"那行崩了，torchrun 报 exitcode 1。**训练成功但退出码是失败。**

**绕过方式**：独立 watcher 进程离线评估每个 epoch 的 checkpoint，产出 §4.3 那张表。

### 5.9 固件侧的坑

来源：commit 消息全文

| 坑 | 现象 | 根因 | 数字 |
|---|---|---|---|
| **PC/设备 8% 余弦失配**（`4d4972a`） | PC 仿真与设备 embedding 对不上 | int8 输入换算用 `pixel-128`，PC 用 `round(pixel-128.5)` | 改为 `pixel>128 ? pixel-128 : pixel-129`，消掉 ~8% 失配 |
| **SCRFD 双 sigmoid**（`314f6b6`） | 端上置信度普遍偏低 | 模型输出已是 [0,1] 概率（score 张量 zp=−128, scale=1/256），代码又套一层 sigmoid | 真实 **0.87 被压成 ~0.70** |
| **看门狗复位循环**（`03c8adc`） | 进人脸模式就复位，ESP32 侧 invoke 263 | MobileFaceNet `AllocateTensors` 冷加载阻塞 CPU **>6 s**，硬件看门狗 6 s 超时 | 两次 AllocateTensors 前后关/开 WDT |
| **共享 arena 被 YOLO 清掉**（`d8c58b2` + `7cc1637`） | mode2→mode1→mode2 后野指针 HardFault | 人脸 interpreter 是函数局部 static，arena 与 YOLO 的 elHeap bump 区物理重叠，YOLO 的 `set_model` memset 覆盖了人脸 arena | 改用 placement-new 重建；第二次修：连旧对象析构都不能调（析构会读被覆盖的 `subgraph_allocations`，触发 wild `registration->free()`） |
| **质量分永远 ~0.07**（`dd3a7df`） | `MIN_FACE_QUALITY`(0.3) 门形同虚设 | `estimate_face_quality` 把 roll 项乘进分数，而 `compute_face_alignment` 正好把 roll 转正了 — 惩罚了下一级要修正的东西，再叠两个苛刻乘性项 | 重写为 yaw 主导（鼻偏移为主 + 嘴偏移 1/4 权重）：同样四帧办公室图 **~0.07 → 0.87–0.99** |
| **两点对齐不够**（`dba8a8d`） | 裁剪歪斜/偏心 | `compute_face_alignment` 只用双眼解相似变换，两点只约束 roll，任何 yaw 或单个噪声 landmark 都会歪 | 改 5 点闭式最小二乘（Umeyama，免 SVD），与 skimage `SimilarityTransform` 对齐到 **1e-14** |

**Ethos-U 缓存一致性守则**（`common_config.h:126-144`）：

> *"The production default cleans the CPU-written input and invalidates the NPU-written output only. Do not invalidate the full tensor arena before Invoke(): TFLM may have prepared dirty CPU-side arena state that the NPU still needs to read."*

---

## 6. 定版方案技术细节

### 6.1 流水线

来源：`qat_pipeline/README.md`

```
distill_mfn.py            ResNet100 teacher → MobileFaceNet 512D student (ArcFace + MSE distill)
      │                   → distill_v2 float 权重
      ▼
train_128d.py             512D student → 128D 头（可训练 Linear，参与 QAT）
      │
      ▼
qat_finetune.py           QAT 精调（自蒸馏，每 block 后插 per-tensor int8 FakeQuantize）
qat_finetune_if.py        *_if = spark 上的 ImageFolder 变体
      │                   LeakyReLU → ReLU6 的替换在这里
      ▼
export_qat_128d_clamp.py  QAT .pt → torch.clamp 硬钳到 FQ 学到的范围 → ONNX
      ▼
quantize_and_vela.py      ONNX --onnx2tf--> TF SavedModel --tf.lite INT8--> vela (ethos-u55-64)
                          → model_128d.int8_vela.tflite
```

运行环境：spark（GB10），`uv` + torch/cu130。

### 6.2 PTQ 崩塌与 ReLU6

**原始问题**（`qat_pipeline/qat_finetune.py:3-11` 文件头）：

> *"Problem: FP32 model achieves 83.5% LFW but collapse to 57% with per-tensor INT8.
> Root cause: LeakyReLU activations have unbounded positive range; per-tensor quantization scale is dominated by outliers, crushing most signal."*

83.5% → 57%，掉 26.5 个点。

**为什么 U55 上特别严重**：Ethos-U55 的激活量化是 per-tensor（不是 per-channel）。一个张量只有一个 scale/zero_point，`scale = (max-min)/255`。LeakyReLU 正半轴无上界，某几个通道有大 outlier 就把 scale 拉大，其余通道信号被压到几个量化格里。

**ReLU6 的三重收益**：

1. **有界**：输出恒在 [0,6]，per-tensor scale 天然是 6/255，与 outlier 无关
2. **可融合进 conv**：作为 fused activation 直接进 conv 算子，NPU 算子数更少
3. **让 TFLite 代表数据集的 min/max 收敛到 QAT 学到的紧范围**

`mfn_cfg.py:20-27` 把激活做成三选一（`leaky` / `relu6` / `relu`），`train_128d.py --act relu6` 触发替换。

### 6.3 QAT 配置与 clamp 导出

**QAT 侧**（`qat_finetune_if.py:2-16`）：

- FakeQuantize 插在每个 ConvBlock/LinearBlock 输出后，`per-tensor affine, int8`，*"matching Ethos-U55 per-tensor activation quantization"*
- **自蒸馏**：teacher 是同一个 distilled student 的 FP32 冻结副本
- **loss = cosine-embedding (1−cos) + 0.1 × MSE**，理由：*"Cosine matches face-verification geometry better than raw MSE"*
- BN 冻结
- 先 calibrate observer（25 batch）→ 冻结 range → 训练
- 每 epoch 报 cosine fidelity（float teacher vs int8-simulated student，40 batch）— *"the metric that predicts int8 tflite quality"*

**导出侧**（`export_qat_128d_clamp.py:3-10`，原文）：

> *"Mechanism: the PTQ collapse came from TFLite recomputing per-tensor activation ranges via min/max over the outlier-heavy LeakyReLU outputs (huge range → signal crushed). Here we replace each trained FakeQuantize with `torch.clamp` to that fake-quant's exact learned [lo,hi] range. The clean model's activations are then bounded, so TFLite's representative-dataset min/max == the tight QAT range → tight scale → the QAT-adapted weights transfer and no collapse."*

即：光做 QAT 不够。导出成 ONNX 再走标准 onnx2tf + TFLite PTQ 时，TFLite 会用代表数据集重算 min/max，把 QAT 学到的紧范围丢掉。解法是把每个 FakeQuantize 换成硬 `torch.clamp(lo, hi)`，让重算的 min/max 必然等于 QAT 范围。

备选路径 `export_qat_128d_fq.py`（QDQ 直出）标为 alternate，clamp 路径标为 recommended。

### 6.4 128D 头不是 PCA

**旧做法**：`records/qat_v1_artifacts/pca_128.npz`（264,706 B），事后 PCA 降维。

**新做法**（`train_128d.py:3-13`）：

> *"the 512→128 projection is a trainable Linear that participates in QAT (not post-hoc PCA), AND its 128D output is FakeQuantized → the projection is quantization-aware. Warm-started from a PCA composition of the 512D model's final Linear+BN so we begin at PCA-float quality."*
>
> *"Distillation: teacher = float distill_v2 512D (LeakyReLU); target = L2-normalized PCA-128 projection of the teacher. Loss = (1−cos) + w·mse."*

`mfn_cfg.py:9-12`：*"an FQ (`embq`) after the final embedding so the 128D projection output is quantization-aware — this is what the old post-hoc PCA-128 lacked."*

### 6.5 训练超参

**Teacher**：InsightFace glint360k_r100（ResNet100，49,029,632 参数），ONNX + CUDAExecutionProvider 推理
**Student**：MobileFaceNet `scale=1, blocks=(1,4,6,2)`，**1,192,960 参数**，512D
**参数比**：41 : 1

**(a) 蒸馏阶段 `distill_mfn.py`**

| 超参 | 值 |
|---|---|
| epochs | 4 |
| lr | 0.01 |
| lambda-distill | **5.0** |
| batch-size | 256 / GPU |
| weight-decay | 1e-4 |
| ArcFace scale (s) | **64.0** |
| ArcFace margin (m) | **0.5** |
| num-classes | **93431** |
| fp16 (AMP) | True |
| num-workers | 4 |
| 数据 | ms1m-retinaface-t1，5,000,000 样本 |
| 并行 | 4×GPU DDP（torchrun） |
| 初始权重 | `model_512d_lrelu.pt`（missing=0, unexpected=0） |
| 吞吐 | 706–723 img/s，每 epoch 1729–1769 s |
| 总时长 | ≈ 7002 s ≈ **1.95 小时** |

损失：`loss = arcface + 5.0 × distill`

**(b) 128D 头 `train_128d.py`**

| 超参 | 默认值 |
|---|---|
| epochs | 30 |
| lr | 1e-4 |
| batch-size | 128 |
| num-workers | 8 |
| mse-w | 0.1 |
| act | `relu6`（需显式传 `--act relu6`，默认 `leaky`） |

**(c) QAT 精调 `qat_finetune_if.py`**

| 超参 | 默认值 |
|---|---|
| epochs | 8 |
| lr | 1e-4 |
| batch-size | 128 |
| num-workers | 8 |
| mse-w | 0.1 |
| observer 校准 | 25 batch，之后冻结 range |
| cosine fidelity 评估 | 每 epoch，40 batch |
| 数据 | ImageFolder 对齐 112×112 人脸 JPG |
| 归一化 | `arr/127.5 − 1.0` → [−1,1] |

（老版 `qat_finetune.py` 默认 epochs=2, lr=0.001，是 vast.ai 时期参数）

**(d) INT8 量化 `quantize_and_vela.py`**

| 项 | 值 |
|---|---|
| ONNX→TF | `onnx2tf.convert(copy_onnx_input_output_names_to_tflite=True, non_verbose=True)` |
| 校准集 | `calibration_data/qat_112/`，默认 500 张（该目录共 13233 张对齐人脸） |
| 校准预处理 | `(arr/127.5) − 1.0`，非 112×112 时 BILINEAR resize |
| optimizations | `[tf.lite.Optimize.DEFAULT]` |
| supported_ops | `[tf.lite.OpsSet.TFLITE_BUILTINS_INT8]` |
| input/output type | `tf.int8` / `tf.int8`（全整数，端上不需 dequant 层） |

### 6.6 Vela 编译命令

来源：`qat_pipeline/quantize_and_vela.py:88-94`（唯一生产入口）

```bash
vela <model>.int8.tflite \
  --accelerator-config ethos-u55-64 \
  --optimise Performance \
  --output-dir <out>
```

只有三个参数。未使用 `--separate-io-regions`（全仓库无此参数）。未指定 `--system-config` / `--memory-mode`，走默认 → `Ethos_U55_High_End_Embedded` + `Shared_Sram`，`arena_cache_size = 4,194,304 B`，`core_clock = 500 MHz`。

### 6.7 定版模型 I/O 契约

来源：`qat_distill_v2_relu6_128d/README.md:17-24`、`sscma_face/common_config.h:60-77`

| 项 | 值 |
|---|---|
| 输入 | `[1,112,112,3]` RGB int8，quant `scale=0.007843137, zp=-1` |
| 输入兼容性 | 与旧模型完全一致，固件 `pixel-129` 预处理不用改，drop-in |
| 输出 | `[1,128]` int8，quant `scale=0.012660, zp=18` |
| 输出兼容性 | scale 变了，但固件从 tensor 读 scale/zp 自动反量化，无需改代码 |
| 归一化 | [−1,1] |
| 输出后处理 | L2 normalize |

---

## 7. 评估方法论的实现细节

### 7.1 生产可行性数据

来源：`HISTORY.md:251-256`

| 模型 | LFW EER | LFW TAR@FAR5 | CFP EER | CFP TAR@FAR5 | 共享阈值下 LFW FAR | CFP FAR |
|---|---|---|---|---|---|---|
| `w600k`（教师） | 3.3% | 96.7% | 36.7% | 26.7% | **56.7%** | **63.3%** |
| `thr_a` | 20.0% | 67.5% | 38.3% | 24.2% | — | — |
| `tpair_hn_a` | 22.9% | 52.5% | 35.0% | 12.5% | **75.0%** | **55.0%** |

原文：*"At the balanced shared threshold, false-accept rates are high for all tested models... balanced accuracy can improve while false-accept risk remains too high."*

score 0.725 的"最优候选"，实际误接受率 75%。这是 S2 路线被判不可生产的直接依据。

### 7.2 评估位置也会骗人

`HISTORY.md:234`：*"WSL2 is appropriate for training and fast smoke checks. Final model selection should currently use the local evaluation data because the WSL2 LFW copy has previously shown missing/mismatched pairs; sync the full local LFW/CFP evaluation tree before trusting WSL2 final metrics."*

同一模型在两台机器上给出不同分数，因为数据集副本不一致。

### 7.3 Vela 模型的验证工作流

Vela 编译后的模型带 `ethos-u` custom op，PC TFLite 跑不了（`qat_pipeline/README.md:56-58`）。固定工作流：

- **精度** → 看 pre-Vela int8 模型（PC 上跑）
- **内存 / NPU 覆盖率** → 看 Vela summary CSV
- **两者一致性** → 端上 embedding 与 PC embedding 的余弦相似度（要求 ≥ 0.99）

`_device_compare.py` 负责这步，模拟固件侧预处理（含 `pixel-129` int8 路径）。

### 7.4 量化诊断工具

**逐层敏感度分析**（`analyze_quantization_sensitivity.py`，四种方法）：

1. 逐层激活统计（mean / std / min / max）— 找 range 最大的层，那是 per-tensor scale 的杀手
2. Float32 vs QAT 模型逐层输出对比
3. TFLite tensor 元数据分析（scale / zero_point / range）
4. 累积误差传播分析

`analyze_tflite_layers.py:355-378` 输出的建议模板：

```
2. MOST PROBLEMATIC LAYERS (by activation range):   ← 按激活 range 排序取 top5
3. QUANTIZATION PRECISION — Current Float32→INT8 similarity: {mean_cosine_sim}
   a) Fine-tune QAT with more epochs and larger dataset
   b) Use mixed-precision: keep problematic layers in float16
   c) Try per-tensor vs per-channel quantization
   d) Increase calibration data diversity
4. ARCHITECTURE MODIFICATIONS
   - Replace PReLU with ReLU6 (bounded activation)     ← 最终采纳的正是这条
   - Add skip connections
   - Use depth-separable convolutions with BN after each
```

该建议 2026-01 就写在脚本里，2026-07 才落地成产品模型。

**从 tensor 元数据读出死分支**（`SCRFD_INT8_EVALUATION_REPORT.md:39-45`）：

QAT v5 的 stride-32 score 分支输出 max score 恒为 `0.000`，查元数据：

```
shape=[50,1], dtype=int8, quant=(7.84313680668447e-09, 0)
```

scale = 7.8e-09，int8 全域 ±128 对应实际范围 ±1e-6，该分支被彻底压扁。人脸裁剪测试集看不出来（stride 16 承担 87% 检出），但大脸/低分辨率场景会挂。

原文（`:79`）：*"Do not ship the current QAT v5 models... The stride-32 score branch is collapsed and should be fixed before deployment."*

**float vs int8 相关性作为质量门**（`SCRFD_INT8_EVALUATION_REPORT.md:51-60`，200 张采样）：

| 配对 | Scores s8 | Scores s16 | BBox range | KPS range |
|---|---|---|---|---|
| QAT v5 nosigmoid float vs int8 | 0.9958 | 0.9990 | 0.9968–0.99997 | 0.9961–0.99975 |
| QAT v5 sigmoid float vs int8 | 0.9972 | 0.9993 | 0.9982–0.99996 | 0.9981–0.99977 |

`TASK_OFFICIAL_MOBILFACENET.md:98` 的硬门槛：`mean cosine similarity(float32, int8) ≥ 0.98` 才算 PASS；判别力门槛：同人 ≥0.6、异人 ≤0.3、分离度 ≥0.3。

**按 stride 分别判读**（`SCRFD_INT8_EVALUATION_REPORT.md:29-37`）：

| 模型 | s8 | s16 | s32 |
|---|---:|---:|---:|
| PTQ INT8 生产 | 0.141 | **0.870** | 0.039 |

原文：*"A global log line such as `max_score: s8=... s16=... s32=...` should be judged primarily by the best stride, not by all strides being high."*

**解码规则的 PC/固件一致性**（`SCRFD_INT8_EVALUATION_REPORT.md:47-49`）：

`analyze_scrfd_quantization.py` 用 `(x + 0.5) * stride` 解 bbox，而 `SCRFD_DECODING.md` / `compute_embedding.py` / 固件 `scrfd_postprocessing.cc` 用 anchor 角点 `x * stride`。PC 分析工具与设备行为不一致。

`SCRFD_DECODING.md:222-234` 的 "Common Mistakes"：box 和 keypoints 必须用同一参考点（anchor 角点，无 +0.5 偏移）；anchor 迭代顺序是 `for y: for x: [anchor1, anchor2]`。

---

## 8. 数据管线自检清单

从 §5.3 / §5.6 / §5.7 / §5.8 四个真实事故反推：

| 检查项 | 对应事故 | 做法 |
|---|---|---|
| 打印并断言有效样本数 / 期望样本数 | `stream_valid=0/200000` | 启动时打印 `valid=N/M`，`assert N > 0`，并断言占比下限 |
| 打印 fallback 数和 failure 数 | CFP-FP 44s/15d | `Computed 2218 embeddings (failures=146, fallbacks=730)` 是正确示范 |
| 样本数不满不出结论 | CFP-FP 44s/15d | 样本不足时输出 `FAILED`，不给基于残缺数据的分数 |
| 数据规模上量级前审 dataloader 驻留策略 | 2.0 GiB OOM | 区分必须常驻的（pair loss 图 5103 张）与可流式的（ArcFace 身份图 50000 条路径） |
| 辅助 loss 看下游指标，不看训练曲线 | ArcFace 1046 类 | loss 降 28%，下游 score 从 0.725 掉到 0.681 |
| 验证 in-training eval 真的在跑 | distill_v2 8 次 eval 全 FAIL | 不可靠时用独立 watcher 进程离线评估每个 epoch checkpoint |
| 注意"训练成功但退出码失败" | `UnboundLocalError: final_path` | 导出后一行 print 崩溃导致 torchrun exitcode 1，自动化流水线会误判 |

---

## 9. 明确"未找到记录"的项

1. **蒸馏温度（temperature）**：`distill_mfn.py` 无此参数。该项目蒸馏是 embedding 层的 cosine/MSE 回归，不是 logits 软标签蒸馏，不涉及温度。
2. **`--separate-io-regions`**：全仓库未出现。生产 Vela 命令只有三个参数。
3. **Vela summary CSV 的 CPU 算子数**：字段不在 schema 里，见 §0.3。
4. **SCRFD 的 WIDER FACE mAP**：`SCRFD_INT8_EVALUATION_REPORT.md:89-93` 明确未做，只有 1000 张裁剪图上的检出率代理指标。
5. **端上端到端延迟**（含预处理/对齐/后处理/UART）：只有 Vela 报的纯推理时间（3.97 + 20.19 ms），完整 pipeline 耗时未找到记录。
6. **PTQ 崩塌（83.5%→57%）的具体模型来源**：`qat_finetune.py` 文件头记录是 512D LeakyReLU MobileFaceNet，未标注是哪一轮 checkpoint。
7. **office4 数据集细节**：`office4_eval.py` 显示是 4 人 6 个 impostor 对，`firmware_4dof` 对齐，图片路径 `<face-eval-root>/office4/crops` 不在本仓库内。

---

## 10. 来源索引

| 数据类别 | 文件 |
|---|---|
| 技术路线全史（S2 30+ 轮） | `HISTORY.md`（318 行） |
| 已提炼教训 | `LESSONS.md` |
| 定版模型规格与精度 | `README.md`、`qat_distill_v2_relu6_128d/README.md` |
| 95 份 Vela 性能数据 | `records/vela_csv/*.csv` + 定版模型 summary CSV |
| distill_v2 训练日志 | `records/logs/distill_v2_distill_ddp.log`（15 MB） |
| distill_v2 逐 epoch 评估 | `records/logs/distill_v2_eval_watcher.log` |
| ArcFace 实验日志 | `records/logs/logs_s2_w1_pairft_arc_a_e8.log`、`logs_s2_w1_pairft_glint_arc_c.log` |
| 评估失败样例 | `records/logs/iddistill_v2_lfw7.log` |
| 旧模型（v1）产物 | `records/qat_v1_artifacts/`（含 `pca_128.npz`） |
| 生产训练流程 | `qat_pipeline/` |
| SCRFD 解码与量化评估 | `SCRFD_DECODING.md`、`SCRFD_INT8_EVALUATION_REPORT.md` |
| 迁移任务书与验收标准 | `TASK_OFFICIAL_MOBILFACENET.md` |
| 量化诊断工具 | `analyze_quantization_sensitivity.py`、`analyze_tflite_layers.py`、`analyze_scrfd_quantization.py` |
| 评估方法学实现 | `evaluate_embedding_models.py`、`_device_compare.py`、`compute_embedding.py`、`face_db.py` |
| 固件内存/flash 约束 | `sscma_face/common_config.h`、`linker/grove.ld`、`linker/watcher.ld` |
| 固件侧踩坑 | git commit `d9be1d5` `dba8a8d` `dd3a7df` `7cc1637` `d8c58b2` `03c8adc` `314f6b6` `7238554` `4d4972a` `a72c6bb` |
