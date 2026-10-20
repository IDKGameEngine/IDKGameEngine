project(idkgameplay)

add_library(idkgameplay STATIC
    src/GroundController.cpp src/PlayerController.cpp
    src/TopDownController.cpp src/SideScrollerController.cpp src/PatrolController.cpp)
target_compile_features(idkgameplay PUBLIC cxx_std_23)
target_include_directories(idkgameplay PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(idkgameplay PUBLIC idkengine_character libidk)
target_compile_options(idkgameplay PRIVATE -fno-exceptions)

if(IDK_BUILD_EDITOR_TESTS)
    enable_testing()
    add_executable(idk_player_controller_tests tests/PlayerController.cpp)
    target_link_libraries(idk_player_controller_tests PRIVATE idkgameplay)
    target_compile_options(idk_player_controller_tests PRIVATE -fno-exceptions)
    add_test(NAME idk_player_controller_tests COMMAND idk_player_controller_tests)

    add_executable(idk_controller_variants_tests tests/ControllerVariants.cpp)
    target_link_libraries(idk_controller_variants_tests PRIVATE idkgameplay)
    target_compile_options(idk_controller_variants_tests PRIVATE -fno-exceptions)
    add_test(NAME idk_controller_variants_tests COMMAND idk_controller_variants_tests)
endif()
