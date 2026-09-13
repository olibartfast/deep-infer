# Sample build commands:
#   x86_64:  docker build --build-arg BASE_IMAGE=nvcr.io/nvidia/deepstream:9.1-triton-multiarch -t deep-infer .
#   Jetson:  docker build --build-arg BASE_IMAGE=nvcr.io/nvidia/deepstream:9.0-triton-multiarch --build-arg DEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.0 -t deep-infer .

# The docker scripts pass these build args sourced from versions.env.
ARG BASE_IMAGE=nvcr.io/nvidia/deepstream:9.1-triton-multiarch
FROM ${BASE_IMAGE}

ARG BASE_IMAGE
ARG DEEPSTREAM_VERSION=9.1
ARG DEEPSTREAM_DIR=/opt/nvidia/deepstream/deepstream-9.1

# Install dependencies
# DeepStream containers do not ship every multimedia codec package by default.
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libglib2.0-dev \
    libjbig-dev \
    pkg-config \
    git \
    && apt-get install --reinstall -y libjbig0 \
    && apt-get install --reinstall -y libflac8 libmp3lame0 libxvidcore4 ffmpeg \
    && rm -rf /var/lib/apt/lists/*

# DeepStream installs to a versioned path
ENV DEEPSTREAM_DIR=${DEEPSTREAM_DIR}

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
LABEL deepstream.version="${DEEPSTREAM_VERSION}"
LABEL base.image="${BASE_IMAGE}"

# Create non-root user for security
RUN groupadd -r appuser && useradd -r -g appuser appuser && \
    chown -R appuser:appuser /app

USER appuser

# Set the entry point for the container
ENTRYPOINT ["/app/build/deep-infer"]

# Default command if no arguments are provided
CMD ["--help"]
