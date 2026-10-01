/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

// AK's logging goes through liblog on Android. The JS interpreter layout generator
// is a build-time tool that has to run on the build host, so we link it statically
// and it cannot pull in the stub liblog.so. These entry points keep the tool
// self-contained.
//
// The tool's stdout IS its result: CMake redirects it into layout.conf, which flapc
// then consumes. AK's Android outln() would send everything to liblog instead, so
// these stubs forward the already-formatted text (including any trailing newline)
// to stdout to preserve the tool's contract.
#include <stdarg.h>
#include <stdio.h>

extern "C" int __android_log_write(int priority, char const* tag, char const* text);
extern "C" int __android_log_print(int priority, char const* tag, char const* format, ...);
extern "C" int __android_log_vprint(int priority, char const* tag, char const* format, va_list args);

extern "C" int __android_log_write(int, char const*, char const* text)
{
    if (text == nullptr)
        return 0;
    return static_cast<int>(fputs(text, stdout)) < 0 ? -1 : 0;
}

extern "C" int __android_log_print(int, char const*, char const* format, ...)
{
    if (format == nullptr)
        return 0;
    va_list args;
    va_start(args, format);
    int const result = vprintf(format, args);
    va_end(args);
    return result;
}

extern "C" int __android_log_vprint(int, char const*, char const* format, va_list args)
{
    if (format == nullptr)
        return 0;
    return vprintf(format, args);
}
