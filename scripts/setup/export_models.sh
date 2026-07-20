#!/usr/bin/env bash
# Export the RFDetr-segmentation and YOLO26-pose ONNX models.
# Pose uses the Ultralytics exporter; segmentation uses the rfdetr library
# directly (the neuriplo-tasks rfdetr exporter passes a `simplify` kwarg that
# recent rfdetr releases no longer accept, so we call export() with only the
# kwargs the installed version supports).
#
# Intended to run inside the DeepStream container (python3 + pip; GPU optional
# for ONNX export).
#
# Usage:
#   ./scripts/setup/export_models.sh all       # export both (default)
#   ./scripts/setup/export_models.sh pose
#   ./scripts/setup/export_models.sh seg
#
# Outputs land in ./models/:
#   - yolo26s-pose.onnx
#   - rfdetr_seg_small.onnx

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
MODELS_DIR="${ROOT_DIR}/models"
WHAT="${1:-all}"
SEG_TYPE="${SEG_TYPE:-small}"   # nano|small|medium|large|...

mkdir -p "${MODELS_DIR}"

# The DeepStream base image ships a numpy whose dist-info metadata is corrupt
# ("invalid metadata entry 'name'"), which breaks rfdetr/ultralytics version
# checks. Force a clean numpy<2 first.
fix_numpy() {
    pip install --force-reinstall --no-cache-dir "numpy<2" >/dev/null
}

export_pose() {
    echo "=== Exporting YOLO26 pose (yolo26s-pose) ==="
    fix_numpy
    pip install --quiet "ultralytics>=8.3.0" onnx onnxsim
    python3 - <<PY
from ultralytics import YOLO
out = YOLO("yolo26s-pose").export(format="onnx", imgsz=640, batch=1, simplify=True, opset=12)
import shutil, os
dest = "${MODELS_DIR}/yolo26s-pose.onnx"
if os.path.abspath(out) != os.path.abspath(dest):
    shutil.move(out, dest)
print("Saved", dest)
PY
}

export_seg() {
    echo "=== Exporting RF-DETR segmentation (${SEG_TYPE}) ==="
    fix_numpy
    pip install --quiet rfdetr onnx onnxsim
    python3 - <<PY
import inspect, glob, os, shutil
seg_type = "${SEG_TYPE}"
cls = {"nano":"RFDETRSegNano","small":"RFDETRSegSmall","medium":"RFDETRSegMedium",
       "large":"RFDETRSegLarge","xlarge":"RFDETRSegXLarge","2xlarge":"RFDETRSeg2XLarge"}[seg_type]
import rfdetr
m = getattr(rfdetr, cls)()
sig = inspect.signature(m.export)
print("export signature:", sig)
kwargs = {k: v for k, v in [("output_dir", "${MODELS_DIR}"), ("opset_version", 17),
                            ("batch_size", 1)] if k in sig.parameters}
print("calling export with:", kwargs)
m.export(**kwargs)
produced = [f for f in glob.glob("${MODELS_DIR}/*seg*.onnx")]
target = "${MODELS_DIR}/rfdetr_seg_${SEG_TYPE}.onnx"
if produced:
    if os.path.abspath(produced[0]) != os.path.abspath(target):
        shutil.move(produced[0], target)
    print("Saved", target)
PY
}

case "${WHAT}" in
    pose) export_pose ;;
    seg)  export_seg ;;
    all)  export_pose; export_seg ;;
    *)    echo "Unknown target: ${WHAT} (use pose|seg|all)" >&2; exit 1 ;;
esac

echo "=== Export complete. Models in ${MODELS_DIR}: ==="
ls -lh "${MODELS_DIR}"/*.onnx 2>/dev/null || true
echo
echo "Note: DeepStream builds the TensorRT engine (.engine) from the ONNX on the"
echo "target GPU the first time the pipeline runs."
