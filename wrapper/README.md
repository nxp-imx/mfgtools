# libuuu

A python wrapper for `libuuu`.

Supported Python versions: 3.9 or newer.

## Manual Build and Installation

To build the project you first need to build libuuu dynamic libraries for your
operating system, or download them. In this section, manual build is described.

### Building libraries

We first install `vcpkg` which is a C++ library manager for Windows, Linux, and MacOS. Then we set some necessary environment variables, how you set them depends on your operating system, but it is basically the same. 

#### Linux & MacOS
> Downloading vcpkg and setting up environment variables
```bash
git clone https://github.com/microsoft/vcpkg.git 
cd vcpkg
export VCPKG_ROOT=$(pwd)
export PATH=$VCPKG_ROOT:$PATH
./bootstrap-vcpkg.sh
```

> Dependencies on Linux (Ubuntu)
```bash
sudo apt-get install gcc cmake ninja-build autotools-dev automake autoconf libudev-dev
```
> Dependencies on MacOS 
```bash
brew install ninja cmake autoconf automake libtool
```
> Build
```bash
cd ../wrapper
cmake --preset=unix #Needs to have vcpkg in PATH & VCPKG_ROOT set.
cmake --build build
```

#### Windows
> Downloading vcpkg and setting up environment variables
```powershell
git clone https://github.com/microsoft/vcpkg.git 
cd vcpkg
$env:VCPKG_ROOT = $PWD.Path 
$env:Path = $env:VCPKG_ROOT + ';' + $env:Path
./bootstrap-vcpkg.bat
```
> Dependencies on Windows
```powershell
choco install ninja llvm cmake
```
> Build 
```powershell
cmake --preset=windows -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

### Building python package
We just need to create folder `./wrapper/libuuu/lib` and move the dynamic libraries there.

```bash
mkdir ./libuuu/lib
cp build/*.dll ./libuuu/lib # *.so for Linux, *.dylib for MacOS
pip install -e .
```