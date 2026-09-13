![Deep Infer](docs/assets/deepstream-banner.svg)

# Deep Infer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A C++17 application for NVIDIA DeepStream video inference. It builds a
GStreamer pipeline for file, camera, and RTSP inputs and supports optional
tracking, analytics, display, and frame output.

Model and result types come from
[neuriplo-tasks](https://github.com/olibartfast/neuriplo-tasks), pinned to
`v0.8.2` in [versions.env](versions.env), which CMake fetches automatically
during configuration.

## Supported platforms

The supported SDK baseline is NVIDIA DeepStream 9.1. JetPack 6.x and its
DeepStream 7.x stack are no longer supported by this project.

DeepStream 9.0 is still supported as a fallback; CMake auto-detects whichever
version is installed.

### Desktop and server (x86_64)

The DeepStream 9.1 dGPU baseline is:

- Ubuntu 24.04 LTS
- A supported NVIDIA dGPU
- NVIDIA display driver 580+
- CUDA Toolkit 13.1+
- TensorRT 10.14+
- GStreamer 1.24.2

These versions follow NVIDIA's
[DeepStream 9.1 installation requirements](https://docs.nvidia.com/metropolis/deepstream/dev-guide/text/DS_Installation.html).

### Jetson

The repository's verified Jetson baseline is:

- JetPack 7.2 / L4T R39.2
- Ubuntu 24.04
- NVIDIA Jetson Orin
- CUDA 13.2 and TensorRT 11.0 from JetPack
- DeepStream 9.1

DeepStream 9.0 installation on this Jetson baseline needs a package dependency
workaround. Follow [docs/guides/JetsonSetup.md](docs/guides/JetsonSetup.md)
instead of using the desktop installation procedure.

## Build dependencies

Install the host development tools and libraries:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libglib2.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-dev \
    libopencv-dev \
    ninja-build \
    pkg-config
```

CUDA is required for every build. DeepStream is optional at configure time:
without it, CMake builds a stub executable that supports `--help` and reports
the missing runtime SDK clearly. This keeps ordinary CI builds independent of
a local DeepStream installation.

### Dependency versions

[versions.env](versions.env) at the repository root is the single source of
truth for third-party dependency versions and base images. It is plain
`KEY=VALUE`, consumed by both CMake and the shell scripts. The pinned version
of the `neuriplo-tasks` dependency is `v0.8.2`.

Overrides follow a simple precedence order, per value:

1. An explicit CMake cache entry (`-D<KEY>=...`) wins.
2. Otherwise, an environment variable named `<KEY>` is used.
3. Otherwise, the value from `versions.env` applies.

Values from `versions.env` are never written to the CMake cache, so editing
`versions.env` takes effect on the next configure. The shell scripts in
`scripts/docker/` honor environment overrides the same way, using the
`${VAR:-<pinned value>}` form.

### Toolchain enforcement

By default, CMake requires the pinned `CUDA_MIN_VERSION`, `OPENCV_MIN_VERSION`,
and `GSTREAMER_VERSION` minimums from `versions.env`. Set
`-DDEEPINFER_ENFORCE_TOOLCHAIN=OFF` for stub-only builds whose toolchain is
below the DeepStream baseline (CI does exactly this for its build-only image);
the `CMAKE_MIN_VERSION` check always remains in force.

### Offline builds

To build without network access, point CMake at a pre-provisioned
`neuriplo-tasks` checkout instead of letting `FetchContent` clone it:

```bash
cmake -S . -B build -GNinja \
  -DFETCHCONTENT_SOURCE_DIR_NEURIPLO-TASKS=/path/to/neuriplo-tasks \
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON
```

`FETCHCONTENT_FULLY_DISCONNECTED=ON` is optional and additionally prevents
CMake from attempting any download steps.

## Install DeepStream 9.1

Download the matching DeepStream 9.1 package from the
[NVIDIA DeepStream catalog](https://catalog.ngc.nvidia.com/orgs/nvidia/resources/deepstream).
For a desktop installation:

```bash
sudo apt install ./deepstream-9.1_9.1.0-1_amd64.deb
sudo ldconfig

deepstream-app --version
```

The expected SDK path is `/opt/nvidia/deepstream/deepstream-9.1`. Pass
`-DDEEPSTREAM_DIR=<path>` if it is installed elsewhere.

DeepStream 9.0 is also supported as a fallback at `/opt/nvidia/deepstream/deepstream-9.0`.

## Build

```bash
git clone https://github.com/olibartfast/deep-infer.git
cd deep-infer

cmake -S . -B build -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.1
cmake --build build --parallel
```

CMake also searches common DeepStream installation paths automatically, so the
explicit `DEEPSTREAM_DIR` argument can be omitted for a standard installation.

Build options:

| CMake option | Default | Purpose |
|---|---:|---|
| `WITH_SHOW_FRAME` | `OFF` | Compile frame display support |
| `WITH_WRITE_FRAME` | `ON` | Compile frame output support |
| `BUILD_TESTING` | `OFF` | Build the test targets |

For example:

```bash
cmake -S . -B build -GNinja \
    -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.1 \
    -DWITH_SHOW_FRAME=ON
cmake --build build --parallel
```

## Configure a model

DeepStream inference settings are supplied through an `nvinfer` configuration
file. [configs/yolov8_config.txt](configs/yolov8_config.txt) is the included
example:

```ini
[property]
gpu-id=0
onnx-file=../models/yolov8n.onnx
model-engine-file=../models/yolov8n.engine
labelfile-path=../data/labels/coco.names
batch-size=1
network-mode=2
num-detected-classes=80
gie-unique-id=1
process-mode=1
network-type=0
cluster-mode=2
maintain-aspect-ratio=1

[class-attrs-all]
nms-iou-threshold=0.45
pre-cluster-threshold=0.25
topk=300
```

TensorRT engines are specific to the target GPU, TensorRT release, precision,
and model shapes. Rebuild an engine on the deployment system instead of copying
one from a different platform.

## Run

```bash
./build/deep-infer \
    --source /path/to/video.mp4 \
    --config configs/yolov8_config.txt \
    --model_type yolov8 \
    --labels data/labels/coco.names \
    --output output \
    --verbose
```

RTSP input with tracking:

```bash
./build/deep-infer \
    --source rtsp://camera-ip:8554/stream \
    --config configs/yolov8_config.txt \
    --model_type yolov8 \
    --tracker /path/to/tracker-config.yml \
    --show
```

### Command-line options

| Option | Description |
|---|---|
| `-s, --source <path>` | Video, image, camera, or RTSP input |
| `-c, --config <path>` | DeepStream inference configuration |
| `-mt, --model_type <type>` | Model/task type |
| `-l, --labels <path>` | Label file |
| `-o, --output <path>` | Output directory |
| `-conf, --confidence <value>` | Confidence threshold; default `0.5` |
| `-nms, --nms <value>` | NMS threshold; default `0.4` |
| `-g, --gpu <id>` | GPU device; default `0` |
| `--tracker [config]` | Enable tracking with an optional config |
| `--analytics` | Enable DeepStream analytics |
| `--show` | Display output frames |
| `--no-write` | Disable frame output |
| `-v, --verbose` | Enable debug logging |
| `-h, --help` | Print usage |

## Docker

The helper scripts use NVIDIA's upstream DeepStream 9.1 image and mount this
repository into the container. Select the platform explicitly:

```bash
# Inspect the resolved image and SDK path without pulling it.
./scripts/docker/build_docker.sh x86-ds9.1 --print-config
./scripts/docker/build_docker.sh jetson-ds9.1 --print-config

# Pull and build.
./scripts/docker/build_docker.sh x86-ds9.1
./scripts/docker/build_in_container.sh x86-ds9.1

# Run the examples.
./scripts/docker/run_yolo_detection.sh x86-ds9.1
./scripts/docker/run_rtsp_stream.sh rtsp://camera-ip:8554/stream x86-ds9.1
```

Use `jetson-ds9.1` in place of `x86-ds9.1` on the supported Jetson baseline.
Docker requires the NVIDIA Container Toolkit on desktop or the JetPack NVIDIA
container runtime on Jetson.

## End-to-end examples (segmentation & pose)

`deep-infer` supports instance segmentation and pose estimation through
[neuriplo-tasks](https://github.com/olibartfast/neuriplo-tasks) postprocessors.
For these models DeepStream runs with `network-type=100` and
`output-tensor-meta=1`, and the OSD sink probe converts the raw nvinfer output
tensors into typed results (`InstanceSegmentation` / `PoseEstimation`).

The full flow runs inside the NGC container: build, export the model to ONNX,
then run.

### 1. Build inside the container

```bash
./scripts/docker/build_in_container.sh x86-ds9.1
```

This produces `build/deep-infer` linked against the local DeepStream SDK.

### 2. Export the models

Use the neuriplo-tasks exporters to create the ONNX models (run inside the
container, or anywhere with Python + the listed deps):

```bash
# Both models:
docker run --rm --gpus all -v "$PWD":/ws -w /ws \
    nvcr.io/nvidia/deepstream:9.1-triton-multiarch \
    ./scripts/setup/export_models.sh all

# Or one at a time:
#   ./scripts/setup/export_models.sh pose   -> models/yolo26s-pose.onnx
#   ./scripts/setup/export_models.sh seg    -> models/rfdetr_seg_small.onnx
```

DeepStream generates the TensorRT `.engine` from the ONNX on the target GPU the
first time the pipeline starts.

### 3. RF-DETR segmentation

```bash
./scripts/docker/run_rfdetr_segmentation.sh x86-ds9.1
```

This runs `configs/rfdetr_segmentation_config.txt` with
`--model_type rfdetr_segmentation` over `people-walking.mp4`. Each frame yields
`neuriplo_tasks::InstanceSegmentation` results (bounding box, class, and a
binary mask) via `RfDetrSegmentationPostprocessor` on the model's three output
tensors (`dets`, `labels`, `masks`). Verified end-to-end on `people-walking.mp4`
(1–3 segmented people per person-frame).

### 4. YOLO26 pose

```bash
./scripts/docker/run_yolo_pose.sh x86-ds9.1
```

This runs `configs/yolo26_pose_config.txt` with `--model_type yolo_pose` over
`people-walking.mp4`. Each frame yields `neuriplo_tasks::PoseEstimation`
results (person bounding box plus 17 COCO keypoints).

The pose bridge auto-detects the export format: the modern Ultralytics decoded
layout `[batch, 300, 57]` (YOLO26, `yolo26s-pose`) is handled by a built-in
top-N decoder, while the classic anchor layout `[batch, 56, 8400]`
(YOLOv8/YOLO11) goes through `YoloPosePostprocessor` directly. Verified
end-to-end on `people-walking.mp4` (poses emitted on person frames).

| `--model_type` | Postprocessor | Result type |
|---|---|---|
| `rfdetr_segmentation` | `RfDetrSegmentationPostprocessor` | `InstanceSegmentation` |
| `yolo_pose` | `YoloPosePostprocessor` / top-N decoder | `PoseEstimation` |
| `yolov8` (default) | DeepStream object metadata | `Detection` |

## Troubleshooting

### CMake builds the stub pipeline

DeepStream headers were not found. Remove the CMake cache and configure again
with the SDK path:

```bash
rm -rf build
cmake -S . -B build -GNinja \
    -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.1
cmake --build build --parallel
```

### GStreamer cannot load a codec

Install the full plugin set needed by your media:

```bash
sudo apt install -y \
    gstreamer1.0-libav \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-tools
```

### A TensorRT engine does not load

Delete the incompatible engine and regenerate it with the TensorRT version and
GPU used for deployment. Prefer FP16 unless the model has a validated INT8
calibration path.

### Display fails

Enable `WITH_SHOW_FRAME`, confirm `DISPLAY` is set, and provide X11 or Wayland
access to the process or container. Headless runs should omit `--show`.

## Documentation

- [Getting started](docs/guides/GettingStarted.md)
- [JetPack 7.2 / DeepStream 9.1 setup](docs/guides/JetsonSetup.md)
- [NVIDIA DeepStream documentation](https://docs.nvidia.com/metropolis/deepstream/dev-guide/)
- [GStreamer documentation](https://gstreamer.freedesktop.org/documentation/)
- [TensorRT documentation](https://docs.nvidia.com/deeplearning/tensorrt/)

## Agent Skills

This project includes agent skills in the `skills/` directory, based on
[NVIDIA DeepStream 9.1 skills](https://github.com/NVIDIA/DeepStream/tree/main/skills).
These are markdown-based skill files that coding agents (Claude Code, Codex, etc.)
can use to automate DeepStream workflows:

| Skill | Purpose |
|---|---|
| `deepstream-run-mv3dt` | Multi-View 3D Tracking deployment and operation |
| `deepstream-generate-pipeline` | Interactive GStreamer pipeline builder |
| `amc-setup-calibration-stack` | AutoMagicCalib microservice setup |
| `amc-run-video-calibration` | Camera calibration from video files |
| `amc-run-rtsp-calibration` | Camera calibration from RTSP streams |
| `amc-run-sample-calibration` | Verify AMC with sample data |
| `deepstream-dev` | DeepStream application development |
| `deepstream-import-vision-model` | Import models into DeepStream |
| `deepstream-profile-pipeline` | Pipeline performance profiling |
| `deepstream-sop` | SOP microservice (step-sequence compliance via GEBD + VLM) |

To use these skills with a coding agent, copy them into your agent's skill
directory or point your agent at this project. See the
[NVIDIA DeepStream skills README](https://github.com/NVIDIA/DeepStream/blob/main/skills/README.md)
for installation instructions.

## License

[MIT](LICENSE)
