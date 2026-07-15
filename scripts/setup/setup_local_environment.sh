#!/bin/bash
# Setup DeepStream development environment
# This script installs DeepStream SDK and dependencies

set -e

echo "=== DeepStream Development Environment Setup ==="

# Check for NVIDIA GPU
if ! command -v nvidia-smi &> /dev/null; then
    echo "❌ Error: nvidia-smi not found. Please install NVIDIA drivers first."
    exit 1
fi

echo "✅ NVIDIA GPU detected:"
nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader

# Install system dependencies
echo "📦 Installing system dependencies..."
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    wget \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    libglib2.0-dev \
    libjson-glib-dev

echo "✅ System dependencies installed"

# Check if DeepStream is already installed
if [ -d "/opt/nvidia/deepstream/deepstream" ]; then
    echo "ℹ️  DeepStream already installed at /opt/nvidia/deepstream/deepstream"
    DEEPSTREAM_VERSION=$(cat /opt/nvidia/deepstream/deepstream/version 2>/dev/null || echo "unknown")
    echo "   Version: $DEEPSTREAM_VERSION"
else
    echo "📥 DeepStream not found. Please install manually:"
    echo "   1. Download from: https://developer.nvidia.com/deepstream-sdk"
    echo "   2. Extract and run: sudo ./install.sh"
    echo ""
    echo "   Or use Docker:"
    echo "   docker pull nvcr.io/nvidia/deepstream:7.1-triton-multiarch"
fi

# Verify GStreamer plugins
echo "🔍 Verifying GStreamer plugins..."
if gst-inspect-1.0 nvstreammux &> /dev/null; then
    echo "✅ DeepStream GStreamer plugins found"
else
    echo "⚠️  DeepStream GStreamer plugins not found"
    echo "   Please install DeepStream SDK"
fi

# Setup directory structure
echo "📁 Creating project directories..."
mkdir -p models data/videos data/images data/labels output configs

echo ""
echo "✅ Setup complete!"
echo ""
echo "Next steps:"
echo "1. Build the project:"
echo "   mkdir build && cd build"
echo "   cmake -DCMAKE_BUILD_TYPE=Release -GNinja .."
echo "   ninja"
echo ""
echo "2. Download a model and convert to TensorRT:"
echo "   ./scripts/setup/convert_to_tensorrt.sh model.onnx fp16"
echo ""
echo "3. Run inference:"
echo "   ./build/deep-infer --help"
