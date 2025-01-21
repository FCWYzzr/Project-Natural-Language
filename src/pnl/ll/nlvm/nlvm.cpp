//
// Created by FCWY on 24-12-28.
//
import pnl.ll;
#include <filesystem>
#include <iostream>
#include <memory_resource>
#include <array>
using namespace pnl::ll;
using namespace std::chrono_literals;



int main(const int argc, char* argv[]){
    using enum OPCode;
    auto cache = std::array<std::byte, 4096>{};
    auto mem_base = std::pmr::monotonic_buffer_resource(
        cache.data(),
        cache.size(),
        std::pmr::new_delete_resource()
    );

    auto main = FFamilyRepr{{FOverrideRepr{{}, U"::unit", FOverrideRepr::ImplRepr{TFlag<List<Instruction>>, {
        P_REF_AT              + 1,    // "Hello World!"
        P_LOAD_REF          + 0,
        INVOKE_FIRST        ++,

        RETURN              ++
    }, &mem_base}}}, &mem_base};

    auto package = Package{
        {{
            Package::Content{TFlag<ObjRefRepr>, U"io::println"},
            Package::Content{TFlag<CharArrayRepr>, U"Hello World!"},
            Package::Content{TFlag<FFamilyRepr>, std::move(main)}
        }, &mem_base},
        {{{2ull, U"main"}}, &mem_base}
    };

    try {
        auto process = Process{
            &mem_base,
            Datapack::from_pkg({{
                Pair<Str, Package>{U"test", std::move(package)}
            }, &mem_base}),
            std::filesystem::current_path() /= "rt"
        };
    }catch (std::runtime_error& e) {
        std::cerr << e.what() << '\n';
    }
}
