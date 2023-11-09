# Building with CMake

## Build

This project doesn't require any special command-line flags to build to keep
things simple.

Here are the steps for building in release mode with a single-configuration
generator, like the Ninja one:

```sh
cmake -S . -B build -G Ninja -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

[1]: https://cmake.org/download/
[2]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project
