#!/usr/bin/env python3
import re
import argparse
import pathlib
import locale 
from datetime import datetime

locale.setlocale(locale.LC_TIME, "de_DE.UTF-8")

LOCAL_FOLDER = pathlib.Path(__file__).parent

def parse_args(cmds):
    parser = argparse.ArgumentParser(prog="ResultCollector")
    parser.add_argument("--bdir", default=LOCAL_FOLDER, type=pathlib.Path)
    parser.add_argument("--locale", choices=["none", "de"], default="none")
    parser.add_argument("--identifier",  "-i", default=None)
    parser.add_argument("--id_name", "-n", default="SecondID")
    return parser.parse_args(cmds)


def parse_file(args, _f: pathlib.Path, sec_id=None):

    _id = _f.stem
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
        algo_iterations,
        algo_mean_preselect_savings,
        algo_distance_score, 
        algo_t_preprocess,
        algo_t_algo,
        algo_t_wall,
        algo_max_memory,
    ) = ([None] * 21)
    algo_preselect_savings = []
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
           
            
            algo_t_preprocess   = easy_split_parse(line, algo_t_preprocess,  "Initial Data Processing (seconds):")
            algo_t_algo         = easy_split_parse(line, algo_t_algo,        "Algorithm Total Runtime (seconds):")
            algo_t_wall         = easy_split_parse(line, algo_t_wall,        "Elapsed (wall clock) time (h:mm:ss or m:ss):")
            algo_max_memory     = easy_split_parse(line, algo_max_memory,    "Maximum resident set size (kbytes):")
            algo_distance_score = easy_split_parse(line, algo_distance_score, "Sum of Total Distances:")
            algo_iterations     = easy_split_parse(line, algo_iterations, "Total Iterations:")

            line_parse = re.match(pattern=r".*Avg Preselect Savings: *([\d\.]+)%.*", string=line)
            if line_parse is not None:
                algo_preselect_savings.append(float(line_parse.group(1)))

    if algo_preselect_savings and algo_iterations is not None:
        algo_mean_preselect_savings = sum(algo_preselect_savings) / float(algo_iterations)

    data_row = [_id]
    if sec_id is not None:
         data_row.append(sec_id)
    
    data_row.extend([    
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
        algo_iterations,
        algo_mean_preselect_savings,
        algo_distance_score,
        algo_t_preprocess,
        algo_t_algo,
        algo_t_wall,
        algo_max_memory
    ])

    print(
        *data_row,  
        sep="\t")
         

def easy_split_parse(line, var, split_str, default=None):
    ret = var
    if ret is None or ret == default:
        line_parse = line.split(split_str)
        if len(line_parse) == 2:
            ret = line_parse[1].strip()
    return ret

def main(CMD=None):
    args = parse_args(CMD)
    
    if args.locale != "none":
         locales_dict = {"de": "de_DE.UTF-8"}
         locale.setlocale(locale.LC_TIME, locales_dict[args.locale])

    header = ["id"]
    if args.identifier is not None:
         header.append(args.id_name)
    header.extend([ 
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
        "algo_iterations",
        "algo_mean_preselect_savings",
        "algo_distance_score",
        "algo_t_preprocess",
        "algo_t_algo",
        "algo_t_wall",
        "algo_max_memory"
        ])     
    print(*header,
        sep="\t"
    )
    all_results = (args.bdir / "results").glob("*.txt")
    for i, _file in enumerate(all_results):
        #if i >= 1:
        #    break 
        parse_file(args, _file, sec_id=args.identifier)
         


if __name__ == "__main__":
    main()