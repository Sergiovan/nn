#include <iostream>
#include <cassert>

#include "common/grammar.h"

int main(int argc, char** argv) {
    Grammar::string_to_keyword.get("void");
    
    return EXIT_SUCCESS;
}
