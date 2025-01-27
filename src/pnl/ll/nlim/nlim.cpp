//
// Created by FCWY on 25-1-24.
//
#include <project-nl.h>
#include <print>
import pnl.ll;
using namespace pnl::ll;

constexpr static char help[] = R"(
usage
    nlim <input> [-d <dependency>] [-o <output>]

[xxx]           optional param xxx
<input>         input file(s) or directory(s)
<dependency>    dependent image(s)
<output>        output file

example
    nlim core.nlpkg
    nlim ./io -d core.nlimg
)";

static auto buffer = std::array<UByte, 16_KB>{};
static auto mem = std::pmr::monotonic_buffer_resource{
    buffer.data(), buffer.size(),
    std::pmr::new_delete_resource()
};

[[noreturn]]
static void print_help()  {
    std::println(help);
    exit(0);
}

int main(int argc, char* argv[]) {
    auto inputs = Queue<Path>{&mem};
    auto dependencies = Queue<Path>{&mem};
    auto output = Path{};

    if (argc <= 1)
        print_help();

    {
        int phase = 0;
        for (auto cur = &inputs;
            auto s: std::views::iota(1, argc)
            | std::views::transform([=](const auto i){ return argv[i]; })) {
            if (phase == 2) [[unlikely]]{
                output = s;
                break;
            }
            if (strcmp(s, "-d") == 0) [[unlikely]] {
                cur = &dependencies;
                phase = 1;
                continue;
            }
            if (strcmp(s, "-o") == 0) [[unlikely]] {
                phase = 2;
                continue;
            }
            if (auto path = Path{s};
                is_directory(path)) [[unlikely]]
                for (auto entry: std::filesystem::recursive_directory_iterator{path}) {
                    if (entry.is_regular_file()) {
                        if (phase == 0 && entry.path().extension() == PKG_EXT)
                            inputs.emplace(entry.path());
                        else if (phase == 1 && entry.path().extension() == IMG_EXT)
                            dependencies.emplace(entry.path());
                    }
                }
            else
                cur->emplace(std::move(path));
        }
    }

    if (inputs.empty()) [[unlikely]]{
        std::println(std::cerr, "no input avaliable!");
        print_help();
    }

    const auto i_size = inputs.size();
    auto err = Str{&mem};
    auto process = Process{&mem};
    while (!dependencies.empty()) {
        auto patch = Patch{&mem};
        auto ifs = BIFStream{dependencies.front(), std::ios::binary | std::ios::in};
        Serializer<Patch>{patch}.deserialize(ifs);
        process.load_patch(std::move(patch), &err);

        if (!err.empty()) {
            std::cerr << cvt(err) << std::endl;
            print_help();
        }
        dependencies.pop();
    }
    
    
    auto datapack = Datapack{&mem};
    while (!inputs.empty()) {
        if (output.empty()) [[unlikely]] {
            output = (std::filesystem::current_path() /= inputs.front().stem());
            if (inputs.size() > 1)
                output += ".etc";
            output += IMG_EXT;
        }
        auto bis = BIFStream{inputs.front(), std::ios::binary | std::ios::in};
        auto pkg = Package{&mem};
        Serializer<Package>{pkg}.deserialize(bis);
        datapack.emplace(cvt(inputs.front().stem().string(), mem), std::move(pkg));
        inputs.pop();
    }

    auto maybe_patch = process.compile_datapack(std::move(datapack), &err);

    if (!maybe_patch.has_value()) {
        std::println(std::cerr, "{}\n", cvt(err));
        print_help();
    }

    auto& patch = maybe_patch.value();

    auto ofs = BOFStream{output, std::ios::binary | std::ios::out};
    Serializer<Patch>{patch}.serialize(ofs);
    std::println("build image from {} pkgs => {}", i_size, output.string());
}