/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

// AK's logging goes through liblog on Android. The JS interpreter layout generator
// is a build-time tool that has to run on the build host, so we link it statically
// and it cannot pull in the stub liblog.so. These entry points keep the tool
// self-contained; it only ever writes to stdout.
#include <stdarg.h>

extern "C" int __android_log_write(int priority, char const* tag, char const* text);
extern "C" int __android_log_print(int priority, char const* tag, char const* format, ...);
extern "C" int __android_log_vprint(int priority, char const* tag, char const* format, va_list args);

extern "C" int __android_log_write(int, char const*, char const*)
{
    return 0;
}

extern "C" int __android_log_print(int, char const*, char const*, ...)
{
    return 0;
}

extern "C" int __android_log_vprint(int, char const*, char const*, va_list)
{
    return 0;
}
