#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.cpp"

PARSER_ALLOCATOR(custom_alloc) {
    printf("%zd bytes reserviert\n", size);
    void *mem = malloc(size);

    return mem;
}

int main(int argc, char const* argv[]) {
    using namespace Urq::api;

    Urq::parser_alloc = custom_alloc;

    Parser_State result = {};
    Parser parser = {};

    parser = letters;
    result = run(parser, "asd12fas");

    parser = digits;
    result = run(parser, "12fas");

    parser = sequence_of({
        letters,
        digits,
        letters
    });
    result = run(parser, "abcd1234efgh");

    parser = choice({
        letters,
        digits
    });
    result = run(parser, "123abc");
    result = run(parser, "abc123");

    parser = many(choice({
        letters,
        digits
    }));
    result = run(parser, "**123abc");

    parser = many1(choice({
        letters,
        digits
    }));
    result = run(parser, "**123abc");

    return 0;
}
