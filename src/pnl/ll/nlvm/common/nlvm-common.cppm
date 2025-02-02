//
// Created by FCWY on 25-1-23.
//
module;
#include <project-nl.h>
export module nlvm.common;
import pnl.ll;

using namespace std::filesystem;
using namespace pnl::ll;





export namespace nlvm::inline common{
    constexpr char help[] = R"(
usage
    nlvm [-n <library>] [-nd <library dir>] [-l <image>] -r <input> [-p <params>]
    nlvm <input> [-p <params>]

[xxx]           optional param xxx
<image>         image(s) to be loaded (IN INPUT ORDER)
<library>       platform library
<library dir>   platform library directory
<input>         input package file(s)
<params>        params given to proc main

example
    nlvm test.nlpkg builtins.nlpkg
    nlvm -n rt -l builtins.nlimg -r test.nlpkg "hello world"

)";

    // todo: add param-controlled memory limit
    std::array<UByte, 4_KB> cache;
    std::pmr::monotonic_buffer_resource buffer{cache.data(), cache.size(), std::pmr::new_delete_resource()};
    std::pmr::synchronized_pool_resource memory{&buffer};

    void prepare(Process&, int, const char*[]);
    void prepare(Process&, int, const wchar_t*[]);
}
