set(SOURCES
    ExtensionBridge.cpp
)

serenity_lib(LibGeckoShim geckoshim)
target_link_libraries(LibGeckoShim PRIVATE AK)
