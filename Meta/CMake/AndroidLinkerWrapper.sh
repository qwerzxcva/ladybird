#!/bin/sh
# Wrapper script to add NDK sysroot to linker arguments for Rust cross-compilation.
# This ensures that liblog and other Android system libraries can be found.

# Determine NDK home from environment or default path
NDK_HOME="${ANDROID_NDK_HOME:-$NDK_HOME}"
if [ -z "$NDK_HOME" ]; then
    # Fallback: try to detect from common locations
    NDK_HOME="/usr/local/lib/android/sdk/ndk/29.0.13599879"
fi

SYSROOT="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot"

# Find the real linker (avoid recursion by not using $0)
REAL_LINKER="${CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER_REAL:-${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang}"

exec "$REAL_LINKER" "--sysroot=${SYSROOT}" "$@"
