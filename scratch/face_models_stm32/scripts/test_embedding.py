import tensorflow as tf
import numpy as np
from PIL import Image
from pathlib import Path


MODEL_PATH = Path(
    "reference_face_model/qat_distill_v2_relu6_128d/model_128d.int8.tflite"
)


def get_embedding(image_path):
    # Load TFLite model
    interpreter = tf.lite.Interpreter(model_path=str(MODEL_PATH))
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    # Load image and convert to RGB
    image = Image.open(image_path).convert("RGB")
    image = image.resize((112, 112))

    image = np.array(image).astype(np.float32)

    # Normalize from [0,255] to approximately [-1,1]
    image = (image / 127.5) - 1.0

    # Quantize input using model parameters
    scale, zero_point = input_details["quantization"]

    image_int8 = np.round(image / scale + zero_point)
    image_int8 = np.clip(image_int8, -128, 127).astype(np.int8)

    # Add batch dimension
    image_int8 = np.expand_dims(image_int8, axis=0)

    # Run inference
    interpreter.set_tensor(input_details["index"], image_int8)
    interpreter.invoke()

    output = interpreter.get_tensor(output_details["index"])[0]

    # Dequantize output
    output_scale, output_zero_point = output_details["quantization"]

    embedding = output_scale * (
        output.astype(np.float32) - output_zero_point
    )

    # L2 normalize embedding
    embedding = embedding / np.linalg.norm(embedding)

    return embedding


def cosine_similarity(a, b):
    return np.dot(a, b)


tom1 = get_embedding("test_images/tomcruise.jpg")
tom2 = get_embedding("test_images/tomcruise2.jpg")
zendaya = get_embedding("test_images/zendaya.jpg")

same_person = cosine_similarity(tom1, tom2)
different_person = cosine_similarity(tom1, zendaya)

print("\nEmbedding Comparison")
print("-" * 50)

print(f"Tom Cruise vs Tom Cruise: {same_person:.4f}")
print(f"Tom Cruise vs Zendaya:   {different_person:.4f}")