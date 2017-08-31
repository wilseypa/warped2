import numpy as np
import networkx as nx
import community as comm # install from python-louvain
from collections import OrderedDict
from operator import itemgetter

"""
Split the group into n partition with approximately equals sum of values
"""
def balancedPartition(group, n):
    result  = [[] for i in range(n)]
    sums    = {i:0 for i in range(n)}
    min_sum = 0
    for entry in group:
        for i in sums:
            if min_sum == sums[i]:
                result[i].append(entry[1])
                break
        sums[i] += entry[0]
        min_sum  = min(sums.values())
    return result

"""
Build a undirected weighted graph using data inside data_file and partition it at multiple levels 
using the Louvain algorithm. Returns the node-level partitions.
"""
def buildPartitions(data_file, n):
    data    = np.loadtxt(data_file, dtype=np.intc, delimiter = ",", skiprows=1, usecols=(0,1,2))
    node_x  = [x[0] for x in data]
    node_y  = [x[1] for x in data]
    weight  = [int(x[2]) for x in data]
    G = nx.Graph()
    for i in np.arange(len(data)):
        G.add_node(int(node_x[i]))
        G.add_edge(int(node_x[i]),int(node_y[i]), weight=int(weight[i])) 

    dendo = comm.generate_dendrogram(G)

    level = len(dendo)-2
    louvain_partitions = comm.partition_at_level(dendo, level)
    group = {}
    for node_id, partition_id in louvain_partitions.iteritems() :
        group.setdefault(partition_id, []).append(node_id)

    ordered_group = []
    for k in sorted(group, key=lambda k: len(group[k])) :
        ordered_group.append( (len(group[k]), k) )

    partitions = []
    for entry in balancedPartition(ordered_group, n):
        nodes = []
        for partition_id in entry:
            nodes.extend(group[partition_id])
        partitions.append(nodes)

    return partitions

#print buildPartitions("/home/sounak/ssd/ecl_warped/warped2-models/models/traffic/statistics.out", 5)

