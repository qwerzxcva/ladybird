#!/bin/sh
# Wrapper script to add NDK sysroot to linker arguments for Rust cross-compilation.
# This ensures that liblog and other Android system libraries can be found.

# Determine NDK home from environment or default path
NDK_HOME="${ANDROID_NDK_HOME:-$NDK_HOME}"
if [ -z "$NDK_HOME" ]; then
    NDK_HOME="/usr/local/lib/android/sdk/ndk/29.0.13599879"
fi

SYSROOT="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot"

# Find the real linker (avoid recursion)
REAL_LINKER="${CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER_REAL:-${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang}"

# Use -Bsymbolic and -B ${SYSROOT}/lib to find startup files
exec "$REAL_LINKER" "-B" "${SYSROOT}/usr/lib/aarch64-linux-android/30" "$@"
