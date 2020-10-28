#include <assert.h>

#include "combinator.cpp"

PARSER_ALLOCATOR(custom_alloc) {
    printf("%zd bytes reserviert\n", size);
    void *mem = malloc(size);

    return mem;
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

int evaluate(Expr *expr) {
    if ( expr->kind == EXPR_NUM ) {
        return expr->num;
    }

    else if ( expr->kind == EXPR_BIN ) {
        switch ( expr->bin.op ) {
            case '+': {
                return evaluate(expr->bin.a) + evaluate(expr->bin.b);
            } break;

            case '-': {
                return evaluate(expr->bin.a) - evaluate(expr->bin.b);
            } break;

            case '*': {
                return evaluate(expr->bin.a) * evaluate(expr->bin.b);
            } break;

            case '/': {
                return evaluate(expr->bin.a) / evaluate(expr->bin.b);
            } break;
        }
    }

    assert(!"nicht unterstützter ausdruck");
    return 0;
}

void interpreter(Urq::Parser *parser, char *program) {
    auto result = run(parser, program);

    if ( !result.success ) {
        printf("programm konnte nicht erfolgreich geparst werden\n");
        return;
    }

    Expr *expr = (Expr *)result.result.custom.val;
    int out = evaluate(expr);

    printf("ergebnis: %d\n", out);
}

int main(int argc, char const* argv[]) {
    using namespace Urq::api;

    auto parens = Between(Chr('('), Chr(')'));
    auto number = Map(Digits, [](Parser_Result result, size_t index, void *user_data) {
        int num = 0;
        for ( int i = 0; i < result.str.len; ++i ) {
            num *= 10;
            num += result.str.val[i] - '0';
        }

        return parser_result_custom(expr_num(num));
    });

    auto op = Choice({
        Chr('+'), Chr('-'), Chr('*'), Chr('/')
    });

    auto expr = Choice({ number, Empty });

    auto program = Map(parens(Seq_Of({
        op,
        Whitespace,
        expr,
        Whitespace,
        expr
    })), [](Parser_Result result, size_t index, void *user_data) {
        assert(result.arr.val.num_elems == 5);

        char op = parser_result_entry(&result.arr.val, 0).chr.val;
        Expr *a = (Expr *)parser_result_entry(&result.arr.val, 2).custom.val;
        Expr *b = (Expr *)parser_result_entry(&result.arr.val, 4).custom.val;

        return parser_result_custom(expr_bin(op, a, b));
    });

    fill_empty(expr, program);

    interpreter(program, "(+ 1 (- 10 5))");

    return 0;
}

