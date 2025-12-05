import numpy as np
from itertools import chain


DEFAULT_STR = '   '


def read_file(file_path):
    """Same function used by 
        https://github.com/uef-machine-learning/K-sets_and_K-swaps
    
    copied for comparability
    """
    import numpy as np
    data_list = []

    with open(file_path, 'r') as f:
        for line in f.readlines():
            line = line.rstrip()
            line = line.split(' ')
            data_list.append(np.array(line))

    return np.array(data_list, dtype="object")


def convert_to_np(max_number_of_features, data):

    np_data = np.full((data.shape[0], max_number_of_features), DEFAULT_STR)

    for i in range(data.shape[0]):
        np_data[i, 0: len(data[i])] = data[i]
    return np_data

def get_max_features(data):
    return len(max(data.tolist(), key=len))

def get_features(np_data):
    np_data_1d = list(chain.from_iterable(np_data))
    unique_set = set(np_data_1d)
    unique_set.discard('')
    features = np.array(list(unique_set))
    return features