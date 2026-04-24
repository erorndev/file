# Native File(1) for Windows

A native Windows port of Ian Darwin's `file` command and `libmagic`, built
with CMake and Clang on Windows without requiring MSYS2 and Cygwin.

## Build

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

This produces:

- `build/file.exe`
- `build/magic.mgc`