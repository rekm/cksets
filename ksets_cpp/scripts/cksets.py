import argparse
import pathlib
import ksets_cpp.uef_clustering.ksets as ksets
import ksets_cpp as cksets
import numpy as np
import sys
import time


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


# ChatGTP Version 5 was given my cpp sharg parser and asked to just transform
# it into an python argparse version
def get_args(args=None):
    parser = argparse.ArgumentParser(description="Clustering Algorithm")

    # Input file
    parser.add_argument(
        '-f', '--file',
        type=pathlib.Path,
        required=True,
        help="Input File"
    )

    # Number of clusters
    parser.add_argument(
        '-k', '--clusters',
        type=int,
        required=True,
        help="Number of Clusters"
    )

    # Maximum number of iterations
    parser.add_argument(
        '-i', '--max-iter',
        type=int,
        default=50,
        help="Maximum number of iterations (optional)"
    )

    
    # Pre-selection strategy
    parser.add_argument(
        '-p', '--preselect',
        type=str,
        choices=['none', 'ibf'],
        default='none',
        help="Pre-selection strategy for speeding up distance comparisons ('ibf' or 'none'). Default is 'none'"
    )

    # Histogram rebuild strategy
    parser.add_argument(
        '-r', '--rebuild',
        type=str,
        choices=['full', 'memorize'],
        default='full',
        help="Rebuild strategy for histograms ('full' or 'memorize'). Default is 'full'"
    )
    
    # Seed
    parser.add_argument(
        '-s', '--seed',
        type=int,
        default=42,
        help="Seed (optional, default=42)"
    )

    # Do nothing flag
    parser.add_argument(
        '-n', '--do-nothing',
        action='store_true',
        help="Only runs file IO"
    )

    return parser.parse_args(args)

def main(CMD=None):
    args = get_args(CMD)
    MAX_HIST_LENGTH = 0
    data = cksets.read_file(args.file)
    rand_seed = args.seed 
    np.random.seed(rand_seed)
    initial_hist = cksets.build_initial_histograms(data, args.clusters)
    start = time.perf_counter()
    labels, khist, sdh = cksets.cluster(data, args.clusters, MAX_HIST_LENGTH, 
                                        initial_hist,args.max_iter,hist_rebuild=args.rebuild, preselect=args.preselect)
    #ksets.cluster(data, args.clusters, MAX_HIST_LENGTH, initial_hist, 50)
    end = time.perf_counter()
    elapsed = end - start
    eprint(f"Algorithm Total Runtime (seconds): {elapsed}")
    print(*labels, sep="\n")
    #print("Histograms:")
    #print(khist)
    #print("Sum(sdh):", sum(sdh))
    #print(sdh)


if __name__ == "__main__": 
    main()