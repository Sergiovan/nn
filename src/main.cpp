#include "common/defs.h"
#include "common/logger.h"

#include "common/token_stream.h"

#include "frontend/parser.h"
#include "common/ast.h"

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    
    if (argc < 2) {
        logger::error() << "Needs to be called with at least 1 argument";
        return -1;
    }
    
    logger::info() << "Program start";
    
    parser p{};
    auto m = p.parse(argv[1]);
    
    logger::info() << "Program end";
    
    if (argc > 2 && !m->errors.size()) {
        logger::debug() << m->root->to_string(true);
    }
    
    return 0;
}
