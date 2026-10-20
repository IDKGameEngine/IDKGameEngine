#!/bin/bash
set -e

THIS_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
REPO_DIR=$(cd "${THIS_DIR}/.." && pwd)
THIRDPARTY_DIR="${REPO_DIR}/thirdparty"

INSTALL_PREFIX=""
CMAKE_FLAGS=()
PLATFORM=$(uname -s)
if [ "$PLATFORM" == "Linux" ]; then
    echo "Linux environment detected"
    INSTALL_PREFIX="${REPO_DIR}/local"
    CMAKE_FLAGS+=("-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}")
elif [[ "$PLATFORM" == *"Windows"* ]]; then
    echo "Windows environment detected"
    INSTALL_PREFIX="${REPO_DIR}/local-win"
    CMAKE_FLAGS+=("-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}"
                 "-DCMAKE_TOOLCHAIN_FILE=${REPO_DIR}/cmake/mingw-toolchain.cmake")
else
    echo "Unknown platform: $PLATFORM"
    exit 1
fi
echo "CMAKE_FLAGS: ${CMAKE_FLAGS[*]}"


build_assimp()
{
    cd "${THIRDPARTY_DIR}/assimp"
    cmake -S . -B build "${CMAKE_FLAGS[@]}" -DBUILD_SHARED_LIBS=ON -DASSIMP_BUILD_TESTS=OFF -DASSIMP_BUILD_ZLIB=ON
    cmake --build build --parallel "$(nproc)"
    cmake --install build
}

build_glm()
{
    cd "${THIRDPARTY_DIR}/glm"
    cmake -S . -B build "${CMAKE_FLAGS[@]}" -DGLM_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=OFF
    cmake --build build
    cmake --install build
}

build_jolt()
{
    cd "${THIRDPARTY_DIR}/JoltPhysics/Build"
    ./cmake_linux_clang_gcc.sh \
        Release g++ \
        "${CMAKE_FLAGS[@]}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DJPH_USE_VK=OFF \
        -DJPH_USE_DX12=OFF \
        -DJPH_USE_MTL=OFF
    cmake --build Linux_Release --parallel "$(nproc)"
    cmake --install Linux_Release
}

build_steamworks-sdk()
{
    cd "${THIRDPARTY_DIR}/steamworks-sdk"
    cmake -S . -B build "${CMAKE_FLAGS[@]}"
    cmake --build build
    cmake --install build
}

build_vulkan-sdk()
{
    cd "${THIRDPARTY_DIR}/vulkan-sdk"
    cmake -S . -B build "${CMAKE_FLAGS[@]}"
    cmake --build build
    cmake --install build
}

build_repos()
{
    local dependency
    for dependency in "$@"; do
        printf "\n\n-------------------------------- Building ${dependency} --------------------------------\n"
        "build_${dependency}"
    done
}

if [[ $# -eq 0 ]]; then
    set -- assimp glm jolt steamworks-sdk vulkan-sdk
fi

for dependency in "$@"; do
    case "$dependency" in
        assimp|glm|jolt|steamworks-sdk|vulkan-sdk) ;;
        *)
            echo "Unknown dependency: $dependency" >&2
            echo "Available dependencies: assimp glm jolt steamworks-sdk vulkan-sdk" >&2
            exit 1
            ;;
    esac
done

build_repos "$@"
