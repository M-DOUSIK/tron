import tensorflow as tf
import sys

model_path = sys.argv[1]

print("=" * 60)
print("MODEL:", model_path)
print("=" * 60)

interpreter = tf.lite.Interpreter(model_path=model_path)
interpreter.allocate_tensors()

inputs = interpreter.get_input_details()
outputs = interpreter.get_output_details()

print("\nINPUT")
print("-" * 60)

for x in inputs:
    print("Name:", x["name"])
    print("Shape:", x["shape"])
    print("Dtype:", x["dtype"])
    print("Quantization:", x["quantization"])

print("\nOUTPUT")
print("-" * 60)

for x in outputs:
    print("Name:", x["name"])
    print("Shape:", x["shape"])
    print("Dtype:", x["dtype"])
    print("Quantization:", x["quantization"])

print("\nOPERATORS")
print("-" * 60)

for x in interpreter._get_ops_details():
    print(x["op_name"])

print("\n" + "=" * 60)
print("INSPECTION COMPLETE")
print("=" * 60)