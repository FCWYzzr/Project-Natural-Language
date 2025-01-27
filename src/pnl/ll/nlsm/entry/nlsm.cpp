//
// Created by FCWY on 24-12-28.
//
import pnl.ll;
#include <project-nl.h>
#include <print>
import nlsm.grammar;

using namespace pnl::ll;
using namespace pnl::ll::collections;
using namespace nlsm;

constexpr static char help[] = R"(
usage
    nlsm <input> [-o <output>]
    nlsm -i <input dir> [-o <output dir>]

[xxx]           optional param xxx
<input>         input package file(s)
<output>        output package file(s) or directory
<input dir>     input directory
<output dir>    output directory

example
    nlsm i1.nlsm i2.nlsm i3.nlsm i4.nlsm i5.nlsm
    nlsm -i ./src -o ./out
)";

static auto buffer = std::array<UByte, 16_KB>{};
static auto mem = std::pmr::monotonic_buffer_resource{
    buffer.data(), buffer.size(),
    std::pmr::new_delete_resource()
};

static void compile(const Path& input_file, const Path& output_file) {
    auto fi = std::ifstream{input_file};
    auto ret = compile(fi, mem, Str{input_file.stem().u32string(), &mem});
    auto fo = BOFStream{output_file, std::ios::binary | std::ios::out};
    Serializer<Package>{ret}.serialize(fo);
}
[[noreturn]]
static void print_help()  {
    std::println(std::cerr, help);
    exit(0);
}

int main(const int argc, char* const argv[]) {
    auto dir_inputs = false;
    auto inputs = Queue<Path>{&mem};
    auto outputs = Queue<Path>{&mem};

    if (argc <= 1)
        print_help();


    for (auto cur = &inputs;
        const auto i: std::views::iota(1, argc))
        if (strcmp(argv[i], "-i") == 0)
            dir_inputs = true;
        else if (strcmp(argv[i], "-o") == 0)
            cur = &outputs;
        else
            cur -> emplace(Path{argv[i]});

    if (outputs.size() == 0) {
        outputs.emplace(absolute(std::filesystem::current_path() /= "compiled"));
        create_directory(outputs.front());
    }
    if (is_directory(outputs.front())) {
        if (outputs.size() > 1) {
            std::println(std::cerr, "more than one output directories\n");
            print_help();
        }
        if (!dir_inputs) {
            const auto dir = std::move(outputs.front());
            outputs.pop();

            for (auto[[maybe_unused]]_: std::views::iota(0u, inputs.size())) {
                outputs.emplace((dir / inputs.front().stem()) += PKG_EXT);
                inputs.emplace(std::move(inputs.front()));
                inputs.pop();
            }
        }
    }
    else if (dir_inputs) {
        std::println(std::cerr, "file output with directory input is NOT allow\n");
        print_help();
    }
    else if (outputs.size() != inputs.size()) {
        std::println(std::cerr, "i/o file count mismatch: I:{} -> O:{}\n", inputs.size(), outputs.size());
        print_help();
    }


    if (dir_inputs) {
        std::print("output directory: {}\n", outputs.front().string());
        for (auto i = 0;
            auto& entry:
            std::filesystem::recursive_directory_iterator{inputs.front()})
            if (entry.path().extension() == ASM_EXT){
                auto out = (outputs.front() / entry.path().stem()) += PKG_EXT;
                compile(entry.path(), out);
                std::println("{}/? {} => {}", ++i, entry.path().string(), relative(out, outputs.front()).string());
            }
    }
    else {
        auto tot = inputs.size();
        auto ii = 0;
        while (!inputs.empty()) {
            auto i = std::move(inputs.front());
            auto o = std::move(outputs.front());
            inputs.pop();
            outputs.pop();

            compile(i, o);
            std::println("{}/{} {} => {}", ++ii, tot , i.string(), o.string());
        }
    }
}
