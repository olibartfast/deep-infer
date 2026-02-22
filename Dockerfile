# Production Dockerfile for DeepStream Inference Lab
# Build: docker build --rm -t deepstream-infer-lab .
# Run: docker run --rm --gpus all deepstream-infer-lab [args]

ARG DEEPSTREAM_VERSION=8.0
FROM nvcr.io/nvidia/deepstream:${DEEPSTREAM_VERSION}-gc-triton-devel

# Install dependencies
# Note: DS 8.0 containers do not package some multimedia codec libraries.
# ffmpeg and related libs must be reinstalled explicitly.
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libglib2.0-dev \
    pkg-config \
    git \
    && apt-get install --reinstall -y libflac8 libmp3lame0 libxvidcore4 ffmpeg \
    && rm -rf /var/lib/apt/lists/*

# DeepStream 8.0 installs to a versioned path
ENV DEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-8.0

# Set working directory
WORKDIR /app

# Copy source code
COPY . /app

# Create output directory
RUN mkdir -p /app/output

# Build the application
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
          -DDEEPSTREAM_DIR=${DEEPSTREAM_DIR} .. && \
    ninja

# Add metadata labels
LABEL maintainer="Computer Vision DeepStream Client"
LABEL description="C++ client for computer vision inference with NVIDIA DeepStream"
LABEL version="1.0"
LABEL deepstream.version="8.0"
LABEL base.image="nvcr.io/nvidia/deepstream:8.0-gc-triton-devel"

# Create non-root user for security
RUN groupadd -r appuser && useradd -r -g appuser appuser && \
    chown -R appuser:appuser /app

USER appuser

# Set the entry point for the container
ENTRYPOINT ["/app/build/deepstream-infer-lab"]

# Default command if no arguments are provided
CMD ["--help"]