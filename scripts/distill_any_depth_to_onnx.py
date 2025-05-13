"""
This script exports Hugging Face models for depth estimation to ONNX format.

- It uses the transformers library to load the models and export them using
PyTorch's ONNX export functionality.
- The script iterates through a list of model names, loads each model and its
corresponding image processor, creates a dummy input tensor, and exports the
model to ONNX format.
- The exported ONNX models are saved with names derived from the original model names.
"""
import os
from transformers import AutoModelForDepthEstimation, AutoImageProcessor
import torch
from torch.onnx import export as torch_onnx_export


class Colors:
    """
    ANSI escape sequences for colored terminal output.
    """
    HEADER = '\033[95m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    ENDC = '\033[0m'

# List of model names to export
model_names = [
    "xingyang1/Distill-Any-Depth-Small-hf",
    "xingyang1/Distill-Any-Depth-Large-hf",
]

for model_name in model_names:
    # Define the output ONNX model name
    os.makedirs("models", exist_ok=True)
    onnx_model_name = "models/" + model_name.rsplit("/", maxsplit=1)[-1].lower() + ".onnx"

    if os.path.exists(onnx_model_name):
        print(f"\n{Colors.WARNING}Model {model_name} already exists in"
              f" {onnx_model_name}{Colors.ENDC}")
        continue

    # Load models and processors
    model = AutoModelForDepthEstimation.from_pretrained(model_name)
    processor = AutoImageProcessor.from_pretrained(model_name)

    # Set model to evaluation mode
    model.eval()

    # Create dummy input
    image_size = processor.size # Dictionary with 'height' and 'width'
    h, w = image_size['height'], image_size['width'] # 518, 518
    dummy_input = torch.randn(1, 3, h, w)

    # Export to ONNX
    torch_onnx_export(
        model,
        dummy_input,
        onnx_model_name,
        input_names=["pixel_values"],
        output_names=["predicted_depth"],
        dynamic_axes={
            "pixel_values": {0: "batch_size", 2: "height", 3: "width"},
            "predicted_depth": {0: "batch_size", 1: "height", 2: "width"}
        },
        opset_version=14,
        do_constant_folding=True,
    )

    print(f"\n{Colors.OKGREEN}Model {model_name} successfully exported to"
          f" {onnx_model_name}{Colors.ENDC}")
