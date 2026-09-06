# grove-vision-face-embedding

[English](README.md) · [简体中文](README.zh-CN.md)

**Grove Vision AI Module V2**（Himax WiseEye2 HX6538，Cortex-M55 + Ethos-U55 NPU）上的端侧人脸识别方案。SCRFD 人脸检测 + QAT 蒸馏 ReLU6 MobileFaceNet 128D 特征提取，两个模型**全部跑在 NPU 上，CPU 算子为 0**。

本仓库包含可直接烧录的 INT8 模型、产出这些模型的训练与量化流水线，以及支撑各项设计决策的完整实验记录。

> ### ⚠️ 许可：代码与模型权重不同
>
> **代码、脚本与文档** —— Apache-2.0（[`LICENSE`](LICENSE)）。
>
> **模型权重文件** —— **仅限非商业研究用途**。仓库内所有权重都源自
> [InsightFace](https://github.com/deepinsight/insightface) 的资产（SCRFD 检测模型是
> 格式转换与量化；embedding 模型是从 `glint360k_r100` 蒸馏、用 `ms1m-retinaface-t1`
> 训练而来），而 InsightFace 声明
> *"ALL models are available for non-commercial research purposes only."*
>
> [`qat_pipeline/`](qat_pipeline/) 与 teacher、数据集无关 —— 换成许可宽松的输入重跑，
> 即可得到不带此限制的权重。逐文件的血缘说明与替换方法见
> **[`MODEL_LICENSE.md`](MODEL_LICENSE.md)**。

## 关键指标

| 指标 | 数值 |
|---|---|
| 整机功耗 | **0.35 W** |
| LFW 准确率（INT8） | **99.33%** |
| CFP-FP 准确率（INT8） | **94.26%** |
| Tensor arena（SRAM） | 预留 **840 KiB**（220 KB + 620 KB） |
| 单帧纯 NPU 推理 | **24.2 ms**（SCRFD 3.97 ms + embedding 20.19 ms） |
| Vela 编译后 CPU 算子 | **0** —— 100% NPU |
| 特征维度 | 128D int8 |
| 建议余弦匹配阈值 | **0.30** |

Vela 配置：`Ethos_U55_64`，system config `Ethos_U55_High_End_Embedded`，memory mode `Shared_Sram`。推理时间不含预处理、对齐与后处理。单模型 Vela 数据：embedding SRAM 596.84 KiB / Flash ~1008 KiB，SCRFD SRAM 200.83 KiB / Flash 632.66 KiB。

## 硬件要求

- [Grove Vision AI Module V2](https://wiki.seeedstudio.com/grove_vision_ai_v2/)（Himax WiseEye2 HX6538），或
- [SenseCAP Watcher](https://wiki.seeedstudio.com/getting_started_with_watcher/)（同一颗 WiseEye2 SoC）。

任何 tensor arena 可用空间 ≥ 840 KiB 的 WiseEye2 板卡都能跑。

## 快速开始

### 1. 获取模型

两个部署模型都已提交在仓库里，无需额外下载：

| 用途 | 文件 | 烧录地址 |
|---|---|---|
| 人脸特征 | `qat_distill_v2_relu6_128d/model_128d.int8_vela.tflite` | `0x510000` |
| 人脸检测 | `scrfd/models/scrfd_500m_kps_int8_vela.tflite` | `0x400000` |

`*_vela.tflite` 含 `ethos-u` 自定义算子，**无法在 PC 端 TFLite 解释器运行**。PC 侧做精度验证要用 pre-Vela 的 INT8 模型（`model_128d.int8.tflite`、`scrfd_500m_kps_int8.tflite`）。

### 2. 烧录

用板卡对应的 Himax/SSCMA 烧录流程（SSCMA flasher 通过 USB 串口走 xmodem，或 SenseCraft 网页烧录工具），按上表地址分别写入。固件从这两个固定偏移读取模型。

### 3. 验证

模型 I/O 约定，用于核对固件读到的内容：

- **Embedding 输入**：112×112×3 RGB int8，量化参数 `(scale=0.007843137, zp=-1)`
- **Embedding 输出**：128D int8，量化参数 `(scale=0.012660, zp=18)`

固件从 tensor 里读取 scale/zero-point 并反量化，输出 scale 不需要硬编码。

用 pre-Vela INT8 模型做主机侧自检：

```bash
uv sync
uv run python compute_embedding.py --help          # 端到端：检测 + 对齐 + 提特征
uv run python evaluate_embedding_models.py --help  # 精度对比工具
```

### 4. 设置匹配阈值

**余弦阈值设为 0.30**，不要沿用旧模型的 0.4。本模型 impostor 均值 0.005、genuine 均值 0.608，genuine p5 = 0.385、impostor p99 = 0.231。阈值在主机 / ESP32 侧，不在固件里；沿用 0.4 会误拒部分本人。

注册必须用设备摄像头现场采集。上传照片注册的模板匹配度很低（genuine 约 0.25），原因是成像域差异——设备注册 + 设备识别才是支持的路径。

## 仓库结构

| 路径 | 说明 |
|---|---|
| `qat_distill_v2_relu6_128d/` | 定版 embedding 模型：Vela TFLite、pre-Vela INT8、ONNX、QAT 权重、Vela summary |
| `qat_pipeline/` | 生产训练流水线：蒸馏 → 128D 头 → QAT → INT8 导出 → Vela 编译 |
| `scrfd/` | 人脸检测：`models/` 部署与源模型，`scripts/` 转换脚本，`quantization/` QAT 与 Vela summary |
| `training_extras/` | 主流水线之外的训练辅助脚本（128D 导出、ONNX/TFLite LFW 评估、远程运行脚本） |
| `archive/` | 历史实验脚本归档（official_mobilefacenet / sface / foamliu / ghostfacenet / edgeface 等路线） |
| `records/` | 实验记录：`logs/` 训练与评估日志，`vela_csv/` 各轮 Vela summary 共 95 份 |
| `share/` | 中文长文：`ARTICLE.md` 及其数据底稿 `REFERENCE.md` |
| 根目录 `*.py` | 推理与分析工具：特征计算、人脸库、模型评估、数据集下载、量化分析 |

## 如何获取大文件

三类大文件**不在**仓库内，全部可以从公开来源复现。

### 训练权重（约 2.2 GB）—— 不分发

`checkpoints/` 已加入 gitignore。整个训练过程从公开输入完全可复现：

- **Teacher**：InsightFace `glint360k_r100`（InsightFace model zoo 公开发布）
- **训练数据**：InsightFace `ms1m-retinaface-t1`（公开人脸识别数据集），或用 `download_glint360k_subset.py` 抽取 Glint360K 子集
- **代价**：4×GPU DDP，4 epoch，约 2 小时

```bash
# 可选：构建 Glint360K 的 112x112 对齐子集
uv run python download_glint360k_subset.py \
    --output-dir datasets/glint360k_subset_112 \
    --num-shards 8 --max-images 120000 --max-images-per-id 20

# 从 teacher 蒸馏出 MobileFaceNet 512D student
uv run python qat_pipeline/distill_mfn.py --help
```

完整顺序见下方 [复现训练](#复现训练)。

### 评估集 bin（约 135 MB）

`records/eval_bins/lfw.bin` 与 `records/eval_bins/cfp_fp.bin` 是 InsightFace 格式的验证集 bin（pickle：成对 JPEG 字节 + `issame` 列表）。它们随 InsightFace dataset zoo 的 `ms1m-retinaface-t1` 数据集压缩包一起分发，从该压缩包里取出 `lfw.bin` 和 `cfp_fp.bin` 放到 `records/eval_bins/` 即可。

```bash
mkdir -p records/eval_bins
# 从 ms1m-retinaface-t1 压缩包中解出 lfw.bin 与 cfp_fp.bin 到 records/eval_bins/

uv run python qat_pipeline/eval_tflite_full.py \
    --model qat_distill_v2_relu6_128d/model_128d.int8.tflite \
    --bins records/eval_bins/lfw.bin records/eval_bins/cfp_fp.bin
```

### 校准数据

INT8 representative dataset 从 LFW 生成：

```bash
# 下载 LFW 并产出 112x112 对齐裁剪
uv run python download_datasets.py --dataset lfw --output calibration_data/qat_112 --detect-faces

# 或一次性生成检测 + 特征两套校准集
uv run python prepare_calibration_data.py --qat --num-images 5000
```

## 复现训练

```
distill_mfn.py            ResNet100 teacher -> MobileFaceNet 512D student（ArcFace + MSE 蒸馏）
      │                   （distill_v2 float 权重）
      ▼
train_128d.py             512D student -> 128D 头
      │
      ▼
qat_finetune.py           QAT 精调（自蒸馏，每个 block 后插入 per-tensor int8 FakeQuantize）
qat_finetune_if.py        *_if = ImageFolder 变体
      │                   LeakyReLU -> ReLU6 的替换在这里；ReLU6 把范围钳住，
      │                   TFLite 的 representative dataset min/max 才不会被撑开（避免 PTQ 崩塌）
      ▼
export_qat_128d_clamp.py  QAT .pt -> 在学到的 FQ 范围上做硬 torch.clamp -> ONNX
      │                   （推荐路径；*_fq.py 是 QDQ 备选）
      ▼
quantize_and_vela.py      ONNX --onnx2tf--> TF SavedModel --tf.lite INT8--> vela (ethos-u55-64)
                          -> qat_distill_v2_relu6_128d/model_128d.int8_vela.tflite
```

各脚本入口与配置项见 [`qat_pipeline/README.md`](qat_pipeline/README.md)。

## 关键技术点

量化后还能留住精度，主要靠三个决定。它们的共同前提是 Ethos-U55 只支持 **per-tensor** 量化，没有 per-channel。

1. **per-tensor INT8 是硬约束。** 在 PyTorch 里看着正常的 per-channel 模型，一旦整层通道共用一个 scale 就会崩。QAT 精调在每个 block 后插入 per-tensor `FakeQuantize`，让训练阶段就看到部署时的数值行为。
2. **LeakyReLU → ReLU6。** 有界激活让 representative dataset 的 min/max 保持紧凑，量化范围不会被离群值撑开；ReLU6 还能融进前一层卷积，减少 NPU 算子数。
3. **clamp 导出，不用 PTQ。** 导出 QAT 权重时在学到的范围上做硬 `torch.clamp`，把 QAT 换来的判别力保留下来。朴素 PTQ 导出路径会丢掉它——impostor 相似度从 0.005 涨回 0.237。

完整推导（含否决多个架构的 SRAM 台阶函数现象）见 [`share/ARTICLE.md`](share/ARTICLE.md)，数据底稿见 [`share/REFERENCE.md`](share/REFERENCE.md)。

## 文档索引

| 文档 | 内容 |
|---|---|
| [`HISTORY.md`](HISTORY.md) | 完整实验史，试过的每条路线及放弃原因（英文，约 40K） |
| [`LESSONS.md`](LESSONS.md) | 踩坑记录：现象 / 根因 / 教训（中文） |
| [`share/ARTICLE.md`](share/ARTICLE.md) | 技术长文（中文） |
| [`share/REFERENCE.md`](share/REFERENCE.md) | 长文的数据底稿：表格、CSV 摘录、实测数据（中文） |
| [`SCRFD_DECODING.md`](SCRFD_DECODING.md) | SCRFD 输出张量解码 |
| [`SCRFD_INT8_EVALUATION_REPORT.md`](SCRFD_INT8_EVALUATION_REPORT.md) | SCRFD INT8 量化评估 |
| [`qat_distill_v2_relu6_128d/README.md`](qat_distill_v2_relu6_128d/README.md) | 部署模型卡：规格、来源、精度、阈值 |
| [`qat_pipeline/README.md`](qat_pipeline/README.md) | 训练流水线入口与配置 |

## 第三方组件与许可来源

本仓库自有的代码与文档采用 Apache-2.0，**模型权重不在其覆盖范围内** —— 逐文件血缘见 [`MODEL_LICENSE.md`](MODEL_LICENSE.md)。下列输入**不属于**本项目，各有各的条款：

| 组件 | 来源 | 条款 |
|---|---|---|
| SCRFD 检测器（架构与 `scrfd_500m_kps` 权重） | [InsightFace](https://github.com/deepinsight/insightface) | 权重：**仅限非商业研究用途**（model zoo 声明） |
| Teacher 权重 `glint360k_r100` | [InsightFace model zoo](https://github.com/deepinsight/insightface/tree/master/model_zoo) | **仅限非商业研究用途** —— *"ALL models are available for non-commercial research purposes only."* |
| `ms1m-retinaface-t1` 训练集 | InsightFace dataset zoo（源自 MS-Celeb-1M） | 数据集自有条款；MS-Celeb-1M 衍生集带使用限制 |
| Glint360K 子集 | InsightFace | 数据集自有条款 |
| LFW / CFP-FP 评估集 | UMass Amherst / CFP 作者 | 各自发布方的研究用途条款 |
| MobileFaceNet 架构 | Chen et al., *MobileFaceNets: Efficient CNNs for Accurate Real-time Face Verification on Mobile Devices*（arXiv:1804.07573） | 论文；本仓库为自行实现 |
| Vela 编译器 | [ARM ethos-u-vela](https://github.com/ARM-software/ethos-u-vela) | Apache-2.0 |

本仓库提供的 embedding 模型由 `glint360k_r100` 蒸馏得到、在 InsightFace 数据上训练。**商用部署前请确认上游模型与数据集许可是否允许你的用途。** 如果不允许，用条款可满足的 teacher 和数据集重新训练 student —— `qat_pipeline/` 的流程与 teacher 无关。

人脸识别在很多司法辖区属于受监管应用，部署时需关注知情同意、数据留存以及适用的生物识别隐私法规。

## 许可证

| 内容 | 许可 |
|---|---|
| 源代码、脚本、文档 | [Apache-2.0](LICENSE) |
| 模型权重文件（`.pth` / `.onnx` / `.tflite` / `.pt`） | **仅限非商业研究用途** —— 见 [`MODEL_LICENSE.md`](MODEL_LICENSE.md) |

Copyright 2026 Seeed Studio。第三方组件署名见 [`NOTICE`](NOTICE)。

权重源自 InsightFace 资产。[`MODEL_LICENSE.md`](MODEL_LICENSE.md) 记录了每个文件的血缘，
并说明如何用不同输入重跑 [`qat_pipeline/`](qat_pipeline/) 得到可商用的权重。
