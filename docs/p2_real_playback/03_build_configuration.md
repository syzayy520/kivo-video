# Build Configuration Guide

## CMake Options

### KIVO_ENABLE_FFMPEG
- **Type**: `BOOL`
- **Default**: `OFF`
- **Purpose**: Enables FFmpeg-based real media processing
- **Effect**: Compiles full FFmpeg adapter implementations, adds `KIVO_ENABLE_FFMPEG` compile definition

### KIVO_ENABLE_REAL_MEDIA_TESTS
- **Type**: `BOOL`
- **Default**: `OFF`
- **Purpose**: Enables integration tests requiring real media samples
- **Effect**: Registers integration tests with CTest, adds `KIVO_ENABLE_REAL_MEDIA_TESTS` compile definition

## CMake Presets

### ninja-debug
- **Generator**: Ninja
- **Build Type**: Debug
- **Binary Dir**: `build/ninja-debug`
- **FFmpeg**: Disabled
- **Integration Tests**: Not registered

### ninja-debug-ffmpeg
- **Generator**: Ninja
- **Build Type**: Debug
- **Binary Dir**: `build/ninja-debug-ffmpeg`
- **FFmpeg**: Enabled
- **Integration Tests**: Registered

## Build Commands

### Default Build (No FFmpeg)
```powershell
cmake --preset ninja-debug
cmake --build --preset ninja-debug
ctest --preset ninja-debug
```

### FFmpeg-Enabled Build
```powershell
cmake --preset ninja-debug-ffmpeg
cmake --build --preset ninja-debug-ffmpeg
ctest --preset ninja-debug
```

### Manual Configuration
```powershell
# Create build directory
mkdir build\ffmpeg-build
cd build\ffmpeg-build

# Configure with FFmpeg
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DKIVO_ENABLE_FFMPEG=ON -DKIVO_ENABLE_REAL_MEDIA_TESTS=ON

# Build
cmake --build .

# Test
ctest --output-on-failure
```

## Sample Media Setup

### Environment Variables
| Variable | Purpose | Example |
|----------|---------|---------|
| `KIVO_SAMPLE_H264_AAC_MP4` | H.264+AAC MP4 sample | `C:\media\sample.mp4` |

### Setting Environment Variables

#### PowerShell (Temporary)
```powershell
$env:KIVO_SAMPLE_H264_AAC_MP4 = "C:\path\to\sample.mp4"
```

#### PowerShell (Permanent)
```powershell
[System.Environment]::SetEnvironmentVariable("KIVO_SAMPLE_H264_AAC_MP4", "C:\path\to\sample.mp4", "User")
```

#### Batch File
```batch
set KIVO_SAMPLE_H264_AAC_MP4=C:\path\to\sample.mp4
```

## Test Behavior

### Unit Tests
- Always run regardless of FFmpeg availability
- Test structural correctness, default values, lifecycle
- FFmpeg-dependent tests check `is_ffmpeg_available()` and skip or test stub behavior

### Integration Tests
- **Skip** if environment variable is not set
- **Skip** if FFmpeg is not enabled
- **Skip** if sample file does not exist
- Report SKIP status in CTest output

## Troubleshooting

### FFmpeg Not Found
```
error: Could not find a package configuration file provided by "avformat"
```
**Solution**: Install FFmpeg via vcpkg:
```powershell
vcpkg install ffmpeg:x64-windows
```

### Ninja Not Found
```
error: CMake could not find generator "Ninja"
```
**Solution**: Install Ninja:
```powershell
winget install Ninja-build.Ninja
# or
choco install ninja
```

### Tests Not Registered
```
No tests were found!!!
```
**Solution**: Ensure `KIVO_ENABLE_REAL_MEDIA_TESTS=ON`:
```powershell
cmake .. -DKIVO_ENABLE_REAL_MEDIA_TESTS=ON
```
