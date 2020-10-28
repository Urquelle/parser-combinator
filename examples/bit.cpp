#include "binary.cpp"

int main(int argc, char const* argv[]) {
    using namespace Urq::api;

    auto parser = Seq_Of({
        Uint(8), Uint(8)
    });

    char program[2] = { (char)234, (char)235 };
    auto result = run(parser, program);

    parser = Raw_String("Hallo Welz!");
    result = run(parser, "Hallo Welt!");

    return 0;
}
