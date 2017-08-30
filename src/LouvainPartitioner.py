import numpy as np
import networkx as nx
import community as comm # install from python-louvain

def partition(filename):
    data   = np.loadtxt(filename, dtype=np.intc, delimiter = ",", skiprows=1, usecols=(0,1,2))
    node1  = [x[0] for x in data]
    node2  = [x[1] for x in data]
    weight = [int(x[2]) for x in data]
    G = nx.Graph()
    for i in np.arange(len(data)):
        G.add_node(int(node1[i]))
        G.add_edge(int(node1[i]),int(node2[i]), weight=int(weight[i])) 

    dendogram = comm.generate_dendogram(G)
        print("partition at level", level, "is", comm.partition_at_level(dendo, level))

    return 1
