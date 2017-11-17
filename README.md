# RationaLP
## Rational dense linear program solver

To run the solver, execute
```
RationaLP input.txt [--pivot {bland, random or maxincrease}] [--verbose]
```
Note that in order to build the solver, you need Boost and GMP/MPIR installed somewhere where CMake can find them. On Debian-based systems, one would install `libboost-dev libboost-program-options-dev libgmp-dev`.

The input format is as follows:
```
n
m
c1 c2 ... cn
b1 b2 ... bm
a11 a12 ... a1n
a21 a22 ... a2n
...
am1 am2 ... amn
```
The problem that is solved is 
```
    max  c^T x
    s.t. A x <= b
           x >= 0
```

There are no restrictions on the range or number of (rational) variables, however, take note that each pivot step takes O(number of variables * number of constraints) time.