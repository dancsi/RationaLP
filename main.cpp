#include <functional>
#include <iostream>
#include <map>
#include <utility>

#include <boost/program_options.hpp>

#include "LinearProgram.h"
#include "Pivot.h"

namespace po = boost::program_options;

std::map<std::string, std::function<PivotFunctionReturnType(Tableau& tableau)>> pivotFunctions;
std::string allowedPivotFunctions;

void registerPivotFunctions() {
	pivotFunctions = decltype(pivotFunctions){
		{"bland", Bland()}, 
		{"random", Random()}
	};
	allowedPivotFunctions.clear();
	for (auto& it : pivotFunctions) {
		allowedPivotFunctions += it.first;
		allowedPivotFunctions += ',';
	}
	allowedPivotFunctions.pop_back();
}

auto parseOptions(int argc, char **argv) {
	po::options_description optionsDescription("Allowed options");
	std::string pivotHelpText = "the pivot rule that is used. Can be one of {" + allowedPivotFunctions + "}";
	optionsDescription.add_options()
		("verbose", po::value<bool>()->implicit_value(true)->default_value(false), "verbose output")
		("pivot", po::value<std::string>()->default_value("bland"), pivotHelpText.c_str())
		("input", po::value<std::string>(), "input linear program")
		;
	po::positional_options_description positionalOptionsDescription;
	positionalOptionsDescription.add("input", -1);

	po::variables_map variablesMap;
	po::store(
		po::command_line_parser(argc, argv)
		.options(optionsDescription)
		.positional(positionalOptionsDescription)
		.run(),
		variablesMap);
	po::notify(variablesMap);

	std::string inputPath;
	if (variablesMap.count("input")) {
		inputPath = variablesMap["input"].as<std::string>();
	}
	else {
		std::cout << "You must provide a valid path\n"<<optionsDescription;
		std::exit(1);
	}

	auto verboseOutput = variablesMap["verbose"].as<bool>();
	auto pivotAlgorithm = variablesMap["pivot"].as<std::string>();

	if (pivotFunctions.find(pivotAlgorithm) == pivotFunctions.end()) {
		std::cout << "Invalid pivot algorithm \"" << pivotAlgorithm << "\"\n";
		std::cout << "Allowed values are {" << allowedPivotFunctions << "}\n";
		std::exit(1);
	}

	return std::make_tuple(inputPath, verboseOutput, pivotAlgorithm);
}

int main(int argc, char **argv)
{
	registerPivotFunctions();

	std::string inputPath, pivotAlgorithm;
	bool verboseOutput;

	std::tie(inputPath, verboseOutput, pivotAlgorithm) = parseOptions(argc, argv);

	LinearProgram lp(inputPath, verboseOutput);
	lp.printFancyStatement();

	auto result = lp.solve(pivotFunctions[pivotAlgorithm]);

	switch (result) {
	case LinearProgram::Result::INFEASIBLE:
		std::cout << "The linear program is infeasible\n";
		break;
	case LinearProgram::Result::FEASIBLE_UNBOUNDED:
		std::cout << "The linear program is unbounded\n";
		break;
	case LinearProgram::Result::FEASIBLE_BOUNDED:
		std::cout << "An optimal solution is: ";
		lp.printFancySolution();
		std::cout << "\nThe value of the objective function is: " << lp.tableau.value();
		std::cout << "\nThe number of pivots is: " << lp.numPivots;
		std::cout << "\nThe pivot rule used: " << pivotAlgorithm;
	}
	return 0;
}