#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#ifdef _WIN32

#ifndef WINVER
#define WINVER 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif
// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run
// your application.  The macros work by enabling all features available on platform versions up to and
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER						  // Specifies that the minimum required platform is Windows Vista.
#define WINVER 0x0600		   // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT			// Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT WINVER	 // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS		  // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE					   // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700		// Change this to the appropriate value to target other versions of IE.
#endif

#ifndef EDITOR
#include <windows.h>
#else
#include <afx.h>
#endif

#ifdef _DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#undef DeleteFile
#undef GetCurrentDirectory
#undef SetCurrentDirectory
#undef DrawText
#undef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline
#endif

#define USE_STDERR

#else // _WIN32

#include <sys/param.h>
#include <unistd.h>

#define INFINITE 0xFFFFFFFF  // Infinite timeout

#if defined(__APPLE__) && defined(__MACH__)
typedef void* HWND;
// Apple OSX and iOS (Darwin)
#include <TargetConditionals.h>

#if TARGET_OS_IPHONE == 1 || TARGET_IPHONE_SIMULATOR == 1
#define EMBEDDED_SYSTEM
#define __IOS__

#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
#define USE_CLOCK_GETTIME
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED

#else // __IOS__
#define __MACOSX__
struct XVisualInfo;

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
#define USE_CLOCK_GETTIME
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED

//#define USE_X11
#endif // __MACOSX__

#elif defined(__ANDROID__)
#define EMBEDDED_SYSTEM
#define USE_CLOCK_GETTIME
#include <android/api-level.h>
#include <android/log.h>
#include <android/window.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <android/sensor.h>
typedef ANativeWindow* HWND;
typedef ANativeWindow* Window;
#elif defined(__linux__) || defined(__unix__) || defined(__gnu_linux) || 1
#define USE_CLOCK_GETTIME
#define USE_X11
typedef unsigned long HWND;
#endif

#include <pthread.h>
#include <sys/resource.h>

typedef void* HINSTANCE;
struct WNDCLASS;
#define GetModuleHandle(x) (x)

#ifndef EMBEDDED_SYSTEM
#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xcb/xcb.h>
#else
typedef void* Window;
struct Display;
#endif // USE_X11
#endif // EMBEDDED_SYSTEM

#define USE_STDERR

#endif // _WIN32

#include "types.h"

#if defined(__GNUC__) || defined(__clang__)

#ifndef stricmp
#define stricmp strcasecmp
#endif // stricmp
#ifndef strnicmp
#define strnicmp strncasecmp
#endif // strnicmp

#endif // defined(__GNUC__) || defined(__clang__)

#ifndef INLINE
#define INLINE inline
#endif

#ifndef MAKEFOURCC
	#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
		(uint(uchar(ch0)) | (uint(uchar(ch1)) << 8) | (uint(uchar(ch2)) << 16) | (uint(uchar(ch3)) << 24))
#endif

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#ifndef DLL_EXPORT
#define DLL_EXPORT
#endif

#endif // __PLATFORM_H__
