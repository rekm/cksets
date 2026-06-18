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


BENCH_DIR="benchmarks"
LOCK_DIR="locks"
RESULT_DIR="results"
MAX_AGE=3600
BIN_FOLDER="../build/"
DATA_FOLDER="../data/" 
SOURCE_DIR="$(get_script_dir)"

main() {
    local benchmark_root="${1:-$SOURCE_DIR}"
    pushd "$benchmark_root"
    #larger_data_easy | rm_todos  
    set_todos
    run_worker
    popd
}


set_todos(){
    mkdir -p "$BENCH_DIR"
    get_benchmarks | while IFS="\n" read  opts;
    do 
        local hash=$(md5sum <<< $opts | cut -d" " -f1)
        mkdir -p "$BENCH_DIR"
        echo -n $opts > "$BENCH_DIR"/"$hash".todo;
    done
}

rm_todos() {
    mkdir -p "$BENCH_DIR"
    mkdir -p "$RESULT_DIR"
    while IFS="\n" read  opts;
    do 
        local hash=$(md5sum <<< $opts | cut -d" " -f1)
        [ -f "$RESULT_DIR"/"$hash".txt ] && rm "$RESULT_DIR"/"$hash".txt;
    done
}


art_benchmarks() {
    mapfile -t art_data < <(for f in $(ls -1 ${DATA_FOLDER}/artificial/data*) ; do 
        basename ${f%.txt}
    done | tr  "_" ' '| awk '{print "ksets_art_" $0}' ) 
    arraylength=${#art_data[@]}
    arraylength=1
    for (( i=0; i<${arraylength}; i++ )); do 
        printf "%s\n" "${art_data[$i]} --- cksets "{1..4}" dry 2 0 none full"
        printf "${art_data[$i]} --- %s\n" \
         "cksets "{1..4}" run "{2,4,8,16,32,64,128,256,384,512,768,1024}" "{0,0}" "{none,ibf}" "{full,memorize}
    done
}

larger_data_easy() {
    local clust_size=${1:-512}
    mapfile -t sim_data_4 < <(printf "%s\n" "sim_icd_data_40000_${clust_size}_1600 "{1200,2000,3000,4000,5000,6000}" NA ${clust_size} 0 1" )
    arraylength=${#sim_data_4[@]}
    for (( i=0; i<${arraylength}; i++ )); do 
        #printf "%s\n" "${sim_data_4[$i]} --- cksets "{1..4}" dry 2 0 none full"
        printf "${sim_data_4[$i]} --- %s\n" \
         "cksets "{1..3}" run "{${clust_size},${clust_size}}" "{0,0}" "{none,ibf}" "{full,memorize}    
    done
}

larger_data_intermediate() {
    local clust_size=${1:-512}
    mapfile -t sim_data_4 < <(printf "%s\n" "sim_icd_data_40000_${clust_size}_1600 "{7000,10000,15000}" NA ${clust_size} 0 1" )
    arraylength=${#sim_data_4[@]}
    for (( i=0; i<${arraylength}; i++ )); do 
        #printf "%s\n" "${sim_data_4[$i]} --- cksets "{1..4}" dry 2 0 none full"
        printf "${sim_data_4[$i]} --- %s\n" \
         "cksets "{1..3}" run "{${clust_size},${clust_size}}" "{0,0}" "{none,ibf}" "{full,memorize}    
    done
}

larger_data_hard() {
    local clust_size=${1:-512}
    mapfile -t sim_data_4 < <(printf "%s\n" "sim_icd_data_40000_${clust_size}_1600 "{30000,40000}" NA ${clust_size} 0 1" )
    arraylength=${#sim_data_4[@]}
    for (( i=0; i<${arraylength}; i++ )); do 
        #printf "%s\n" "${sim_data_4[$i]} --- cksets "{1..4}" dry 2 0 none full"
        printf "${sim_data_4[$i]} --- %s\n" \
         "cksets "{1..3}" run "{${clust_size},${clust_size}}" "{0,0}" "{none,ibf}" "{full,memorize}    
    done
}

get_benchmarks() {
    #art_benchmarks
    larger_data_easy #| tee /dev/stderr
    larger_data_intermediate 
    #mapfile -t sim_data_2 < <(printf "%s\n" "sim_icd_data_38451 "{1200,2000,4000,8000,10000,20000,30000,38451,50000}" NA 500 0 1" )
    #arraylength=${#sim_data_2[@]}
    #for (( i=0; i<${arraylength}; i++ )); do 
    #    #printf "%s\n" "${sim_data_2[$i]} --- cksets "{1..4}" dry 2 0 none full"
    #    printf "${sim_data_2[$i]} --- %s\n" \
    #     "cksets "{1..3}" run "{16,512}" "{0,0}" "{none,ibf}" "{full,memorize}
    #done
    #mapfile -t sim_data_3 < <(printf "%s\n" "sim_icd_data_38451_500_4600 "{1200,2000,3000,4000,5000,6000,7000,15000}" NA 500 0 1" )
    #arraylength=${#sim_data_3[@]}
    #for (( i=0; i<${arraylength}; i++ )); do 
    #    #printf "%s\n" "${sim_data_3[$i]} --- cksets "{1..4}" dry 2 0 none full"
    #    printf "${sim_data_3[$i]} --- %s\n" \
    #     "cksets "{1..3}" run "{16,16}" "{0,0}" "{none,ibf}" "{full,memorize}
    #
    #done
    
}



benchmark() {
    
    local bench_str="$*"
    1>&2 echo "------------ Starting Benchmark Run ---------------" 
    1>&2 echo "      Time: $(date)"
    1>&2 echo "Benchmark Definition: $bench_str" 
    local dataset_config="$( awk -vFS=' --- ' '{print $1}' <<< $bench_str )"
    local algorithm_config="$( awk -vFS=' --- ' '{print $2}' <<< $bench_str )"
    1>&2 echo "Dataset Definition: $dataset_config"
    1>&2 echo "Algorithm Definition: $algorithm_config"

    local dataset_file=$(get_dataset_file $dataset_config)
    1>&2 echo "Dataset File: $dataset_file"
    local run_cmd=$(get_run_cmd "$dataset_file" $algorithm_config )
    1>&2 echo "RUN CMD: $run_cmd"
    /usr/bin/time -v $run_cmd

}

truncate_or_duplicate() {
    local target_num=$1
    local s_file="$2"
    local out_file="$3" 
    awk -v N="$target_num" '{
                lines[NR] = $0
            }
            END {
                for (i=1; i<=N; i++) {
                    print lines[(i-1) % NR + 1]
                }
            }' "$s_file" > "$out_file"
}


get_dataset_file() {
    if [ "$1" == "ksets_art_data" ] ; then
        local fname=$(tr " " "_" <<< "${*:2}" )
        echo "${DATA_FOLDER}/artificial/data_${fname}.txt" 
    elif [ "$1" == "sim_icd_data_38451" ] ; then
        local fname=$(tr " " "_" <<< "${*:1}" )
        local source_f="${DATA_FOLDER}/my_sim/sim_icd_38451_500.txt"
        local target_f="${DATA_FOLDER}/my_sim/${fname}.txt"
        truncate_or_duplicate $2 $source_f $target_f
        echo $target_f 
    elif [ "$1" == "sim_icd_data_38451_500_4600" ] ; then
        local fname=$(tr " " "_" <<< "${*:1}" )
        local source_f="${DATA_FOLDER}/my_sim/sim_icd_38451_500_4600.txt"
        local target_f="${DATA_FOLDER}/my_sim/${fname}.txt"
        truncate_or_duplicate $2 $source_f $target_f
        echo $target_f
    elif [ "$1" == "sim_icd_data_40000_128_1600" ] ; then
        local fname=$(tr " " "_" <<< "${*:1}" )
        local source_f="${DATA_FOLDER}/my_sim/sim_icd_40000_128_1600.txt"
        local target_f="${DATA_FOLDER}/my_sim/${fname}.txt"
        truncate_or_duplicate $2 $source_f $target_f
        echo $target_f
    elif [ "$1" == "sim_icd_data_40000_512_1600" ] ; then
        local fname=$(tr " " "_" <<< "${*:1}" )
        local source_f="${DATA_FOLDER}/my_sim/sim_icd_40000_512_1600.txt"
        local target_f="${DATA_FOLDER}/my_sim/${fname}.txt"
        truncate_or_duplicate $2 $source_f $target_f
        echo $target_f
    else
        1>&2 echo "Unknown DataSet Configuration" 
        return 277
    fi
}

get_run_cmd() {
    local dataset="$1"
    local rep="$3"
    if [ "$2" == "cksets" ] ; then
        if [ "$4" == "dry" ] ; then
            echo "${BIN_FOLDER}/cksets" --do_nothing -f "$dataset"
        elif [ "$4" == "run" ] ; then
            local num_clust="$5"
            local hist_len="$6"
            local preselect="$7"
            local rebuild="$8"
            echo "${BIN_FOLDER}/cksets" -p "$preselect" -r "$rebuild"  -f "$dataset" -k "$num_clust"
        else
            1>&2 echo "Unknown Algorithm Configuration" 
            return 277
        fi
    else
        1>&2 echo "Unknown Algorithm Configuration" 
        return 277
    fi
}


run_worker() {
    mkdir -p "$LOCK_DIR" "$RESULT_DIR"
    for todo in "$BENCH_DIR"/*.todo; do
        name=$(basename "$todo" .todo)
        lock="$LOCK_DIR/$name.lock"
        result="$RESULT_DIR/$name.txt"

        # Skip if already done
        [[ -f "$result" ]] && continue
        if [[ -d "$lock" ]]; then
            lock_age=$(($(date +%s) - $(stat -c %Y "$lock")))
            if (( lock_age > MAX_AGE )); then
                echo "Reclaiming stale lock: $name"
                rm -rf "$lock"
            fi
        fi

        # Try to acquire lock
        if mkdir "$lock" 2>/dev/null; then
            # Ensure lock is released even on failure
            trap 'rm -rf "$lock"' EXIT

            echo "[$$] Running $name"

            tmp_result="$RESULT_DIR/$name.tmp"

            cmd=$(echo "benchmark" $(cat $todo))
            $cmd 2> "$tmp_result" 1>/dev/null
            # -------------------------------- 
            mv "$tmp_result" "$result"

            rm -rf "$lock"
            trap - EXIT
        fi
        
    done
}


main $@