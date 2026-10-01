#!/bin/sh
# Wrapper script to add NDK sysroot to linker arguments for Rust cross-compilation.
# This ensures that liblog and other Android system libraries can be found.

# Determine NDK home from environment or default path
NDK_HOME="${ANDROID_NDK_HOME:-$NDK_HOME}"
if [ -z "$NDK_HOME" ]; then
    # Try to detect from the compiler path
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    NDK_HOME="$(dirname "$(dirname "$SCRIPT_DIR")")"
fi

SYSROOT="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/sysroot"

# Find the real linker
LINKER="${CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER:-${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin/clang}"

exec "$LINKER" "--sysroot=${SYSROOT}" "$@"
