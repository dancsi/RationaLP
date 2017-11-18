import sys

import networkx as nx
import numpy as np

from generate_random import pack_save


def generate_vertex_cover(n, m):
    G = nx.generators.random_graphs.dense_gnm_random_graph(n, m)
    A = -nx.incidence_matrix(G).toarray().T
    c = -np.ones((1,n))
    b = -np.ones((1,m))
    return A, b, c


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("The program needs two integer command line arguments")
        sys.exit(1)

    n = int(sys.argv[1])
    m = int(sys.argv[2])

    name_template = name_template = 'inputs/test_vertex_cover_{}x{}.txt'.format(m, n)
    pack_save(name_template, *generate_vertex_cover(n, m))
