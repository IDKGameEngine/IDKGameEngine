#!/bin/bash
set -e

THIS_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
ROOT_DIR=$(cd "${THIS_DIR}/.." && pwd)
BUILD_DIR="${ROOT_DIR}/${1}"
mkdir -p "$BUILD_DIR"

write_version()
{
    local name="$1"
    local directory="$2"
    local commit status
    if [[ ! -d "$directory" ]]; then
        return
    fi
    commit=$(git -C "$directory" rev-parse HEAD)
    status=clean
    if [[ -n "$(git -C "$directory" status --porcelain)" ]]; then
        status=dirty
    fi
    printf '%s %s %s\n' "$name" "$commit" "$status" >> "$BUILD_DIR/version.txt"
}

: > "$BUILD_DIR/version.txt"
write_version IDKGameEngine "$ROOT_DIR"
write_version libidk "${ROOT_DIR}/../libidk"
write_version game "${ROOT_DIR}/../idk_game"
