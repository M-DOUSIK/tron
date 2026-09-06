# MFN-128D Training (vast.ai)

从头训练 MobileFaceNet scale=1, 128D，匹配 esp-dl MFN 架构。

## 架构

| 属性 | 值 |
|------|-----|
| 网络 | MobileFaceNet, scale=1, blocks=(1,4,6,2) |
| 参数量 | ~1.2M |
| 输入 | 112×112×3 RGB |
| 输出 | 128D embedding |
| BN | 训练时有 BN，导出 ONNX 后通过 esp-ppq 折叠 |
| 训练框架 | insightface/recognition/arcface_torch |

## vast.ai 部署

### 1. 选实例
- 最低：1× RTX 3090 / A4000 / A5000
- 推荐：4-8× RTX 3090 / A4000（一天内完成）
- 磁盘：≥ 50 GB
- 镜像：`pytorch/pytorch:2.1.0-cuda12.1-cudnn8-devel` 或任意 CUDA 12.1 镜像

### 2. 上传代码

```bash
# 本地打包
# 在仓库里的训练脚本目录执行（按自己的 checkout 路径替换）
cd "$REPO_ROOT/qat_pipeline"
tar czf /tmp/mfn_training.tar.gz .

# 上传到 vast.ai 实例
scp /tmp/mfn_training.tar.gz root@<vast-ip>:/workspace/
ssh root@<vast-ip> "cd /workspace && mkdir -p mfn_training && tar xzf mfn_training.tar.gz -C mfn_training"
```

### 3. 安装 & 训练

```bash
ssh root@<vast-ip>
cd /workspace
bash mfn_training/setup_vast.sh     # 安装依赖 + 下载数据（~15min）
nohup bash mfn_training/nohup_train.sh > train.log 2>&1 &
tail -f train.log
```

### 4. 训练时间预估

| GPU 数量 | 型号 | 预估时间 |
|----------|------|---------|
| 1× | 3090 | ~3-4 天 |
| 4× | 3090 | ~18-24 小时 |
| 8× | A4000/3090 | ~10-14 小时 |
| 8× | A100 | ~5-6 小时 |

单卡也能跑，就是等得久。vast.ai 上 4×3090 大概 $1-2/hr。

### 5. 产出

训练完成后 `/workspace/output/mfn128_<timestamp>/` 里有：

- `model.pt` — PyTorch checkpoint
- `mfn128_float32.onnx` — 可直接用的 ONNX（带 BN）
- `full.log` — 完整训练日志含 LFW/CFP-FP/AgeDB-30 验证结果

### 6. 后续流水线

ONNX → esp-ppq 量化 → .espdl (ESP32)，或 ONNX → onnx2tf → TFLite → Vela (WE2/Ethos-U55)。

BN 折叠在量化时由 esp-ppq 或 onnx2tf 自动完成，无需额外处理。
