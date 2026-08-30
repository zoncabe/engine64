#!/usr/bin/env bash

set -e

# The engine never builds on its own: it is always compiled from a consuming
# project, so what there is to build here are the examples.
#
#   ./build.sh          clean and build every example
#   ./build.sh clean    clean only
cd "$(dirname "$0")"

clean_only=0
[ "$1" = clean ] && clean_only=1

for dir in examples/*/; do
	[ -f "$dir/Makefile" ] || continue

	echo "Cleaning $dir"
	make -C "$dir" clean

	[ "$clean_only" = 1 ] && continue

	echo "Building $dir"
	make -C "$dir" -j4
done

if [ "$clean_only" = 1 ]; then
	echo "Clean done!"
else
	echo "Build done!"
fi
