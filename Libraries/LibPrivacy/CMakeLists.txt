set(SOURCES
    PrivacyConfig.cpp
)

serenity_lib(LibPrivacy privacy)
target_link_libraries(LibPrivacy PRIVATE AK LibCrypto)
