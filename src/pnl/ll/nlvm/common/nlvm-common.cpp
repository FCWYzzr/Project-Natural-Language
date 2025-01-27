//
// Created by FCWY on 25-1-25.
//
module;
#include <project-nl.h>
#include <print>
#include <codecvt>
module nlvm.common;
import pnl.ll;

using namespace std::filesystem;
using namespace pnl::ll;
using namespace nlvm;

static void print_help()  {
    std::println(std::cerr, help);
    exit(0);
}


static void vm_load_lib(
        Process& process,
        const Path& path
    ) {
    if (!exists(path) || !is_regular_file(path)) {
        std::print(std::cerr, "file not exist: {}", path.string());
        print_help();
    }
    process.load_library(VMLib{path});
}

static void vm_load_libs(
    Process& process,
    const Path& path
) {
    if (!exists(path) || !is_directory(path)) {
        std::print(std::cerr, "dir not exist: {}", path.string());
        print_help();
    }
    for (auto entry: recursive_directory_iterator{path})
        if (entry.is_regular_file()
            && entry.path().extension() == DLL_EXT)
            process.load_library(VMLib{entry.path()});
}

static void vm_load_img(
    Process& process,
    const Path& path,
    Str* const err = nullptr
) {
    if (!exists(path) || !is_regular_file(path)) {
        std::print(std::cerr, "file not exist: {}", path.string());
        print_help();
    }
    auto patch = Patch{&process.process_memory};
    auto ifs = BIFStream{path, std::ios::binary | std::ios::in};
    Serializer<Patch>{patch}.deserialize(ifs);
    process.load_patch(std::move(patch), err);
}

static void vm_load_pkgs(
    Process& process,
    Queue<Path>&& pkg_pathes,
    Str* const err = nullptr
) {
    auto datapack = Datapack{&process.process_memory};
    while(!pkg_pathes.empty()) {
        auto path = std::move(pkg_pathes.front());
        pkg_pathes.pop();
        auto pkg = Package{&process.process_memory};
        auto ifs = BIFStream{path, std::ios::binary | std::ios::in};
        Serializer<Package>{pkg}.deserialize(ifs);
        datapack.emplace(cvt(path.stem().string(), process.process_memory), std::move(pkg));
    }
    auto maybe_patch = process
        .compile_datapack(std::move(datapack), err);

    if (!err->empty())
        return;

    process.load_patch(std::move(maybe_patch.value()), err);
}

struct cvt_t: std::codecvt<char, wchar_t, std::mbstate_t>{};

