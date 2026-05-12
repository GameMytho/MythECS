# MythECS
**MythECS** is a header-only ECS framework, based on C++20 and CMake, heavily referenced by the structure and api design of *Bevy ecs*, as well as *Unity dots*, *EnTT*, etc.

## Prerequisites
To build MythoECS, we need to check the following tools.

**Windows 10/11**
- MSVC v143 (VS2022) or more recent
- Cmake 3.27 (or more recent)
- Git 2.1 (or more recent)

**Ubuntu 22.04**
- Compiler Support C++20 (gcc 11.4.0/clang 13.0.1 or more recent)
- Cmake 3.27 (or more recent)
- Git 2.1 (or more recent)

## How To Build

For Windows, try

```
mkdir <path-to-your-build-dir>
cmake -S . -B <path-to-your-dir> -DENABLE_TESTS=ON
```

then open the solution in the build directory and build it manually by Visual Studio. Surely, you can try

```
cmake --build <path-to-your-build-dir> --config <Debug/RelWithDebInfo/Release/MinSizeRel>
```

For Linux, try

```
mkdir build
cmake -S . -B <path-to-your-build-dir> -DENABLE_TESTS=ON -DCAMKE_BUILD_TYPE=<Debug/RelWithDebInfo/Release/MinSizeRel>
cmake --build <path-to-your-build-dir>
```

### Run Tests

After compile, try

```
ctest --test-dir <path-to-your-build-dir>
```

If wanna tests output, try

```
ctest --test-dir <path-to-your-build-dir> --verbose
```

or maybe you can run test by Visual Studio in windows.