#!/bin/bash

echo "> distrobox enter steamrt4"
distrobox enter steamrt4 -- env PS1='\[\e[1;35m\][📦 \h]:\[\033[01;34m\]\w\[\033[00m\]\$ ' bash --noprofile --norc -i
