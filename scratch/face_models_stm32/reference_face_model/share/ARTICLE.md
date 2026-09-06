# 840 KB 内存、0.35 W：在 MCU 上跑完一条端侧人脸识别流水线

人脸识别的常规部署形态是服务器 GPU，往下压到边缘，通常也要一块 Jetson。

这个项目把完整的一条链路 —— 摄像头出图、人脸检测、五点对齐、特征提取、比对 —— 全部放进了一颗 MCU：**Himax WiseEye2 HX6538，Cortex-M55 + Ethos-U55 NPU，整块模组 0.35 W**。

## 数字速览

| 项 | 数字 |
|---|---|
| **整机功耗** | **0.35 W**（70 mA，第三方实测） |
| **识别精度** | **LFW 99.33% / CFP-FP 94.26%** |
| **SRAM 占用** | **840 KiB**（检测 200.83 + 识别 596.84，含对齐余量） |
| **模型体积** | **1.63 MB**（检测 647.6 KB + 识别 1020.3 KB） |
| **纯 NPU 推理** | **24 ms**（检测 3.97 + 识别 20.19，@500 MHz） |
| **CPU 算子数** | **0**（检测 100% / 识别 99.51% 跑在 NPU） |
| **云端依赖** | **无**，识别全程在设备内完成 |
| 教师 → 学生 | ResNet100 **49.03 M 参数** → MobileFaceNet **1.19 M 参数**（41:1） |
| 部署阈值 | 0.30（impostor p99 = 0.231，genuine p5 = 0.385） |
| 端上陌生人相似度 | **0.045** |

芯片的全部系统内存是 2432 KB，留给模型 tensor arena 的预算只有 **1112 KB** —— 检测和识别两个模型要在这里面装完，还要和 640×480 的摄像头 DMA 抢地址。

**LFW 99.33% 这个成绩本身不稀奇，稀奇的是它跑在 840 KB 里、功耗 0.35 W、不联网。**

达到这个状态之前换了五轮模型，其中一版上线后收到客户反馈「认错人」。下面是完整记录。

配套数据底稿见 `REFERENCE.md`，本文每个数字都能在那里找到来源文件和行号。

---

## 一、背景：为什么要在 MCU 上做人脸识别

SenseCAP Watcher 这个设备里装的就是 Grove Vision AI Module V2 —— ESP32-S3 负责联网和交互，Himax WiseEye2 HX6538（Cortex-M55 + Ethos-U55 NPU）负责视觉推理。

