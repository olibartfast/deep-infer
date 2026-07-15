![DeepStream Inference Lab](docs/assets/deepstream-banner.svg)

# DeepStream Inference Lab - Computer Vision with NVIDIA DeepStream

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A C++ application for computer vision tasks (object detection, classification, instance segmentation, optical flow) using NVIDIA DeepStream SDK. DeepStream provides a complete streaming analytics toolkit for AI-based video and image understanding with optimized performance on NVIDIA GPUs.


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

## Models

Supported models are provided by the [vision-core](https://github.com/olibartfast/vision-core) library, which is automatically fetched at build time. Please refer to that repository for the full and up-to-date list of supported architectures.

## Prerequisites

### System Requirements

#### For x86_64 Systems (Desktop/Server)
- Ubuntu 20.04 / 22.04 / 24.04
- NVIDIA GPU (Tesla T4, RTX/Ampere/Hopper/Blackwell or better)
- NVIDIA Driver **>= 570.133.20**
- CUDA **>= 12.8**
- TensorRT **>= 10.9.0.34**

#### For Jetson Orin with JetPack 7.x
- JetPack 7.2 (L4T R39.2) or later
- NVIDIA Orin series GPU (Orin Nano, Orin NX, or AGX Orin)
- CUDA **13.2** (included with JetPack 7.2)
- TensorRT **11.0** (included with JetPack 7.2)

#### For Jetson Orin with JetPack 6.x
- JetPack 6.2 (L4T 36.4.x) or later
- NVIDIA Orin series GPU (Orin Nano, Orin NX, or AGX Orin)
- CUDA **12.6** (included with JetPack 6.2)
- TensorRT **10.3** (included with JetPack 6.2)

### Software Dependencies

#### 1. NVIDIA DeepStream SDK

##### For x86_64 Systems (DeepStream 8.0):
```bash
# Download deepstream-8.0_8.0.0-1_amd64.deb from NVIDIA NGC:
# https://catalog.ngc.nvidia.com/orgs/nvidia/resources/deepstream
sudo apt-get install ./deepstream-8.0_8.0.0-1_amd64.deb

# Or using the tar archive:
sudo tar -xvf deepstream_sdk_v8.0.0_x86_64.tbz2 -C /
cd /opt/nvidia/deepstream/deepstream-8.0
sudo ./install.sh
sudo ldconfig
```

##### For Jetson Orin with JetPack 7.x (DeepStream 9.0):
```bash
# Download deepstream-9.0_9.0.0-1_arm64.deb from NVIDIA NGC:
# https://catalog.ngc.nvidia.com/orgs/nvidia/resources/deepstream
sudo apt-get install ./deepstream-9.0_9.0.0-1_arm64.deb

# Verify
ls /opt/nvidia/deepstream/deepstream-9.0/
deepstream-app --version
```

##### For Jetson Orin with JetPack 6.x (DeepStream 7.1):
```bash
# Install DeepStream 7.1 from NVIDIA repository
sudo apt update
sudo apt install deepstream-7.1

# Optional: Install RTSP server library if needed (resolves libgstrtspserver-1.0.so.0 warnings)
sudo apt install libgstrtspserver-1.0-0
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

This project supports both x86_64 systems (with DeepStream 8.0+) and Jetson Orin devices (with DeepStream 7.1+).

1. Clone the repository:
```bash
git clone https://github.com/olibartfast/deep-infer.git
cd deep-infer
```

2. Ensure host build tools are installed:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config
```

If you prefer not to install Ninja, you can omit `-GNinja` in the configure
step below and use CMake's default generator instead.

3. Create build directory:
```bash
mkdir build && cd build
```

4. Configure with CMake:
   
   #### For x86_64 Systems (DeepStream 8.0+):
   ```bash
   # DS 8.0 uses a versioned install directory — pass it explicitly:
   cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
         -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-8.0 ..
   ```
   
   #### For Jetson Orin with JetPack 7.x (DeepStream 9.0):
   ```bash
   # DS 9.0 uses a versioned install directory — pass it explicitly:
   cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
         -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.0 ..
   ```
   
   #### For Jetson Orin with JetPack 6.x (DeepStream 7.1):
   ```bash
   # DS 7.1 uses a versioned install directory:
   cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
         -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-7.1 ..
   ```
   
   If DeepStream is installed in a different versioned directory, pass that path instead.
   When DeepStream is not installed, the project still builds, but the resulting binary
   prints a clear runtime error until the SDK is installed and CMake is re-run.

   Optional flags:
   - `-DWITH_SHOW_FRAME=ON`: Enable frame display
   - `-DWITH_WRITE_FRAME=ON`: Enable frame writing (default: ON)

5. Build:
```bash
ninja
```

### Docker Installation

Docker mode uses the upstream NGC DeepStream image directly. The repository is
mounted into the container at runtime; no derived image build is required for
the Jetson workflow.

```bash
./scripts/docker/build_docker.sh
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

This repository supports multiple DeepStream container profiles. On Jetson Orin devices running JetPack 7.x / L4T R39.x, the default tested profile is `nvcr.io/nvidia/deepstream:9.0-triton-multiarch`. On Jetson Orin devices running JetPack 6.x / L4T 36.4.x, the default tested profile is `nvcr.io/nvidia/deepstream:7.1-samples-multiarch`. On x86_64 hosts, the default profile remains `nvcr.io/nvidia/deepstream:8.0-gc-triton-devel`.

### Jetson Orin Nano: install Docker and NVIDIA runtime

If you want to run DeepStream containers on a Jetson without installing DeepStream locally, install Docker and the Jetson container runtime from the JetPack/L4T apt repositories:

```bash
sudo apt update
sudo apt install -y docker.io nvidia-container nvidia-l4t-gstreamer
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
```

Then log out and back in, or start a new login shell, before testing Docker access:

```bash
docker version
docker ps
```

If `docker` commands fail with `permission denied while trying to connect to the docker API at unix:///var/run/docker.sock`, your user is not yet using the `docker` group in the current session. Fix it with:

```bash
sudo usermod -aG docker $USER
newgrp docker
docker version
```

If `newgrp docker` is not convenient, log out and back in, then retry the pull or helper script.

To verify which Jetson base system you have before choosing a DeepStream container:

```bash
. /etc/os-release && echo "$NAME $VERSION"
cat /etc/nv_tegra_release
```

For Jetson compatibility, match DeepStream to JetPack/L4T:

- L4T R39.x / Ubuntu 24.04 / JetPack 7.x: use DeepStream 9.0 on Jetson
- L4T 36.x / Ubuntu 22.04 / JetPack 6.x: use DeepStream 7.1 on Jetson
- L4T 38.4 / Ubuntu 24.04 / JetPack 7.1: use DeepStream 9.0

So a Jetson Orin Nano still on Ubuntu 22.04 should not target the DeepStream 9.0 Jetson container yet; upgrade to JetPack 7.1 first if you want `nvcr.io/nvidia/deepstream:9.0-triton-multiarch`.

### Prepare Docker Runtime

```bash
./scripts/docker/build_docker.sh
```

`build_docker.sh` now resolves a target profile automatically from the current host and pulls the matching upstream NGC image:

- `jetson-ds9.0` for Jetson devices on L4T R39.x / JetPack 7.x, using `9.0-triton-multiarch`
- `jetson-ds7.1` for Jetson devices on L4T 36.4.x / JetPack 6.x, using `7.1-samples-multiarch`
- `x86-ds8.0` for x86_64 hosts by default

You can also force a specific profile:

```bash
./scripts/docker/build_docker.sh jetson-ds7.1
./scripts/docker/build_docker.sh x86-ds8.0
./scripts/docker/build_docker.sh x86-ds9.0
./scripts/docker/build_docker.sh --print-config
```

> **Note:** On Jetson, build the executable on the host against the local DeepStream SDK, then run that host-built binary inside the mounted NGC runtime container.

### Run with Docker

```bash
# Ensure the helper scripts are executable:
chmod +x scripts/docker/build_docker.sh \
         scripts/docker/build_in_container.sh \
         scripts/docker/run_yolo_detection.sh \
         scripts/docker/run_rtsp_stream.sh

# Then pull the matching DeepStream runtime image:
./scripts/docker/build_docker.sh jetson-ds7.1

# Build inside the mounted NGC container:
./scripts/docker/build_in_container.sh jetson-ds7.1

# Object detection on video
./scripts/docker/run_yolo_detection.sh jetson-ds7.1

# RTSP stream processing
./scripts/docker/run_rtsp_stream.sh rtsp://camera-ip:8554/stream jetson-ds7.1

# Custom command using the upstream NGC runtime image directly
docker run --rm --gpus all --privileged --network host --ipc=host \
    --ulimit memlock=-1 --ulimit stack=67108864 \
    -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
    -v ${PWD}:/workspace/deepstream-infer-lab \
    -w /workspace/deepstream-infer-lab \
    nvcr.io/nvidia/deepstream:7.1-samples-multiarch \
    /workspace/deepstream-infer-lab/build/deepstream-infer-lab \
      --source=/workspace/deepstream-infer-lab/data/video.mp4 \
      --config=/workspace/deepstream-infer-lab/configs/yolov8_config.txt \
      --model_type=yolov8
```

The helper scripts mount the repository root into `/workspace/deepstream-infer-lab`.
`build_in_container.sh` builds the executable inside the upstream NGC image and
writes the resulting `build/` directory back to the host through that mount.
The runtime scripts then execute that built binary from the mounted workspace.
On Jetson, the runtime scripts also install the required media and OpenCV codec
packages inside the container session before launching the binary.

If the in-container link step fails on Jetson with `libheif.so.1` unresolved
references to `libde265` or `libx265`, rerun `build_in_container.sh`. The script
now installs NVIDIA's optional codec additions when available and also installs
the missing HEIF codec runtime packages inside the container before building.
The in-container build also passes `-DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda` to
avoid `FindCUDA` detection failures inside the NGC image.

If the host configure step fails with `CMake was unable to find a build program
corresponding to "Ninja"`, install `ninja-build` or rerun CMake without `-GNinja`.

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

The dev container now builds from `.devcontainer/Dockerfile` and is preconfigured against the Jetson-compatible DeepStream 7.1 headers.

To get started, open the repository in VS Code and select **Reopen in Container** when prompted, or run it manually from the Command Palette:

```
Dev Containers: Reopen in Container
```

CMake Tools is pre-configured to build in Debug mode against the Jetson-compatible DeepStream 7.1 headers inside the dev container.

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

Any feedback is greatly appreciated. If you have suggestions, bug reports, or questions, please open an [issue](https://github.com/olibartfast/deep-infer/issues).
