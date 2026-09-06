# grove-vision-face-embedding

[English](README.md) · [简体中文](README.zh-CN.md)

On-device face recognition for the **Grove Vision AI Module V2** (Himax WiseEye2 HX6538,
Cortex-M55 + Ethos-U55 NPU). SCRFD face detection plus a QAT-distilled ReLU6
MobileFaceNet 128-D embedding extractor — both compiled to run **entirely on the NPU,
with zero CPU fallback operators**.

This repository holds the deployable INT8 models, the training/quantization pipeline
that produced them, and the full experiment record behind the design decisions.

> ### ⚠️ Licensing: code and weights differ
>
> **Code, scripts and documentation** — Apache-2.0 ([`LICENSE`](LICENSE)).
>
> **Model weight files** — **non-commercial research use only**. Every shipped weight
> derives from [InsightFace](https://github.com/deepinsight/insightface) assets
> (the SCRFD detector directly; the embedding model through distillation from
> `glint360k_r100` trained on `ms1m-retinaface-t1`), and InsightFace states that
> *"ALL models are available for non-commercial research purposes only."*
>
> The pipeline in [`qat_pipeline/`](qat_pipeline/) is teacher- and dataset-agnostic —
> substituting permissively licensed inputs and re-running it produces weights without
> this restriction. See **[`MODEL_LICENSE.md`](MODEL_LICENSE.md)** for per-file
> provenance and instructions.

## Key numbers

| Metric | Value |
|---|---|
| Whole-board power | **0.35 W** |
| LFW accuracy (INT8) | **99.33%** |
| CFP-FP accuracy (INT8) | **94.26%** |
| Tensor arena (SRAM) | **840 KiB** reserved (220 KB + 620 KB) |
| Pure NPU inference, one frame | **24.2 ms** (SCRFD 3.97 ms + embedding 20.19 ms) |
| CPU operators after Vela | **0** — 100% NPU |
| Embedding dimension | 128-D int8 |
| Recommended cosine match threshold | **0.30** |

Measured with Vela for `Ethos_U55_64`, system config `Ethos_U55_High_End_Embedded`,
memory mode `Shared_Sram`. Inference time excludes preprocessing, alignment and
post-processing. Per-model Vela detail: embedding SRAM 596.84 KiB / Flash ~1008 KiB,
SCRFD SRAM 200.83 KiB / Flash 632.66 KiB.

## Hardware

- [Grove Vision AI Module V2](https://wiki.seeedstudio.com/grove_vision_ai_v2/) (Himax WiseEye2 HX6538), or
- [SenseCAP Watcher](https://wiki.seeedstudio.com/getting_started_with_watcher/), which uses the same WiseEye2 SoC.

Any WiseEye2 board with at least 840 KiB of tensor arena available will work.

## Quick start

### 1. Get the models

Both deployable models are checked into this repository — no download step needed:

| Role | File | Flash address |
|---|---|---|
| Face embedding | `qat_distill_v2_relu6_128d/model_128d.int8_vela.tflite` | `0x510000` |
| Face detection | `scrfd/models/scrfd_500m_kps_int8_vela.tflite` | `0x400000` |

The `*_vela.tflite` files contain the `ethos-u` custom operator and **cannot run on a
PC TFLite interpreter**. For host-side accuracy checks use the pre-Vela INT8 models
(`model_128d.int8.tflite`, `scrfd_500m_kps_int8.tflite`).

### 2. Flash

Use the standard Himax/SSCMA flashing flow for your board (`xmodem` over the USB serial
port via the SSCMA firmware flasher, or the SenseCraft web flasher). Write each model to
the address in the table above; the firmware reads them from those fixed offsets.

### 3. Verify

Model I/O contract, so you can check what the firmware sees:

- **Embedding input:** 112×112×3 RGB int8, quantization `(scale=0.007843137, zp=-1)`
- **Embedding output:** 128-D int8, quantization `(scale=0.012660, zp=18)`

The firmware reads scale/zero-point from the tensor and dequantizes, so the output scale
does not need to be hardcoded.

Host-side sanity check against the pre-Vela INT8 model:

```bash
uv sync
uv run python compute_embedding.py --help          # end-to-end detect + align + embed
uv run python evaluate_embedding_models.py --help  # accuracy comparison harness
```

### 4. Set the match threshold

**Use a cosine threshold of 0.30**, not the 0.4 that older embedding models used. This
model centers impostor pairs near 0.005 and genuine pairs near 0.608 — genuine p5 =
0.385, impostor p99 = 0.231. The threshold lives on the host/ESP32 side, not in the
firmware.

Enroll using the device camera. Templates enrolled from uploaded photos match poorly
(genuine similarity around 0.25) because of the imaging-domain gap; device-enroll plus
device-recognize is the supported path.

## Repository layout

| Path | Contents |
|---|---|
| `qat_distill_v2_relu6_128d/` | Deployed embedding model: Vela TFLite, pre-Vela INT8, ONNX, QAT checkpoint, Vela summary |
| `qat_pipeline/` | Production training pipeline: distill → 128-D head → QAT → INT8 export → Vela |
| `scrfd/` | Face detection: `models/` deployable and source models, `scripts/` conversion, `quantization/` QAT and Vela summaries |
| `training_extras/` | Training helpers outside the main pipeline (128-D export, ONNX/TFLite LFW eval, remote run scripts) |
| `archive/` | Historical experiment scripts (official_mobilefacenet / sface / foamliu / ghostfacenet / edgeface routes) |
| `records/` | Experiment record: `logs/` training and eval logs, `vela_csv/` 95 Vela summaries across all attempts |
| `share/` | Long-form write-up (Chinese): `ARTICLE.md` and its source-data companion `REFERENCE.md` |
| `*.py` (root) | Inference and analysis tools: embedding computation, face DB, model evaluation, dataset download, quantization analysis |

## Getting the large files

Three categories of large files are deliberately **not** in this repository. Every one of
them is reproducible from public sources.

### Training checkpoints (~2.2 GB) — not distributed

`checkpoints/` is git-ignored. The full training run is reproducible from public inputs:

- **Teacher:** InsightFace `glint360k_r100` (public release from the InsightFace model zoo).
- **Training data:** InsightFace `ms1m-retinaface-t1` (public recognition dataset), or a
  Glint360K subset via `download_glint360k_subset.py`.
- **Cost:** 4×GPU DDP, 4 epochs, roughly 2 hours.

```bash
# Optional: build a Glint360K subset of aligned 112x112 crops
uv run python download_glint360k_subset.py \
    --output-dir datasets/glint360k_subset_112 \
    --num-shards 8 --max-images 120000 --max-images-per-id 20

# Distill teacher -> MobileFaceNet 512D student
uv run python qat_pipeline/distill_mfn.py --help
```

See [Reproducing the training](#reproducing-the-training) for the full sequence.

### Evaluation bins (~135 MB)

`records/eval_bins/lfw.bin` and `records/eval_bins/cfp_fp.bin` are InsightFace-format
verification bins (a pickle of paired JPEG bytes plus an `issame` list). They ship inside
the InsightFace `ms1m-retinaface-t1` recognition dataset archive from the InsightFace
dataset zoo — extract `lfw.bin` and `cfp_fp.bin` from that archive into
`records/eval_bins/`.

```bash
mkdir -p records/eval_bins
# extract lfw.bin and cfp_fp.bin from the ms1m-retinaface-t1 archive into records/eval_bins/

uv run python qat_pipeline/eval_tflite_full.py \
    --model qat_distill_v2_relu6_128d/model_128d.int8.tflite \
    --bins records/eval_bins/lfw.bin records/eval_bins/cfp_fp.bin
```

### Calibration data

The INT8 representative dataset is generated from LFW:

```bash
# Download LFW and produce aligned 112x112 crops
uv run python download_datasets.py --dataset lfw --output calibration_data/qat_112 --detect-faces

# Or build both detection and embedding calibration sets in one pass
uv run python prepare_calibration_data.py --qat --num-images 5000
```

## Reproducing the training

```
distill_mfn.py            ResNet100 teacher -> MobileFaceNet 512D student (ArcFace + MSE distill)
      │                   (distill_v2 float weights)
      ▼
train_128d.py             512D student -> 128D head
      │
      ▼
qat_finetune.py           QAT fine-tune (self-distillation, per-tensor int8 FakeQuantize
qat_finetune_if.py        after every block). *_if = ImageFolder variant.
      │                   LeakyReLU -> ReLU6 swap lives here; ReLU6 clamps the range so
      │                   TFLite's representative-dataset min/max stays tight (no PTQ collapse).
      ▼
export_qat_128d_clamp.py  QAT .pt -> clean model with hard torch.clamp at the learned FQ
      │                   ranges -> ONNX (recommended; *_fq.py is the QDQ alternative)
      ▼
quantize_and_vela.py      ONNX --onnx2tf--> TF SavedModel --tf.lite INT8--> vela (ethos-u55-64)
                          -> qat_distill_v2_relu6_128d/model_128d.int8_vela.tflite
```

Details, entry points and configuration flags: [`qat_pipeline/README.md`](qat_pipeline/README.md).

## Key technical points

Three decisions account for most of the accuracy that survives quantization. All of them
exist because the Ethos-U55 quantizes **per-tensor**, not per-channel.

1. **Per-tensor INT8 is the constraint, not an implementation detail.** A per-channel
   model that looks fine in PyTorch collapses when every channel in a layer has to share
   one scale. The QAT fine-tune inserts per-tensor `FakeQuantize` after every block so
   training sees the deployment numerics.
2. **LeakyReLU → ReLU6.** Bounded activations keep the representative-dataset min/max
   tight, so the quantization range does not get stretched by outliers. ReLU6 also fuses
   into the preceding convolution, cutting NPU operator count.
3. **Clamp export instead of PTQ.** Exporting the QAT checkpoint with hard `torch.clamp`
   at the QAT-learned ranges preserves the discrimination the QAT run earned. The naive
   PTQ export path lost it — impostor similarity rose from 0.005 to 0.237.

Full reasoning, including the SRAM step-function behaviour that ruled out several
architectures, is in [`share/ARTICLE.md`](share/ARTICLE.md) (Chinese) with its
source-data companion [`share/REFERENCE.md`](share/REFERENCE.md).

## Documentation index

| Document | What it is |
|---|---|
| [`HISTORY.md`](HISTORY.md) | Complete experiment history, every route tried and why it was dropped (English, ~40K) |
| [`LESSONS.md`](LESSONS.md) | Post-mortem notes: symptom / root cause / lesson (Chinese) |
| [`share/ARTICLE.md`](share/ARTICLE.md) | Long-form technical write-up (Chinese) |
| [`share/REFERENCE.md`](share/REFERENCE.md) | Source data backing the write-up: tables, CSV extracts, measurements (Chinese) |
| [`SCRFD_DECODING.md`](SCRFD_DECODING.md) | SCRFD output tensor decoding |
| [`SCRFD_INT8_EVALUATION_REPORT.md`](SCRFD_INT8_EVALUATION_REPORT.md) | SCRFD INT8 quantization evaluation |
| [`qat_distill_v2_relu6_128d/README.md`](qat_distill_v2_relu6_128d/README.md) | Deployed model card: specs, provenance, accuracy, threshold |
| [`qat_pipeline/README.md`](qat_pipeline/README.md) | Training pipeline entry points and configuration |

## Third-party components and licensing

This repository's own code and documentation are Apache-2.0. The model weights are not
covered by it — see [`MODEL_LICENSE.md`](MODEL_LICENSE.md) for per-file provenance.
The inputs below are **not** ours and carry their own terms:

| Component | Source | Terms |
|---|---|---|
| SCRFD detector (architecture, `scrfd_500m_kps` weights) | [InsightFace](https://github.com/deepinsight/insightface) | Weights: **non-commercial research only** (model zoo) |
| Teacher weights `glint360k_r100` | [InsightFace model zoo](https://github.com/deepinsight/insightface/tree/master/model_zoo) | **Non-commercial research only** — *"ALL models are available for non-commercial research purposes only."* |
| `ms1m-retinaface-t1` training set | InsightFace dataset zoo (derived from MS-Celeb-1M) | Dataset-specific terms; MS-Celeb-1M derivatives carry usage restrictions |
| Glint360K subset | InsightFace | Dataset-specific terms |
| LFW / CFP-FP evaluation sets | UMass Amherst / CFP authors | Research-use terms from the respective publishers |
| MobileFaceNet architecture | Chen et al., *MobileFaceNets: Efficient CNNs for Accurate Real-time Face Verification on Mobile Devices* (arXiv:1804.07573) | Paper; implementation here is our own |
| Vela compiler | [ARM ethos-u-vela](https://github.com/ARM-software/ethos-u-vela) | Apache-2.0 |

The embedding model shipped here was distilled from `glint360k_r100` and trained on
InsightFace data. **Confirm that the upstream model and dataset licenses permit your
intended use before deploying commercially.** If they do not, retrain the student from a
teacher and dataset whose terms you can satisfy — the pipeline in `qat_pipeline/` is
teacher-agnostic.

Face recognition is a regulated application in many jurisdictions. Deploying this
requires attention to consent, data retention and applicable biometric-privacy law.

## License

| What | License |
|---|---|
| Source code, scripts, documentation | [Apache-2.0](LICENSE) |
| Model weight files (`.pth` / `.onnx` / `.tflite` / `.pt`) | **Non-commercial research use only** — see [`MODEL_LICENSE.md`](MODEL_LICENSE.md) |

Copyright 2026 Seeed Studio. Third-party attributions are listed in [`NOTICE`](NOTICE).

The weights derive from InsightFace assets. [`MODEL_LICENSE.md`](MODEL_LICENSE.md)
records the provenance of each file and explains how to produce commercially usable
weights by re-running [`qat_pipeline/`](qat_pipeline/) with different inputs.
