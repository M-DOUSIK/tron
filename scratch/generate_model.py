import tensorflow as tf
import numpy as np

# Create a trivial sequential model
model = tf.keras.Sequential([
    tf.keras.layers.InputLayer(input_shape=(28, 28, 1)),
    tf.keras.layers.Conv2D(8, (3, 3), activation='relu'),
    tf.keras.layers.Flatten(),
    tf.keras.layers.Dense(10)
])

# Convert the model to TFLite with int8 quantization
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
def representative_dataset():
    for _ in range(100):
      data = np.random.rand(1, 28, 28, 1)
      yield [data.astype(np.float32)]

converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

tflite_quant_model = converter.convert()

with open('trivial_int8.tflite', 'wb') as f:
    f.write(tflite_quant_model)
print('Generated trivial_int8.tflite')
