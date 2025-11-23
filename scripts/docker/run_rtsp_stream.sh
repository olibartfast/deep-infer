#!/bin/bash
# Run DeepStream with RTSP stream
# Usage: ./run_rtsp_stream.sh <rtsp_url>

RTSP_URL=${1:-"rtsp://localhost:8554/stream"}

docker run --rm \
  --gpus all \
  --network host \
  -v ${PWD}/configs:/app/configs \
  -v ${PWD}/models:/app/models \
  -v ${PWD}/output:/app/output \
  deepstream-infer-lab:latest \
  --source=${RTSP_URL} \
  --config=/app/configs/yolov8_config.txt \
  --model_type=yolov8 \
  --labels=/app/data/labels/coco.names \
  --tracker \
  --show \
  --verbose
