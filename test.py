from itertools import product
from pathlib import Path
from scipy.optimize import linprog
from fractions import Fraction
from subprocess import run, PIPE
from regex import search
import numpy as np
import pytest


def get_fractions_in_line(line, approx=True):
    line = line.strip()
    tokens = line.split(' ')
    ret = []
    for token in tokens:
        parts = token.split('/')
        if len(parts) == 1:
            parts.append("1")
        frac = Fraction(int(parts[0]), int(parts[1]))
        if approx:
            frac = float(frac)
        ret.append(frac)
    return ret


def read_file(path):
    with open(path) as f:
        n = int(f.readline())
        m = int(f.readline())
        c = []
        b = []
        A = []
        c = get_fractions_in_line(f.readline())
        b = get_fractions_in_line(f.readline())
        for i in range(m):
            A.append(get_fractions_in_line(f.readline()))

        return np.array(c), np.array(A), np.array(b)


test_path = Path("inputs/")
files = [f for f in test_path.iterdir() if f.is_file()]

statuses = ['feasible', 'iterations exceeded', 'infeasible', 'unbounded']
pivot_functions = ['bland', 'random', 'maxincrease']


@pytest.mark.parametrize('path,pivot_fun', [(file, fun) for file in files for fun in pivot_functions])
def test_file(path, pivot_fun):
    c, A, b = read_file(path)
    res = linprog(c=-c, A_ub=A, b_ub=b,
                  options={'bland': True}, method='simplex')
    output_str = run(["./build/RationaLP", str(path), '--pivot', pivot_fun],
                     stdout=PIPE, encoding='ascii').stdout
    problem_line = search(r'(infeasible|unbounded)', output_str)
    if problem_line is not None:
        output = problem_line.group(0)
        expected = statuses[res.status]
        print(output, res.message)
        assert output == expected
    else:
        result_line = search(r'objective function is: (.*)\nThe', output_str)
        output = Fraction(result_line.group(1).strip())
        print(result_line)
        output_fl = float(output)
        expected = res.fun
        assert (output_fl + expected) / abs(output_fl) < 0.01
