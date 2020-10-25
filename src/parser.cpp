#include <initializer_list>

namespace Urq {

struct Parser;
struct Parser_Value;
struct Parser_State;
struct Parser_Result;
struct Parser_List;

Parser_Result parser_result_str(char *str, size_t len);
Parser_State  parser_update_state(Parser_State in, size_t index, Parser_Result r);
Parser_State  parser_update_result(Parser_State in, Parser_Result r);
Parser_State  parser_update_error(Parser_State in, char *msg);

#define PARSER_PROC(name) Parser_State name(Parser p, Parser_State state)
typedef PARSER_PROC(Parser_Proc);

#define PARSER_MAP_PROC(name) Parser_Result name(Parser_Result result, size_t index)
typedef PARSER_MAP_PROC(Parser_Map_Proc);

#define PARSER_ALLOCATOR(name) void * name(size_t size)
typedef PARSER_ALLOCATOR(Alloc);

PARSER_ALLOCATOR(parser_alloc_default) {
    void *result = malloc(size);

    return result;
}

Alloc *parser_alloc = parser_alloc_default;

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
    char *str;
    char n[10];
    Parser_Map_Proc *map_proc;
    Parser *p;

    Parser_List   sequence;
    Parser_Proc * proc;
};

void
parser_push(Parser_List *list, Parser p) {
    if ( list->num_elems >= list->cap ) {
        size_t new_cap = (list->cap < 16) ? 16 : list->cap*2;
        void *mem = parser_alloc(sizeof(Parser)*new_cap);
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
        void *mem = parser_alloc(sizeof(Parser_Result)*new_cap);
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
    char *msg;

    char *val;
    Parser_Result result;
    size_t index;
};

Parser
parser_create(Parser_Proc *proc) {
    Parser result = {};

    result.proc = proc;

    return result;
}

Parser whitespace = parser_create(
    [](Parser p, Parser_State state) {
        char *s = state.val+state.index;
        while ( *s == ' ' || *s == '\t' || *s == '\r' || *s == '\v' || *s == '\n' ) {
            s++;
        }

        return parser_update_state(state, s-state.val, parser_result_str(state.val+state.index, s-(state.val+state.index)));
    }
);

Parser digits = parser_create(
    [](Parser p, Parser_State state) {
        char *s = state.val+state.index;

        if ( !strlen(s) ) {
            return parser_update_error(state, "digits: ende der eingabe erreicht");
        }

        while ( *s >= '0' && *s <= '9' ) {
            s++;
        }

        if (s == state.val) {
            return parser_update_error(state, "digits: keine ziffern gefunden");
        }

        return parser_update_state(state, s-state.val, parser_result_str(state.val+state.index, s-(state.val+state.index)));
    }
);

Parser letters = parser_create(
    [](Parser p, Parser_State state) {
        char *s = state.val+state.index;

        if ( !strlen(s) ) {
            return parser_update_error(state, "letters: ende der eingabe erreicht");
        }

        while ( *s >= 'A' && *s <= 'Z' || *s >= 'a' && *s <= 'z' ) {
            s++;
        }

        if (s == state.val) {
            return parser_update_error(state, "letters: keine buchstaben gefunden");
        }

        return parser_update_state(state, s-state.val, parser_result_str(state.val+state.index, s-(state.val+state.index)));
    }
);

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

Parser_State
parser_update_state(Parser_State in, size_t index, Parser_Result r) {
    Parser_State result = {};

    result.success = true;
    result.val     = in.val;
    result.result  = r;
    result.index   = index;

    return result;
}

Parser_State
parser_update_result(Parser_State in, Parser_Result r) {
    Parser_State result = {};

    result.success = true;
    result.val     = in.val;
    result.index   = in.index;
    result.result  = r;

    return result;
}

Parser_State
parser_update_error(Parser_State in, char *msg) {
    Parser_State result = {};

    result.success = false;
    result.val     = in.val;
    result.index   = in.index;
    result.result  = in.result;
    result.msg     = msg;

    return result;
}

Parser
chr(char c) {
    Parser p = {};

    p.str  = (char *)parser_alloc(1);
    p.str[0] = c;
    p.proc = [](Parser p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        Parser_State result = {};

        if ( state.val[state.index] == p.str[0] ) {
            return parser_update_state(state, state.index + 1, parser_result_chr(p.str[0]));
        }

        return parser_update_error(state, "chr: das gesuchte zeichen wurde nicht gefunden");
    };

    return p;
}

Parser
str(char *str) {
    Parser p = {};

    p.str  = str;
    p.proc = [](Parser p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

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
            return parser_update_state(state, state.index + len,
                    parser_result_str(state.val+state.index, len));
        }

        return parser_update_error(state, "str: die gesuchte zeichenkette wurde nicht gefunden");
    };

    return p;
}

