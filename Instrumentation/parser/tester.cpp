#include <iostream>
#include "Instrumentation/CCAPatternGraph.hpp"
#include "Instrumentation/parser/parser.hpp"

int main(void) {
	llvm::cca::CCAPatternGraph* G = llvm::cca::parser::parsePatternStr("0: o24 = i24 + i25");
	G->print(0, std::cout);
	return 0;
}
