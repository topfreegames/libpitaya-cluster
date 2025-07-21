# Using Ubuntu 22.04 (jammy) which has GLIBC 2.35 for compatibility with Ubuntu 22.04 runtime
# This ensures the built library is compatible with Ubuntu 22.04 and newer versions
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    clang-14 \
    clang-format-14 \
    cmake \
    ninja-build \
    golang-go \
    python3 \
    python3-pip \
    git \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Install Conan
RUN pip3 install --upgrade conan cmake

# Setup Conan
RUN conan profile detect
RUN mkdir ~/.conan2/p

WORKDIR /app