Parser
number(int n) {
    Parser p = {};

    int count = 0;
    int cnd = 0;
    int temp = 0;
    int i;

    if ( n >> 31 ) {
        n = ~n + 1;
        for(temp = n; temp != 0; temp /= 10, count++);
        p.n[0] = 0x2D;
        count++;
        cnd = 1;
    } else {
        for(temp = n; temp != 0; temp /= 10, count++);
    }

    for(i = count-1, temp = n; i >= cnd; i--) {
        p.n[i] = (temp % 10) + 0x30;
        temp /= 10;
    }

    p.proc = [](Parser p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        size_t len = strlen(p.n);

        if (strlen(state.val+state.index) < len) {
            return parser_update_error(state, "number: unerwartet das ende erreicht");
        }

        for ( int i = 0; i < len; ++i ) {
            if ( p.n[i] != (state.val+state.index)[i] ) {
                return parser_update_error(state, "number: nummer wurde nicht erkannt");
            }
        }

        return parser_update_state(state, state.index+len, parser_result_str(p.n, len));
    };

    return p;
}

Parser
sequence_of(std::initializer_list<Parser> s) {
    Parser p = {};

    Parser_List sequence = {};
    for ( Parser i : s ) {
        parser_push(&sequence, i);
    }

    p.sequence  = sequence;
    p.proc = [](Parser p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        Parser_State result = {};

        Parser_State new_state = state;
        Parser_Result_List results = {};

        for ( int i = 0; i < p.sequence.num_elems; ++i ) {
            Parser seq_p = parser_entry(&p.sequence, i);
            new_state = seq_p.proc(seq_p, new_state);

            if ( !new_state.success ) {
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

Parser
choice(std::initializer_list<Parser> s) {
    Parser p = {};

    Parser_List sequence = {};
    for ( Parser i : s ) {
        parser_push(&sequence, i);
    }

    p.sequence  = sequence;
    p.proc = [](Parser p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        Parser_State result = {};
        for ( int i = 0; i < p.sequence.num_elems; ++i ) {
            Parser seq_p = parser_entry(&p.sequence, i);
            Parser_State new_state = seq_p.proc(seq_p, state);

            if ( new_state.success ) {
                return new_state;
            }

            result = new_state;
        }

        return result;
    };

    return p;
}

Parser
map(Parser p, Parser_Map_Proc *proc) {
    Parser result = {};

    /* @INFO: weil c++ dämlich ist, kann im lambda kein "einfangen" der kontextvariablen
     * erfolgen, denn sonst entspricht das lambda nicht mehr dem typedef
     */
    result.p = (Parser *)parser_alloc(sizeof(Parser));
    result.p->str = p.str;
    for ( int i = 0; i < 10; ++i ) {
        result.p->n[i]   = p.n[i];
    }
    result.p->p   = p.p;
    result.p->sequence = p.sequence;
    result.p->proc = p.proc;
    result.p->map_proc = p.map_proc;

    result.map_proc = proc;

    result.proc = [](Parser p, Parser_State state) {
        Parser_State new_state = p.p->proc(*p.p, state);

        if ( !new_state.success ) {
            return new_state;
        }

        return parser_update_state(new_state, new_state.index, p.map_proc(new_state.result, new_state.index));
    };

    return result;
}

Parser
error_map(Parser p, Parser_Map_Proc *proc) {
    Parser result = {};

    /* @INFO: weil c++ dämlich ist, kann im lambda kein "einfangen" der kontextvariablen
     * erfolgen, denn sonst entspricht das lambda nicht mehr dem typedef
     */
    result.p = (Parser *)parser_alloc(sizeof(Parser));
    result.p->str = p.str;
    for ( int i = 0; i < 10; ++i ) {
        result.p->n[i]   = p.n[i];
    }
    result.p->p   = p.p;
    result.p->sequence = p.sequence;
    result.p->proc = p.proc;
    result.p->map_proc = p.map_proc;

    result.map_proc = proc;

    result.proc = [](Parser p, Parser_State state) {
        Parser_State new_state = p.p->proc(*p.p, state);

        if ( new_state.success ) {
            return new_state;
        }

        Parser_Result map_proc_result = p.map_proc(new_state.result, new_state.index);

        return parser_update_error(new_state, map_proc_result.str.val);
    };

    return result;
}

Parser
many(Parser p) {
    Parser result = {};

    /* @INFO: weil c++ dämlich ist, kann im lambda kein "einfangen" der kontextvariablen
     * erfolgen, denn sonst entspricht das lambda nicht mehr dem typedef
     */
    result.p = (Parser *)parser_alloc(sizeof(Parser));
    result.p->str = p.str;
    for ( int i = 0; i < 10; ++i ) {
        result.p->n[i]   = p.n[i];
    }
    result.p->p   = p.p;
    result.p->sequence = p.sequence;
    result.p->proc = p.proc;
    result.p->map_proc = p.map_proc;

    result.proc = [](Parser p, Parser_State state) {
        Parser_Result_List results = {};
        Parser_State new_state = state;

        for ( ;; ) {
            new_state = p.p->proc(*p.p, new_state);

            if ( new_state.success ) {
                parser_result_push(&results, new_state.result);
                continue;
            }

            break;
        }

        return parser_update_result(new_state, parser_result_arr(results));
    };

    return result;
}

Parser
many1(Parser p) {
    Parser result = {};

    /* @INFO: weil c++ dämlich ist, kann im lambda kein "einfangen" der kontextvariablen
     * erfolgen, denn sonst entspricht das lambda nicht mehr dem typedef
     */
    result.p = (Parser *)parser_alloc(sizeof(Parser));
    result.p->str = p.str;
    for ( int i = 0; i < 10; ++i ) {
        result.p->n[i]   = p.n[i];
    }
    result.p->p   = p.p;
    result.p->sequence = p.sequence;
    result.p->proc = p.proc;
    result.p->map_proc = p.map_proc;

    result.proc = [](Parser p, Parser_State state) {
        Parser_Result_List results = {};
        Parser_State new_state = state;

        for ( ;; ) {
            new_state = p.p->proc(*p.p, new_state);

            if ( new_state.success ) {
                parser_result_push(&results, new_state.result);
                continue;
            }

            break;
        }

        if ( results.num_elems == 0 ) {
            return parser_update_error(state, "many1: konnte keinen treffer erzielen");
        }

        return parser_update_result(new_state, parser_result_arr(results));
    };

    return result;
}

void
chain() {
    /* @AUFGABE: implementieren */
}

Parser_State
run(Parser p, char *str) {
    Parser_State state = {};

    state.success = true;
    state.val     = str;
    state.index   = 0;

    Parser_State result = p.proc(p, state);

    return result;
}

namespace api {
    using Urq::chr;
    using Urq::str;
    using Urq::number;
    using Urq::sequence_of;
    using Urq::choice;
    using Urq::many;
    using Urq::many1;
    using Urq::whitespace;
    using Urq::digits;
    using Urq::letters;
    using Urq::run;
    using Urq::parser_result_str;
    using Urq::parser_update_state;
    using Urq::parser_update_error;

    using Urq::Parser;
    using Urq::Parser_State;
    using Urq::Parser_Result;
};

};

