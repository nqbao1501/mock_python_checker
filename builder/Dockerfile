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



RUN wget https://downloads.openwrt.org/releases/21.02.7/targets/x86/64/openwrt-sdk-21.02.7-x86-64_gcc-8.4.0_musl.Linux-x86_64.tar.xz \
    && tar -xf openwrt-sdk-21.02.7-x86-64_gcc-8.4.0_musl.Linux-x86_64.tar.xz \
    && mv openwrt-sdk-21.02.7-x86-64_gcc-8.4.0_musl.Linux-x86_64 openwrt-sdk \
    && rm *.tar.xz


WORKDIR /workspace

CMD ["/bin/bash"]