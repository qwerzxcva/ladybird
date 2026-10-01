#!/bin/sh
# Wrapper for Rust Android cross-compilation.
# Adds sysroot and library paths so the linker can find crt*.o and system libs.
NDK_HOME="${ANDROID_NDK_HOME:-/usr/local/lib/android/sdk/ndk/29.0.13599879}"
SYSROOT="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
REAL="${CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER_REAL:-${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang}"
# Pass --sysroot AND -B to ensure startup files and libs are found
exec "$REAL" "--sysroot=${SYSROOT}" "-B${SYSROOT}/usr/lib/aarch64-linux-android/30" "$@"
