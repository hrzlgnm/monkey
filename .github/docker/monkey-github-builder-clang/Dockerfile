FROM debian:bookworm

RUN \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
    build-essential \
    ca-certificates \
    clang-13 \
    cmake \
    git \
    libc++-13-dev \
    libc++abi-13-dev \
    libc6-dev \
    libstdc++-11-dev \
    ninja-build \
    python3 && \
    rm -rf /var/lib/apt/lists/*
