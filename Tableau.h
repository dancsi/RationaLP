#pragma once

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <iterator>
#include <set>
#include <string>

#include <cassert>

#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector.hpp>

#define DEBUG_RATIONALS

/*
A class representing a simplex tableau in standard form
*/
class Tableau {
public:
#ifndef DEBUG_RATIONALS
	using num_t = boost::multiprecision::mpq_rational; //type of numbers we operate with
#else
	using num_t = boost::multiprecision::number<boost::multiprecision::debug_adaptor<boost::multiprecision::gmp_rational>>;
#endif
	using vector_t = boost::numeric::ublas::vector<num_t>;
	using matrix_t = std::vector<vector_t>;
	using idx_t = size_t;

	using zero_vector = boost::numeric::ublas::zero_vector<num_t>;

	size_t n; //number of variables
	size_t m; //number of constraints

	matrix_t A; //the constraint matrix
	vector_t c; //cost vector
	vector_t b; //constraint RHS
	vector_t x; //current solution
	num_t score; //current_score

	std::vector<idx_t> basic; //the basic variables, always indexed by the row number they appear in

private:
	vector_t c_backup;

public:

	num_t dot(vector_t &x, vector_t &y) {
		return std::inner_product(begin(x), end(x), begin(y), num_t(0));
	}

	/*
	Construct a simplex tableau from the canonical form of the problem,
		max   c^T x
		s.t.  A x <= b
			  x >= 0
	Store the data in the standard form, i.e.
		max   c'^T x'
		s.t.  A' x' = b
			  x >= 0
	*/
	template<typename MatrixLike, typename VectorLike>
	Tableau(VectorLike c, MatrixLike A, VectorLike b) :
		m(std::size(b)), //we have m constraints
		n(std::size(c) + std::size(b)), //the number of variables is the number of variables in the canonical form + the number of constraints 
		x(n), //an array of n zeros
		c(n), //initialize the cost vector to n zeros
		A(m, vector_t(n)), //initialize the constraint matrix to be m*n, filled with zeros
		b(m), //initialize the constraint RHS to m zeros
		score(0)
	{
		using namespace std;
		auto&& original_n = size(c);

		//initialize the score vector, by copying the original score vector, and padding it with 0s for slack variables
		copy(begin(c), end(c), begin(Tableau::c));
		fill(begin(Tableau::c) + original_n, end(Tableau::c), num_t(0));

		//initialize the constraint RHS
		copy(begin(b), end(b), begin(Tableau::b));

		//initialize the constraint matrix, and pad Tableau::A with the identity matrix
		//the basic variables are the slack variables
		idx_t i = 0;
		for (auto&& row : A) {
			copy(begin(row), end(row), begin(Tableau::A[i]));
			fill(begin(Tableau::A[i]) + original_n, end(Tableau::A[i]), 0);
			Tableau::A[i][original_n + i] = 1;
			basic.push_back(original_n + i);
			x[original_n + i] = b[i];
			i++;
		}
	}

	/*
		A solution x is feasible if x >= 0 and Ax = b
	*/
	bool isFeasible() {
		if (std::any_of(std::begin(x), std::end(x), [](auto el) {return el < 0; })) return false;

		for (idx_t i = 0; i < m; i++) {
			if (dot(A[i], x) != b[i]) return false;
		}

		return true;
	}

	num_t value() {
		return -score;
	}

	void pivot(idx_t leaving, idx_t entering) {
		idx_t leaving_row = std::distance(basic.begin(), std::find(basic.begin(), basic.end(), leaving));
		basic[leaving_row] = entering;

		num_t divide_by = A[leaving_row][entering];
		A[leaving_row] /= divide_by;
		b[leaving_row] /= divide_by;

		for (idx_t i = 0; i < m; i++) {
			if (i != leaving_row && A[i][entering] != 0) {
				num_t multiply_by = A[i][entering];
				A[i] -= A[leaving_row] * multiply_by;
				b[i] -= b[leaving_row] * multiply_by;
			}
		}

		score -= c[entering] * b[leaving_row];
		c -= c[entering] * A[leaving_row];


		for (idx_t i = 0; i < n; i++) x[i] = 0;
		for (idx_t i = 0; i < m; i++) {
			x[basic[i]] = b[i];
		}
	}

