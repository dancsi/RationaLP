import csv
from fractions import Fraction
from pathlib import Path
from subprocess import PIPE, run
from time import time

import numpy as np
import pytest
from gurobipy import GRB, LinExpr, Model, quicksum
from regex import search


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


def embedded_lp_solver(A, b, c):
    solver = Model()
    m, n = A.shape
    x = [solver.addVar(lb=0.0, ub=GRB.INFINITY, vtype=GRB.CONTINUOUS)
         for i in range(n)]

    for row, bb in zip(A, b):
        expr = quicksum([coef * var for coef, var in zip(row, x)])
        solver.addConstr(expr, GRB.LESS_EQUAL, float(bb))

    objective = quicksum([float(coef) * var for coef, var in zip(c, x)])
    solver.setObjective(objective, GRB.MAXIMIZE)
    solver.setParam("DualReductions", 0)

    solver.optimize()

    sol = solver.status

    statuses = {GRB.OPTIMAL: "optimal",
                GRB.UNBOUNDED: "unbounded", GRB.INFEASIBLE: "infeasible"}
    status = statuses[sol]
    val = None
    if status == 'optimal':
        val = objective.getValue()
    return (status, val)


test_path = Path("inputs/")
files = [f for f in test_path.iterdir() if f.is_file()]
pivot_functions = ['bland', 'random', 'maxincrease', 'maxcoef']

log_file = open('test_output.csv', 'w', newline='')
log_writer = csv.writer(log_file)
log_writer.writerow(['name', 'pivot', 'iters', 'time', 'type'])

@pytest.mark.parametrize('path,pivot_fun', [(file, fun) for file in files for fun in pivot_functions])
def test_file(path, pivot_fun):
    c, A, b = read_file(path)
    expected_status, expected_sol = embedded_lp_solver(A, b, c)
    start_time = time()
    output_str = run(["./build/RationaLP", str(path), '--pivot', pivot_fun],
                     stdout=PIPE, encoding='ascii').stdout
    elapsed_time = time() - start_time
    problem_line = search(r'(infeasible|unbounded)', output_str)
    num_iterations = 0
    if problem_line is not None:
        output = problem_line.group(0)
        assert output == expected_status
    else:
        result_line = search(r'objective function is: (.*)\nThe', output_str)
        output = Fraction(result_line.group(1).strip())
        output_fl = float(output)
        assert abs(output_fl - expected_sol) / abs(output_fl) < 0.01

        num_iterations_line = search(
            r'number of pivots is: (.*)\nThe', output_str)
        num_iterations = int(num_iterations_line.group(1).strip())
    
    log_writer.writerow([path.stem, pivot_fun, num_iterations, elapsed_time, expected_status])
