import numpy as np
import networkx as nx

def create_comm_graph(filename):
    data   = np.loadtxt(filename, dtype=np.intc, delimiter = ",", skiprows=2, usecols=(0,1,2))
    node1  = [x[0] for x in data]
    node2  = [x[1] for x in data]
    weight = [int(x[2]) for x in data]
    G = nx.Graph()
    for i in np.arange(len(data)):
        G.add_node(int(node1[i]))
        G.add_edge(int(node1[i]),int(node2[i]), weight=int(weight[i])) 
    return G

