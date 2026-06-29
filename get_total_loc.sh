#!/bin/bash

EXCLUDE=(
    "./stb_ds.h"
)

find_args=()

for path in "${EXCLUDE[@]}"; do
    find_args+=( -path "$path" -prune -o )
done

total=0

while IFS= read -r -d '' file; do
    lines=$(wc -l < "$file")
    printf "%6d  %s\n" "$lines" "$file"
    ((total += lines))
done < <(
    find . \
        "${find_args[@]}" \
        \( -name "*.c" -o -name "*.h" \) \
        -type f -print0
)

echo "------------------------------"
echo "Total LOC: $total"
