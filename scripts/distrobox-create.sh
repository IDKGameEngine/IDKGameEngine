#!/bin/bash

distrobox \
    create -i registry.gitlab.steamos.cloud/steamrt/steamrt4/sdk steamrt4 \
    --hostname steamrt4 \
    --additional-packages "libgl-dev pkg-config cmake ninja-build mesa-utils lsb-release gpg wget python3-gi gir1.2-gtk-3.0"

