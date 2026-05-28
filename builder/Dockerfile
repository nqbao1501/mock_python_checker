FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    git \
    wget \
    file \
    python3 \
    python3-distutils \
    libncurses5-dev \
    gawk \
    unzip \
    zlib1g-dev \
    flex \
    bison \
    gettext \
    bash \
    rsync \
    tar \
    gzip \
    make \
    && apt-get clean

WORKDIR /opt



RUN wget https://downloads.openwrt.org/releases/23.05.0/targets/x86/64/openwrt-sdk-23.05.0-x86-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz \
    && tar -xf openwrt-sdk-23.05.0-x86-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz \
    && mv openwrt-sdk-23.05.0-x86-64_gcc-12.3.0_musl.Linux-x86_64 openwrt-sdk \
    && rm *.tar.xz



WORKDIR /workspace

CMD ["/bin/bash"]