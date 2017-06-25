#include <iostream>
#include <cassert>

#include "common/logger.h"
#include "common/grammar.h"
#include "frontend/reader.h"

int main(int argc, char** argv) {
    logger::get(); // Initialize logger
    { reader r("../other/nn.xox"); } // Error
    reader r("../other/nn.xml");
    while(!r.has_finished()) {
        std::putchar(r.next());
    }
    
    return EXIT_SUCCESS;
}
