#pragma once
#include "Tableau.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <random>
#include <tuple>
#include <vector>

enum class PivotResult { FOUND, NOT_FOUND, INFEASIBLE, UNBOUNDED };

/*
Convenience base class for all pivot functions. 
Provides helpers for finding the possible entering and leaving variables.
*/
class PivotFunction {
public:
	using idx_t = Tableau::idx_t;
	using CandidateIndexContainer = std::vector<idx_t>;
	using ReturnType = std::tuple<PivotResult, idx_t, idx_t>;

	static CandidateIndexContainer getLeavingCandidates(Tableau& tableau, Tableau::idx_t entering) {
		CandidateIndexContainer leaving;
		Tableau::num_t best_ratio;

		for (Tableau::idx_t row = 0; row < std::size(tableau.A); row++) {
			if (tableau.A[row][entering] > 0) {
				Tableau::num_t ratio = tableau.b[row] / tableau.A[row][entering];
				if (leaving.empty() || ratio < best_ratio) {
					best_ratio = ratio;
					leaving.clear();
				}
				if (ratio <= best_ratio) {
					leaving.push_back(tableau.basic[row]);
				}
			}
		}

		return leaving;
	}

	static CandidateIndexContainer getEnteringCandidates(Tableau& tableau) {
		std::vector<Tableau::idx_t> entering;
		for (Tableau::idx_t i = 0; i < tableau.n; i++) {
			if (tableau.c[i] > 0) {
				entering.push_back(i);
			}
		}
		return entering;
	}

	virtual idx_t chooseEnteringVariable(Tableau& tableau, CandidateIndexContainer& enteringCandidates) = 0;
	virtual idx_t chooseLeavingVariable(Tableau& tableau, idx_t enteringVariable, CandidateIndexContainer& leavingCandidates) = 0;

	/*
	Calls the entering and leaving variable choice functions, while returning the appropriate status if there is no possible pivot, or the LP is unbounded.
	*/
	ReturnType pivotRuleHelper(
		Tableau& tableau)
	{
		auto &&enteringCandidates = getEnteringCandidates(tableau);
		if (enteringCandidates.empty())
			return { PivotResult::NOT_FOUND, -1, -1 };
		auto entering = chooseEnteringVariable(tableau, enteringCandidates);

		auto &&leavingCandidates = getLeavingCandidates(tableau, entering);
		if (leavingCandidates.empty())
			return { PivotResult::UNBOUNDED, -1, -1 };
		auto leaving = chooseLeavingVariable(tableau, entering, leavingCandidates);
		return { PivotResult::FOUND, leaving, entering };
	}

	/*
	Returns the pivot function result, because we want PivotFunction instances to be proper callable objects.
	*/
	ReturnType operator()(Tableau& tableau) {
		return pivotRuleHelper(tableau);
	}
};

using PivotFunctionReturnType = PivotFunction::ReturnType;

/*
Bland's rule
*/
class Bland : public PivotFunction {
public:
	idx_t chooseEnteringVariable(Tableau& tableau, CandidateIndexContainer& enteringCandidates) override {
		return enteringCandidates[0];
	}
	idx_t chooseLeavingVariable(Tableau& tableau, idx_t enteringVariable, CandidateIndexContainer& leavingCandidates) override {
		return leavingCandidates[0];
	}
};

/*
Random pivoting
*/
class Random : public PivotFunction {
private:
	std::mt19937 gen;
public:
	idx_t chooseEnteringVariable(Tableau& tableau, CandidateIndexContainer& enteringCandidates) override {
		std::uniform_int_distribution<> dist(0, enteringCandidates.size() - 1);
		return enteringCandidates[dist(gen)];
	}
	idx_t chooseLeavingVariable(Tableau& tableau, idx_t enteringVariable, CandidateIndexContainer& leavingCandidates) override {
		std::uniform_int_distribution<> dist(0, leavingCandidates.size() - 1);
		return leavingCandidates[dist(gen)];
	}
};

/*
Maximum increase pivot rule, where the entering variable is the one that provides the greatest increase in the score.
*/
class MaxIncrease : public PivotFunction {
public:
	idx_t chooseEnteringVariable(Tableau& tableau, CandidateIndexContainer& enteringCandidates) override {
		Tableau::num_t bestScoreIncrease(0);
		idx_t bestEnteringCandidate = enteringCandidates[0];
		for (auto entering : enteringCandidates) {
			auto &&leavingCandidates = PivotFunction::getLeavingCandidates(tableau, entering);
			if (leavingCandidates.empty()) return entering; //the LP is unbounded
			auto leaving = leavingCandidates[0];
			auto row = std::distance(tableau.basic.begin(), std::find(tableau.basic.begin(), tableau.basic.end(), leaving));
			Tableau::num_t scoreIncrease = tableau.c[entering] * tableau.b[row] / tableau.A[row][entering];
			if (scoreIncrease > bestScoreIncrease) {
				bestScoreIncrease.swap(scoreIncrease);
				bestEnteringCandidate = entering;
			}
		}
		return bestEnteringCandidate;
	}
	idx_t chooseLeavingVariable(Tableau& tableau, idx_t enteringVariable, CandidateIndexContainer& leavingCandidates) override {
		return leavingCandidates[0];
	}
};

/*
Maximum coefficient rule
*/
class MaxCoef : public PivotFunction {
public:
	idx_t chooseEnteringVariable(Tableau& tableau, CandidateIndexContainer& enteringCandidates) override {
		return *std::max_element(
			enteringCandidates.begin(),
			enteringCandidates.end(), 
			[&tableau](auto lhs, auto rhs) {return tableau.c[lhs] > tableau.c[rhs]; }
		);
	}
	idx_t chooseLeavingVariable(Tableau& tableau, idx_t enteringVariable, CandidateIndexContainer& leavingCandidates) override {
		std::uniform_int_distribution<> dist(0, leavingCandidates.size() - 1);
		return leavingCandidates[0];
	}
};