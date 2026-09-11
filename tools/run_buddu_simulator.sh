#!/bin/zsh
set -euo pipefail

repo_dir="${0:A:h:h}"
source_file="$repo_dir/tools/buddu_simulator.m"
binary_file="/tmp/tron-buddu-simulator"

/usr/bin/clang -fobjc-arc -Wall -Wextra -Werror -O2 \
  -framework Cocoa -framework CoreGraphics \
  "$source_file" -o "$binary_file"

exec "$binary_file"
