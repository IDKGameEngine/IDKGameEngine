find_package(assimp REQUIRED)
find_package(glm REQUIRED)
find_package(Jolt REQUIRED)
find_package(Steamworks CONFIG REQUIRED)

add_subdirectory("thirdparty/glad")
add_subdirectory("thirdparty/imgui")
add_subdirectory("thirdparty/ImGuizmo")

# Reuse the engine ImGui build when ImGuizmo is embedded.
target_link_libraries(imguizmo PUBLIC IdkThirdParty::imgui)
