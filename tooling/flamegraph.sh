#!/usr/bin/env bash
set -euo pipefail

get_script_dir() {
    local SOURCE=$0
    while [ -h "$SOURCE" ]; do # Resolve $SOURCE until the file is no longer a symlink
        DIR=$(cd -P "$(dirname "$SOURCE")" && pwd)
        SOURCE=$(readlink "$SOURCE")
        [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE # If $SOURCE was a relative symlink, resolve it relative to the symlink base directory
    done
    DIR=$(cd -P "$(dirname "$SOURCE")" && pwd)
    echo "$DIR"
}

SOURCE_DIR="$(get_script_dir)"

main() {
    local name=$1
    perf record -F 999 -g -o "$name.data" -- timeout 120s ${@:2} || true
    echo "Processing $name ..."
    perf script -i "$name.data" > "$name.perf"
    "${SOURCE_DIR}/stackcollapse-perf.pl" "$name.perf" > "$name.folded"
    "${SOURCE_DIR}/flamegraph.pl" "$name.folded"  > "$name.svg" 
}






main $@