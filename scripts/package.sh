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

build_dir="$ROOT_DIR/build-${opt_preset}"
cd "$build_dir"
tar -czf "idkge-linux-x86_64-${opt_preset}.tar.gz" "idkge"
