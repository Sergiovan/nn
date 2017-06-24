#include <iostream>
#include <cassert>

#include "common/logger.h"
#include "common/grammar.h"

int main(int argc, char** argv) {
    logger::get(); // Initialize logger
    Logger::info("Hello", "darkness", "my", "old", 10);
    Logger::debug("whops");
    
    Grammar::string_to_keyword.get("void");
    
    return EXIT_SUCCESS;
}
