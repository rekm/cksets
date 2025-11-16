

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

