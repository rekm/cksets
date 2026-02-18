#!/usr/bin/env python3
import re
import argparse
import pathlib
from datetime import datetime

LOCAL_FOLDER = pathlib.Path(__file__).parent

def parse_args(cmds):
    parser = argparse.ArgumentParser(prog="ResultCollector")
    parser.add_argument("--bdir", default=LOCAL_FOLDER, type=pathlib.Path)

    return parser.parse_args(cmds)


def parse_file(args, _f: pathlib.Path):
    _id          = _f.stem
    (
        start_time,   
        dataset_type, 
        dataset_len,
        dataset_reso,
        dataset_tn_clust,
        dataset_overlap,
        dataset_imb_type,
        algo_id,    
        algo_rep,   
        algo_runtype,
        algo_nclust,  
        algo_histlen,  
        algo_preselect,
        algo_rebuild,
        algo_t_preprocess,
        algo_t_algo,
        algo_t_wall,
        algo_max_memory,
    ) = ([None] * 18)
    with _f.open("r") as fh:
        for line in fh.readlines():
            line = line.strip()
            line_parse = line.split("Time:")
            if len(line_parse) == 2:
                start_time=line_parse[1].strip()
                clean_time = ' '.join(start_time.split()[1:4] + start_time.split()[5:])
                clean_time = clean_time.replace('.', '')
                dt_time =  datetime.strptime(clean_time, "%d %b %H:%M:%S %Y")
                start_time=dt_time
            line_parse = line.split("Dataset Definition:")
            if len(line_parse) == 2:
                dataset_opts=line_parse[1].strip().split(" ")
                dataset_type     = dataset_opts[0]
                dataset_len      = dataset_opts[1]
                dataset_reso     = dataset_opts[2]
                dataset_tn_clust = dataset_opts[3]
                dataset_overlap  = dataset_opts[4]
                dataset_imb_type = dataset_opts[5]
            line_parse = line.split("Algorithm Definition:")
            # Example: cksets 2 run 512 0 none full
            if len(line_parse) == 2:
                algo_opts = line_parse[1].strip().split(" ")
                algo_id      = algo_opts[0] 
                algo_rep     = algo_opts[1]
                algo_runtype = algo_opts[2]
                if len(algo_opts) >= 7:
                    algo_nclust    = algo_opts[3]
                    algo_histlen   = algo_opts[4]
                    algo_preselect = algo_opts[5]
                    algo_rebuild   = algo_opts[6]
            line_parse = line.split("Initial Data Processing (seconds):")
            if len(line_parse) == 2:
                algo_t_preprocess = line_parse[1].strip()
            line_parse = line.split("Algorithm Total Runtime (seconds):")
            if len(line_parse) == 2:
                algo_t_algo = line_parse[1].strip()
            line_parse = line.split("Elapsed (wall clock) time (h:mm:ss or m:ss):")
            if len(line_parse) == 2:
                algo_t_wall = line_parse[1].strip()
            line_parse = line.split("Maximum resident set size (kbytes):")
            if len(line_parse) == 2:
                algo_max_memory = line_parse[1].strip()
            
    print(
        _id, 
        start_time,
        dataset_type,
        dataset_len,
        dataset_reso,
        dataset_tn_clust,
        dataset_overlap,
        dataset_imb_type,
        algo_id,    
        algo_rep,   
        algo_runtype,
        algo_nclust,  
        algo_histlen,  
        algo_preselect,
        algo_rebuild,
        algo_t_preprocess,
        algo_t_algo,
        algo_t_wall,
        algo_max_memory,  
        sep="\t")
         

def main(CMD=None):
    args = parse_args(CMD)
    
    print(
        "id", 
        "start_time",
        "dataset_type",
        "dataset_len",
        "dataset_reso",
        "dataset_tn_clust",
        "dataset_overlap",
        "dataset_imb_type",
        "algo_id",    
        "algo_rep",   
        "algo_runtype",
        "algo_nclust",  
        "algo_histlen",  
        "algo_preselect",
        "algo_rebuild",
        "algo_t_preprocess",
        "algo_t_algo",
        "algo_t_wall",
        "algo_max_memory",
        sep="\t"
    )
    all_results = (args.bdir / "results").glob("*.txt")
    for i, _file in enumerate(all_results):
        #if i >= 1:
        #    break 
        parse_file(args, _file)
         


if __name__ == "__main__":
    main()