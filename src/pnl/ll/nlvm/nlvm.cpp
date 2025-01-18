//
// Created by FCWY on 24-12-28.
//

#include <Windows.h>
import pnl.ll;
import <fstream>;
import <iostream>;
import <memory_resource>;
import <chrono>;
import <array>;
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

    auto main = FFamilyRepr{{FOverrideRepr{{}, L"::unit", FOverrideRepr::ImplRepr{TFlag<List<Instruction>>, {
        LOAD_CHAR           + 1,    // H
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 2,    // e
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 3,    // l
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 3,    // l
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 4,    // o
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 6,    // ' '
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 5,    // w
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 4,    // o
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 7,    // r
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 3,    // l
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 8,    // d
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        LOAD_CHAR           + 9,    // d
        LOAD_REF            + 0,
        INVOKE_FIRST        ++,
        RETURN              ++
    }, &mem_base}}}, &mem_base};

    auto package = Package{
        {{
            Package::Content{TFlag<ObjRefRepr>, L"io::write"},
            Package::Content{TFlag<Char>, L'H'},
            Package::Content{TFlag<Char>, L'e'},
            Package::Content{TFlag<Char>, L'l'},
            Package::Content{TFlag<Char>, L'o'},
            Package::Content{TFlag<Char>, L'w'},
            Package::Content{TFlag<Char>, L' '},
            Package::Content{TFlag<Char>, L'r'},
            Package::Content{TFlag<Char>, L'd'},
            Package::Content{TFlag<Char>, L'!'},
            Package::Content{TFlag<FFamilyRepr>, std::move(main)}
        }, &mem_base},
        {{{10ull, L"main"}}, &mem_base}
    };

    try {
        auto process = Process{
            &mem_base,
            Datapack::from_pkg({{
                Pair<Str, Package>{L"test", std::move(package)}
            }, &mem_base})
        };
    }catch (std::runtime_error& e) {
        std::cerr << e.what() << '\n';
    }
}
