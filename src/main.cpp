#include "common/defs.h"
#include "common/logger.h"

#include "common/token_stream.h"

#include "frontend/parser.h"

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    
    if (argc < 2) {
        logger::error() << "Needs to be called with at least 1 argument";
        return -1;
    }
    
    logger::debug() << "Program start";
    
    parser p{};
    p.parse(argv[1]);
    
    logger::debug() << "Program end";
    
    return 0;
}
