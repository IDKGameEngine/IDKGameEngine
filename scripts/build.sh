#!/bin/bash
set -e

THIS_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
ROOT_DIR=$(cd "${THIS_DIR}/.." && pwd)

opt_appname="game"
opt_preset="debug"
cmake_opts=()

while [[ $# -gt 0 ]]; do
    case $1 in
        --appname=*)
            opt_appname="${1#*=}"
            shift
            ;;
        --preset=*)
            opt_preset="${1#*=}"
            shift
            ;;
        --cmakeopts)
            shift
            cmake_opts=("$@")
            shift $#
            ;;
        *)
            echo "Unknown option $1" >&2
            exit 1
            ;;
    esac
done

if [[ "${opt_appname}" == "" ]]; then
    echo "Must specify --appname"
    exit
fi

cd "$ROOT_DIR"
cmake --preset "$opt_preset" -DIDK_APP_NAME="$opt_appname" "${cmake_opts[@]}"
cmake --build --preset "$opt_preset" --parallel "$(nproc)"
cmake --install "${ROOT_DIR}/build-${opt_preset}/cmake"
"${THIS_DIR}/version.sh" "build-${opt_preset}"
