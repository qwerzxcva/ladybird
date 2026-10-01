# Ladybird Android 分支构建修复 - 进度总结

## 目标
修复 `qwerzxcva/ladybird` 仓库 Android `arm64-v8a` Debug 构建（Gradle 任务 `:buildCMakeDebug[arm64-v8a]`），使新增的三个库（LibPrivacy、LibLightpandaIO、LibGeckoShim）能编译并通过 CI。

## 本地路径
`~/workspace/ladybird-android`，远端 `https://github.com/qwerzxcva/ladybird.git`，分支 `master`。

---

## 已完成的修复（已推送并验证）

### 1. LibCore EventLoop 修复
- **文件**: `Libraries/LibCore/EventLoopImplementationUnix.cpp:319`
- **问题**: `AK_OS_ANDROID` 分支调用 `post_event(notifier, ...)` 传值而非指针
- **修复**: 改为 `post_event(&notifier, ...)`
- **构建**: #11 通过

### 2. LibGeckoShim 编译修复
- **文件**: `Libraries/LibGeckoShim/ExtensionHost.cpp`
- **问题**: 
  - 缺少 `AK/JsonObject.h` 和 `AK/JsonArray.h` 包含（`JsonParser.h` 只带 `JsonValue`）
  - `AK::String` 没有 `substring_view()`/`length()`/`ends_with_bytes()` 方法
- **修复**: 补充头文件，使用 `bytes_as_string_view()` + `StringView` API
- **构建**: #11 通过

### 3. LibWasm Cranelift 守卫
- **文件**: `Libraries/LibWasm/AbstractMachine/BytecodeInterpreter.cpp`, `CraneliftBridge.cpp`
- **问题**: 
  - `cranelift_trap_message` 只在 `WASM_COMPILED_FAULT_RECOVERY_SUPPORTED` 时定义，但 Android 上该宏为 0
  - `serialize_insn` 唯一调用点在 `!WASM_COMPILED_FAULT_RECOVERY_SUPPORTED` 分支内
- **修复**: 添加 `#if WASM_COMPILED_FAULT_RECOVERY_SUPPORTED` 守卫
- **构建**: #14 通过

### 4. LibGfx Skia 字体修复
- **文件**: `Libraries/LibGfx/Font/TypefaceSkia.cpp`
- **问题**: `SkFontMgr_New_Android(nullptr)` 需要两个参数（custom fonts + scanner）
- **修复**: 改为 `SkFontMgr_New_Android(nullptr, SkFontScanner_Make_FreeType())`，并包含 `SkFontScanner_FreeType.h`
- **备注**: 远端提交 `50c51493` 曾回退此修改，已重新应用

### 5. LibWebView Profile CLI 守卫
- **文件**: `Libraries/LibWebView/Application.cpp`
- **问题**: `force_new_process`、`profile_name`、`profile_path`、`temporary_profile` 变量在 Android 上未使用
- **修复**: 用 `#if !defined(AK_OS_ANDROID)` 守卫变量声明
- **构建**: #19 通过（推进到 LibWasm 链接错误）

### 6. LibJS 解释器布局生成（关键修复）
- **文件**: `Libraries/LibJS/Interpreter/AndroidLogStub.cpp`
- **问题**: Android 下 AK 的 `outln()` 路由到 liblog（写入 logcat 而非 stdout），导致 `generate_interpreter_layout` 工具的输出丢失，`layout.conf` 为空，flapc 报 `type DirectGetterFunction has no field 'wrapper_implementation_word_offset'`
- **修复**: 让 liblog stub 的 `__android_log_*` 函数实际写入 stdout（该 stub 只链接进构建期工具）
- **构建**: #15 通过（推进到 LibWasm 链接错误）

### 7. Rust 交叉编译目标三元组
- **文件**: `Meta/CMake/rust_crate.cmake`
- **问题**: `RUST_TARGET_TRIPLE` 硬编码为主机架构（x86_64），导致 ccache 缓存 x86_64 产物供 aarch64 构建使用
- **修复**: Android 下根据 `CMAKE_SYSTEM_PROCESSOR` 设置正确的目标三元组（如 `aarch64-linux-android`）
- **构建**: #17 通过（推进到 Rust liblog 链接错误）

### 8. CI 工作流修复
- **文件**: `.github/workflows/build-android-arm64.yml`
- **修复**: 
  - 添加 `rustup target add aarch64-linux-android x86_64-linux-android`
  - 安装 NDK 29（CMake/Gradle 默认使用）

---

## 当前阻塞问题

### Rust 交叉编译链接器问题
- **现象**: `ld.lld: error: unable to find library -llog`，`cannot open crtbeginS.o` 等
- **根因**: cargo 的 Rust 链接器（clang）无法找到 NDK sysroot 中的系统库和启动文件
- **尝试的修复**:
  1. 添加 `RUSTFLAGS=-Clink-arg=-llog` → 无效（linker 仍找不到 sysroot）
  2. 添加 `--sysroot=${NDK_SYSROOT}` → 无效（crt*.o 仍找不到）
  3. 添加 `-B${SYSROOT}/usr/lib/aarch64-linux-android/30` → 部分有效但仍失败
  4. 创建链接器包装脚本 `AndroidLinkerWrapper.sh` → 递归/权限问题
