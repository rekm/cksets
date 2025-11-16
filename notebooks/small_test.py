import ksets_cpp as cksets
import pathlib
datadir = pathlib.Path("..") / "data" / "artificial"
data, rep, gt_labels = list(datadir.glob("*1200_200_16_5_1*"))
data = cksets.read_file(str(data))
K = 16 # Number of clusters
MAX_HIST_LENGTH = 20 # if zero no limitation
RS_ITER = 300 # number of iterations for kswaps
rand_seed = 10

labels, khist, sdh = cksets.cluster(data, K, MAX_HIST_LENGTH)