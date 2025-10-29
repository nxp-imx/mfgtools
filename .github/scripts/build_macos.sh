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
        brew install ninja cmake autoconf automake libtool

    export PROJECT_DIR=$(pwd)

    # Setup vcpkg
    log_and_run "Setting up vcpkg" setup_vcpkg

    # Build libuuu
    log_and_run "Building libuuu for $ARCH" build_libuuu "$ARCH"

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
    cd "$PROJECT_DIR/wrapper"
}

build_libuuu() {
    local arch="$1"
    cmake --preset=unix -DCMAKE_OSX_ARCHITECTURES="$arch"
    cmake --build build
}

copy_artifacts() {
    local arch="$1"
    mkdir -p "libuuu/lib/darwin/$arch/"
    cp build/libuuu.dylib "libuuu/lib/darwin/$arch/"
}

main "$@"