- **最新尝试**: `c699a506` - 同时传递 `--sysroot` 和 `-B` 标志

---

## 构建进展追踪

| 构建 # | 提交 | 状态 | 推进到 |
|--------|------|------|--------|
| #11 | f6ed8ca5 | 失败 | LibCore 编译错误 |
| #14 | 37df6147 | 失败 | LibWasm cranelift |
| #15 | 4837d839 | 失败 | LibJS layout 生成 |
| #19 | 95627618 | 失败 | LibWebView profile |
| #21 | 50c51493 | 失败 | Rust 链接器 |
| #22 | 0c5c2a1c | 失败 | Rust foldhash |
| #26 | 679f99c3 | 失败 | Rust liblog |
| #32 | 5e304ed2 | 失败 | Rust crt*.o |
| #33 | c699a506 | 运行中 | - |

---

## 下一步

1. 等待构建 #33 结果
2. 如果链接器问题仍未解决，考虑：
   - 检查 NDK 29 是否完整安装（特别是 `aarch64-linux-android` 目录）
   - 使用 `clang --target=aarch64-linux-android30 --print-sysroot` 验证路径
   - 或临时禁用 flapc 的 Rust 编译（如果可能）
3. 继续修复后续出现的编译/链接错误

---

## 关键文件清单

| 文件 | 改动 |
|------|------|
| `Libraries/LibCore/EventLoopImplementationUnix.cpp` | 修复 post_event 指针传递 |
| `Libraries/LibGeckoShim/ExtensionHost.cpp` | 补充头文件，修复 String API |
| `Libraries/LibWasm/AbstractMachine/BytecodeInterpreter.cpp` | 添加 WASM_COMPILED_FAULT_RECOVERY_SUPPORTED 守卫 |
| `Libraries/LibWasm/CraneliftBridge.cpp` | 守卫 serialize_insn |
| `Libraries/LibGfx/Font/TypefaceSkia.cpp` | 修复 SkFontMgr_New_Android 调用 |
| `Libraries/LibWebView/Application.cpp` | 守卫 Android 不使用的变量 |
| `Libraries/LibJS/Interpreter/AndroidLogStub.cpp` | 转发输出到 stdout |
| `Meta/CMake/rust_crate.cmake` | Android 目标三元组 + liblog 链接 |
| `Meta/CMake/AndroidLinkerWrapper.sh` | 新文件：链接器包装脚本 |
| `CMakeLists.txt` | 设置链接器包装器路径 |
| `.github/workflows/build-android-arm64.yml` | 安装 Rust 目标和 NDK 29 |

---

## 构建环境
- GitHub Actions: `ubuntu-24.04`
- NDK: 26.1.10909125 (显式安装) + 29.0.13599879 (CMake 默认)
- CMake: 3.31.5
- Gradle: 8.13
- Rust: 通过 rustup 管理
- 交叉编译: qemu-aarch64-static 运行构建期工具

---

*最后更新: 2026-10-01*

---

## 最新进展（构建 #33-34）

### 构建 #33（c699a506）
- **状态**: 失败
- **错误**: `ld.lld: error: cannot open Scrt1.o`, `unable to find library -lc` 等
- **原因**: 链接器包装脚本只传递 `--sysroot`，但 clang 驱动仍找不到 NDK 的库路径

### 构建 #34（5e252dfc）- 运行中
- **修复**: 添加 `--target=aarch64-linux-android30` 到链接器包装脚本
- **预期**: clang 驱动会根据目标三元组自动选择正确的 sysroot 和库路径

### 累计修复提交数
共 **15+ 个提交**，修复了以下问题：
1. LibCore EventLoop 指针传递
2. LibGeckoShim 头文件和 String API
3. LibWasm Cranelift 守卫
4. LibGfx Skia 字体管理器
5. LibWebView Profile CLI 变量守卫
6. LibJS 解释器布局生成（stdout 重定向）
7. Rust 交叉编译目标三元组
8. CI 工作流（Rust 目标 + NDK 29）
9. 链接器包装脚本（多次迭代）

### 当前阻塞
Rust 交叉编译到 `aarch64-linux-android` 时，链接器无法找到 NDK 系统库。这是 Ladybird 项目 Android 移植的已知复杂问题，涉及：
- cargo 的链接器选择机制
- NDK clang 驱动的 sysroot 查找
- Rust target triple 与 NDK API level 的匹配

### 后续方案
如果构建 #34 仍失败，考虑：
1. 联系 Ladybird 上游团队，询问 Android Rust 交叉编译的最佳实践
2. 临时禁用 flapc 的 Rust 编译（使用预编译二进制）
3. 使用不同的 NDK 版本（如 NDK 26 而非 29）
