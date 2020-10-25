#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.cpp"

int main(int argc, char const* argv[]) {
    Parser p = parser_sequence_of({
        parser_str("hallo du!"),
        whitespace,
        parser_str("wie gehts?")
    });

    parser_run(p, "hallo du!   wie gehts?");

    return 0;
}
