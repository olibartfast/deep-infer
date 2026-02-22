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

## Project Structure

```
deepstream-infer-lab/
├── src/                          # Source code
│   ├── main/                     # Main application
│   │   └── main.cpp             # Entry point
│   ├── deepstream/               # DeepStream pipeline wrapper
│   │   └── DeepStreamPipeline.cpp
│   ├── tasks/                    # Task implementations
│   │   ├── object_detection/
│   │   ├── classification/
│   │   └── instance_segmentation/
│   └── utils/                    # Utility classes
│       ├── utils.cpp
│       └── ConfigManager.cpp
├── include/                      # Header files
│   ├── common.hpp
│   ├── Config.hpp
│   ├── Logger.hpp
│   ├── TaskInterface.hpp
│   └── DeepStreamPipeline.hpp
├── configs/                      # DeepStream config files
│   ├── yolov8_config.txt
│   ├── yolov5_config.txt
│   └── tracker_config.txt
├── models/                       # TensorRT engine files
├── data/                         # Test data
│   ├── videos/
│   ├── images/
│   └── labels/
├── scripts/                      # Helper scripts
│   ├── docker/                   # Docker scripts
│   │   ├── build_docker.sh
│   │   ├── run_yolo_detection.sh
│   │   └── run_rtsp_stream.sh
│   └── setup/                    # Setup scripts
├── docs/                         # Documentation
│   └── guides/                   # User guides
├── output/                       # Output directory
└── tests/                        # Test files
```

## Tested Models

### Object Detection
- YOLOv5
- YOLOv8
- YOLOv11
- YOLOv12
- RT-DETR
- D-FINE

### Classification
- ResNet
- EfficientNet
- Vision Transformer (ViT)

### Instance Segmentation
- YOLOv8-seg
- YOLOv11-seg

## Prerequisites

### System Requirements
- Ubuntu 20.04/22.04
- NVIDIA GPU (Tesla T4, RTX series, or better)
- NVIDIA Driver >= 525.x
- CUDA >= 12.x
- TensorRT >= 8.6

### Software Dependencies

1. **NVIDIA DeepStream SDK**:
```bash
# Download from NVIDIA NGC
wget https://developer.download.nvidia.com/compute/deepstream/7.1/DeepStream_7.1.tar.gz
tar -xvf DeepStream_7.1.tar.gz
cd deepstream_sdk_v7.1_x86_64
sudo ./install.sh
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
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
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
# Build Docker image
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
custom-lib-path=/opt/nvidia/deepstream/deepstream/lib/libnvds_infercustomparser.so

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
    --tracker /opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml \
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

### Build Docker Image

```bash
./scripts/docker/build_docker.sh [deepstream_version]
```

### Run with Docker

```bash
# Object detection on video
./scripts/docker/run_yolo_detection.sh

# RTSP stream processing
./scripts/docker/run_rtsp_stream.sh rtsp://camera-ip:8554/stream

# Custom command
docker run --rm --gpus all \
    -v ${PWD}/data:/app/data \
    -v ${PWD}/configs:/app/configs \
    -v ${PWD}/models:/app/models \
    deepstream-infer-lab:latest \
    --source=/app/data/video.mp4 \
    --config=/app/configs/yolov8_config.txt \
    --model_type=yolov8
```

## Performance Tips

1. **Use TensorRT Engines**: Pre-generate TensorRT engines for best performance
2. **Batch Processing**: Increase batch size for multiple streams
3. **GPU Memory**: Monitor GPU memory usage with `nvidia-smi`
4. **Decode Surfaces**: Adjust `num-decode-surfaces` for better throughput
5. **Network Mode**: Use FP16 or INT8 for faster inference
6. **Tracker**: NvDCF tracker provides best accuracy, IOU tracker is faster

## Troubleshooting

### Common Issues

**Pipeline fails to start:**
- Check DeepStream installation: `deepstream-app --version`
- Verify GPU driver: `nvidia-smi`
- Check config file paths

**Low FPS:**
- Use TensorRT engine instead of ONNX
- Reduce input resolution
- Enable FP16 mode
- Adjust batch size

**Out of memory:**
- Reduce batch size
- Lower `num-decode-surfaces`
- Use smaller model

**No display:**
- Install display dependencies: `apt install libgstreamer1.0-0 gstreamer1.0-plugins-base`
- Check X11 forwarding for SSH: `export DISPLAY=:0`

## References

- [NVIDIA DeepStream SDK](https://developer.nvidia.com/deepstream-sdk)
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