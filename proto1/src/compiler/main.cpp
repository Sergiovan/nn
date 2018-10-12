#include <iostream>

#include "common/logger.h"
#include "common/grammar.h"
#include "frontend/reader.h"
#include "frontend/tokenizer.h"
#include "common/symbol_table.h"

int main(int argc, char** argv) {
    logger::get(); // Initialize logger

    reader r{".spec/proto_examples/mastermind2.nn"};
    tokenizer t{r};

    token tok;

    while((tok = t.next()).tokenType != Grammar::TokenType::END_OF_FILE)
        Logger::info(tok.value, " ", (int) tok.tokenType);

    return EXIT_SUCCESS;
}
