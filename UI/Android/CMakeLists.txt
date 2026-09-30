add_library(ladybird SHARED
    src/main/cpp/LadybirdActivity.cpp
    src/main/cpp/WebViewImplementationNative.cpp
    src/main/cpp/WebViewImplementationNativeJNI.cpp
    src/main/cpp/ALooperEventLoopImplementation.cpp
    src/main/cpp/TimerExecutorService.cpp
    src/main/cpp/JNIHelpers.cpp
)

# Link all custom modules: Privacy, Lightpanda IO, Gecko Extension Shim
target_link_libraries(ladybird PRIVATE
    LibFileSystem
    LibPrivacy
    LibLightpandaIO
    LibGeckoShim
    jnigraphics
    android
    log
)

include(../cmake/AndroidExtras.cmake)
create_ladybird_bundle(ladybird)
