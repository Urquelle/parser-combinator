#include <initializer_list>

struct Parser;
struct Parser_Value;
struct Parser_State;
struct Parser_Result;
struct Parser_List;

Parser_Result parser_result_str(char *str, size_t len);

#define PARSER_PROC(name) Parser_State name(Parser p, Parser_State state)
typedef PARSER_PROC(Parser_Proc);

struct Parser_List {
    Parser * elems;
    size_t   num_elems;
    size_t   cap;
};

struct Parser_Result_List {
    Parser_Result * elems;
    size_t          num_elems;
    size_t          cap;
};

struct Parser {
    char c;
    char *str;

    Parser_List   sequence;
    Parser_Proc * proc;
};

void
parser_push(Parser_List *list, Parser p) {
    if ( list->num_elems >= list->cap ) {
        size_t new_cap = (list->cap < 16) ? 16 : list->cap*2;
        void *mem = malloc(sizeof(Parser)*new_cap);
        memcpy(mem, list->elems, list->num_elems*sizeof(Parser));
        free(list->elems);

        list->elems = (Parser *)mem;
        list->cap   = new_cap;
    }

    list->elems[list->num_elems++] = p;
}

Parser
parser_entry(Parser_List *list, size_t i) {
    Parser result = {};

    if ( i >= list->num_elems ) {
        return result;
    }

    result = list->elems[i];

    return result;
}

enum {
    PARSER_RESULT_NONE,
    PARSER_RESULT_CHR,
    PARSER_RESULT_STR,
    PARSER_RESULT_ARR,
};
struct Parser_Result {
    uint32_t kind;

    union {
        struct {
            char val;
        } chr;

        struct {
            char * val;
            size_t len;
        } str;

        struct {
            Parser_Result_List val;
        } arr;
    };
};

void
parser_result_push(Parser_Result_List *list, Parser_Result p) {
    if ( list->num_elems >= list->cap ) {
        size_t new_cap = (list->cap < 16) ? 16 : list->cap*2;
        void *mem = malloc(sizeof(Parser_Result)*new_cap);
        memcpy(mem, list->elems, list->num_elems*sizeof(Parser_Result));
        free(list->elems);

        list->elems = (Parser_Result *)mem;
        list->cap   = new_cap;
    }

    list->elems[list->num_elems++] = p;
}

Parser_Result
parser_result_entry(Parser_Result_List *list, size_t i) {
    Parser_Result result = {};

    if ( i >= list->num_elems ) {
        return result;
    }

    result = list->elems[i];

    return result;
}

struct Parser_State {
    bool success;
    char *val;
    Parser_Result result;
    size_t index;
};

Parser whitespace = {
    ' ', NULL, {}, [](Parser p, Parser_State state) {
        Parser_State result = {};

        char *s = state.val+state.index;
        while ( *s == ' ' || *s == '\t' || *s == '\r' || *s == '\v' || *s == '\n' ) {
            s++;
        }

        result.success = true;
        result.val     = state.val;
        result.result  = parser_result_str(" ", 1);
        result.index   = s-state.val;

        return result;
    }
};

Parser_Result
parser_result_chr(char c) {
    Parser_Result result = {};

    result.kind = PARSER_RESULT_CHR;
    result.chr.val = c;

    return result;
}

Parser_Result
parser_result_str(char *str, size_t len) {
    Parser_Result result = {};

    result.kind = PARSER_RESULT_STR;
    result.str.val = str;
    result.str.len = len;

    return result;
}

Parser_Result
parser_result_arr(Parser_Result_List arr) {
    Parser_Result result = {};

    result.kind = PARSER_RESULT_ARR;
    result.arr.val = arr;

    return result;
}

Parser
parser_char(char c) {
    Parser p = {};

    p.c  = c;
    p.proc = [](Parser p, Parser_State state) {
        Parser_State result = {};

        if ( state.val[state.index] == p.c ) {
            result.success = true;
            result.val = state.val;
            result.result = parser_result_chr(p.c);
            result.index = state.index + 1;
        }

        return result;
    };

    return p;
}

Parser
parser_str(char *str) {
    Parser p = {};

    p.str  = str;
    p.proc = [](Parser p, Parser_State state) {
        Parser_State result = {};

        size_t len = strlen(p.str);
        if ( strlen(state.val+state.index) < len ) {
            return result;
        }

        bool string_found = true;
        for ( int i = 0; i < len; ++i ) {
            if ( (state.val+state.index)[i] != p.str[i] ) {
                string_found = false;
                break;
            }
        }

        if ( string_found ) {
            result.success = true;
            result.val = state.val;
            result.result = parser_result_str(state.val+state.index, len);
            result.index = state.index + len;
        }

        return result;
    };

    return p;
}

Parser
parser_sequence_of(std::initializer_list<Parser> s) {
    Parser p = {};

    Parser_List sequence = {};
    for ( Parser i : s ) {
        parser_push(&sequence, i);
    }

    p.sequence  = sequence;
    p.proc = [](Parser p, Parser_State state) {
        Parser_State result = {};

        Parser_State new_state = state;
        Parser_Result_List results = {};

        for ( int i = 0; i < p.sequence.num_elems; ++i ) {
            Parser seq_p = parser_entry(&p.sequence, i);
            new_state = seq_p.proc(seq_p, new_state);

            if ( !new_state.success ) {
                printf("fehler aufgetreten\n");
                return new_state;
            }

            parser_result_push(&results, new_state.result);
        }

        result.result = parser_result_arr(results);
        result.success = true;

        return result;
    };

    return p;
}

void
parser_run(Parser p, char *str) {
    Parser_State state = {};

    state.val = str;
    state.index = 0;

    Parser_State result = p.proc(p, state);

    if ( result.success ) {
        printf("yippie\n");
        return;
    }

    printf("hat nicht gegeht\n");
}

