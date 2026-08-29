#!/usr/bin/env bash

set -e

# The engine never builds on its own: it is always compiled from a consuming
# project, so what there is to build here are the examples.
cd "$(dirname "$0")"

for dir in examples/*/; do
	[ -f "$dir/Makefile" ] || continue

	echo "Building $dir"
	make -C "$dir" clean
	make -C "$dir" -j4
done

echo "Build done!"
