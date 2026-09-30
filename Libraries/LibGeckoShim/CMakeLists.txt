set(SOURCES
    ExtensionBridge.cpp
    ExtensionHost.cpp
)

ladybird_lib(LibGeckoShim geckoshim)
target_link_libraries(LibGeckoShim PRIVATE LibCore)
