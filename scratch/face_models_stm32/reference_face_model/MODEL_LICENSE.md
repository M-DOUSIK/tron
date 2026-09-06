# Model Licensing and Provenance

**The Apache-2.0 license in [`LICENSE`](LICENSE) covers the source code, scripts and
documentation in this repository. It does NOT cover the model weight files.**

Every model weight shipped here derives from [InsightFace](https://github.com/deepinsight/insightface)
assets, whose model zoo states:

> **"ALL models are available for non-commercial research purposes only."**

Accordingly, **all model weight files in this repository are made available for
non-commercial research purposes only.** Commercial use requires obtaining the
appropriate rights from the upstream holders listed below.

---

## Provenance of every shipped weight

### Face detection — `scrfd/models/`

| File | Relationship to upstream |
|---|---|
| `scrfd_500m_kps.pth` | InsightFace SCRFD-500M-KPS pretrained weights, **unmodified** |
| `scrfd_500m_kps.onnx` | Format conversion of the above |
| `scrfd_500m_kps_int8.tflite` | INT8 post-training quantization of the above |
| `scrfd_500m_kps_int8_vela.tflite` | Vela compilation of the above for Ethos-U55 |

These are **format conversions and quantizations of an InsightFace model**, not
retrained networks. The upstream restriction applies directly and without ambiguity.

- Upstream: <https://github.com/deepinsight/insightface/tree/master/detection/scrfd>
- Model zoo terms: <https://github.com/deepinsight/insightface/tree/master/model_zoo>

### Face embedding — `qat_distill_v2_relu6_128d/`

| File | Relationship to upstream |
|---|---|
| `model_qat_relu6_best.pt` | QAT-trained student weights |
| `model_128d.onnx` | Export of the above |
| `model_128d.int8.tflite` | INT8 quantization of the above |
| `model_128d.int8_vela.tflite` | Vela compilation of the above for Ethos-U55 |

These weights were **not** copied from upstream. They were produced by training a
MobileFaceNet student, using:

| Input | Source | Terms |
|---|---|---|
| Teacher model | InsightFace `glint360k_r100` (ResNet100) | Non-commercial research only |
| Training data | `ms1m-retinaface-t1` (MS1MV3), 93,431 identities / 5 M images | Distributed by InsightFace; derived from MS-Celeb-1M, which Microsoft withdrew in 2019 |
| Student architecture | MobileFaceNet | Architecture from published literature |

**On distillation specifically.** Whether distilled weights constitute a derivative
work of the teacher is not settled law, and the student's parameter values bear no
direct numerical relationship to the teacher's. However, the upstream terms restrict
*use of the model*, and running the teacher to generate distillation targets is such a
use. This project therefore treats the student weights as carrying the same
restriction. Users who reach a different conclusion should obtain their own legal
advice rather than relying on this repository's characterization.

---

## Using this work commercially

The pipeline in [`qat_pipeline/`](qat_pipeline/) is **teacher-agnostic and
dataset-agnostic**. Nothing in the training code depends on InsightFace assets — they
are supplied as arguments. To obtain commercially usable weights:

1. Substitute a teacher model whose license permits your intended use.
2. Substitute a face dataset whose license permits your intended use.
3. Re-run the pipeline as documented in the README. On 4 GPUs the distillation stage
   takes roughly two hours.

For face detection, substitute a detector with compatible licensing; the firmware-side
decoding contract is documented in [`SCRFD_DECODING.md`](SCRFD_DECODING.md) so an
alternative detector can be adapted to the same output format.

Be aware that most large-scale public face datasets carry research-only terms. Sourcing
permissively licensed face data is typically the binding constraint, not the code.

---

## Third-party components

| Component | Source | License |
|---|---|---|
| SCRFD detector and pretrained weights | InsightFace | Code: see upstream repository. Models: non-commercial research only |
| `glint360k_r100` teacher | InsightFace | Non-commercial research only |
| `ms1m-retinaface-t1` dataset | InsightFace / MS-Celeb-1M derivative | Research use; MS-Celeb-1M withdrawn by Microsoft in 2019 |
| LFW, CFP-FP evaluation sets | Respective publishers | See each dataset's terms |
| Ethos-U Vela compiler | Arm | Apache-2.0 |
| TensorFlow Lite / TensorFlow | Google | Apache-2.0 |
| PyTorch | Meta | BSD-3-Clause |
| onnx2tf | Katsuya Hyodo (PINTO0309) | MIT |

---

## Disclaimer

This document records the provenance of the files in this repository and the project's
own reading of the applicable terms. **It is not legal advice.** Anyone intending to use
these models outside non-commercial research should verify the current upstream terms
and obtain their own advice.
