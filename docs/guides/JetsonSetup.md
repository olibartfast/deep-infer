# Jetson Orin Nano Super — Build & Setup Guide

Last verified: 2026-07-10

## Target Hardware

| Item | Detail |
|------|--------|
| Board | NVIDIA Jetson Orin Nano Super (Developer Kit) |
| Architecture | aarch64 (ARM64) |
| JetPack | 7.2 |
| L4T | R39.2.0 |
| CUDA | 13.2 (via `nvidia-jetpack` meta-package) |
| DeepStream | 9.0 (from NGC, force-installed) |

## One-shot System Setup

```bash
sudo apt update
sudo apt install -y cmake ninja-build nvidia-jetpack
```

`nvidia-jetpack` (7.2-b187 from Jetson repo) pulls in CUDA 13.2, cuDNN, TensorRT, VPI, multimedia APIs, and container runtime.

## DeepStream 9.0 Install (with Jetson CUDA workaround)

DeepStream 9.0 .deb has CUDA dependencies named for desktop CUDA (`cuda-cudart-12-8 | cuda-cudart-13-0`),
which don't match Jetson's L4T BSP CUDA packaging (CUDA 13.2 lives under `nvidia-l4t-cuda`).
The CUDA libraries ARE present — they just aren't registered as the packages DeepStream expects.

```bash
# 1. Download from NGC (no auth required)
curl -L \
  "https://api.ngc.nvidia.com/v2/resources/nvidia/deepstream/versions/9.0/files/deepstream-9.0_9.0.0-1_arm64.deb" \
  -o /mnt/sdcard/deepstream-9.0_9.0.0-1_arm64.deb

# 2. Force-install ignoring CUDA package name mismatches
sudo dpkg --force-depends -i /mnt/sdcard/deepstream-9.0_9.0.0-1_arm64.deb

# 3. Hold the package so apt doesn't try to remove it
sudo apt-mark hold deepstream-9.0

# 4. Install missing runtime deps (force if needed)
apt-get download libgstrtspserver-1.0-0 libyaml-cpp0.8
sudo dpkg --force-depends -i libgstrtspserver-1.0-0_*.deb libyaml-cpp0.8_*.deb
rm -f libgstrtspserver-1.0-0_*.deb libyaml-cpp0.8_*.deb

# 5. Verify
deepstream-app --version
# Expected: deepstream-app version 9.0.0
#          DeepStreamSDK 9.0.0
```

### Side effect: apt broken state

Force-installing DeepStream leaves apt in a broken state because of the CUDA dependency mismatch.
apt will refuse to install new packages until fixed. Workaround: use `apt-get download` + `dpkg --force-depends -i`
for any additional packages you need.

## Build deep-infer

### Without DeepStream (stub fallback — compiles everywhere)

```bash
cmake -S . -B build -GNinja
cmake --build build
```

Binary runs but logs "DeepStream not available" at runtime.

### With DeepStream 9.0 (full GPU pipeline)

```bash
cmake -S . -B build -GNinja -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.0
cmake --build build
```

Linking picks up `libnvdsgst_meta.so` and friends from `/opt/nvidia/deepstream/deepstream-9.0/lib/`.

## Pitfalls Encountered

### 1. `cv::Rect2f` → `neuriplo_tasks::vision::Rect` type break

vision-core v0.6.0 replaced OpenCV `cv::Rect2f` with its own `neuriplo_tasks::vision::Rect`
(int-based: `x, y, width, height`). The probe callback in `DeepStreamPipeline.cpp` needed
`cv::Rect2f(...)` → `neuriplo_tasks::vision::Rect(static_cast<int>(...), ...)`.

### 2. DeepStream .deb CUDA dependencies don't match Jetson

See DeepStream install section above. The fix is `dpkg --force-depends` + `apt-mark hold`.

### 3. `libgstrtspserver-1.0.so.0` missing after DS 9.0 install

`deepstream-app` fails to load. Install manually: `apt-get download libgstrtspserver-1.0-0 && dpkg -i`.

### 4. `libyaml-cpp.so.0.8` missing

Same pattern — download and force-install the `.deb`.

### 5. GStreamer deprecation warnings

`gst_element_get_request_pad` is deprecated in GStreamer 1.24. The replacement is
`gst_element_request_pad_simple`. Cosmetic — doesn't affect functionality.

### 6. Python venv: `ensurepip` not available

JetPack 7.2 doesn't include `python3.12-venv` by default. Force-install it:

```bash
apt-get download python3-pip-whl python3-setuptools-whl python3.12-venv
sudo dpkg --force-depends -i python3-pip-whl_*.deb python3-setuptools-whl_*.deb python3.12-venv_*.deb
```

### 7. RF-DETR ONNX export: `_upsample_bicubic2d_aa` not supported

PyTorch 2.13's legacy ONNX exporter doesn't support the anti-aliased bicubic upsample op
used in the DINOv2 backbone. Two workarounds:

**Option A**: Use dynamo exporter (needs `pip install onnxscript`, then patch
`rfdetr/export/_onnx/exporter.py` to set `dynamo=True` instead of `False`).

**Option B**: Monkey-patch `F.interpolate` to force `antialias=False` before export.

### 8. pip install rfdetr timeout

`rfdetr[onnx]==1.8.3` pulls ~2.5GB of packages (Torch, cuDNN, cuBLAS, etc.) and
compiles `onnxsim` from source on aarch64. Expect 8-10 minutes.

## CI Build (x86_64, no DeepStream)

The GitHub Actions CI (`nvidia/cuda:12.6.2-devel-ubuntu22.04` container) builds without
DeepStream using the stub fallback. Changes to `DeepStreamPipeline.cpp` only affect
the full-pipeline path and don't break the stub build.

## Model File Locations

| Artifact | Path |
|----------|------|
| DeepStream .deb | `/mnt/sdcard/deepstream-9.0_9.0.0-1_arm64.deb` (617 MB) |
| RF-DETR weights | `~/.roboflow/models/rf-detr-medium.pth` (386 MB) |
| DeepStream SDK | `/opt/nvidia/deepstream/deepstream-9.0/` |
| Built binary | `build/deep-infer` |

## Verification Checklist

- [ ] `deepstream-app --version` → 9.0.0
- [ ] `cmake -S . -B build -GNinja -DDEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.0` succeeds
- [ ] `cmake --build build` → 57/57 targets, zero errors
- [ ] `ldd build/deep-infer | grep deepstream-9.0` shows at least 2 libs
- [ ] `build/deep-infer --help` prints usage
