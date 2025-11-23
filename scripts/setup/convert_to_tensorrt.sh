#!/bin/bash
# Convert ONNX model to TensorRT engine for DeepStream
# Usage: ./convert_to_tensorrt.sh model.onnx [precision]

set -e

MODEL_PATH=$1
PRECISION=${2:-fp16}

if [ -z "$MODEL_PATH" ]; then
    echo "Usage: $0 <model.onnx> [fp32|fp16|int8]"
    exit 1
fi

if [ ! -f "$MODEL_PATH" ]; then
    echo "Error: Model file not found: $MODEL_PATH"
    exit 1
fi

# Extract model name without extension
MODEL_NAME=$(basename "$MODEL_PATH" .onnx)
ENGINE_PATH="${MODEL_NAME}_${PRECISION}.engine"

echo "Converting ONNX model to TensorRT..."
echo "Input:  $MODEL_PATH"
echo "Output: $ENGINE_PATH"
echo "Precision: $PRECISION"

# Build command based on precision
TRTEXEC_CMD="/usr/src/tensorrt/bin/trtexec \
    --onnx=$MODEL_PATH \
    --saveEngine=$ENGINE_PATH"

case $PRECISION in
    fp16)
        TRTEXEC_CMD="$TRTEXEC_CMD --fp16"
        ;;
    int8)
        TRTEXEC_CMD="$TRTEXEC_CMD --int8"
        echo "Warning: INT8 requires calibration data"
        ;;
    fp32)
        # Default precision, no flag needed
        ;;
    *)
        echo "Error: Invalid precision. Use fp32, fp16, or int8"
        exit 1
        ;;
esac

# Execute conversion
echo "Running TensorRT conversion..."
$TRTEXEC_CMD

if [ $? -eq 0 ]; then
    echo "✅ Conversion successful!"
    echo "Engine saved to: $ENGINE_PATH"
    
    # Print engine info
    echo ""
    echo "Engine Information:"
    ls -lh "$ENGINE_PATH"
else
    echo "❌ Conversion failed"
    exit 1
fi
