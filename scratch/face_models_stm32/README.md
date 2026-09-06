# STM32N6 Face Recognition Models

## Overview

This package contains two INT8 TensorFlow Lite models for an embedding-based face recognition system designed and tested for STM32N6 compatibility.

The recognition pipeline is:

Image
→ Face Detection
→ Face Alignment
→ Face Embedding
→ Cosine Similarity Comparison
→ Match / No Match

---

# 1. Face Detector

## File

`face_detector_int8.tflite`

## Model

SCRFD face detector.

## Input

- Shape: `1 × 160 × 160 × 3`
- Format: RGB
- Data type: `INT8`
- Quantization scale: `0.003921568859368563`
- Zero point: `-128`

## Output

The model produces predictions at three detection scales.

Outputs include:

- Face confidence scores
- Face bounding boxes
- Five facial landmarks

Output tensor sizes:

- Confidence: `800 × 1`, `200 × 1`, `50 × 1`
- Bounding boxes: `800 × 4`, `200 × 4`, `50 × 4`
- Facial landmarks: `800 × 10`, `200 × 10`, `50 × 10`

The five landmarks are used for face alignment before sending the face to the embedding model.

---

# 2. Face Embedding Model

## File

`face_embedding_int8.tflite`

## Model

MobileFaceNet-style 128-dimensional face embedding model.

## Input

- Shape: `1 × 112 × 112 × 3`
- Format: RGB
- Data type: `INT8`
- Quantization scale: `0.007843137718737125`
- Zero point: `-1`

## Preprocessing

The detected face is:

1. Detected using the SCRFD face detector.
2. Aligned using the detected facial landmarks.
3. Cropped to the face region.
4. Resized to `112 × 112`.
5. Converted to the required INT8 input format.

The input normalization used is approximately:

`pixel = (pixel / 127.5) - 1.0`

The normalized values are then quantized using the input scale and zero point.

## Output

- Shape: `1 × 128`
- Data type: `INT8`
- Quantization scale: `0.012659978121519089`
- Zero point: `18`

The INT8 output is dequantized using:

`real_value = scale × (quantized_value - zero_point)`

The resulting 128-dimensional embedding should be L2-normalized before comparison.

---

# Recognition Method

During enrollment:

1. Capture a clear face image.
2. Detect and align the face.
3. Generate a 128-dimensional embedding.
4. Store the embedding.

During recognition:

1. Capture a new face image.
2. Detect and align the face.
3. Generate a new embedding.
4. Compare it with stored embeddings using cosine similarity.

A higher cosine similarity indicates that the faces are more likely to belong to the same person.

---

# Validation

The models were tested using the complete pipeline:

Face Image
→ SCRFD Detection
→ Landmark-Based Alignment
→ INT8 Embedding Model
→ Cosine Similarity

Testing confirmed that:

- Images of the same person produced a successful match.
- Images of different people were classified as different.

An example same-person test produced a cosine similarity of:

`0.785453`

The reference pipeline recommends a cosine similarity threshold of approximately:

`0.30`

The final threshold should be validated and adjusted using images captured by the actual target camera.

---

# STM32N6 Compatibility

Both models were analyzed using:

ST Edge AI Core v4.0.1

Target:

`stm32n6`

Both models completed the STM32N6 analysis successfully.

The models included in this package are the standard INT8 TFLite versions and not the Ethos-U Vela versions.

---

# Files

- `face_detector_int8.tflite`
- `face_embedding_int8.tflite`
- `README.md`