	/*
		Add an artificial variable for each constraint, and update the internal state
	*/
	void addArtificialVariables() {
		c_backup = c;
		c.resize(n + m, false);
		x.resize(n + m);
		score = 0;

		for (idx_t i = 0; i < m; i++) {
			if (b[i] < 0) {
				b[i] *= -1;
				A[i] *= -1;
			}
			A[i].resize(n + m); 
			c += A[i];
			A[i][n + i] = 1;
			x[n + i] = b[i];
			basic[i] = n + i;
			score += b[i];
		}

		n += m;
	}

	/*
		Try to remove artificial variables, and prepare for Phase 2.
		Returns true if we do have a feasible point to start with, and false otherwise.
	*/
	bool removeArtificialVariables() {
		if (score != 0) return false;

		/*
		In case there are some artificial variables in the basis (that have value 0), we remove them, 
		by pivoting to some non-artificial variables.
		*/
		for (idx_t i = 0; i < m; i++) {
			if (basic[i] >= n - m) {
				for (idx_t j = 0; j < n - m; j++) {
					if (A[i][j] != 0) {
						pivot(basic[i], j);
					}
				}
			}
		}

		n -= m;
		c = c_backup;

		x.resize(n);
		for (idx_t i = 0; i < m; i++) {
			A[i].resize(n);
			score -= c[basic[i]] * b[i];
			c -= c[basic[i]] * A[i];
		}

		return true;
	}

	void dump(std::ostream& ost = std::cout) {
		using namespace std;
		vector<vector<std::string>> strings_to_print(1 + m);
		vector<int> field_widths(1 + n);
		for (auto&& cc : c) {
			strings_to_print[0].push_back(cc.str());
		}
		strings_to_print[0].push_back(score.str());
		for (idx_t i = 0; i < m; i++) {
			for (auto&& cc : A[i]) {
				strings_to_print[i + 1].push_back(cc.str());
			}
			strings_to_print[i + 1].push_back(b[i].str());
		}

		for (idx_t j = 0; j <= n; j++) {
			field_widths[j] = 1 + accumulate(strings_to_print.begin(), strings_to_print.end(), 0,
				[j](size_t cur, vector<string>& row2) {return max(cur, row2[j].length()); }
			);
		}

		auto print_spaced_row = [&](auto& row) {
			for (idx_t j = 0; j < n; j++) {
				ost << setw(field_widths[j]) << row[j] << " ";
			}
			ost << "|" << setw(field_widths.back()) << row.back() << '\n';
		};

		print_spaced_row(strings_to_print[0]);
		auto total_row_length = accumulate(field_widths.begin(), field_widths.end(), 0) + n + 1;
		for (decltype(total_row_length) i = 0; i < total_row_length; i++) ost << '-'; ost << '\n';
		for (idx_t i = 1; i <= m; i++) print_spaced_row(strings_to_print[i]);
		ost << endl;
	}

	static Tableau fromFile(const std::string& path) {
		size_t n, m;
		std::ifstream fin(path);
        if(!fin.good()) {
            std::cout << "Problem opening the input file "<<path;
            std::exit(1);
        }

		fin >> n >> m;
		std::vector<num_t> b(m), c(n);
		std::vector<std::vector<num_t>> A(m, c);

		for (idx_t i = 0; i < n; i++) fin >> c[i];
		for (idx_t i = 0; i < m; i++) fin >> b[i];

		for (idx_t i = 0; i < m; i++) {
			for (idx_t j = 0; j < n; j++) {
				fin >> A[i][j];
			}
		}
		fin.close();

		return Tableau(c, A, b);
	}

	void printFancyCoefs(vector_t& row) {
		for (idx_t i = 0; i < row.size(); i++) {
			if (row[i] == 0) continue;
			if (i > 0 && row[i] > 0) std::cout << "+";
			std::cout << row[i] << "x" << i + 1 << " ";
		}
	}

	void printFancyConstraints() {
		for (idx_t i = 0; i < m; i++) {
			printFancyCoefs(A[i]);
			std::cout << "<= " << b[i] << std::endl;
		}
	}

	void printFancySolution() {
		for (idx_t i = 0; i < n; i++) {
			if (i > 0) std::cout << ", ";
			std::cout << "x" << i + 1 << " = "<<x[i];
		}
	}
};