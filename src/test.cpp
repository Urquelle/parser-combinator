#include <assert.h>

#include "combinator.cpp"

ALLOCATOR(custom_alloc) {
    printf("%zd bytes reserviert\n", size);
    void *mem = malloc(size);

    return mem;
}

void
parser_test() {
    using namespace Urq::api;

    Urq::parser_alloc = custom_alloc;

    Parser_State result = {};
    Parser *parser = NULL;

    parser = Letters;
    result = run(parser, "asd12fas");
    assert(result.success && result.result.str.len == 3);

    parser = Digits;
    result = run(parser, "12fas");
    assert(result.success && result.result.str.len == 2);

    parser = Seq_Of({
        Letters,
        Digits,
        Letters
    });
    result = run(parser, "abcd1234efgh");
    assert(result.success && result.result.arr.len == 3);

    parser = Choice({
        Letters,
        Digits
    });
    result = run(parser, "123abc");
    assert(result.success && result.result.str.len == 3);
    result = run(parser, "abc123");
    assert(result.success && result.result.str.len == 3);

    parser = Many(Choice({
        Letters,
        Digits
    }));
    result = run(parser, "123abc");
    assert(result.success && result.result.arr.len == 2);

    parser = Many1(Choice({
        Letters,
        Digits
    }));
    result = run(parser, "**123abc");
    assert(!result.success);

    auto between_proc = Between(Chr('('), Chr(')'));
    parser = between_proc(Letters);
    result = run(parser, "(hallowiegehts)");
    assert(result.success && result.result.str.len == strlen("hallowiegehts"));

    parser = Chain(
        Map(
            Seq_Of({ Letters, Chr(':') }),
            [](Parser_Result result, size_t index, void *user_data) {
                return parser_result_entry(&result.arr.val, 0);
            }
        ),
        [](Parser_Result result, void *user_data) {
            if ( strncmp(result.str.val, "string", 6) == 0 ) {
                return Letters;
            }

            return Digits;
        }
    );
    result = run(parser, "string:test");
    assert(result.success && result.result.str.len == strlen("test") && strcmp(result.result.str.val, "test") == 0);

    auto comma_proc = Sep_By(Chr(','));
    auto brckt_proc = Between(Chr('['), Chr(']'));

    parser = brckt_proc(comma_proc(Digits));
    result = run(parser, "[1,2,3,4,5]");
    assert(result.success && result.result.arr.len == 5);

    parser = Regex("[a-z]+");
    result = run(parser, "abcasj");
    int x = 5;
}

int main(int argc, char const* argv[]) {
    using namespace Urq::api;

    parser_test();

    return 0;
}

