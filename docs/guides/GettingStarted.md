# DeepStream Getting Started Guide

## Quick Start

This guide will help you get started with DeepStream Inference Lab using DeepStream 8.0.

### Prerequisites

- Ubuntu 20.04 / 22.04 / 24.04 (DS 8.0 container is Ubuntu 24.04-based)
- NVIDIA GPU (Tesla T4, RTX/Ampere/Hopper/Blackwell or better)
- NVIDIA Driver **>= 570.133.20**
- CUDA **>= 12.8**
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

## Docker Usage

The project supports host-specific DeepStream container profiles. On Jetson Orin devices running JetPack 6.x / L4T 36.4.x, the default tested profile is **`7.1-samples-multiarch`**. On x86_64, the default profile remains **`8.0-gc-triton-devel`**.

### Jetson Orin Nano: Docker prerequisites

To run DeepStream containers on Jetson without a local DeepStream install:

```bash
sudo apt update
sudo apt install -y docker.io nvidia-container nvidia-l4t-gstreamer
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
```

Log out and back in before validating access:

```bash
docker version
docker ps
```

To identify the Jetson software baseline:

```bash
. /etc/os-release && echo "$NAME $VERSION"
cat /etc/nv_tegra_release
```

Use the matching DeepStream generation for Jetson:

- JetPack 6.x / L4T 36.x / Ubuntu 22.04 -> DeepStream 7.1
- JetPack 7.0 / L4T 38.2 -> DeepStream 8.0
- JetPack 7.1 / L4T 38.4 / Ubuntu 24.04 -> DeepStream 9.0

### Pull the matching base image

```bash
docker pull nvcr.io/nvidia/deepstream:7.1-samples-multiarch   # Jetson Orin / JetPack 6.x
docker pull nvcr.io/nvidia/deepstream:8.0-gc-triton-devel
```

### Prepare Docker runtime

```bash
./scripts/docker/build_docker.sh
```

The script auto-selects a Docker target profile from the current host and pulls
the matching upstream NGC DeepStream image. On Jetson Orin devices running
JetPack 6.x / L4T 36.x it resolves to `jetson-ds7.1`; on x86_64 it resolves to
`x86-ds8.0`.

To inspect or override the selection:

```bash
./scripts/docker/build_docker.sh --print-config
./scripts/docker/build_docker.sh jetson-ds7.1
./scripts/docker/build_docker.sh x86-ds8.0
./scripts/docker/build_docker.sh x86-ds9.0
```

> **Note:** NVIDIA treats Jetson DeepStream containers primarily as deployment images. For Jetson, compile the executable natively on the host against the local DeepStream SDK, then run that host-built binary inside the mounted NGC runtime container.

### Run interactively (for development/debugging)

```bash
xhost +
docker run -it --entrypoint /bin/bash \
    --gpus all --rm --network=host --privileged \
    -e DISPLAY=${DISPLAY} \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ${PWD}:/workspace/deepstream-infer-lab \
    -w /workspace/deepstream-infer-lab \
    nvcr.io/nvidia/deepstream:7.1-samples-multiarch
```

### Run inference

```bash
# First build locally on the host:
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config
chmod +x scripts/docker/build_docker.sh \
         scripts/docker/build_in_container.sh \
         scripts/docker/run_yolo_detection.sh \
         scripts/docker/run_rtsp_stream.sh

# Then pull the matching runtime image:
./scripts/docker/build_docker.sh jetson-ds7.1

# Build inside the mounted NGC container:
./scripts/docker/build_in_container.sh jetson-ds7.1

# Helper script:
./scripts/docker/run_yolo_detection.sh jetson-ds7.1

# Equivalent direct docker run:
docker run --rm --gpus all --privileged --network host --ipc=host \
    --ulimit memlock=-1 --ulimit stack=67108864 \
    -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video,graphics \
    -e DISPLAY=${DISPLAY:-:0} \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v ${PWD}:/workspace/deepstream-infer-lab \
    -w /workspace/deepstream-infer-lab \
    nvcr.io/nvidia/deepstream:7.1-samples-multiarch \
    /workspace/deepstream-infer-lab/build/deepstream-infer-lab \
      --source=/workspace/deepstream-infer-lab/data/videos/sample.mp4 \
      --config=/workspace/deepstream-infer-lab/configs/yolov8_config.txt \
      --model_type=yolov8
```

If the link step fails with `libheif.so.1` unresolved references to `libde265`
or `libx265`, rerun `build_in_container.sh`. The script installs NVIDIA's
optional codec additions when available and also installs the HEIF codec runtime
packages inside the container before building.
The runtime helper scripts install the same required media and OpenCV codec
packages inside the container session before launching the built binary.
The build helper also passes `-DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda` to avoid
`FindCUDA` detection failures inside the NGC image.

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

### Issue: `permission denied while trying to connect to the docker API`

Your user is not able to access `/var/run/docker.sock`. Add the user to the
`docker` group and refresh the session:

```bash
sudo usermod -aG docker $USER
newgrp docker
docker version
docker ps
```

If `newgrp docker` is not convenient, log out and back in before retrying the
DeepStream NGC image pull or any of the helper scripts.

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

## TODO Next Steps 

- [Model Deployment Guide](ModelDeployment.md)
- [DeepStream Configuration Reference](DeepStreamConfig.md)
- [Tracker Configuration](TrackerSetup.md)
- [Analytics Setup](AnalyticsGuide.md)
