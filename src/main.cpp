#include "common/defs.h"
#include "common/logger.h"

#include "common/token_stream.h"

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    
    if (argc < 2) {
        logger::error() << "Needs to be called with at least 1 argument";
        return -1;
    }
    
    logger::debug() << "Program start";
    
    token_stream ts{argv[1]};
    ts.read();
    
    for (auto tok : ts) {
        if (tok->tt == token_type::COMMENT || tok->tt == token_type::WHITESPACE || 
            tok->tt == token_type::NEWLINE) {
            // Skip
        } else {
            logger::info() << *tok;
        }
    }
    
    logger::debug() << "Program end";
    
    return 0;
}
