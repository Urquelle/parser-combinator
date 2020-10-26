#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "parser.cpp"

PARSER_ALLOCATOR(custom_alloc) {
    printf("%zd bytes reserviert\n", size);
    void *mem = malloc(size);

    return mem;
}

struct Custom_Type {
    char *name;
};

Custom_Type *
custom_type_new(char *name) {
    Custom_Type *result = (Custom_Type *)custom_alloc(sizeof(Custom_Type));

    result->name = name;

    return result;
}

void
parser_test() {
    using namespace Urq::api;

    Urq::parser_alloc = custom_alloc;

    Parser_State result = {};
    Parser *parser = NULL;

    parser = letters;
    result = run(parser, "asd12fas");
    assert(result.success && result.result.str.len == 3);

    parser = digits;
    result = run(parser, "12fas");
    assert(result.success && result.result.str.len == 2);

    parser = seq_of({
        letters,
        digits,
        letters
    });
    result = run(parser, "abcd1234efgh");
    assert(result.success && result.result.arr.val.num_elems == 3);

    parser = choice({
        letters,
        digits
    });
    result = run(parser, "123abc");
    assert(result.success && result.result.str.len == 3);
    result = run(parser, "abc123");
    assert(result.success && result.result.str.len == 3);

    parser = many(choice({
        letters,
        digits
    }));
    result = run(parser, "123abc");
    assert(result.success && result.result.arr.val.num_elems == 2);

    parser = many1(choice({
        letters,
        digits
    }));
    result = run(parser, "**123abc");
    assert(!result.success);

    auto between_proc = between(chr('('), chr(')'));
    parser = between_proc(letters);
    result = run(parser, "(hallowiegehts)");
    assert(result.success && result.result.str.len == strlen("hallowiegehts"));

    parser = chain(
        map(
            seq_of({letters, chr(':')}),
            [](Parser_Result result, size_t index) {
                return parser_result_entry(&result.arr.val, 0);
            }
        ),
        [](Parser_Result result) {
            if ( strncmp(result.str.val, "string", 6) == 0 ) {
                return letters;
            }

            return digits;
        }
    );
    result = run(parser, "string:test");
    assert(result.success && result.result.str.len == strlen("test") && strcmp(result.result.str.val, "test") == 0);

    auto comma_proc = sep_by(chr(','));
    auto brckt_proc = between(chr('['), chr(']'));

    parser = brckt_proc(comma_proc(digits));
    result = run(parser, "[1,2,3,4,5]");
    assert(result.success && result.result.arr.val.num_elems == 5);
}

#define CUSTOM_ALLOC_STRUCT(name) (name *)custom_alloc(sizeof(name))

enum Expr_Kind {
    EXPR_NONE,
    EXPR_NUM,
    EXPR_BIN,
};
struct Expr {
    Expr_Kind kind;
    uint32_t num;

    union {
        struct {
            char op;
            Expr *a;
            Expr *b;
        } bin;
    };
};

Expr *
expr_num(uint32_t num) {
    Expr *result = CUSTOM_ALLOC_STRUCT(Expr);

    result->kind = EXPR_NUM;
    result->num  = num;

    return result;
}

Expr *
expr_bin(char op, Expr *a, Expr *b) {
    Expr *result = CUSTOM_ALLOC_STRUCT(Expr);
    *result = {};

    result->kind   = EXPR_BIN;
    result->bin.op = op;
    result->bin.a  = a;
    result->bin.b  = b;

    return result;
}

int main(int argc, char const* argv[]) {
    using namespace Urq::api;

    parser_test();

    auto parens = between(chr('('), chr(')'));
    auto number = map(digits, [](Parser_Result result, size_t index) {
        int num = 0;
        for ( int i = 0; i < result.str.len; ++i ) {
            num *= 10;
            num += result.str.val[i] - '0';
        }

        return parser_result_custom(expr_num(num));
    });

    auto op = choice({
        chr('+'), chr('-'), chr('*'), chr('/')
    });

    auto expr = choice({ number, empty() });

    auto operation = map(parens(seq_of({
        op,
        whitespace,
        expr,
        whitespace,
        expr
    })), [](Parser_Result result, size_t index) {
        assert(result.arr.val.num_elems == 5);

        char op = parser_result_entry(&result.arr.val, 0).chr.val;
        Expr *a = (Expr *)parser_result_entry(&result.arr.val, 2).custom.val;
        Expr *b = (Expr *)parser_result_entry(&result.arr.val, 4).custom.val;

        return parser_result_custom(expr_bin(op, a, b));
    });

    fill_empty(expr, operation);
    auto result = run(operation, "(+ 1 (- 10 5))");

    return 0;
}