namespace nlvm::common {
    void prepare(Process& process, int argc, const char *argv[]) {
        memory.upstream_resource()
        Str err{&memory};
        enum class State {
            LOAD_LIB,
            LOAD_LIBS,
            LOAD_IMG,
            LOAD_PKG,
            DUMP_PARAM
        };
        using enum State;
        set_default_resource(&memory);

        auto pkgs = Queue<Path>{&memory};
        auto params = Queue<VirtualAddress>{&memory};


        for (auto state = LOAD_PKG;
             auto i: std::views::iota(1, argc)) {
            if (strcmp(argv[i], "-n") == 0) {
                state = LOAD_LIB;
                continue;
            }
            if (strcmp(argv[i], "-nd") == 0) {
                state = LOAD_LIBS;
                continue;
            }
            if (strcmp(argv[i], "-l") == 0) {
                state = LOAD_IMG;
                continue;
            }
            if (strcmp(argv[i], "-r") == 0) {
                state = LOAD_PKG;
                continue;
            }
            if (strcmp(argv[i], "-p") == 0) {
                state = DUMP_PARAM;
            }

            switch (state) {
                case LOAD_LIB:
                    vm_load_lib(process, argv[i]);
                    break;
                case LOAD_LIBS:
                    vm_load_libs(process, argv[i]);
                    break;
                case LOAD_IMG:
                    vm_load_img(process, argv[i], &err);
                    if (!err.empty()) {
                        std::cerr << cvt(err) << std::endl;
                        print_help();
                    }
                    break;
                case LOAD_PKG:
                    pkgs.emplace(argv[i]);
                    break;
                case DUMP_PARAM: {
                    auto cache = cvt(argv[i], memory);
                    VirtualAddress tp;{
                        if (auto tp_name = typename_arr(&memory, VM_TEXT("core::char"), cache.size());
                            process.named_exports.contains(tp_name))
                            tp = process.named_exports.at(tp_name);
                        else {
                            const auto ch_tp = process.named_exports.at({VM_TEXT("core::char"), &process.process_memory});
                            const auto arr_tp = process.named_exports.at({VM_TEXT("core::Array"), &process.process_memory});
                            const auto arr_maker = process.named_exports.at({VM_TEXT("core::Array::maker"), &process.process_memory});
                            const auto arr_collector = process.named_exports.at({VM_TEXT("core::Array::collector"), &process.process_memory});

                            process.named_exports.emplace(
                                std::move(tp_name),
                                tp = VirtualAddress::from_process_offset(process.process_page.milestone())
                            );
                            process.process_page.emplace_top(
                                TFlag<ArrayType>,
                                Type{RTTObject{arr_tp},
                                    static_cast<Long>(sizeof(RTTObject) + sizeof(Char) * cache.size()),
                                    arr_maker,
                                    arr_collector},
                                ch_tp,
                                static_cast<Long>(cache.size())
                            );
                        }
                    }

                    params.emplace(process.process_page.milestone());
                    process.process_page.placeholder_push(
                        sizeof(RTTObject) + sizeof(Char) * cache.size()
                    );
                    auto repr = &process.process_page.ref_top<UByte>();
                    reinterpret_cast<RTTObject*&>(repr) ++ -> type = tp;
                    for (const auto ch: cache)
                        *reinterpret_cast<Char*&>(repr) ++ = ch;
                }
            }
        }

        if (process.named_exports.empty())
            print_help();


        {
            VirtualAddress addr_arr_t;{
                try {
                    auto addr_arr_name = typename_arr(&process.process_memory, VM_TEXT("core::address"), params.size());
                }catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }

                if (auto addr_arr_name = typename_arr(&process.process_memory, VM_TEXT("core::address"), params.size());
                    process.named_exports.contains(addr_arr_name))
                    addr_arr_t = process.named_exports.at(addr_arr_name);
                else {
                    const auto addr_tp = process.named_exports.at({VM_TEXT("core::address"), &process.process_memory});
                    const auto arr_tp = process.named_exports.at({VM_TEXT("core::Array"), &process.process_memory});
                    const auto arr_maker = process.named_exports.at({VM_TEXT("core::Array::maker"), &process.process_memory});
                    const auto arr_collector = process.named_exports.at({VM_TEXT("core::Array::collector"), &process.process_memory});

                    process.named_exports.emplace(
                        std::move(addr_arr_name),
                        addr_arr_t = VirtualAddress::from_process_offset(process.process_page.milestone())
                    );
                    process.process_page.emplace_top(
                        TFlag<ArrayType>,
                        Type{RTTObject{arr_tp},
                            static_cast<Long>(sizeof(RTTObject) + sizeof(VirtualAddress) * params.size()),
                            arr_maker,
                            arr_collector},
                        addr_tp,
                        static_cast<Long>(cache.size())
                    );

                }
            }

            process.main_thread().eval_deque.emplace_back(VirtualAddress::from_process_offset(process.process_page.milestone()));
            process.process_page.placeholder_push(
                sizeof(RTTObject) + sizeof(VirtualAddress) * cache.size()
            );
            auto repr = &process.process_page.ref_top<UByte>();
            reinterpret_cast<RTTObject*&>(repr) ++ -> type = addr_arr_t;
            while (!params.empty()) {
                *reinterpret_cast<VirtualAddress*&>(repr) ++ = params.front();
                params.pop();
            }
        }

        vm_load_pkgs(process, std::move(pkgs), &err);

        if (!err.empty()) {
            std::cerr << cvt(err) << std::endl;
            std::println("{}", help);
            exit(0);
        }


        assert(
            process.named_exports.contains({VM_TEXT("main"), &process.process_memory}),
            "main proc not found"
        );

        auto main = process.named_exports.at({VM_TEXT("main"), &process.process_memory});
        process.main_thread().call_function(
            process.deref<FFamily>(main),
            0
        );

        if constexpr (SAFE_LEVEL == 1)
            // force std::pmr use same mman as vm
            set_default_resource(&memory);
        else if constexpr (SAFE_LEVEL == 0)
            // disable default std::pmr allocation
            set_default_resource(std::pmr::null_memory_resource());
        else
            // allow std::pmr to use new/delete as a fallback
            set_default_resource(std::pmr::new_delete_resource());
    }
    void prepare(Process& process, int argc, const wchar_t *w_argv[]) {
        auto argv = List<const char*>{&process.process_memory};
        // ReSharper disable once CppTooWideScope
        // keep arg str alive
        auto cache = Deque<MBStr>{&process.process_memory};
        {
            argv.resize(argc);
            auto mb = std::mbstate_t{};
            auto cvt = cvt_t{};
            for (const auto i: std::views::iota(0, argc)) {
                auto internal = MBStr{&process.process_memory};
                const wchar_t* from_next;
                char* to_next;
                const auto len = wcslen(w_argv[i]);
                internal.resize_and_overwrite(len + 1, [&](char* buf, USize buf_size) {
                    cvt.in(mb,
                        w_argv[i],
                        w_argv[i] + len,
                        from_next,
                        buf,
                        buf + len,
                        to_next);
                    return to_next - buf;
                });
                cache.emplace_back(std::move(internal));
                argv[i] = cache.back().c_str();
            }
        }
        return prepare(process, argc, argv.data());
    }
}
