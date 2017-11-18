from fractions import Fraction
import numpy as np
from numpy.random import randint
import sys


class RationLP:
    def __init__(self, A, b, c):
        self.A = [[Fraction(x) for x in row] for row in A]
        self.b = [Fraction(x) for x in b.flat]
        self.c = [Fraction(x) for x in c.flat]
        self.n = len(self.c)
        self.m = len(self.b)

    def print_row(self, vec, fout):
        for x in vec:
            print(x, end=' ', file=fout)
        print(file=fout)

    def save(self, path):
        with open(path, 'w') as fout:
            print(self.n, file=fout)
            print(self.m, file=fout)
            self.print_row(self.c, fout)
            self.print_row(self.b, fout)
            for row in self.A:
                self.print_row(row, fout)

# Adapted from https://math.stackexchange.com/a/244164/76028
def generate_feasible(n, m):
    v = randint(0, 2 * n, size=(n, 1))
    A = randint(0, n * m, size=(m, n))
    b = A @ v + randint(0, 4, size=(m, 1)) / 4.0
    c = randint(-n, n, size=(n, 1))
    return A, b, c


def generate_unbounded(n, m):
    u = randint(0, 2 * n, size=(n, 1))
    v = randint(0, 2 * n, size=(n, 1))
    v[v == 0] = 1
    A = randint(0, 100, size=(m, n))
    Av = A @ v
    A[Av[:, 0] > 0, :] *= -1
    assert (A@v < 0).all()
    b = A @ u + randint(0, 4, size=(m, 1)) / 4.0
    c = randint(-2 * n, 2 * n, size=(n, 1))
    if c.T @ v <= 0:
        c = -c
    return A, b, c


def generate_infeasible(n, m):
    A, b, c = generate_unbounded(m, n)
    return -A.T, -c, -b


def pack_save(fname, A, b, c):
    prog = RationLP(A, b, c)
    prog.save(fname)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("The program needs two integer command line arguments")
        sys.exit(1)

    n = int(sys.argv[1])
    m = int(sys.argv[2])

    name_template = 'inputs/test_{{}}_{}x{}.txt'.format(m, n)

    pack_save(name_template.format('feasible'), *generate_feasible(n, m))
    pack_save(name_template.format('infeasible'), *generate_infeasible(n, m))
    pack_save(name_template.format('unbounded'), *generate_unbounded(n, m))
