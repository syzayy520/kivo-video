#ifndef KIVO_VIDEO_SUBTITLE_PLANE_FOUNDATION_P8_EXPORT_H
#define KIVO_VIDEO_SUBTITLE_PLANE_FOUNDATION_P8_EXPORT_H

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
#if defined(KIVO_P8_BUILD_DLL)
#define KIVO_P8_API __declspec(dllexport)
#elif defined(KIVO_P8_USE_DLL)
#define KIVO_P8_API __declspec(dllimport)
#else
#define KIVO_P8_API
#endif
#define KIVO_P8_CALL __cdecl
#else
#define KIVO_P8_API
#define KIVO_P8_CALL
#endif

#define KIVO_P8_NOEXCEPT

#ifdef __cplusplus
#define KIVO_P8_EXTERN_C_BEGIN extern "C" {
#define KIVO_P8_EXTERN_C_END }
#else
#define KIVO_P8_EXTERN_C_BEGIN
#define KIVO_P8_EXTERN_C_END
#endif

#endif
