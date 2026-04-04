# DeepStream Getting Started Guide

## Quick Start

This guide will help you get started with DeepStream Inference Lab using DeepStream 8.0.

### Prerequisites

- Ubuntu 20.04 / 22.04 / 24.04 (DS 8.0 container is Ubuntu 24.04-based) or Jetson device (JetPack 6+)
- NVIDIA GPU (Tesla T4, RTX/Ampere/Hopper/Blackwell or better) or Jetson
- NVIDIA Driver **>= 570.133.20** (desktop/server) or JetPack driver on Jetson
- CUDA **>= 12.8** (Jetson uses JetPack CUDA)
- TensorRT **>= 10.9.0.34**

### 1. Install Prerequisites

```bash
# Install DeepStream SDK 8.0 (deb method)
# Download deepstream-8.0_8.0.0-1_amd64.deb from NGC:
# https://catalog.ngc.nvidia.com/orgs/nvidia/resources/deepstream
sudo apt-get install ./deepstream-8.0_8.0.0-1_amd64.deb

# Or tar method:
sudo tar -xvf deepstream_sdk_v8.0.0_x86_64.tbz2 -C /
cd /opt/nvidia/deepstream/deepstream-8.0/
sudo ./install.sh
sudo ldconfig

# Jetson (L4T) example:
# sudo tar -xvf deepstream_sdk_v6.4.0_aarch64.tbz2 -C /
# cd /opt/nvidia/deepstream/deepstream-6.4 && sudo ./install.sh && sudo ldconfig

# Install dependencies
sudo apt update
sudo apt install -y \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    build-essential \
    cmake \
    ninja-build

# DS 8.0 removes some codec libs — reinstall ffmpeg explicitly if needed
sudo apt-get install --reinstall libflac8 libmp3lame0 libxvidcore4 ffmpeg
```

### 2. Build the Project

```bash
git clone https://github.com/olibartfast/deepstream-infer-lab.git
cd deepstream-infer-lab
mkdir build && cd build

# DS 8.0 uses a versioned directory. Pass it explicitly if auto-detection fails.
cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
      -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-8.0 ..
ninja

# Jetson (L4T) example:
# cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
#       -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-6.4 ..
# ninja
```

### 3. Prepare Your Model

#### Convert ONNX to TensorRT Engine (Recommended)

DS 8.0 ships with TensorRT 10.9. Use `trtexec` for best performance:

```bash
/usr/src/tensorrt/bin/trtexec \
    --onnx=yolov8n.onnx \
    --saveEngine=yolov8n.engine \
    --fp16
```

> **Note:** INT8 calibration support for older TAO models has been removed in DS 8.0. FP16 is the recommended precision.

#### Or Use ONNX Directly

DeepStream will automatically convert ONNX to a TensorRT engine on first run (slower initial startup).

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

## Docker Usage (8.0-gc-triton-devel)

The project uses the **`8.0-gc-triton-devel`** container as its base image. This includes the Graph Composer tools, Triton Inference Server backends, and the full DeepStream 8.0 development SDK. On Jetson the build script switches to the DeepStream L4T base (for example `nvcr.io/nvidia/deepstream-l4t:6.4-triton`).

### Pull the base image

```bash
docker pull nvcr.io/nvidia/deepstream:8.0-gc-triton-devel

# Jetson (L4T) base example
docker pull nvcr.io/nvidia/deepstream-l4t:6.4-triton
```

### Build project image

```bash
./scripts/docker/build_docker.sh        # x86_64 default
./scripts/docker/build_docker.sh 6.4    # Jetson example
```

### Run interactively (for development/debugging)

```bash
xhost +
docker run -it --entrypoint /bin/bash \
    --gpus all --rm --network=host --privileged \
    -e DISPLAY=${DISPLAY} \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v /var/run/docker.sock:/var/run/docker.sock \
    deepstream-infer-lab:latest
```

### Run inference

```bash
docker run --rm --gpus all --privileged --network host \
    -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ${PWD}/data:/app/data \
    -v ${PWD}/configs:/app/configs \
    -v ${PWD}/models:/app/models \
    -v ${PWD}/output:/app/output \
    deepstream-infer-lab:latest \
    --source=/app/data/videos/sample.mp4 \
    --config=/app/configs/yolov8_config.txt \
    --model_type=yolov8

# Jetson note: if `--gpus all` is unavailable, use `--runtime nvidia`.
```

## Performance Optimization

### 1. Use TensorRT FP16 (recommended for DS 8.0)

```bash
/usr/src/tensorrt/bin/trtexec \
    --onnx=model.onnx \
    --saveEngine=model_fp16.engine \
    --fp16
```

> INT8 support for previous DeepStream TAO models has been removed in DS 8.0. FP16 is recommended.

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

```bash
--num_decode_surfaces=16
```

## Troubleshooting

### Issue: "GStreamer plugin not found"

```bash
# Check DeepStream GStreamer plugins
gst-inspect-1.0 nvstreammux

# Check DeepStream version
deepstream-app --version

# If missing, re-run the DS install script
sudo /opt/nvidia/deepstream/deepstream-8.0/install.sh
```

### Issue: Missing codec / ffmpeg warnings

DS 8.0 removed some open-source codec libraries. You may see GStreamer warnings like `Failed to load plugin ... libgstfaad.so`. These can usually be safely ignored, or fix with:

```bash
sudo apt-get install --reinstall libflac8 libmp3lame0 libxvidcore4 ffmpeg
```

### Issue: "Failed to create TensorRT engine"

- Verify TensorRT version matches DS 8.0 requirements (TRT >= 10.9)
- Try generating the engine manually with `trtexec`
- Check ONNX model compatibility

### Issue: "Out of GPU memory"

- Reduce batch size
- Use a smaller model
- Enable FP16 mode

## Next Steps

- [Model Deployment Guide](ModelDeployment.md)
- [DeepStream Configuration Reference](DeepStreamConfig.md)
- [Tracker Configuration](TrackerSetup.md)
- [Analytics Setup](AnalyticsGuide.md)
