#!/bin/bash
set -e

THIS_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
ROOT_DIR=$(cd "${THIS_DIR}/.." && pwd)

opt_preset="debug"
opt_appname="game"

while [[ $# -gt 0 ]]; do
    case $1 in
        --preset=*)
            opt_preset="${1#*=}"
            shift
            ;;
        --appname=*)
            opt_appname="${1#*=}"
            shift
            ;;
        *)
            echo "Unknown option $1" >&2
            exit 1
            ;;
    esac
done

case "$opt_preset" in
    debug|release) ;;
    *)
        echo "Unknown preset: $opt_preset" >&2
        exit 1
        ;;
esac

build_dir="build-${opt_preset}"
for binary in "$opt_appname" editor mapedit mapc; do
    if [[ ! -x "${ROOT_DIR}/${build_dir}/${binary}" ]]; then
        echo "Missing executable: ${build_dir}/${binary}" >&2
        exit 1
    fi
done
if [[ ! -d "${ROOT_DIR}/${build_dir}/content" ]]; then
    echo "Missing package content: ${build_dir}/content" >&2
    exit 1
fi
for path in version.txt steam_appid.txt; do
    if [[ ! -s "${ROOT_DIR}/${build_dir}/${path}" ]]; then
        echo "Missing package content: ${build_dir}/${path}" >&2
        exit 1
    fi
done

cd "$ROOT_DIR"
tar -czf "idk-linux-x86_64-${opt_preset}.tar.gz" "$build_dir"
