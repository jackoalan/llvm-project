#pragma once

#if HSH_ENABLE_METAL

#if !defined(__OBJC__) || !defined(__cplusplus)
#error Code must be compiled as Objective-C++ [-xobjective-c++]
#endif
#if !defined(__has_feature)
#error Incompatible compiler
#endif
#if !__has_feature(objc_arc)
#error Code must be compiled with ARC [-fobjc-arc]
#endif

#include <Metal/Metal.h>
#include <QuartzCore/CoreAnimation.h>

#if HSH_ENABLE_METAL_BIN_MAC
#define HSH_METAL_TARGET METAL_BIN_MAC
#define HSH_GET_METAL_TARGET get_METAL_BIN_MAC
#define HSH_METAL_DATA data_METAL_BIN_MAC
#define HSH_METAL_CDATA cdata_METAL_BIN_MAC
#elif HSH_ENABLE_METAL_BIN_IOS
#define HSH_METAL_TARGET METAL_BIN_IOS
#define HSH_GET_METAL_TARGET get_METAL_BIN_IOS
#define HSH_METAL_DATA data_METAL_BIN_IOS
#define HSH_METAL_CDATA cdata_METAL_BIN_IOS
#endif

#endif
