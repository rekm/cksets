"""
K-sets clustering algorithm directly copied from 
Git: https://github.com/uef-machine-learning/K-sets_and_K-swaps
Commit: 5d8029adc0d7bcb616997eb39e462713d6995ac0
Authors: 
Cite: M. Rezaei, P. Fränti. “K-sets and k-swaps algorithms for clustering sets”, submitted to Pattern Recognition Journal.
"""
import numpy as np
import ksets_cpp.data_utils as data_utility
from scipy import spatial
import math

# Constant
LOG_ENABLE = False
LOG_ITERCOUNT = 20

# Private starts
def _get_data_per_cluster(current_cluster_id, labels, data):
    return data[np.where(labels == current_cluster_id)[0]]

def _build_initial_histograms(data, number_of_cluster):
    histograms = []
    N = len(data)
    indices = np.random.choice(N, number_of_cluster, replace=False)
    for i in range(number_of_cluster):
        cluster_i_data = data[indices[i]]
        unique, counts = np.unique(cluster_i_data, return_counts=True)
        histogram_dict = dict(zip(unique, counts))
        histogram_dict.pop(data_utility.DEFAULT_STR, None)
        histograms.append(histogram_dict)
    
    return histograms

def _get_histogram(labels, data, number_of_cluster, max_hist_length):
    histograms = []
    for i in range(number_of_cluster):
        cluster_i_data = _get_data_per_cluster(i, labels, data)
        unique, counts = np.unique(cluster_i_data, return_counts=True)
        idx = np.argsort(counts)[::-1]
        if max_hist_length != 0:
            counts = counts[idx[0:max_hist_length+1]]
            unique = unique[idx[0:max_hist_length+1]]
        histogram_dict = dict(zip(unique, counts))
        histogram_dict.pop(data_utility.DEFAULT_STR, None)

        histograms.append(histogram_dict)
    return histograms

def _distance_to_histogram(data, histogram):
    not_match = 0
    summation = 0
    
    for k in range(len(data)):
        item = data[k]
        if item in histogram:
            summation = summation + histogram[item]
        else:
            not_match = not_match + 1

    distance = 1 - (summation / (sum(histogram.values()) + not_match))

    return distance

# Sum of distances to histogram for each cluster
def _get_sumd(data, labels, histograms, number_of_cluster):
    
    sumd = np.zeros(number_of_cluster)

    for i in range(data.shape[0]):
        j = labels[i]
        distance = _distance_to_histogram(data[i], histograms[j])
        sumd[j] = sumd[j] + distance
        
    return sumd


def _progress_log(i):
    if i % LOG_ITERCOUNT == 0:
        print('\nin progress...', i)
    return i + 1
# Private ends

# If there is an empty cluster, get one object far away as new cluster
def fix_empty_cluster(labels, number_of_cluster, distances, sumd, available_ids):
    clusters_ids = np.arange(number_of_cluster)
    
    missing_labels = list(set(clusters_ids) - set(available_ids))
    for i in range(len(missing_labels)):
        #print(f"Cluster: {i} is empty!")
        idx_label = missing_labels[i]
        idx_data = np.argmax(distances)
        idx_selected_cluster = labels[idx_data]
        labels[idx_data] = idx_label
        sumd[idx_selected_cluster] = sumd[idx_selected_cluster] - distances[idx_data] # the point has been excluded from the cluster
        distances[idx_data] = 0 # not select for the next missing label
        
    return (labels, distances, sumd)

def _get_labels(data, histograms, number_of_cluster):
    labels = np.full(data.shape[0], -1)
    sumd = np.zeros(number_of_cluster)
    distances = np.zeros(len(labels))
            
    for i in range(data.shape[0]):
        distance_min = np.inf
        old_label = labels[i] 
        #print(f"JDISTS[{i}] = ", end="")
        for j in range(number_of_cluster):
            distance = _distance_to_histogram(data[i], histograms[j])
            #print(f"{distance:0.6f}",  end=", ")
            if distance < distance_min:
                distance_min = distance
                labels[i] = j
        #print(f"D_change[{i}] = {old_label} -> {labels[i]}")
        #print(f"Best: {labels[i]} => {distance_min:0.6f}")
        distances[i] = distance_min
        sumd[labels[i]] = sumd[labels[i]] + distance_min
        #print(f"SUMD[BEST] = {sumd[labels[i]]:0.6f}")
    
    available_ids = np.unique(labels)
    nClusters = len(available_ids)
        
    # Check if there is empty cluster
    # Assign a point from other clusters (with length > 1) to this cluster
    if nClusters < number_of_cluster:
#        print('Fixing empty cluster...')
         #labels, distances, sumd = fix_empty_cluster(labels, number_of_cluster, distances, sumd, available_ids)
         pass       
    return (labels, distances, sumd)

# First histograms and then calculate labels
def cluster(data, number_of_cluster, max_hist_length, histograms = None, max_iter = 0):
    if histograms is None:
        histograms = []
    if max_iter == 0:
        max_iter = 50

    # Get maximun features
    max_feature = data_utility.get_max_features(data)
    
    # Randomly build histograms from objects
    if len(histograms) == 0:
        histograms = _build_initial_histograms(data, number_of_cluster)
        
    # Convert data to 2d np array
    np_data = data_utility.convert_to_np(max_feature, data)

    labels_prev = []
    labels_cur = []
    histograms_prev = histograms
    
    sdth_old = 0 
    sdth = np.inf # sum of distances to histograms
    sumd_prev = []
    sumd = []

    j = 0
    i = 0
    # Main loop of k-sets-histograms algorithm
    while abs(sdth_old-sdth) > 0.00001 and j < max_iter:
        if LOG_ENABLE:
            i = _progress_log(i)
            
        sdth_old = sdth
        labels_prev = labels_cur        
        sumd_prev = sumd
        
        labels_cur, distances, sumd = _get_labels(data, histograms, number_of_cluster)
        #print("SUMD = ",end="")
        #print(*(f"{ds:0.6f}" for ds in sumd), sep=", ")
        sdth = np.sum(sumd)
        #print(f"SDTH = {sdth}")
        
        if sdth > sdth_old:
            #print(f"Error Increased: sdth={sdth:0.6f} > sdth_old={sdth_old:0.6f}") 
            labels_cur = labels_prev
            sumd = sumd_prev
            histograms = histograms_prev
            #print('Stopping k-sets-histograms loop because of increasing error!')
            break
        
        histograms_prev = histograms
        histograms = _get_histogram(labels_cur, np_data, number_of_cluster, max_hist_length)
        #print(*[", ".join([f"{k} -> {v}" for k,v in histogram.items() ]) for histogram in histograms], sep="\n")
        j = j + 1
        
    return (labels_cur, histograms, sumd)
# API ends