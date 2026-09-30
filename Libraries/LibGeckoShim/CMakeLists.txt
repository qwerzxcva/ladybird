set(SOURCES
    ExtensionBridge.cpp
    ExtensionHost.cpp
)

serenity_lib(LibGeckoShim geckoshim)
target_link_libraries(LibGeckoShim PRIVATE AK)
