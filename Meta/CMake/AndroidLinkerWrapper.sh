#!/bin/sh
# Wrapper script for Rust Android cross-compilation linker.
NDK_HOME="${ANDROID_NDK_HOME:-/usr/local/lib/android/sdk/ndk/29.0.13599879}"
SYSROOT="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot"
REAL="${CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER_REAL:-${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang}"
exec "$REAL" "--sysroot=${SYSROOT}" "$@"
