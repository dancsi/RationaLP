#pragma once

#include <functional>
#include <iostream>

#include "Tableau.h"
#include "Pivot.h"

enum class LogLevel { CONCISE, VERBOSE };

/*
Wrapper class for Tableau, that guides the whole simplex algorithm
*/
class LinearProgram {
public:
	enum class Result { FEASIBLE_BOUNDED, FEASIBLE_UNBOUNDED, INFEASIBLE };

	Tableau tableau;
	LogLevel logLevel;
	int numPivots;

	LinearProgram(const std::string& path, bool verbose)
		: tableau(Tableau::fromFile(path)),
		logLevel(verbose ? LogLevel::VERBOSE : LogLevel::CONCISE),
		numPivots(0) {}

	/*
	Perform one step of a single phase of the simplex algorithm.
	Returns PivotResult::FOUND if it terminates successfully, and the appropriate condition (UNBOUNDED, NOT_FOUND) otherwise.
	*/
	template<typename PivotFun>
	PivotResult step(PivotFun pivotFun) {
		Tableau::idx_t leaving, entering;
		PivotResult result;
		std::tie(result, leaving, entering) = std::invoke(pivotFun, tableau);
		if (result != PivotResult::FOUND) return result;

		if (logLevel == LogLevel::VERBOSE) {
			std::cout << "The entering variable is x" << entering + 1;
			std::cout << "\nThe leaving variable is x" << leaving + 1 << "\n";
		}
		tableau.pivot(leaving, entering); numPivots++;

		return PivotResult::FOUND;
	}

	/*
	Perform a single phase of the simplex algorithm, starting from the zero-vector as the initial BFS.
	*/
	template<typename PivotFun>
	Result solveOnePhase(PivotFun pivotFun) {
		PivotResult res;
		while ((res = step(pivotFun)) == PivotResult::FOUND) {
			if (logLevel == LogLevel::VERBOSE) {
				tableau.dump();
			}
		}
		switch (res) {
		case PivotResult::NOT_FOUND:
			return Result::FEASIBLE_BOUNDED;
		case PivotResult::INFEASIBLE:
			return Result::INFEASIBLE;
		case PivotResult::UNBOUNDED:
			return Result::FEASIBLE_UNBOUNDED;
		default:
			throw std::logic_error("Unexpected return value from the pivot function");
		}
	}

	/*
	Performs the 2-phase simplex algorithm. Only launches phase 1 if the 0-vector is not a BFS.
	*/
	template<typename PivotFun>
	Result solve(PivotFun& pivotFun) {
		if (logLevel == LogLevel::VERBOSE) {
			std::cout << "The initial tableau is:\n";
			tableau.dump();
		}
		if (!tableau.isFeasible()) {
			tableau.addArtificialVariables();
			solveOnePhase(pivotFun);
			if (!tableau.removeArtificialVariables()) {
				return Result::INFEASIBLE;
			}
		}

		return solveOnePhase(pivotFun);
	}

	void printFancyStatement() {
		std::cout << "Maximize\n";
		tableau.printFancyCoefs(tableau.c);
		std::cout << "\nSubject to\n";
		tableau.printFancyConstraints();
		for (Tableau::idx_t i = 1; i <= tableau.n; i++) {
			if (i > 1) {
				std::cout << ", ";
			}
			std::cout << "x" << i;
		}
		std::cout << " are non-negative\n";
	}

	void printFancySolution() {
		tableau.printFancySolution();
	}
};