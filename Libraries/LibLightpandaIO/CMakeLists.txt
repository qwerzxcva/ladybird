set(SOURCES
    AsyncIOScheduler.cpp
)

serenity_lib(LibLightpandaIO lightpandaio)
target_link_libraries(LibLightpandaIO PRIVATE AK)
