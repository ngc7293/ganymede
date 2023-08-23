# Ganymede

Collection of cloud-based services for smart agriculture/hydroponics.

## Contents

- [Ganymede](#ganymede)
  - [Contents](#contents)
  - [Requirements](#requirements)
  - [Building](#building)

## Requirements

- GCC 9 or later
- CMake 3.17 or later
- Ninja
- Docker
- Conan

## Building

```bash
# Don't change this path! It's used by CMakeLists to find Conan files
mkdir cmake/conan
cd cmake/conan

# Install Conan depedencies. We need to make sure Conan install packages using the C++11 ABI
# You can do this using profiles, or passing arguments as below
conan install ../.. -s cppstd=17 -s compiler.libstdcxx=libstdc++11 -s Debug

cd ..
mkdir build
cd build
# Should now be in ganymede/cmake/build

# Build project
cmake -G Ninja -D CMAKE_BUILD_TYPE=Debug ../..
cmake --build . --target all

# Build Docker images
cmake --build . --target ganymede.services.device_config.docker
cmake --build . --target ganymede.services.measurements.docker
```