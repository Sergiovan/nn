#include <iostream>

#include "common/logger.h"
#include "common/grammar.h"
#include "frontend/reader.h"
#include "frontend/tokenizer.h"

int main(int argc, char** argv) {
    logger::get(); // Initialize logger
    reader r("test.txt");
    tokenizer t(r);
    while(!r.has_finished()) {
        token tok = t.next();
        std::cout << tok.value << ": " << static_cast<int>(tok.tokenType) << std::endl;
    }
    
    getchar();
    return EXIT_SUCCESS;
}
