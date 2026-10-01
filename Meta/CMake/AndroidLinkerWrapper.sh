#!/bin/sh
# Wrapper for Rust Android cross-compilation linker.
NDK_HOME="${ANDROID_NDK_HOME:-/usr/local/lib/android/sdk/ndk/29.0.13599879}"
SYSROOT="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
REAL="${CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER_REAL:-${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang}"
# Pass target and sysroot so clang driver knows where to find libs
exec "$REAL" "--target=aarch64-linux-android30" "--sysroot=${SYSROOT}" "$@"
