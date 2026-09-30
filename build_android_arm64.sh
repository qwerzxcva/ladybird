#!/bin/bash
# Ladybird Android ARMv8 Build Script
# Builds a privacy-focused, Gecko-extension-compatible APK for arm64-v8a only.
#
# Prerequisites:
#   - Android SDK with NDK r29+ installed
#   - ANDROID_HOME and ANDROID_NDK_HOME environment variables set
#   - vcpkg installed and VCPKG_ROOT set
#   - CMake 3.25+, Ninja, Java 17+
#
# Usage:
#   chmod +x build_android_arm64.sh
#   ./build_android_arm64.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Ladybird Android ARMv8 Build ==="
echo "Working directory: $SCRIPT_DIR"

# Verify environment
if [ -z "${ANDROID_HOME:-}" ]; then
    echo "ERROR: ANDROID_HOME is not set. Please install Android SDK and set ANDROID_HOME."
    exit 1
fi

if [ -z "${ANDROID_NDK_HOME:-}" ]; then
    # Try to auto-detect NDK from ANDROID_HOME
    NDK_CANDIDATE="$ANDROID_HOME/ndk/29.0.13599879"
    if [ -d "$NDK_CANDIDATE" ]; then
        export ANDROID_NDK_HOME="$NDK_CANDIDATE"
        echo "Auto-detected NDK: $ANDROID_NDK_HOME"
    else
        echo "ERROR: ANDROID_NDK_HOME is not set and could not auto-detect NDK."
        exit 1
    fi
fi

if [ -z "${VCPKG_ROOT:-}" ]; then
    echo "WARNING: VCPKG_ROOT is not set. vcpkg dependencies may fail to resolve."
    echo "         Set VCPKG_ROOT to your vcpkg installation directory."
fi

echo "ANDROID_HOME:     $ANDROID_HOME"
echo "ANDROID_NDK_HOME: $ANDROID_NDK_HOME"
echo "VCPKG_ROOT:       ${VCPKG_ROOT:-<not set>}"
echo ""

# Build the APK via Gradle (which drives CMake + NDK internally)
echo "=== Starting Gradle build for arm64-v8a ==="
cd UI/Android

# Use Gradle wrapper if available, otherwise system gradle
if [ -f "./gradlew" ]; then
    GRADLE_CMD="./gradlew"
elif [ -f "../../gradlew" ]; then
    GRADLE_CMD="../../gradlew"
else
    GRADLE_CMD="gradle"
fi

# Assemble debug APK for arm64-v8a
$GRADLE_CMD assembleDebug \
    -PandroidBuildOnlyArm64=true \
    --no-daemon \
    --stacktrace

APK_PATH="app/build/outputs/apk/debug/app-debug.apk"

if [ -f "$APK_PATH" ]; then
    echo ""
    echo "=== BUILD SUCCESSFUL ==="
    echo "APK location: $SCRIPT_DIR/UI/Android/$APK_PATH"
    ls -lh "$APK_PATH"
    echo ""
    echo "To install on a connected device:"
    echo "  adb install $APK_PATH"
else
    echo ""
    echo "=== BUILD FAILED ==="
    echo "Expected APK not found at: $APK_PATH"
    echo "Check the build output above for errors."
    exit 1
fi
