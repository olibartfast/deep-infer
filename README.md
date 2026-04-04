![DeepStream Inference Lab](docs/assets/deepstream-banner.svg)

# DeepStream Inference Lab - Computer Vision with NVIDIA DeepStream

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A C++ application for computer vision tasks (object detection, classification, instance segmentation, optical flow) using NVIDIA DeepStream SDK. DeepStream provides a complete streaming analytics toolkit for AI-based video and image understanding with optimized performance on NVIDIA GPUs.

## Table of Contents
- [Overview](#overview)
- [Key Differences from Triton](#key-differences-from-triton)
- [Project Structure](#project-structure)
- [Tested Models](#tested-models)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Configuration](#configuration)
- [Running Inference](#running-inference)
- [Docker Support](#docker-support)
- [Performance Tips](#performance-tips)
- [Troubleshooting](#troubleshooting)
- [References](#references)

## Overview

DeepStream SDK is NVIDIA's complete streaming analytics toolkit for AI-based multi-sensor processing, video, image, and audio understanding. This project provides a C++ framework for deploying computer vision models using DeepStream's optimized pipeline.

### Key Features

- **High Performance**: Leverages DeepStream's optimized GStreamer pipeline and TensorRT for maximum throughput
- **Multi-Stream Support**: Process multiple video streams simultaneously
- **Flexible Input Sources**: Support for video files, RTSP streams, USB cameras, and image sequences
- **Built-in Tracker**: Optional object tracking across frames
- **Analytics Module**: DeepStream analytics for line crossing, ROI counting, etc.
- **Low Latency**: Optimized for real-time video analytics
- **GPU Acceleration**: Full GPU pipeline from decode to inference to rendering

## Key Differences from Triton

While both DeepStream and Triton Server are NVIDIA inference solutions, they serve different purposes:

| Feature | DeepStream | Triton Server |
|---------|------------|---------------|
| **Primary Use Case** | Video streaming analytics | Model serving (any data type) |
| **Pipeline** | GStreamer-based | Request/Response based |
| **Video Decode** | Hardware-accelerated (NVDEC) | Not included |
| **Tracker** | Built-in multi-object tracker | Not included |
| **Analytics** | Spatial analytics, line crossing, etc. | Not included |
| **Best For** | Video surveillance, smart cities, traffic | ML model deployment, microservices |
| **Client Type** | GStreamer pipeline | HTTP/gRPC clients |
| **Throughput** | Optimized for video streams | Optimized for batch inference |

**When to use DeepStream:**
- Processing video streams (RTSP, files, cameras)
- Real-time video analytics applications
- Need tracking across frames
- Require spatial analytics (line crossing, ROI)
- Want end-to-end GPU pipeline

**When to use Triton:**
- General purpose model serving
- Non-video inference workloads
- Microservices architecture
- Need framework flexibility (TensorFlow, PyTorch, ONNX, etc.)
- REST/gRPC API requirements
## Models

Supported models are provided by the [vision-core](https://github.com/olibartfast/vision-core) library, which is automatically fetched at build time. Please refer to that repository for the full and up-to-date list of supported architectures.

## Prerequisites

### System Requirements
- Ubuntu 20.04 / 22.04 / 24.04
- NVIDIA GPU (Tesla T4, RTX/Ampere/Hopper/Blackwell or better) or Jetson (JetPack 6+)
- NVIDIA Driver **>= 570.133.20** (desktop/server) or JetPack-provided driver on Jetson
- CUDA **>= 12.8** (Jetson uses JetPack CUDA)
- TensorRT **>= 10.9.0.34**

### Software Dependencies

1. **NVIDIA DeepStream SDK**:
```bash
# Download deepstream-8.0_8.0.0-1_amd64.deb from NVIDIA NGC:
# https://catalog.ngc.nvidia.com/orgs/nvidia/resources/deepstream
sudo apt-get install ./deepstream-8.0_8.0.0-1_amd64.deb

# Or using the tar archive:
sudo tar -xvf deepstream_sdk_v8.0.0_x86_64.tbz2 -C /
cd /opt/nvidia/deepstream/deepstream-8.0
sudo ./install.sh
sudo ldconfig

# Jetson (L4T) example – uses DeepStream 6.4 container/JetPack package:
# sudo tar -xvf deepstream_sdk_v6.4.0_aarch64.tbz2 -C /
# cd /opt/nvidia/deepstream/deepstream-6.4 && sudo ./install.sh && sudo ldconfig
```

2. **OpenCV 4**:
```bash
sudo apt install libopencv-dev
```

3. **GStreamer**:
```bash
sudo apt install \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools
```

> **Note:** DeepStream 8.0 removes some open-source codec libraries from the container. If you see GStreamer plugin warnings, reinstall them with:
> ```bash
> sudo apt-get install --reinstall libflac8 libmp3lame0 libxvidcore4 ffmpeg
> ```

4. **CMake and Build Tools**:
```bash
sudo apt install build-essential cmake ninja-build pkg-config
```

5. **Fetched Dependencies**:
   - **vision-core**: Automatically fetched via CMake during build from [https://github.com/olibartfast/vision-core](https://github.com/olibartfast/vision-core).

## Installation

### Build from Source

1. Clone the repository:
```bash
git clone https://github.com/olibartfast/deepstream-infer-lab.git
cd deepstream-infer-lab
```

2. Create build directory:
```bash
mkdir build && cd build
```

3. Configure with CMake:
```bash
# DS 8.0 uses a versioned install directory — pass it explicitly:
cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
      -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-8.0 ..

# Jetson (L4T) DeepStream path example:
# cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
#       -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-6.4 ..
```

Optional flags:
- `-DWITH_SHOW_FRAME=ON`: Enable frame display
- `-DWITH_WRITE_FRAME=ON`: Enable frame writing (default: ON)

4. Build:
```bash
ninja
```

### Docker Installation

```bash
# Build Docker image (uses nvcr.io/nvidia/deepstream:8.0-gc-triton-devel as base)
./scripts/docker/build_docker.sh

# Or manually
docker build -t deepstream-infer-lab .
```

## Configuration

DeepStream uses configuration files to define the inference pipeline. Example config file structure:

```ini
[property]
gpu-id=0
net-scale-factor=0.0039215697906911373
model-color-format=0
onnx-file=../models/yolov8n.onnx
model-engine-file=../models/yolov8n.engine
labelfile-path=../data/labels/coco.names
batch-size=1
network-mode=0
num-detected-classes=80
interval=0
gie-unique-id=1
process-mode=1
network-type=0
cluster-mode=2
maintain-aspect-ratio=1
parse-bbox-func-name=NvDsInferParseYolo
custom-lib-path=/opt/nvidia/deepstream/deepstream-8.0/lib/libnvds_infercustomparser.so

[class-attrs-all]
nms-iou-threshold=0.45
pre-cluster-threshold=0.25
topk=300
```

See `configs/` directory for examples.

## Running Inference

### Command Line

```bash
./build/deepstream-infer-lab \
    --source=/path/to/video.mp4 \
    --config=/path/to/config.txt \
    --model_type=yolov8 \
    --labels=/path/to/labels.txt \
    --output=/path/to/output \
    --verbose
```

### With Tracker

```bash
./build/deepstream-infer-lab \
    --source=/path/to/video.mp4 \
    --config=/path/to/config.txt \
    --model_type=yolov8 \
    --tracker /opt/nvidia/deepstream/deepstream-8.0/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml \
    --show
```

### RTSP Stream

```bash
./build/deepstream-infer-lab \
    --source=rtsp://camera-ip:8554/stream \
    --config=/path/to/config.txt \
    --model_type=yolov8 \
    --tracker \
    --analytics
```

### Options

| Option | Description |
|--------|-------------|
| `-s, --source` | Input source (video file, image, RTSP stream) |
| `-c, --config` | DeepStream config file path |
| `-mt, --model_type` | Model type (yolov5, yolov8, etc.) |
| `-l, --labels` | Path to labels file |
| `-o, --output` | Output path for results |
| `--confidence` | Confidence threshold (default: 0.5) |
| `--nms` | NMS threshold (default: 0.4) |
| `-g, --gpu` | GPU device ID (default: 0) |
| `--tracker` | Enable tracker (optional config file) |
| `--analytics` | Enable DeepStream analytics |
| `--show` | Display output frames |
| `--no-write` | Disable writing output |
| `-v, --verbose` | Enable verbose logging |

## Docker Support

This project uses `nvcr.io/nvidia/deepstream:8.0-gc-triton-devel` as its base image. This container includes the Graph Composer tools, Triton Inference Server backends, and the full DeepStream 8.0 development SDK.

### Build Docker Image

```bash
./scripts/docker/build_docker.sh [deepstream_version]
```

#### Jetson (L4T) container builds

The build script auto-detects `aarch64` and switches to the DeepStream L4T base. For Jetson, use the Jetson DeepStream tag (for example 6.4):

```bash
# Build on a Jetson device
./scripts/docker/build_docker.sh 6.4

# Or override the base image explicitly
BASE_IMAGE=nvcr.io/nvidia/deepstream-l4t:6.4-triton ./scripts/docker/build_docker.sh
```

### Run with Docker

```bash
# Object detection on video
./scripts/docker/run_yolo_detection.sh

# RTSP stream processing
./scripts/docker/run_rtsp_stream.sh rtsp://camera-ip:8554/stream

# Custom command
docker run --rm --gpus all --privileged \
    -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
    -v ${PWD}/data:/app/data \
    -v ${PWD}/configs:/app/configs \
    -v ${PWD}/models:/app/models \
    -v ${PWD}/output:/app/output \
    deepstream-infer-lab:latest \
    --source=/app/data/video.mp4 \
    --config=/app/configs/yolov8_config.txt \
    --model_type=yolov8

# Jetson note: if `--gpus all` is unavailable, use `--runtime nvidia` instead.
```

### Interactive development shell

```bash
xhost +
docker run -it --entrypoint /bin/bash \
    --gpus all --rm --network=host --privileged \
    -e DISPLAY=${DISPLAY} \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    deepstream-infer-lab:latest
```

### Dev Container (VS Code)

A `.devcontainer/devcontainer.json` is provided for development inside VS Code using the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).

The container uses `nvcr.io/nvidia/deepstream:8.0-gc-triton-devel` directly as its base (no build step needed) and mounts your workspace into `/app` with GPU passthrough, X11 display, and all required `NVIDIA_DRIVER_CAPABILITIES` pre-configured.

To get started, open the repository in VS Code and select **Reopen in Container** when prompted, or run it manually from the Command Palette:

```
Dev Containers: Reopen in Container
```

CMake Tools is pre-configured to build in Debug mode against the DeepStream 8.0 headers automatically.

## Performance Tips

1. **Use TensorRT Engines**: Pre-generate TensorRT FP16 engines for best performance (INT8 support for TAO models removed in DS 8.0)
2. **Batch Processing**: Increase batch size for multiple streams
3. **GPU Memory**: Monitor GPU memory usage with `nvidia-smi`
4. **Decode Surfaces**: Adjust `num-decode-surfaces` for better throughput
5. **Network Mode**: Use FP16 (`network-mode=2`) for faster inference
6. **Tracker**: NvDCF tracker provides best accuracy, IOU tracker is faster

## Troubleshooting

### Common Issues

**Pipeline fails to start:**
- Check DeepStream installation: `deepstream-app --version`
- Verify GPU driver >= 570.133.20: `nvidia-smi`
- Check config file paths — note DS 8.0 uses `/opt/nvidia/deepstream/deepstream-8.0/`

**GStreamer codec warnings:**
- DS 8.0 removed some open-source codec libraries. Run:
  ```bash
  sudo apt-get install --reinstall libflac8 libmp3lame0 libxvidcore4 ffmpeg
  ```

**Low FPS:**
- Use TensorRT FP16 engine instead of ONNX
- Reduce input resolution
- Adjust batch size

**Out of memory:**
- Reduce batch size
- Lower `num-decode-surfaces`
- Use smaller model

**No display:**
- Install display dependencies: `apt install libgstreamer1.0-0 gstreamer1.0-plugins-base`
- Check X11 forwarding for SSH: `export DISPLAY=:0`
- When running in Docker, mount the X11 socket: `-v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=${DISPLAY}`

## References

- [NVIDIA DeepStream SDK](https://developer.nvidia.com/deepstream-sdk)
- [DeepStream 8.0 NGC Container](https://catalog.ngc.nvidia.com/orgs/nvidia/containers/deepstream)
- [DeepStream Documentation](https://docs.nvidia.com/metropolis/deepstream/dev-guide/)
- [DeepStream Python API](https://github.com/NVIDIA-AI-IOT/deepstream_python_apps)
- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [TensorRT Documentation](https://docs.nvidia.com/deeplearning/tensorrt/)

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Feedback

Any feedback is greatly appreciated. If you have suggestions, bug reports, or questions, please open an [issue](https://github.com/olibartfast/deepstream-infer-lab/issues).
