#ifndef __PARSER_COMBINATOR_BASE__
#include "combinator.cpp"
#endif

namespace Urq {
    Parser *Bit = parser_create([](Parser *p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        size_t byte_offset = (state.index / 8);
        uint32_t bit_offset  = 7 - (state.index % 8);

        size_t len = strlen(state.val);
        if ( byte_offset >= len ) {
            return parser_update_error(state, "Bit: unerwartet ende der eingabe erreicht");
        }

        uint8_t value = ((uint8_t)(state.val+byte_offset)[0] & (1 << bit_offset)) >> bit_offset;

        return parser_update_state(state, state.index+1, parser_result_u64(value));
    });

    Parser *One = parser_create([](Parser *p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        size_t byte_offset = (state.index / 8);
        uint32_t bit_offset  = 7 - (state.index % 8);

        size_t len = strlen(state.val);
        if ( byte_offset >= len ) {
            return parser_update_error(state, "Bit: unerwartet ende der eingabe erreicht");
        }

        uint8_t value = ((uint8_t)(state.val+byte_offset)[0] & (1 << bit_offset)) >> bit_offset;

        if ( value != 1 ) {
            return parser_update_error(state, "One: eine 1 erwartet, aber keine 1 gefunden");
        }

        return parser_update_state(state, state.index+1, parser_result_u64(value));
    });

    Parser *Zero = parser_create([](Parser *p, Parser_State state) {
        if ( !state.success ) {
            return state;
        }

        size_t byte_offset = (state.index / 8);
        uint32_t bit_offset  = 7 - (state.index % 8);

        size_t len = strlen(state.val);
        if ( byte_offset >= len ) {
            return parser_update_error(state, "Bit: unerwartet ende der eingabe erreicht");
        }

        uint8_t value = ((uint8_t)(state.val+byte_offset)[0] & (1 << bit_offset)) >> bit_offset;

        if ( value != 0 ) {
            return parser_update_error(state, "Zero: eine 0 erwartet, aber keine 0 gefunden");
        }

        return parser_update_state(state, state.index+1, parser_result_u64(value));
    });

    Parser *
    Uint(uint16_t num) {
        if ( num <= 0 || num > 64 ) {
            return Fail("Uint: der übergebene wert muß zwischen 1 und 64 liegen");
        }

        Parser_List sequence = {};
        for ( size_t i = 0; i < num; ++i ) {
            parser_push(&sequence, Bit);
        }

        auto result = Map(Seq_Of(sequence), [](Parser_Result result, size_t index, void *user_data) {
            uint64_t acc = 0;

            for ( int i = 0; i < result.arr.len; ++i ) {
                acc += parser_result_entry(&result.arr.val, i).u64.val
                    << (result.arr.len - 1 - i);
            }

            return parser_result_u64(acc);
        });

        return result;
    }

    Parser *
    Int(uint16_t num) {
        if ( num <= 0 || num > 64 ) {
            return Fail("Int: der übergebene wert muß zwischen 1 und 64 liegen");
        }

        Parser_List sequence = {};
        for ( size_t i = 0; i < num; ++i ) {
            parser_push(&sequence, Bit);
        }

        auto result = Map(Seq_Of(sequence), [](Parser_Result result, size_t index, void *user_data) {
            int64_t acc = 0;

            if ( parser_result_entry(&result.arr.val, 0).u64.val == 0 ) {
                for ( int i = 0; i < result.arr.len; ++i ) {
                    acc += parser_result_entry(&result.arr.val, i).u64.val
                        << (result.arr.len - 1 - i);
                }
            } else {
                for ( int i = 0; i < result.arr.len; ++i ) {
                    acc += 
                        (parser_result_entry(&result.arr.val, i).u64.val ? 0 : 1)
                        << (result.arr.len - 1 - i);
                }

                acc = -(1 + acc);
            }

            return parser_result_s64(acc);
        });

        return result;
    }

    Parser *
    Raw_String(char *str) {
        size_t len = strlen(str);

        if ( len < 1 ) {
            return Fail("Raw_String: die länge der zeichenkette darf nicht kürzer als 1 sein");
        }

        Parser_List sequence = {};
        for ( int i = 0; i < len; ++i ) {
            uint8_t n = (uint8_t)str[i];

            auto parser = Chain(Uint(8), [](Parser_Result result, void *user_data) {
                uint8_t n = *(uint8_t *)user_data;

                if ( result.u64.val == n ) {
                    return Succeed(result);
                } else {
                    return Fail("Raw_String: erwartet %c, aber %c erhalten", n, result.u64.val);
                }
            });

            uint8_t *user_data = (uint8_t *)parser_alloc(sizeof(uint8_t));
            *user_data = n;

            parser->user_data = user_data;
            parser_push(&sequence, parser);
        }

        return Seq_Of(sequence);
    }

    namespace api {
        using Urq::Bit;
        using Urq::One;
        using Urq::Zero;

        using Urq::Uint;
        using Urq::Int;
        using Urq::Raw_String;
    }
}
