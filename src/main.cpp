#include <iomanip>
#include <iostream>
#include <utility>
#include <map>
#include <chrono>

#include "common/grammar.h"
#include "frontend/nn/reader.h"
#include "frontend/nn/lexer.h"
#include "common/utils.h"

#include "common/ast.h"
#include "frontend/nn/parser.h"
#include "common/ir.h"
#include "frontend/nn/ast_to_ir.h"

#include "backend/nnasm.h"
#include "frontend/nnasm/compiler.h"
#include "vm/machine.h"

int main(int argc, char** argv) {
    /*
    parser p{};
    auto start = std::chrono::high_resolution_clock::now();
    asm_compiler asmc{"examples/fib.nna"};
    asmc.compile();
    auto end = std::chrono::high_resolution_clock::now();
    
    virtualmachine vm{p};
    vm.load(asmc.get(), asmc.size());
    
    vm.run();
    logger::info() << vm.print_info() << logger::nend;
    */
    
    auto start = std::chrono::high_resolution_clock::now();
    nnasm::compiler ac{argc > 1 ? argv[1] : "examples/pithagoras.nna"};
    ac.compile();    
    u8* program = ac.get_program();
    u64 size = ac.get_size();
    auto end = std::chrono::high_resolution_clock::now();
    
    ac.print_errors();
    logger::info() << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << logger::nend;
    
    virtualmachine vm{};
    vm.load(ac.get_program(), ac.get_size());
    vm.run();
    
    logger::debug() << "\n" << vm.print_info() << logger::nend;
    // logger::debug() << "\n" << vm.print_registers() << logger::nend;
    
    /* parser p{};
    
    auto start = std::chrono::high_resolution_clock::now();
    parse_info res = p.parse(argc > 1 ? argv[1] : "examples/morse.nn", true);
    
    auto end = std::chrono::high_resolution_clock::now();
    
    if (p.has_errors()) {
        p.print_errors();
    } else {
        // p.print_info();
        logger::log() << res.result->print() << logger::nend;
        logger::info() << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << logger::nend;
        
        start = std::chrono::high_resolution_clock::now();
        ir_builder b{res};
        b.build();
        end = std::chrono::high_resolution_clock::now();
        
        logger::log() << '\n' << print_sequence(b.get()) << logger::nend;
        logger::info() << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << logger::nend;
    } */
    
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
    return 0;
}
