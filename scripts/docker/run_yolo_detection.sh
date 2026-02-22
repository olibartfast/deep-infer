#!/bin/bash
# Run DeepStream inference with object detection
# Usage: ./run_yolo_detection.sh
# Requires: NVIDIA driver >= 570.x, nvidia-container-toolkit

docker run --rm \
  --gpus all \
  --privileged \
  --network host \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v ${PWD}/data:/app/data \
  -v ${PWD}/configs:/app/configs \
  -v ${PWD}/models:/app/models \
  -v ${PWD}/output:/app/output \
  deepstream-infer-lab:latest \
  --source=/app/data/videos/sample.mp4 \
  --config=/app/configs/yolov8_config.txt \
  --model_type=yolov8 \
  --labels=/app/data/labels/coco.names \
  --output=/app/output \
  --verbose