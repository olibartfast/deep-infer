# DeepStream Getting Started Guide

## Quick Start

This guide will help you get started with DeepStream Inference Lab.

### 1. Install Prerequisites

```bash
# Install DeepStream SDK
wget https://developer.download.nvidia.com/compute/deepstream/7.1/DeepStream_7.1.tar.gz
tar -xvf DeepStream_7.1.tar.gz
cd deepstream_sdk_v7.1_x86_64
sudo ./install.sh

# Install dependencies
sudo apt update
sudo apt install -y \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    build-essential \
    cmake \
    ninja-build
```

### 2. Build the Project

```bash
git clone https://github.com/olibartfast/deepstream-infer-lab.git
cd deepstream-infer-lab
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
ninja
```

### 3. Prepare Your Model

#### Convert ONNX to TensorRT Engine (Recommended)

```bash
# Using trtexec from TensorRT
/usr/src/tensorrt/bin/trtexec \
    --onnx=yolov8n.onnx \
    --saveEngine=yolov8n.engine \
    --fp16
```

#### Or Use ONNX Directly

DeepStream will automatically convert ONNX to TensorRT engine on first run (slower initial startup).

### 4. Create DeepStream Config File

See `configs/yolov8_config.txt` for a complete example. Key parameters:

```ini
[property]
gpu-id=0
onnx-file=../models/yolov8n.onnx
model-engine-file=../models/yolov8n.engine
labelfile-path=../data/labels/coco.names
batch-size=1
num-detected-classes=80

[class-attrs-all]
nms-iou-threshold=0.45
pre-cluster-threshold=0.25
```

### 5. Run Inference

```bash
./build/deepstream-infer-lab \
    --source=../data/videos/sample.mp4 \
    --config=../configs/yolov8_config.txt \
    --model_type=yolov8 \
    --labels=../data/labels/coco.names \
    --verbose
```

## Common Use Cases

### Video File Processing

```bash
./build/deepstream-infer-lab \
    -s /path/to/video.mp4 \
    -c configs/yolov8_config.txt \
    -mt yolov8 \
    -l data/labels/coco.names
```

### RTSP Stream

```bash
./build/deepstream-infer-lab \
    -s rtsp://192.168.1.100:8554/stream \
    -c configs/yolov8_config.txt \
    -mt yolov8 \
    --tracker \
    --show
```

### USB Camera

```bash
./build/deepstream-infer-lab \
    -s /dev/video0 \
    -c configs/yolov8_config.txt \
    -mt yolov8 \
    --show
```

### Multiple Streams

Configure multiple sources in the config file or use DeepStream's multi-stream capabilities.

## Performance Optimization

### 1. Use TensorRT FP16

```bash
/usr/src/tensorrt/bin/trtexec \
    --onnx=model.onnx \
    --saveEngine=model_fp16.engine \
    --fp16
```

### 2. Batch Multiple Streams

```ini
[property]
batch-size=4
```

### 3. Reduce Inference Interval

```ini
[property]
interval=2  # Inference every 2 frames
```

### 4. Optimize Decode Surfaces

Adjust based on available GPU memory:
```bash
--num_decode_surfaces=16
```

## Troubleshooting

### Issue: "GStreamer plugin not found"

```bash
# Check GStreamer installation
gst-inspect-1.0 nvstreammux

# If missing, reinstall DeepStream
sudo ./install.sh
```

### Issue: "Failed to create TensorRT engine"

- Check ONNX model compatibility
- Verify TensorRT version
- Try generating engine manually with trtexec

### Issue: "Out of GPU memory"

- Reduce batch size
- Use smaller model
- Enable FP16 mode

## Next Steps

- [Model Deployment Guide](ModelDeployment.md)
- [DeepStream Configuration Reference](DeepStreamConfig.md)
- [Tracker Configuration](TrackerSetup.md)
- [Analytics Setup](AnalyticsGuide.md)
