import sys

import numpy as np

from generate_random import pack_save


def generate_klee_minty(n):
    A = np.array([[2**(i - j + 1) if j <= i else 0 for j in range(n)] for i in range(n)], dtype=np.int64)
    A -= np.eye(n, dtype=np.int64)
    b = np.array([5**(i+1) for i in range(n)])
    c = np.array([2**(n-i-1) for i in range(n)])
    return A, b, c

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("The program needs an integer command line argument")
        sys.exit(1)

    n = int(sys.argv[1])

    name_template = 'inputs/test_klee_minty_{}.txt'.format(n)
    pack_save(name_template, *generate_klee_minty(n))
