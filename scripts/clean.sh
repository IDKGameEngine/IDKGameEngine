#!/bin/bash
set -e

THIS_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
ROOT_DIR=$(cd "${THIS_DIR}/.." && pwd)

rm -r $THIS_DIR/../build-{debug,release}
