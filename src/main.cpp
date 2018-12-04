#include <iomanip>
#include <iostream>
#include <utility>
#include <map>
#include <chrono>

#include "common/grammar.h"
#include "frontend/reader.h"
#include "frontend/lexer.h"
#include "common/utils.h"

#include "common/ast.h"
#include "frontend/parser.h"

int main(int argc, char** argv) {
    parser p{};
    
    auto start = std::chrono::high_resolution_clock::now();
    ast* res = p.parse("examples/fizzbuzz.nn", true);
    
    if (p.has_errors()) {
        p.print_errors();
    } else {
        logger::log() << "\n" << res->print() << logger::nend;
    }
    
    /*
    reader* r = reader::from_file("examples/mastermind2.nn");
    lexer  l{r};
    
    logger::log() << "\n[INDEX ] | [LINE  ] | [COLUMN] | [TOKEN  TYPE] | VALUE\n";
    
    while (!r->is_done()) {
        token t = l.next();
        logger::log() << '[';
        logger::log() << std::setw(6) << t.index;
        logger::log() << "] | [" << std::setw(6) << t.line;
        logger::log() << "] | [" << std::setw(6) << t.column;
        logger::log() << "] | [" << logger::color::cyan << std::setw(11) << Grammar::tokentype_names.at(t.type);
        logger::log() << logger::color::white << "] | " << t.value;
        logger::log() << "\n";
    } */
    
    auto end = std::chrono::high_resolution_clock::now();
    logger::log() << logger::end;
    logger::info() << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << logger::nend;
    return 0;
}
