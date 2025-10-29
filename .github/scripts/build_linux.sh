#!/bin/bash
set -euo pipefail

ARCH="${1:-x86_64}"

# Source common functions (same dump_logs function)
source "$(dirname "$0")/common_functions.sh"

main() {
    log_and_run "Installing pytest" \
        python -m pip install pytest

    # Install build dependencies
    log_and_run "Installing build dependencies" \
        yum install -y git cmake3 ninja-build gcc-c++ libudev-devel \
                       autoconf automake libtool curl zip unzip tar \
                       perl-core kernel-headers make

    # Setup vcpkg
    log_and_run "Setting up vcpkg" setup_vcpkg

    # Build libuuu
    log_and_run "Building libuuu" build_libuuu

    # Copy artifacts
    log_and_run "Copying artifacts" copy_artifacts "$ARCH"
}

setup_vcpkg() {
    cd /tmp
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh || {
        echo "VCPKG BOOTSTRAP FAILED"
        dump_logs
        exit 1
    }
    export VCPKG_ROOT=/tmp/vcpkg
    export PATH=$VCPKG_ROOT:$PATH
    cd /project/wrapper
}

build_libuuu() {
    cmake --preset=unix
    cmake --build build
}

copy_artifacts() {
    local arch="$1"
    mkdir -p "libuuu/lib/linux/$arch/"
    cp build/libuuu.so "libuuu/lib/linux/$arch/"
    mkdir -p "/output/libuuu/lib/linux/$arch/"
    cp build/libuuu.so "/output/libuuu/lib/linux/$arch/"
    echo "[INFO] Copied libuuu.so to /output/libuuu/lib/linux/$arch/"
}

main "$@"
