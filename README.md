# Carnet

A small desktop budget app. It imports the CSV file your bank lets you export, sorts each
transaction into a category with rules you write, and shows where your money went each month.

Work in progress: only the core library (amount and date parsing) exists so far.

## Building on Windows

You need Visual Studio 2026 with the C++ workload, CMake 3.25 or newer, and
[vcpkg](https://github.com/microsoft/vcpkg). GoogleTest is pulled in by vcpkg.

A Developer PowerShell for VS 2026 already sets `VCPKG_ROOT` to the vcpkg that ships with Visual
Studio, so the `windows-msvc` preset works as is. To use your own vcpkg checkout instead, create a
`CMakeUserPresets.json` next to `CMakePresets.json` (it's gitignored):

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local",
      "inherits": "windows-msvc",
      "environment": { "VCPKG_ROOT": "C:/path/to/vcpkg" }
    }
  ],
  "buildPresets": [{ "name": "local", "configurePreset": "local" }],
  "testPresets": [{ "name": "local", "configurePreset": "local", "output": { "outputOnFailure": true } }]
}
```

Then, from a Developer PowerShell for VS 2026 (use `windows-msvc` instead of `local` if you skipped
the user preset):

```
cmake --preset local
cmake --build --preset local
ctest --preset local
```

In CLion, pick the Visual Studio toolchain and the `windows-msvc` or `local` preset.