工业管理场景对人脸识别的需求很具体。以[智慧仓储管理](https://www.seeed.cc/solutions/smart-warehouse-management)为例：出入库登记、盘点、人员追踪 —— **人脸识别在这里的作用是操作追溯**，把每一次出入库操作绑定到具体的人，而不是靠工牌或手动输入。

这类场景的部署条件决定了技术路线：现场往往没有稳定的网络和布线，设备要能独立工作。把识别放在云端意味着每一帧都要上传，既有带宽成本也有隐私问题；放在本地的 x86 或 Jetson 上，功耗和成本都上一个数量级。

### 为什么是 NPU

Grove Vision AI V2 整块模组跑目标检测的实测功耗是 **70 mA / 0.35 W**（[Core Electronics 用 otiiarc 功耗仪实测](https://core-electronics.com.au/guides/getting-started-with-the-grove-vision-ai-v2-power-efficient-object-detection/)，跑 YOLOv8 @ 25 FPS，55 ms/帧）。

换个参照物：**一颗 RGB LED 全亮的功耗，约等于整块板子跑目标检测的功耗**（原文的比喻）。

芯片级的数字来自 [Himax 官方](https://www.himax.com.tw/products/wiseeye-ai-sensing/wiseeye2-ai-processor/)：WiseEye2 可以 always-on，功耗**个位数毫瓦**；相比上一代 WE1，**推理速度 32 倍、能效 50 倍**。芯片内置 0.8/0.9 V 的 DVFS 动态调压调频。

0.35 W 这个量级决定了几件事：设备可以纯电池运行、可以部署在没有布线条件的位置、可以常开而不是按需唤醒。

还有一件事是功耗换不来但同样重要的 —— **识别全程在设备内完成，不需要把画面传出去**。没有带宽成本，没有云端往返延迟，断网也照常工作。对仓储、厂区这类现场，这往往比省电更关键。

> **数据口径提醒**：0.35 W 是第三方实测（跑 YOLOv8），不是官方标称值；芯片级的"个位数毫瓦"是官方表述。本项目自身没有做过功耗测量，也没有做过 NPU 与 CPU（CMSIS-NN 路径）的推理耗时对照实验 —— 文中不提供这两类数字。

---

## 二、人脸识别在这个固件里分哪几步

先把流程摊开，后面讲的每个问题都能定位到具体环节。

| 步骤 | 内容 | 输出 |
|---|---|---|
| 0 | 摄像头出图 | YUV422P，**640×480**（帧缓冲 614 KB，DMA 占 SRAM1 尾部） |
| 1 | resize + 色彩空间转换（一次完成，最近邻，无 letterbox） | RGB888 **160×160×3** |
| 2 | 量化进检测输入张量 | INT8（zp = −128，等价 `pixel−128`） |
| 3 | **SCRFD-500M-KPS 检测** + 解码 + NMS | bbox + 5 点 landmark |
| 3a | 质量门 1：bbox 最小边 < **40 px** 丢弃 | |
| 3b | 质量门 2：landmark 姿态质量分 < **0.3** 丢弃 | |
| 4 | **人脸对齐**：5 点闭式最小二乘相似变换 + 反向双线性 warp | RGB **112×112** |
| 5 | 量化进 embedding 输入 | INT8（`scale=0.007843137, zp=−1`） |
| 6 | **MobileFaceNet 128D 推理** | 128 维 int8 |
| 7 | 反量化 + **L2 归一化** | 128 维单位向量 |
| 8 | 打包 JSON（含 base64 JPEG、box、score、quality、landmark、embedding）经 AT 口发给 host | |

几个容易被忽略的设计点：

**对齐的 warp 源是原始 640×480 全分辨率帧，不是 160×160 的缩图。** 检测在缩图上做，对齐回到原图取像素 —— 否则 112×112 的人脸会是从 160×160 里抠出来再放大的糊图。

**每帧只处理一张脸。** 取检测置信度最高的那个。

**固件只产出 embedding，不做比对。** 余弦阈值、人脸库、注册决策全在 host 侧（ESP32 或 SBC）。这个分工让阈值可以随模型换代调整，不用重刷固件。

**对齐用的是 ArcFace 标准 112×112 五点模板**，闭式解，不做 SVD。

推理耗时（Vela 报，@500 MHz）：

| 模型 | 推理时间 | nn_macs | SRAM | Flash |
|---|---|---|---|---|
| SCRFD-500M-KPS | **3.97 ms** | 45.9 M | 200.83 KiB | 632.66 KiB |
| MobileFaceNet 128D | **20.19 ms** | 221.0 M | 596.84 KiB | 1007.86 KiB |

**两个模型合计约 24 ms 纯 NPU 推理。** 端到端还要加上色彩转换、全分辨率 warp、NMS、JPEG 编码和串口传输 —— 固件里有完整的分步计时框架，但开关默认关闭，这部分**没有实测数据**，本文不给端到端帧率。

### 卡在哪

流程本身在 2026 年 2 月就跑通了。真正花掉大半年的是**这条流程里的 embedding 模型换了五轮**。

约束长这样：

```
芯片全部系统内存                     2432 KB
  ├─ APP ROM                          256 KB
  ├─ APP DATA（堆/栈）                256 KB
  ├─ 系统 SRAM / FreeRTOS heap        336 KB
  ├─ 模型 tensor arena  ←── 全部预算  1112 KB
  └─ 摄像头 YUV422 DMA 帧缓冲         614 KB（640×480，占 SRAM1 尾部）
```

**1112 KB 要装下检测和识别两个模型**，而且它和摄像头帧缓冲在物理地址上是紧挨着的。扣掉固定图像缓冲后，边流边推的安全线是 **1000.25 KiB**。

这个预算不是"能跑就行"的那种约束，它直接决定产品形态：

| MFN arena | 检测+识别合计 | headroom | 设备行为 |
|---|---|---|---|
| 760 KB | 980 KiB | ≈ 20 KB | **每次推理都要先停掉摄像头流** |
| **620 KB** | **840 KiB** | **≈ 160 KB** | **流一直开着，边看边认** |

**140 KB 的差别，决定了这台设备是"拍一张认一次"还是"一直在看"。**

而要把 embedding 模型压进 620 KB，同时还要保持能用的识别精度 —— 这就是接下来五轮选型的全部内容。

---

## 三、五轮选型，四次失败

### 第一版：GhostFaceNet-0.5（512D）

第一个真正烧进设备的 embedding 模型。SRAM 245.00 KiB，输出 512 维，Vela 报 14.79 ms —— 预算完全够。

放弃原因有两条，都是记录原文：

> - Quantization accuracy dropped too much (QAT didn't help)
> - Vela-compiled model caused AllocateTensors hang — tensor graph incompatible with firmware

第二条的机制值得单说：固件的 arena 尺寸、变量命名、后处理全是按 MobileFaceNet 128D 写的。默认发 GhostFaceNet 会让 `AllocateTensors` 挂住，**触发 6 秒看门狗复位循环**。

> **说明**：GhostFaceNet 在本项目里**没有留下任何精度数字**（LFW / CFP-FP 都没有），只有定性的"量化掉太多"。

不过它留下了一个有价值的数据点。对比同样落在预算内的 MobileFaceNet：

| 模型 | SRAM KiB | passes | nn_macs | cycles_npu | 推理 ms |
|---|---:|---:|---:|---:|---:|
| GhostFaceNet-0.5 | 245.00 | **370** | **11.4 M** | 5.61 M | 14.79 |
| MobileFaceNet 128D | 599.03 | **62** | **221.0 M** | 6.38 M | 12.85 |

**GhostFaceNet 的计算量只有 MobileFaceNet 的 1/19，推理反而更慢。** 每 MAC 消耗的 NPU cycle 数相差约 10 倍。

原因在 passes 数：370 vs 62。Ghost module 的算子结构在 Vela 里被拆成大量小 pass，每个 pass 都有固定开销，NPU 算力没被喂饱。它的 `cycles_total` 是 `cycles_npu` 的 2.3 倍，大量时间卡在访存而不是计算。

**在 Ethos-U 上选架构，passes 数比参数量和 MAC 数更能反映实际效率。**

### 第二版：foamliu MobileFaceNet 128D

替掉 GhostFaceNet 之后的线上模型。跑得起来，但判别力有问题。

引入官方模型时做的对比很说明问题：

| | foamliu | InsightFace 官方 |
|---|---|---|
| MegaFace | **82.55%** | **92.59%** |
| LFW | 99.48% | — |
| loss | softmax / focal，无角度 margin | **ArcFace 加性角度 margin** |

**LFW 99.48% 是误导性的 —— 跨域验证（MegaFace）差了 10 个百分点。**

实测证据更直接：跑完整 SCRFD 流程，float32 下同一个人的相似度在 0.55–0.83 之间不稳定，而**不同人经常超过 0.6**（正常应该低于 0.3）。INT8 量化只再掉约 2%，不是主因。

问题不在量化，在模型本身的类间区分度 —— 没有角度 margin 的 loss 训出来的 embedding，类间距离拉不开。

### 第三版：InsightFace 官方 w600k MobileFaceNet

判别力确实好，SRAM 塞不下。

Vela 报 **1176.09 KiB**，超出 1112 KB 的整个 arena 区。为了塞进去，EL_ALLOC 一度从 1112/1304 KB 扩到 1712 KB —— 摄像头帧缓冲被挤，没有任何余量，不是能出货的状态。

然后是这个项目里代价最大的一次误判。

**"512 维太大，压到 128 维不就行了"** —— 训了个 512→128 的线性投影，PC 上指标很好看，LFW 分离度从 0.5560 涨到 0.6008，CFP-FP 从 0.1110 涨到 0.1593，准确率没掉。

跑 Vela：

| 方案 | 输出维度 | SRAM KiB | Flash KiB |
|---|---|---:|---:|
| `w600k_dense128` | 128D | **1176.09** | 2067.38 |
| `w600k_projected_128d` | 128D（带投影层） | **1176.09** | 3199.31 |
| `w600k_tail48_dense128` | 128D | **1176.09** | 1979.39 |
| `mobilefacenet_no_bn_int8` | **512D** | **1176.09** | 2076.59 |

**四个方案的 SRAM 精确到小数点后两位完全相同。** Flash 差了 1.2 MB，arena 峰值一个字节没动。

原因：**Ethos-U 的 arena 峰值取决于骨干中间张量的最大值。** 112×112 输入在网络前段产生的特征图已经把峰值定死，末端加一个 512×128 的矩阵乘只有 0.065 MMAC —— 占总计算量的 0.015%，对前面任何一层的激活尺寸没有影响。

**在输出端降维，降的是输出，不是峰值。** 整整一轮压缩实验对部署毫无意义。

这个教训后来被写进了相关脚本的文件头注释：

> *"It does not reduce Ethos-U intermediate tensor SRAM."*

真正有效的做法是换一个骨干本身就更窄、原生输出 128D 的网络。第一个这样的蒸馏学生直接做到 **449.12 KiB**，比 1176 降 62%。

### 第四版：S2 蒸馏学生（128D）

用 w600k 当 teacher，蒸馏一个 width=1.0、原生输出 128D 的 MobileFaceNet。

SRAM 问题解决得很干净 —— 稳定在 **599.8 KiB**，CPU 算子 0，NPU 100%。

精度问题做了三十多轮：pair fine-tune、阈值感知 hinge loss、hard negative mining、teacher-pair loss、ArcFace head、Glint360K 外部身份数据、top-k 难样本 loss、加宽 width、改 256D……

**共享阈值 score 在 0.667 到 0.815 之间横跳，跨度 0.15，没有任何一次带来数量级提升。**

最终判定它不能上线的，不是准确率，是**误接受率**：

| 模型 | Score | LFW EER | LFW TAR@FAR5 | **共享阈值下 LFW FAR** | **CFP-FP FAR** |
|---|---|---|---|---|---|
| `tpair_hn_a`（当时最优候选） | **0.725** | 22.9% | 52.5% | **75.0%** | 55.0% |
| w600k（教师，作为参考） | — | 3.3% | 96.7% | **56.7%** | 63.3% |

原文结论：

> *"not production-ready for low-FAR/security-sensitive recognition... balanced accuracy can improve while false-accept risk remains too high."*

**score 0.725 的"最优候选"，实际误接受率 75%。**

需要说清楚的是：**同一张表里 w600k 自己的 LFW FAR 也有 56.7%** —— 在这个共享阈值的定义下所有模型 FAR 都高，不是 S2 独有的问题。但这恰恰说明了一件事：**平衡准确率可以持续改善，而误接受风险完全不在这个指标的视野里。** 只看准确率选型，会一路选到一个在真实场景不可用的模型上。

### 客户反馈：「认错人」

这是整个项目里最该记住的一段。

在 S2 路线之外，另一条线（w600k + 事后 PCA 降到 128D + QAT）做出了一个能上线的模型 `qat_distilled_128d`，并且真的上线了。

它的标准 benchmark 成绩看起来没问题：**LFW 98.93%**。

上线后收到的反馈是**「认错人」—— 陌生人被识别成已注册用户**。

拆开数据就明白了：

| 指标 | 旧模型 `qat_distilled_128d` |
|---|---|
| LFW 准确率 | 98.93%（看起来正常） |
| genuine 相似度均值 | 0.699 |
| **impostor 相似度均值** | **0.237** |
| **genuine/impostor 分离度** | **0.46** |
| **端上（office4）陌生人相似度** | **0.25** |
| 部署阈值 | **0.4** |

**陌生人的相似度 0.25，阈值 0.4，中间只隔 0.15。**

在干净的 benchmark 图上这个余量够用，但设备摄像头的成像域和 benchmark 照片差别很大 —— 光线、镜头、压缩、姿态，任何一项漂移都可能把陌生人推过 0.4 这条线。

项目记录里的原话：

> *"That model had weak discrimination -- strangers embedded near each other -- which surfaced through customer feedback as stranger false-accepts. On the standard benchmark it looked fine (LFW 98.93%), but that hid a thin genuine/impostor separation (0.46) that collapsed on the device's imaging domain."*

一句话概括：**陌生人的特征向量彼此挨得太近，而标准 benchmark 看不出来。**

**LFW 98.93% 这个数字本身没有错，错的是把它当成了部署可用性的证明。** 人脸识别的部署风险在 impostor 分布的尾部，而准确率指标对尾部几乎不敏感。

顺带一个相关的现场约束：**注册必须用设备摄像头现场采集**。用手机照片注册会导致 genuine 相似度只有约 0.25 —— 同样是成像域的问题，注册图和识别图必须来自同一个域。

### 定版：QAT distill_v2 ReLU6 128D

回头看，前面四轮真正的共同瓶颈是：**先做浮点训练，再做 PTQ 量化，量化误差吃掉了判别力。**

这个损失有多大，QAT 脚本的文件头里有原始记录：

> *"FP32 model achieves 83.5% LFW but collapse to 57% with per-tensor INT8."*

**83.5% → 57%，掉 26.5 个点。** 在这个前提下，浮点侧调 loss 权重调再多轮，都会被量化环节吃掉 —— 前面三十几轮实验，本质上都是在量化这道闸门的上游做无用功。

换掉范式之后的压缩比是这样的：

```
教师  InsightFace glint360k_r100 (ResNet100)    49,029,632 参数
                    ↓ 蒸馏
学生  MobileFaceNet scale=1, blocks=(1,4,6,2)    1,192,960 参数   （41 : 1）
                    ↓ 512D → 128D 头（参与 QAT）
                    ↓ QAT 精调 + clamp 导出 + INT8
部署  model_128d.int8_vela.tflite                 1020.3 KB 文件
                                                   596.84 KiB 运行时 SRAM
```

**参数量压掉 41 倍，最终落到 1 MB 出头的文件、不到 600 KB 的运行内存，LFW 还剩 99.33%。**

结果对比：

| 指标 | 旧模型（PTQ + 事后 PCA） | 定版（QAT + ReLU6 + clamp 导出） |
|---|---|---|
| LFW | 98.93% | **99.33%** |
| CFP-FP | 91.54% | **94.26%** |
| **impostor 相似度均值** | 0.237 | **0.005** |
| **genuine/impostor 分离度** | 0.46 | **0.60** |
| **端上陌生人相似度** | 0.25 | **≈0.045** |
| 部署阈值 | 0.4 | **0.30** |
| Vela SRAM | 599.41 KiB | 596.84 KiB |
| Vela Flash | 1252.03 KiB | **1007.86 KiB** |

**impostor 相似度降了约 50 倍，端上陌生人相似度从 0.25 降到 0.045。**

新阈值 0.30 的推导有明确余量：impostor 的 p99 是 0.231，genuine 的 p5 是 0.385，两侧各留 0.069 和 0.085。

具体是哪三个技术点起了作用，下一节展开。

---

## 四、per-tensor 量化：这颗 NPU 最大的精度陷阱

### 根因：Ethos-U55 的激活量化是 per-tensor，不是 per-channel

一个张量只有一个 scale 和 zero_point：

```
scale = (max - min) / 255
```

用 LeakyReLU 时，正半轴无上界。只要某几个通道出现大 outlier，scale 就被拉大，**其余通道的信号全被压进少数几个量化格里**。

权重可以 per-channel 量化，激活不行 —— 这是硬件决定的。所以能改的只有激活函数本身。

### 解法一：ReLU6

三重收益：

1. **有界**：输出恒定在 [0,6]，per-tensor scale 天然是 6/255，与 outlier 无关
2. **可融合**：作为 fused activation 直接融进 conv 算子，NPU 算子数更少
3. **让 TFLite 的代表数据集 min/max 收敛到 QAT 学到的紧范围**

这条建议其实早就出现在项目的量化诊断脚本输出模板里：

```
4. ARCHITECTURE MODIFICATIONS
   - Replace PReLU with ReLU6 (bounded activation)
```

半年后才真正落地成产品模型。

### 解法二：QAT，且要按目标硬件的量化方式配置

FakeQuantize 插在每个 ConvBlock/LinearBlock 输出后，明确配成 `per-tensor affine, int8` —— 直接对齐 Ethos-U55 的激活量化方式，而不是用默认的 per-channel 配置。

其他要点：
- teacher 用同一个 distilled student 的 FP32 冻结副本（自蒸馏）
- loss 用 cosine-embedding + 0.1×MSE，理由是 *"Cosine matches face-verification geometry better than raw MSE"*
- 先用 25 个 batch 校准 observer，冻结 range，再训练
- 每 epoch 报一次 cosine fidelity（float teacher vs int8-simulated student），**这个指标预测最终 int8 tflite 的质量**

### 解法三：clamp 导出 —— 光做 QAT 还不够

这一步最不显然。

QAT 训练好之后导出 ONNX，走标准的 onnx2tf + TFLite INT8 量化。问题是：**TFLite 会用代表数据集重新计算一遍 per-tensor 的 min/max**，把 QAT 辛苦学到的紧范围直接丢掉，崩塌照旧发生。

解法是导出时把每个训练好的 FakeQuantize 替换成一个硬 `torch.clamp(lo, hi)`，钳到该 FakeQuantize 学到的精确范围。这样模型的激活本身就是有界的，TFLite 重算出来的 min/max **必然等于** QAT 范围 → scale 是紧的 → QAT 适配过的权重完整迁移过去。

> *"replace each trained FakeQuantize with `torch.clamp` to that fake-quant's exact learned [lo,hi] range... so TFLite's representative-dataset min/max == the tight QAT range → tight scale → the QAT-adapted weights transfer and no collapse."*

**只要导出链路里存在一次"重新统计 min/max"的环节，QAT 的成果就可能被丢弃。** 这对任何 PyTorch → ONNX → TFLite → Vela 的流水线都成立。

### 降维层也应该参与量化感知

旧方案的 128 维是训练完之后做 PCA 降维得到的。新方案里，512→128 的投影是一个**可训练的 Linear 层，参与 QAT，并且它的 128D 输出也被 FakeQuantize**。用 PCA 组合做 warm start，起点就有 PCA-float 的质量，再在 QAT 里继续优化。

事后 PCA 的问题在于：它产生的那一层从来没被量化训练见过。

### 量化损失优先吃掉难样本

float → int8 的实测代价：

| 数据集 | float | int8 | 变化 |
|---|---|---|---|
| LFW（正脸为主） | 99.48% | 99.33% | **−0.15 pt** |
| CFP-FP（正侧脸） | 96.44% | 94.26% | **−2.18 pt** |

**侧脸的掉幅是正脸的 14 倍。** 量化损失不是均匀分布的，难样本先崩。只用简单样本集验证量化质量，会严重高估模型的实际表现。

---

## 五、Ethos-U55 内存模型的另外两个反直觉点

除了前面说的"arena 峰值由骨干中间张量决定"，还有两条。

### SRAM 是台阶函数，不是连续的

调 width 系数时的实测：

| width | SRAM KiB | Flash KiB | MMAC |
|---|---:|---:|---:|
| 1.00 | 599.84 | 1056.22 | 233.8 |
| 1.05 | **599.84** | 1263.44 | 247.6 |
| 1.10 | 702.09 | 1400.61 | 293.6 |
| 1.15 | **702.09** | 1410.91 | 293.9 |
| 1.20 | 754.12 | 1609.45 | 344.2 |

**width 1.00 和 1.05 落在同一档，1.10 和 1.15 也落在同一档。** 通道数向上取整到同样的对齐边界，加宽 5% 和加宽 15% 的内存开销完全一样。

含义：**如果决定加宽模型，就应该加到台阶边缘。** width 1.10 和 1.15 花同样的 SRAM，选 1.10 是纯浪费容量。

（附带一个结果：width 1.15 重训后精度反而大幅下降，根因是沿用了 width 1.0 的训练配方 —— 属于训练侧问题，不是平台特性，此处不展开。）

### Flash 与 SRAM 完全解耦

`w600k_projected_128d` 的 flash 是 3199 KiB，`w600k_tail48_dense128` 是 1979 KiB，差 1.6 倍，两者 SRAM 完全相同。

排布模型时，flash 预留和 arena 预留要分开算。本项目的 flash 布局：

| 区间 | 内容 | 预留 | 实测 |
|---|---|---|---|
| 0x000000 – 0x200000 | 固件 | 2 MB | — |
| 0x400000 – 0x4B4000 | SCRFD（检测） | 720 KiB | 647.6 KiB |
| 0x510000 – 0x64C000 | MobileFaceNet（识别） | 1264 KiB | 1020.3 KiB |

---

## 六、Vela 编译器：能调的和调不动的

在同一个模型上把参数扫了一遍：

| 参数 | SRAM KiB | 结果 |
|---|---:|---|
| 默认（`--optimise Performance`） | 882.00 | 基准 |
| `--optimise Size` | 849.62 | 省 3.7% |
| Size + Greedy 分配器 | 849.62 | 与 Size 相同 |
| Size + HillClimb 分配器 | 849.62 | 与 Size 相同 |
| Size + **LinearAlloc** 分配器 | **11711.75** | **炸到 11.4 MiB，13 倍** |
| `--arena-cache-size` 三个档位 | 849.62 | **全部无效** |

**编译器旋钮最多省 3.7%。** 真正把 SRAM 从 1176 KiB 降到 597 KiB 的是换骨干、原生 128D 输出、width 1.0 —— 架构层面的决策。

生产用的编译命令最后就三个参数：

```bash
vela model.int8.tflite \
  --accelerator-config ethos-u55-64 \
  --optimise Performance \
  --output-dir out/
```

另外记录两个编译实质失败的情况（summary CSV 全 0）：int8/int16 混合精度尝试；对已经 Vela 过的模型再跑一次 Vela。后者容易在自动化脚本里误触发。

还有一个直接的量化选择结论：**把激活换成 int16 会让 arena 精确 ×4**（596 KiB → 2352 KiB），在这个预算下没有可行性。

---

## 七、量化后的验证：Vela 模型在 PC 上跑不了

Vela 编译后的 tflite 带 `ethos-u` custom op，**PC 的 TFLite 解释器无法执行**。验证流程必须拆成三段：

| 验证目标 | 用什么 |
|---|---|
| 精度 | pre-Vela 的 int8 模型，PC 上跑 |
| 内存 / NPU 覆盖率 | Vela summary CSV |
| 两者是否一致 | 端上 embedding 与 PC embedding 的余弦相似度，要求 ≥ 0.99 |

定版模型这个一致性数字是 **0.99**，所以 Vela summary 里的内存数字和 PC 上测的精度数字可以放在一起用。

### QAT 之后的模型不会自动更好，要逐分支验

SCRFD 的 QAT v5 版本里，stride-32 的 score 分支输出恒为 0.000。查 tensor 元数据：

```
shape=[50,1], dtype=int8, quant=(7.84313680668447e-09, 0)
```

**scale = 7.8e-09** —— int8 全域 ±128 对应的实际范围只有 ±1e-6，这一支被彻底压扁了。

在人脸裁剪的测试集上完全看不出来，因为 stride 16 承担了 87% 的检出。但大脸或低分辨率场景会直接失效。

多分支输出的模型，量化后要逐分支检查 tensor 的 scale。一个异常小的 scale 就意味着这条分支已经死了。

### 判读多 stride 输出

同一个生产模型的分 stride 平均最高分：

| 模型 | s8 | s16 | s32 |
|---|---:|---:|---:|
| PTQ INT8 生产 | 0.141 | **0.870** | 0.039 |

对人脸裁剪这类输入，s8 和 s32 低是正常的。**判读时看最好的那个 stride，而不是要求所有 stride 都高。**

### float vs int8 相关性作为质量门

硬门槛定的是 `mean cosine similarity(float32, int8) ≥ 0.98`。实测的 SCRFD QAT v5：

| 配对 | Scores s8 | Scores s16 | BBox | KPS |
|---|---|---|---|---|
| nosigmoid float vs int8 | 0.9958 | 0.9990 | 0.9968–0.99997 | 0.9961–0.99975 |
| sigmoid float vs int8 | 0.9972 | 0.9993 | 0.9982–0.99996 | 0.9981–0.99977 |

---

## 八、固件侧的平台特性

模型定版不等于能用。以下几条是这个硬件/SDK 组合固有的。

### AllocateTensors 会阻塞 CPU 超过看门狗超时

MobileFaceNet 第一次 `AllocateTensors` 冷加载阻塞 CPU **超过 6 秒**，而硬件看门狗的超时正好是 6 秒 —— 一进人脸模式就复位，表现为无限重启循环。

解法是在两次 AllocateTensors 前后关闭再打开 WDT。

（前面提到 GhostFaceNet 被放弃的第二条原因也是这个 —— tensor graph 与固件不匹配导致 AllocateTensors 挂住，同样触发这个复位循环。）

**在这类 MCU 上加载大模型，要预期 AllocateTensors 是秒级阻塞操作**，任何看门狗、心跳、通信超时都要把它算进去。

### 模型 arena 与 SDK heap 的物理重叠

设备上多个模型切换（人脸模式 ↔ YOLO 模式）时出现野指针 HardFault。

根因：人脸的 interpreter 是函数局部 static（建一次就缓存），它的 arena 与 YOLO 使用的 elHeap bump 区**物理地址重叠**。YOLO 的 `set_model` 里的 memset 直接覆盖了人脸的 arena。

第一次修：改成每次进人脸模式用 placement-new 重建 interpreter。

第二次修才发现更深的问题：**连旧对象的析构都不能调** —— 析构过程会去读已经被 YOLO 数据覆盖的 `subgraph_allocations`，触发一个野的 `registration->free()` 调用。

多模型共享内存区的场景下，"重建对象"这个动作本身也可能踩到已被污染的内存。

### Ethos-U 的缓存一致性守则

来自 SDK 配置文件的注释：

> *"The production default cleans the CPU-written input and invalidates the NPU-written output only. Do not invalidate the full tensor arena before Invoke(): TFLM may have prepared dirty CPU-side arena state that the NPU still needs to read."*

推理前只 clean CPU 写入的 input，invalidate NPU 写出的 output。**不要对整个 tensor arena 做 invalidate** —— TFLM 可能在 arena 里准备了 NPU 还要读的 CPU 侧脏数据。

### PC 与设备的 int8 输入换算必须逐位对齐

模型输入的量化参数是 `scale=0.007843137, zp=-1`，对应的归一化是 `pixel/127.5 - 1`。固件对这个组合走了一条整数快路径：

```c
int val = pixel > 128 ? (int)pixel - 128 : (int)pixel - 129;
```

早期固件写的是简单的 `pixel - 128`，与 PC 侧的 `round(pixel - 128.5)` 差半个量化步长，导致 **PC 和设备的 embedding 有约 8% 的余弦失配**。

做端上验证之前，先确认 PC 仿真路径和固件预处理逐位一致，否则后面所有的对比数字都不可信。

---

## 九、最终配置

一条完整的人脸识别流水线，全部装进 840 KiB SRAM：

```
640×480 YUV422P ──resize──▶ 160×160 RGB ──▶ [SCRFD]  200.83 KiB / 3.97 ms
                                                │
                                          bbox + 5 landmark
                                                │
      原始 640×480 帧 ──5点相似变换 warp──▶ 112×112 RGB
                                                │
                                                ▼
                                        [MobileFaceNet]  596.84 KiB / 20.19 ms
                                                │
                                        128D int8 ──L2 norm──▶ 余弦比对（阈值 0.30）

SRAM 合计 840 KiB          Flash 合计 1.63 MB          NPU 推理 24 ms          CPU 算子 0
```

| 项 | SCRFD | MobileFaceNet |
|---|---|---|
| 输入 | 160×160×3 INT8 | 112×112×3 INT8 |
| SRAM | 200.83 KiB | 596.84 KiB |
| Flash | 632.66 KiB | 1007.86 KiB |
| 推理 | 3.97 ms | 20.19 ms |
| nn_macs | 45.9 M | 221.0 M |
| passes | 66 | 129 |
| **CPU 算子** | **0** | **0** |
| NPU cycles 占比 | 100% | 99.51% |
| 烧录地址 | 0x400000 | 0x510000 |

arena 合计 840 KiB，headroom 160 KiB，摄像头流不需要停。纯 NPU 推理约 24 ms。

精度：LFW **99.33%**，CFP-FP **94.26%**，impostor 相似度均值 **0.005**，端上陌生人相似度 **≈0.045**，部署阈值 **0.30**。

**模型 I/O 契约**：

| 项 | 值 |
|---|---|
| 输入 | `[1,112,112,3]` RGB int8，`scale=0.007843137, zp=-1` |
| 输出 | `[1,128]` int8，`scale=0.012660, zp=18` |
| 归一化 | [−1,1] |
| 后处理 | L2 normalize |

输入的量化参数与旧模型完全一致，所以新模型是 drop-in 替换，固件预处理代码不用改。

**生产流水线**：

```
distill_mfn.py            ResNet100 teacher → MobileFaceNet 512D student
      │                   (ArcFace s=64.0 m=0.5, λ_distill=5.0, 4×GPU DDP, 4 epoch)
      ▼
train_128d.py             512D → 128D 头（可训练 Linear，PCA warm start，参与 QAT）
      │                   (30 epoch, lr 1e-4, loss = (1-cos) + 0.1·MSE)
      ▼
qat_finetune_if.py        QAT 精调（自蒸馏，per-tensor int8 FakeQuantize）
      │                   (8 epoch, lr 1e-4, observer 校准 25 batch 后冻结 range)
      │                   LeakyReLU → ReLU6 替换在这一步
      ▼
export_qat_128d_clamp.py  FakeQuantize → torch.clamp 到学到的 [lo,hi] → ONNX
      ▼
quantize_and_vela.py      ONNX → onnx2tf → TFLite INT8（500 张校准图）
                          → vela --accelerator-config ethos-u55-64 --optimise Performance
```

---

## 十、可复用的检查清单

| 场景 | 检查什么 |
|---|---|
| 评估模型能否上 Ethos-U | 看 Vela 报的 **arena 峰值**，不看输出维度或参数量 |
| 想减内存 | 换骨干 / 原生低维输出，不要在输出端加降维层 |
| 决定加宽模型 | 先跑 width 探针看 SRAM 落在哪个**台阶**，加就加到台阶边缘 |
| 选架构 | 看 **passes 数**和 cycles 的比例，比 MAC 数更能反映 NPU 效率 |
| 量化方案 | 激活是 **per-tensor**，用有界激活（ReLU6）；int16 激活会让 arena ×4 |
| QAT 配置 | FakeQuantize 配成 per-tensor affine int8，对齐目标硬件 |
| 导出链路 | 检查是否存在"重新统计 min/max"的环节，有就用 **clamp** 固化 QAT 范围 |
| 降维层 | 让它参与 QAT，不要事后做 PCA |
| **人脸识别选型** | **看 impostor 分布和 FAR，不要用准确率判断能否上线** |
| **验证量化质量** | 用**难样本集**（侧脸/低质量），简单样本集会高估 |
| **注册与识别** | 必须同一成像域 —— 用设备摄像头现场注册，不要用手机照片 |
| 多分支模型量化后 | 逐分支检查 tensor 的 **scale**，异常小的 scale = 该分支已死 |
| 端上验证 | 先对齐 PC/固件的预处理逐位一致，再比 embedding 余弦（≥0.99） |
| 加载大模型 | AllocateTensors 是**秒级阻塞**，看门狗/心跳/超时都要算进去 |
| 多模型共享内存 | 确认 arena 与 SDK heap 不重叠；重建对象时注意旧对象可能已被污染 |
| Ethos-U 缓存维护 | 只 clean input / invalidate output，**不要 invalidate 整个 arena** |
| Vela 调参 | 编译器旋钮最多省 3–4%，省内存靠改架构；避开 LinearAlloc 分配器 |

---

## 关于模型来源与许可

本文记录的工程工作是**让这条流水线在 Ethos-U55 上跑起来** —— 量化方案的选择、内存布局的推敲、导出链路的修补、固件侧的集成。这些是本项目自己的贡献。

模型的上游来源需要说清楚：人脸检测用的是 [InsightFace](https://github.com/deepinsight/insightface) 的 SCRFD；embedding 模型是从 InsightFace 的 `glint360k_r100` 蒸馏得到的。上游声明这些模型**仅限非商业研究用途**。

开源仓库据此做了许可拆分：**代码、脚本、文档采用 Apache-2.0；模型权重沿用上游限制**。逐文件的血缘说明见仓库里的 `MODEL_LICENSE.md`。

训练流水线本身与 teacher、数据集无关 —— 换成许可宽松的输入重跑，就能得到不带此限制的权重。本文讨论的所有平台侧结论（内存模型、量化陷阱、Vela 行为、固件集成）与具体用了哪个 teacher 无关，换模型一样成立。

---

## 参考

- [Grove Vision AI Module V2 产品页](https://www.seeedstudio.com/Grove-Vision-AI-Module-V2-p-5851.html) / [Wiki](https://wiki.seeedstudio.com/grove_vision_ai_v2/)
- [Himax WiseEye2 AI Processor 官方页](https://www.himax.com.tw/products/wiseeye-ai-sensing/wiseeye2-ai-processor/)
- [Grove Vision AI V2 功耗实测（Core Electronics）](https://core-electronics.com.au/guides/getting-started-with-the-grove-vision-ai-v2-power-efficient-object-detection/)
- [智慧仓储管理解决方案](https://www.seeed.cc/solutions/smart-warehouse-management)
