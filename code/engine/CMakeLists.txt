project(idkengine_engine)

# Character input and commands are usable without SDL, rendering or Jolt.
add_library(idkengine_character STATIC src/CharacterInputHandler.cpp)
target_compile_features(idkengine_character PUBLIC cxx_std_23)
target_include_directories(idkengine_character PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(idkengine_character PUBLIC glm::glm-header-only)
target_compile_options(idkengine_character PRIVATE -fno-exceptions)

add_library(
    ${PROJECT_NAME} OBJECT
)
target_compile_features(
    ${PROJECT_NAME} PUBLIC cxx_std_23
)
target_include_directories(
    ${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC idkengine_common idkengine_gfx idkengine_character idkengine_network
    PRIVATE Steamworks::Steamworks Jolt::Jolt
)
target_sources(
    ${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/NetService.cpp"
)

if(IDK_BUILD_EDITOR_TESTS)
    enable_testing()
    add_executable(idk_character_tests tests/CharacterController.cpp)
    target_link_libraries(idk_character_tests PRIVATE idkengine_character)
    target_compile_options(idk_character_tests PRIVATE -fno-exceptions)
    add_test(NAME idk_character_tests COMMAND idk_character_tests)

    add_executable(idk_timing_tests tests/EngineTiming.cpp)
    target_link_libraries(idk_timing_tests PRIVATE idkengine_timing libidk)
    target_compile_options(idk_timing_tests PRIVATE -fno-exceptions)
    add_test(NAME idk_timing_tests COMMAND idk_timing_tests)

    add_executable(idk_timing_reload_tests tests/EngineTimingReload.cpp)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/content")
    target_link_libraries(idk_timing_reload_tests PRIVATE idkengine)
    target_compile_options(idk_timing_reload_tests PRIVATE -fno-exceptions)
    add_test(NAME idk_timing_reload_tests COMMAND idk_timing_reload_tests)
    set_tests_properties(idk_timing_reload_tests PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=offscreen;SDL_AUDIODRIVER=dummy" TIMEOUT 15)
endif()
