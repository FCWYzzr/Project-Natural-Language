//
// Created by FCWY on 25-1-17.
//
// ReSharper disable CppTemplateArgumentsCanBeDeduced
module;
#include "project-nl.h"
module pnl.ll.runtime;
// ReSharper disable CppDFANullDereference



using namespace pnl::ll::runtime;
using namespace pnl::ll::runtime::load;
using namespace pnl::ll::runtime::execute;



VMLib::VMLib(
    const Path & path) noexcept:
    path{path},
    lib_ref(dlopen(absolute(path).string().c_str(), RTLD_LAZY)){
    if (!lib_ref)
        assert(false, dlerror());
}



VMLib::~VMLib() noexcept {
    if (lib_ref != nullptr)
        dlclose(lib_ref);
}

Datapack::Datapack(
    MManager* mem) noexcept:
    extern_libs(mem),
    package_pack(mem) {
}

Datapack::Datapack(Datapack &&other) noexcept: extern_libs(std::move(other.extern_libs)),
                                               package_pack(std::move(other.package_pack)) {
}

Datapack & Datapack::operator+=(Datapack &&pack) noexcept {
    for (auto& elem: pack.extern_libs)
        extern_libs.emplace(std::move(elem));
    extern_libs.clear();

    for (auto& [key, val]: pack.package_pack) {
        assert(!package_pack.contains(key), VM_TEXT("duplicate library loading"));
        package_pack.emplace(key, std::move(val));
    }

    return *this;
}



static bool
merge_datapack(
    MManager& process_memory,
    Dict<VirtualAddress>& named_exports,
    Datapack datapack,
    Str * const err,
    Set<Path>& libraries,
    List<Package::Content>& merged_values,
    BiMap<Str, USize>& exports,
    Queue<USize>& base_addr_queue) noexcept {

    auto package_pack = Dict<Package>{&process_memory};
    package_pack = std::move(datapack.package_pack);

    libraries = std::move(datapack.extern_libs);




    merged_values.reserve(std::ranges::fold_left(
        package_pack
        | std::views::values
        | std::views::transform([](const Package& pkg) {
            return pkg.data.size();
        }),
        0ull, std::plus<USize>{}
    ));


    // preload - confirm f-families' base offset
    for (auto& [name, pkg]: package_pack) {
        const auto name_prefix = name + VM_TEXT("::");
        const auto base_offset = merged_values.size();
        for (const auto& [idx, obj]: pkg.data
                | std::views::enumerate) {

            if (obj.index() == 9)
                base_addr_queue.push(base_offset);

            if (pkg.exports.contains(idx)) [[unlikely]]{
                auto symbol = Str{&process_memory};
                if (obj.index() == 9 && pkg.exports.at(idx) == VM_TEXT("main")) [[unlikely]]{
                    symbol = VM_TEXT("main");
                    if (const auto& main = std::get<9>(obj);
                        main.size() != 1) [[unlikely]]{
                        auto info = build_str(&process_memory,
                            VM_TEXT("main function may only have 1 override, yet provided "),
                            to_string(&process_memory, main.size())
                        );
                        assert(err != nullptr, info);
                        *err = info;
                        return true;
                    }
                    // todo main func sig check
                }
                else [[likely]]{
                    symbol = name_prefix + pkg.exports.at(idx);
                    if (obj.index() == 7) [[unlikely]]
                        std::get<7>(obj).name = symbol;
                    else if (obj.index() == 10) [[unlikely]]
                        std::get<10>(obj).name = symbol;
                }
                if (named_exports.contains(symbol) || exports.contains(symbol))[[unlikely]]{
                    auto info = build_str(&process_memory,
                        VM_TEXT("export name dup: "),
                        symbol
                        );
                    if (symbol == VM_TEXT("main"))
                        info += VM_TEXT("(note that  name 'main' does not belong to ANY named module)");

                    assert(err != nullptr, info);
                    *err = std::move(info);
                    return true;
                }
                exports.emplace(std::move(symbol), base_offset + idx);
            }
            merged_values.emplace_back(std::move(obj));

        }
    }

    return false;
}

std::optional<Patch> Process::compile_datapack(
    Datapack datapack,
    Str *const err
) noexcept {
    // package level addr offset
    auto base_addr_queue = Queue<USize>{&process_memory};
    auto merged_values = List<Package::Content>{&process_memory};
    auto exports = BiMap<Str, USize>{&process_memory};
    auto libraries = Set<Path>{&process_memory};

    if (merge_datapack(
        process_memory,
        named_exports,
        std::move(datapack),
        err,
        libraries,
        merged_values,
        exports,
        base_addr_queue
        ))
        return {};


    // cache preloaded values
    auto preloaded = List<LTValue>(merged_values.size(), &process_memory);

    // loaded primitives
    for (const auto& [idx, obj]: merged_values
        | std::views::enumerate
        | std::views::filter([](const auto& o) {
            return std::get<1>(o).index() <= 6;
        }))
        std::visit([&]<typename T>(const T& v) -> void {
            if constexpr ( !TypePack<NamedTypeRepr,
                ObjRefRepr,
                ClassRepr,
                FFamilyRepr,
                ObjectRepr,
                CharArrayRepr>::contains<T>) {
                preloaded[idx] = v;
            }
            else
                std::unreachable();
        }, obj);


    auto preloaded_strings = Queue<Str>{&process_memory};
    auto named_type_type = named_exports.contains(VM_TEXT("::NamedType"))
        ? named_exports.at(VM_TEXT("::NamedType"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::NamedType")));
    // load named type
    for (const auto [idx, obj]: merged_values
        | std::views::enumerate
        | std::views::filter([](const auto& o) {
            return std::get<1>(o).index() == 7;
        })) {
        if (!exports.contains(std::get<7>(obj).maker)) [[unlikely]] {
            auto info = build_str(
                &process_memory,
                VM_TEXT("could not found maker of obj#"),
                to_string(&process_memory, idx),
                VM_TEXT(" named "),
                std::get<7>(obj).maker
            );
            assert(err != nullptr, info);
            *err = std::move(info);
            return {};
        }
        if (!exports.contains(std::get<7>(obj).collector)) [[unlikely]] {
            auto info = build_str(
                &process_memory,
                VM_TEXT("could not found collector of obj#"),
                to_string(&process_memory, idx),
                VM_TEXT(" named "),
                std::get<7>(obj).collector
            );
            assert(err != nullptr, info);
            *err = std::move(info);
            return {};
        }
        preloaded[idx] = NamedType{
            Type{{named_type_type},
                std::get<7>(obj).size,
            VirtualAddress::from_reserve_offset(
                exports.at(std::get<7>(obj).maker)
            ),
            VirtualAddress::from_reserve_offset(
                exports.at(std::get<7>(obj).collector)
            )},
            VirtualAddress::from_reserve_offset(
                preloaded_strings.size()
            )
        };
        preloaded_strings.emplace(std::move(std::get<7>(obj).name));
    }

    // interpret obj ref -> addr
    for (const auto [idx, obj]: merged_values
        | std::views::enumerate
        | std::views::filter([](const auto& o) {
            return std::get<1>(o).index() == 8;
        })) {
        if (const auto& ref_repr = std::get<8>(obj);
            exports.contains(ref_repr)) [[likely]]{
            preloaded[idx] = VirtualAddress::from_reserve_offset(exports.at(ref_repr));
        }
        else if (named_exports.contains(ref_repr)) [[likely]]
            preloaded[idx] = named_exports.at(ref_repr);
        else [[unlikely]]{
            auto info = build_str(
                &process_memory,
                VM_TEXT("unable to load refer constant "),
                ref_repr
            );
            assert(err != nullptr, info);
            *err = std::move(info);
            return std::nullopt;
        }
    }

    const auto override_type_type = named_exports.contains(VM_TEXT("::FOverride"))
        ? named_exports.at(VM_TEXT("::FOverride"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::FOverride")));
    const auto override_inst_cls_maker = named_exports.contains(VM_TEXT("::FOverride::maker"))
        ? named_exports.at(VM_TEXT("::FOverride::maker"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::FOverride::maker")));
    const auto override_inst_cls_collector = named_exports.contains(VM_TEXT("::FOverride::collector"))
        ? named_exports.at(VM_TEXT("::FOverride::collector"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::FOverride::collector")));


    auto arg_types = Queue<Queue<VirtualAddress>>{&process_memory};
    auto preloaded_override_type = Queue<OverrideType>{&process_memory};
    auto ntv_funcs = Queue<NtvId>{&process_memory};
    auto instructions = Queue<Queue<Instruction>>{&process_memory};
    auto preloaded_overrides = Queue<Queue<FOverride>>{&process_memory};
    // compile functions
    for (auto family_type = named_exports.contains(VM_TEXT("::FFamily"))
        ? named_exports.at(VM_TEXT("::FFamily"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::FFamily")));
        const auto [idx, obj]: merged_values
            | std::views::enumerate
            | std::views::filter([](const auto& o) {
                return std::get<1>(o).index() == 9;
            })) {
        auto& family = std::get<9>(obj);
        auto overrides = Queue<FOverride>{&process_memory};

        for (auto& o_repr: family) {
            auto args = Queue<VirtualAddress>{&process_memory};
            for (auto& t: o_repr.arg_ts) {
                if (exports.contains(t)) [[unlikely]]
                    args.emplace(VirtualAddress::from_reserve_offset(exports.at(t)));
                else if (named_exports.contains(t)) [[likely]]
                    args.emplace(named_exports.at(t));
                else {
                    auto info = build_str(
                        &process_memory,
                        VM_TEXT("func #"), to_string(&process_memory, idx),
                        VM_TEXT(" fail to load: unable to find type "),
                        t
                    );
                    assert(err != nullptr, info);
                    *err = std::move(info);
                    return std::nullopt;
                }
            }
            VirtualAddress ret;{
                if (const auto& t = o_repr.ret_t; exports.contains(t)) [[unlikely]]
                    ret = VirtualAddress::from_reserve_offset(exports.at(t));
                else if (named_exports.contains(t)) [[likely]]
                    ret = named_exports.at(t);
                else {
                    auto info = build_str(
                        &process_memory,
                        VM_TEXT("func #"), to_string(&process_memory, idx),
                        VM_TEXT(" fail to load: unable to find type "),
                        t
                    );
                    assert(err != nullptr, info);
                    *err = std::move(info);
                    return std::nullopt;
                }
            }

            VirtualAddress func_tp;
            if (auto name = typename_fun(&process_memory, o_repr.arg_ts, o_repr.ret_t);
                named_exports.contains(name)) [[unlikely]]
                func_tp = named_exports.at(name);
            else if (auto func_mapping = Dict<VirtualAddress>(&process_memory);
                func_mapping.contains(name)) [[likely]]
                func_tp = func_mapping.at(name);
            else {
                func_tp = VirtualAddress::from_reserve_offset(preloaded_override_type.size());

                preloaded_override_type.emplace(
                    Type{{override_type_type},
                    sizeof(FOverride),
                    override_inst_cls_maker, override_inst_cls_collector},
                    VirtualAddress::from_reserve_offset(arg_types.size()),
                    ret
                );
                arg_types.emplace(std::move(args));
                func_mapping.emplace(
                    std::move(name), func_tp
                );
            }

            auto impl = std::visit([&]<typename T>(T& val) -> std::optional<std::uintptr_t> {
                if constexpr (std::same_as<T, NtvId>) {
                    ntv_funcs.emplace(val);
                    return {0ull};
                }
                else {
                    // ReSharper disable once CppDFAUnreadVariable
                    // ReSharper disable once CppDFAUnusedValue
                    const auto addr = VirtualAddress::from_reserve_offset(instructions.size());
                    auto insts = Queue<Instruction>{&process_memory};
                    insts.push_range(std::move(val));
                    instructions.emplace(std::move(insts));
                    return {static_cast<std::uintptr_t>(addr)};
                }
            }, o_repr.impl);

            if (!impl.has_value())
                return {};

            overrides.emplace(RTTObject{func_tp}, o_repr.impl.index() == 1, impl.value());
        }

        preloaded[idx] = FFamily{
            {family_type},
            VirtualAddress::from_reserve_offset(base_addr_queue.front()),
            VirtualAddress::from_reserve_offset(preloaded_overrides.size())
        };
        preloaded_overrides.emplace(std::move(overrides));
        base_addr_queue.pop();
    }


    // scan class dependency
    // cls index -> {dependency count,  {in count, list<referer index>}}
    auto dependency_graph = Map<USize, Pair<std::uint32_t, Set<USize>>>{&process_memory};
    for (auto [idx, cls]: merged_values
            | std::views::enumerate
            | std::views::filter([](const auto& o) {
                return std::get<1>(o).index() == 10;
            })
            | std::views::transform([](const auto& o) {
                return std::pair<USize, ClassRepr&>{std::get<0>(o), std::get<10>(std::get<1>(o))};
            })) {
        if (!dependency_graph.contains(idx))
            dependency_graph.emplace(Pair{idx, Pair{0, Set<USize>{}}});

        for (const auto lst: {&cls.method_list, &cls.static_method_list, &cls.static_member_list})
            for (auto& dep: *lst) {
                if (!exports.contains(dep)) [[unlikely]] {
                    auto info = build_str(
                        &process_memory,
                        VM_TEXT("class #"), to_string(&process_memory, static_cast<Long>(idx)),
                        VM_TEXT(" fail to load: unable to find dependency function "),
                        dep
                    );
                    assert(err != nullptr, info);
                    *err = std::move(info);
                    return std::nullopt;
                }
            }

        for (auto& [name, dep]: cls.member_list) {
            if (named_exports.contains(dep)) [[unlikely]]
                // skip if compiled
                continue;
            if (!exports.contains(dep)) [[unlikely]] {
                if (name.empty())
                    continue;
                auto info = build_str(
                    &process_memory,
                    VM_TEXT("obj #"), to_string(&process_memory, static_cast<Long>(idx)),
                    VM_TEXT(" fail to load: unable to find class "),
                    dep
                );
                assert(err != nullptr, info);
                *err = std::move(info);
                return std::nullopt;
            }
            if (merged_values.at(exports.at(dep)).index() != 10) [[likely]]
                // is named type
                continue;
            const auto dep_idx = exports.at(dep);

            if (!dependency_graph.contains(dep_idx)) [[unlikely]]
                dependency_graph.emplace(dep_idx, Pair{0, Set<USize>{{idx}, &process_memory}});
            else
                dependency_graph.at(dep_idx).second.emplace(idx);
            ++ dependency_graph.at(idx).first;

        }
    }

    // handle class dependency
    auto load_queue = Queue<USize>{&process_memory};
    // -- load independent class (dep count = 0)
    load_queue.push_range(
        dependency_graph
        | std::views::filter([](const auto& item) {
            return item.second.first == 0;
        })
        | std::views::keys
    );

    const auto cls_type_addr = named_exports.contains(VM_TEXT("::Class"))
        ? named_exports.at(VM_TEXT("::Class"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::Class")));

    auto preloaded_member_segments = Queue<Queue<MemberInfo>>{&process_memory};
    auto preloaded_cls_ext_segments = Queue<Queue<VirtualAddress>>{&process_memory};

    // topo sort
    while (!load_queue.empty()) {
        auto cls_id = load_queue.front();
        load_queue.pop();

        // load cls

        auto& cls = std::get<ClassRepr>(merged_values.at(cls_id));

        auto static_members = Queue<VirtualAddress>{&process_memory};
        auto static_methods = Queue<VirtualAddress>{&process_memory};
        auto members = Queue<MemberInfo>{&process_memory};
        auto methods = Queue<VirtualAddress>{&process_memory};

        auto member_size = Long{0};
        for (auto& [mbr_name, mbr_cls_id]: cls.member_list) {
            if (mbr_name.empty()) [[unlikely]] {
                // find a padding
                member_size += static_cast<Long>(mbr_cls_id.size());
            }
            else if (exports.contains(mbr_cls_id)) [[likely]] {
                const auto member_cls_ref = VirtualAddress::from_reserve_offset(exports.at(mbr_cls_id));
                const auto& member_cls = preloaded.at(exports.at(mbr_cls_id));
                const auto& mem_tp = (member_cls.index() == 7)
                    ? std::get<7>(member_cls)
                    : std::get<10>(member_cls).super;

                members.emplace(
                    VirtualAddress::from_reserve_offset(
                        preloaded_strings.size()
                    ), member_cls_ref, member_size);
                preloaded_strings.emplace(std::move(mbr_name));
                member_size += mem_tp.super.instance_size;
            }
            else
                std::unreachable();
        }

        for (auto &smbr_id: cls.static_member_list) {
            static_members.emplace(VirtualAddress::from_reserve_offset(exports.at(smbr_id)));
        }

        for (auto& method_id: cls.method_list) {
            methods.emplace(VirtualAddress::from_reserve_offset(exports.at(method_id)));
        }

        for (auto& method_id: cls.static_method_list) {
            static_methods.emplace(VirtualAddress::from_reserve_offset(exports.at(method_id)));
        }

        preloaded[cls_id] = Class{
            {{{cls_type_addr},
                member_size,
                VirtualAddress::from_reserve_offset(exports.at(cls.maker)),
                VirtualAddress::from_reserve_offset(exports.at(cls.collector))},
            VirtualAddress::from_reserve_offset(preloaded_strings.size())},
            VirtualAddress::from_reserve_offset(preloaded_member_segments.size()),
            VirtualAddress::from_reserve_offset(preloaded_cls_ext_segments.size()),
            VirtualAddress::from_reserve_offset(preloaded_cls_ext_segments.size() + 1),
            VirtualAddress::from_reserve_offset(preloaded_cls_ext_segments.size() + 2)
        };
        preloaded_strings.emplace(std::move(cls.name));
        preloaded_member_segments.emplace(std::move(members));
        preloaded_cls_ext_segments.emplace(std::move(methods));
        preloaded_cls_ext_segments.emplace(std::move(static_members));
        preloaded_cls_ext_segments.emplace(std::move(static_methods));

        // end load cls

        // decr refer cls' dep count
        for (auto referer_id: dependency_graph.at(cls_id).second)
            if (-- dependency_graph.at(referer_id).first == 0) [[unlikely]]
                load_queue.emplace(referer_id);

        dependency_graph.erase(cls_id);
    }


    // assume all cls loaded (no recu dep)
    if (!dependency_graph.empty()) [[unlikely]]{
        auto info = build_str(
            &process_memory,
            VM_TEXT("fail to load class(es): #")
        );
        info.append_range(
            dependency_graph
                | std::views::keys
                | std::views::transform([&](const auto& id){return to_string(&process_memory, id);})
                | std::views::join_with(Str{VM_TEXT(", #"), &process_memory})
        );
        info += VM_TEXT(". possibly due to recursive dependency.");
        assert(err != nullptr, info);
        *err = std::move(info);
        return std::nullopt;
    }



    assert(load_queue.empty(), "what? how did you get here????");
    assert(dependency_graph.empty(), "what? how did you get here????");

    // handle obj dependency
    for (const auto& [idx, obj]: merged_values
            | std::views::enumerate
            | std::views::filter([](const auto& o) {
                return std::get<1>(o).index() == 11;
            })) {
        auto& obj_repr = std::get<11>(obj);
        if (!named_exports.contains(obj_repr.type) && !exports.contains(obj_repr.type)) [[unlikely]]{
            auto info = build_str(
                &process_memory,
                VM_TEXT("fail to load object #"),
                idx,
                VM_TEXT(": unable to find class "),
                obj_repr.type
            );
        }
        if (!dependency_graph.contains(idx)) [[unlikely]]
            dependency_graph.emplace(idx, std::pair{0, Set<USize>{&process_memory}});
        for (auto& v: obj_repr.constructor_params
                | std::views::filter([](const auto& p){return p.index() == 7;})) {
            auto& vv = std::get<7>(v);

            // refers to loaded obj, skip
            if (named_exports.contains(vv)) [[unlikely]]
                continue;

            if (!exports.contains(vv)) [[unlikely]]{
                auto info = build_str(
                    &process_memory,
                    VM_TEXT("fail to load obj: #"),
                    idx,
                    VM_TEXT(": unable to find depend obj: "),
                    vv
                );
                assert(err != nullptr, info);
                *err = std::move(info);
                return std::nullopt;
            }

            // refers to lib obj, add dep
            const auto dep_obj_idx = exports.at(vv);
            if (!dependency_graph.contains(dep_obj_idx)) [[unlikely]]
                dependency_graph.emplace(dep_obj_idx, std::pair{0, Set<USize>{&process_memory}});
            ++ dependency_graph[dep_obj_idx].first;
            dependency_graph[dep_obj_idx].second.emplace(idx);
        }
    }

    // -- load independent obj (dep count = 0)
    load_queue.push_range(
        dependency_graph
        | std::views::filter([](const auto& item) {
            return item.second.first == 0;
        })
        | std::views::keys
    );

    USize init_id = 0;
    auto preloaded_params = Stack<Value>{&process_memory};
    // topo sort
    while (!load_queue.empty()) {
        // ReSharper disable once CppDFAUnreadVariable
        auto obj_id = load_queue.front();
        load_queue.pop();

        // init obj
        // ReSharper disable once CppUseStructuredBinding
        auto& obj = std::get<ObjectRepr>(merged_values.at(obj_id));
        for (auto& v: obj.constructor_params)
            std::visit([&]<typename T>(const T& vv) {
                if constexpr (std::same_as<T, ObjRefRepr>)
                    preloaded_params.emplace(TFlag<VirtualAddress>, VirtualAddress::from_reserve_offset(exports.at<Str>(vv)));
                else
                    preloaded_params.emplace(TFlag<T>, vv);
            }, v);

        preloaded_params.emplace(TFlag<VirtualAddress>,
            VirtualAddress::from_reserve_offset(obj_id)
        );

        VirtualAddress type;
        USize size;

        if (named_exports.contains(obj.type)) [[unlikely]]{
            type = named_exports.at(obj.type);
            size = deref<Type>(type).instance_size;
        }
        else {
            type = VirtualAddress::from_reserve_offset(exports.at(obj.type));
            auto& t = preloaded.at(exports.at(
                obj.type
            ));
            if (t.index() == 8)
                size = std::get<NamedType>(t).super.instance_size;
            else
                size = std::get<Class>(t).super.super.instance_size;
        }

        preloaded[obj_id] = LTObject {
            init_id ++,
            size,
            type,
            obj.constructor_override_id
        };
        // end init obj

        // decr refer obj' dep count
        for (auto referer_id: dependency_graph.at(obj_id).second)
            if (-- dependency_graph.at(referer_id).first == 0) [[unlikely]]
                load_queue.push(referer_id);

        dependency_graph.erase(obj_id);
    }


    // assume all obj loaded (no recu dep)
    if (!dependency_graph.empty()) [[unlikely]]{
        auto info = build_str(
            &process_memory,
            VM_TEXT("fail to load object(s): #")
        );
        info.append_range(
            dependency_graph
                | std::views::keys
                | std::views::transform([&](const auto& id){return to_string(&process_memory, id);})
                | std::views::join_with(Str{VM_TEXT(", #"), &process_memory})
        );
        info += VM_TEXT(". possible due to recursive dependency.");
        assert(err != nullptr, info);
        *err = std::move(info);
        return {};
    }


    auto preloaded_str_length = Queue<USize>{&process_memory};
    for (const auto [idx, obj]: merged_values
        | std::views::enumerate
        | std::views::filter([](const auto& o) {
            return std::get<1>(o).index() == 12;
        })) {
        preloaded_str_length.emplace(std::get<12>(obj).length());
        preloaded[idx] = std::move(std::get<12>(obj));
    }


    auto preloaded_q = Queue<LTValue>{&process_memory};
    for (auto& v: preloaded)
        preloaded_q.emplace(std::move(v));

    return {Patch{
        std::move(libraries),
        std::move(preloaded_strings),
        std::move(preloaded_override_type),
        std::move(arg_types),
        std::move(instructions),
        std::move(ntv_funcs),
        std::move(preloaded_overrides),
        std::move(preloaded_member_segments),
        std::move(preloaded_cls_ext_segments),
        std::move(preloaded_q),
        std::move(exports),
        std::move(preloaded_params),
        std::move(preloaded_str_length)
    }};
}

static
void call_init_function(Thread& thr, const FFamily &family, const std::uint32_t override_id) noexcept {
    const auto& override = thr.deref<Array<FOverride>>(family.overrides)[override_id];
    thr.init_queue.emplace(
        TFlag<FuncContext>,
        thr.thread_page.milestone(),
        family.base_addr,
        override.vm(),
        *thr.process
    );
}


void Process::load_patch(
    Patch&& patch,
    Str* const err
    ) noexcept {
    auto& thread = main_thread();


    auto [
        extern_libs,
        preloaded_strings,
        preloaded_override_type,
        preloaded_arg_types,
        preloaded_instructions,
        preloaded_ntv_ids, // todo fix
        preloaded_overrides,
        preloaded_member_segments,
        preloaded_cls_ext_segments,
        preloaded,
        exports,
        params,
        preloaded_str_length
    ] = std::move(patch);

    const auto arr_type_t_addr = named_exports.contains(VM_TEXT("::ArrayType"))
        ? named_exports.at(VM_TEXT("::ArrayType"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::ArrayType")));
    const auto arr_constructor_addr = named_exports.contains(VM_TEXT("::ArrayType::maker"))
        ? named_exports.at(VM_TEXT("::ArrayType::maker"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::ArrayType::maker")));
    const auto arr_destructor_addr = named_exports.contains(VM_TEXT("::ArrayType::collector"))
        ? named_exports.at(VM_TEXT("::ArrayType::destructor"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::ArrayType::collector")));


    auto translate_required = Queue<VirtualAddress>{&process_memory};

    // phase 0 find proc idx
    auto ntv_procs = Queue<NtvFunc>{&process_memory};
    while (!preloaded_ntv_ids.empty()) {
        auto& name = preloaded_ntv_ids.front();

        for (auto& lib: libraries)
            if (auto proc_ptr = lib.find_proc(name);
                proc_ptr != nullptr) [[unlikely]] {
                    ntv_procs.emplace(proc_ptr);
                    goto noerr;
                }

        {
            auto info = build_str(
                &process_memory,
                VM_TEXT("unable to find native func in any loaded lib: "),
                cvt(name)
            );
            assert(err != nullptr, info);
            *err = std::move(info);
            return;
        }

        noerr:
        preloaded_ntv_ids.pop();
    }


    // phase - 1 load strings
    VirtualAddress string_base_addr;{

        // phase - 1.1      load str arr classes
        const auto char_addr = named_exports.contains(VM_TEXT("::char"))
            ? named_exports.at(VM_TEXT("::char"))
            : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::char")));
        auto type_queue = Queue<VirtualAddress>{&process_memory};

        for (auto [[maybe_unused]]_: std::views::iota(0u, preloaded_strings.size())) {
            auto str = std::move(preloaded_strings.front());
            preloaded_strings.pop();
            if (auto name = typename_arr(&process_memory, VM_TEXT("::char"), str.size());
                !named_exports.contains(name)) {
                named_exports.emplace(std::move(name), type_queue.emplace(VirtualAddress::from_process_offset(
                    process_page.milestone()
                )));
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, super.super.type)
                );
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, elem_type)
                );
                process_page.emplace_top(
                    TFlag<ArrayType>,
                    Type{{arr_type_t_addr},
                    static_cast<Long>(sizeof(RTTObject) + sizeof(Char) * str.size()),
                    arr_constructor_addr, arr_destructor_addr},
                    char_addr,
                    static_cast<Long>(str.size())
                );
            }
            else {
                type_queue.emplace(named_exports.at(name));
            }
            preloaded_strings.emplace(std::move(str));
        }

        string_base_addr = VirtualAddress::from_process_offset(
            process_page.milestone()
        );

        // phase - 1.2      load str (as arr)
        while (!preloaded_strings.empty()) {
            const auto str = std::move(preloaded_strings.front());
            const auto type_addr = type_queue.front();
            const auto& type = deref<ArrayType>(type_addr);
            preloaded_strings.pop();
            type_queue.pop();

            process_page.placeholder_push(
                type.super.instance_size
            );
            auto repr = &process_page.ref_top<UByte>();
            *reinterpret_cast<RTTObject*&>(repr) ++ = {type_addr};
            std::ranges::copy(str, reinterpret_cast<Char*>(repr));
        }
    }

    // phase - 2 load overrides
    VirtualAddress override_base_addr;{
        // phase - 2.1 load instruction lists
        VirtualAddress instruction_base_addr;{
            // phase - 2.1.1      load inst arr classes
            const auto inst_addr = named_exports.contains(VM_TEXT("::instruction"))
                ? named_exports.at(VM_TEXT("::instruction"))
                : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::instruction")));
            auto type_queue = Queue<VirtualAddress>{&process_memory};

            for (auto [[maybe_unused]]_: std::views::iota(0u, preloaded_instructions.size())) {
                auto inst = std::move(preloaded_instructions.front());
                preloaded_instructions.pop();
                if (auto name = typename_arr(&process_memory, VM_TEXT("::Instruction"), inst.size());
                    !named_exports.contains(name)) {
                    named_exports.emplace(std::move(name), type_queue.emplace(VirtualAddress::from_process_offset(
                        process_page.milestone()
                    )));
                    translate_required.emplace(
                        VirtualAddress::process_page_id,
                        process_page.milestone(),
                        u16offsetof(ArrayType, super.super.type)
                    );
                    translate_required.emplace(
                        VirtualAddress::process_page_id,
                        process_page.milestone(),
                        u16offsetof(ArrayType, elem_type)
                    );
                    process_page.emplace_top(
                        TFlag<ArrayType>,
                        Type{{arr_type_t_addr},
                        static_cast<Long>(sizeof(RTTObject) + sizeof(Instruction) * inst.size()),
                        arr_constructor_addr, arr_destructor_addr},
                        inst_addr,
                        static_cast<Long>(inst.size())
                    );
                    }
                else {
                    type_queue.emplace(named_exports.at(name));
                }
                preloaded_instructions.emplace(std::move(inst));
            }

            instruction_base_addr = VirtualAddress::from_process_offset(
                process_page.milestone()
            );

            // phase - 2.1.2      load inst (as arr)
            while (!preloaded_instructions.empty()) {
                auto inst = std::move(preloaded_instructions.front());
                const auto type_addr = type_queue.front();
                const auto& type = deref<ArrayType>(type_addr);
                preloaded_instructions.pop();
                type_queue.pop();

                process_page.placeholder_push(
                    type.super.instance_size
                );
                auto repr = &process_page.ref_top<UByte>();
                *reinterpret_cast<RTTObject*&>(repr) ++ = {type_addr};
                while (!inst.empty()) {
                    *reinterpret_cast<Instruction*&>(repr) ++ = inst.front();
                    inst.pop();
                }
            }
        }

        // phase - 2.2 load override type
        VirtualAddress override_type_base_addr;{
            // phase - 2.2.1 load args
            VirtualAddress args_base_addr;{
                const auto addr_addr = named_exports.contains(VM_TEXT("::address"))
                    ? named_exports.at(VM_TEXT("::address"))
                    : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::address")));
                auto type_queue = Queue<VirtualAddress>(&process_memory);
                for (auto [[maybe_unused]]_: std::views::iota(0u, preloaded_arg_types.size())) {
                    auto arg_ts = std::move(preloaded_arg_types.front());
                    preloaded_arg_types.pop();
                    if (auto name = typename_arr(&process_memory, VM_TEXT("address"), arg_ts.size());
                        !named_exports.contains(name)) {
                        named_exports.emplace(std::move(name), type_queue.emplace(VirtualAddress::from_process_offset(
                            process_page.milestone()
                        )));
                        translate_required.emplace(
                            VirtualAddress::process_page_id,
                            process_page.milestone(),
                            u16offsetof(ArrayType, super.super.type)
                        );
                        translate_required.emplace(
                            VirtualAddress::process_page_id,
                            process_page.milestone(),
                            u16offsetof(ArrayType, elem_type)
                        );
                        process_page.emplace_top(
                            TFlag<ArrayType>,
                            Type{{arr_type_t_addr},
                                static_cast<Long>(sizeof(RTTObject) + sizeof(VirtualAddress) * arg_ts.size()),
                            arr_constructor_addr, arr_destructor_addr},
                            addr_addr,
                            static_cast<Long>(arg_ts.size())
                        );
                    }
                    else {
                        type_queue.emplace(named_exports.at(name));
                    }
                    preloaded_arg_types.emplace(std::move(arg_ts));
                }

                args_base_addr = VirtualAddress::from_process_offset(
                    process_page.milestone()
                );

                while (!preloaded_arg_types.empty()) {
                    auto arg_ts = std::move(preloaded_arg_types.front());
                    auto arr_type_addr = type_queue.front();
                    auto& type = deref<ArrayType>(arr_type_addr);
                    preloaded_arg_types.pop();
                    type_queue.pop();

                    const auto milestone = process_page.milestone();
                    process_page.placeholder_push(type.super.instance_size);
                    auto repr = &process_page.ref_top<UByte>();

                    *reinterpret_cast<RTTObject*&>(repr) ++ = {arr_type_addr};
                    auto offset = sizeof(RTTObject);
                    while (!arg_ts.empty()) {
                        translate_required.emplace(
                            VirtualAddress::process_page_id,
                            milestone,
                            static_cast<std::uint16_t>(offset)
                        );
                        *reinterpret_cast<VirtualAddress*&>(repr) ++ = arg_ts.front();
                        arg_ts.pop();
                        offset += sizeof(VirtualAddress);
                    }
                }
            }

            override_type_base_addr = VirtualAddress::from_process_offset(
                process_page.milestone()
            );
            const auto override_type_type_addr = named_exports.contains(VM_TEXT("::FOverride"))
                ? named_exports.at(VM_TEXT("::FOverride"))
                : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::FOverride")));
            // phase - 2.2.2 load override type
            while (!preloaded_override_type.empty()) {
                auto& override_type = preloaded_override_type.front();

                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(OverrideType, super.super.type)
                );
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(OverrideType, return_type)
                );
                process_page.emplace_top(TFlag<OverrideType>,
                    Type{{override_type_type_addr},
                        static_cast<Long>(sizeof(FOverride)),
                        null_addr, null_addr},
                    args_base_addr.id_shift(override_type.param_type_array_addr.id()),
                    override_type.return_type
                );

                preloaded_override_type.pop();
            }
        }

        // phase - 2.3 load override arr classes
        const auto override_addr = named_exports.contains(VM_TEXT("::FOverride"))
            ? named_exports.at(VM_TEXT("::FOverride"))
            : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::FOverride")));
        auto type_queue = Queue<VirtualAddress>{&process_memory};

        for (auto [[maybe_unused]]_: std::views::iota(0u, preloaded_overrides.size())) {
            auto override = std::move(preloaded_overrides.front());
            preloaded_overrides.pop();
            if (auto name = typename_arr(&process_memory, VM_TEXT("::FOverride"), override.size());
                !named_exports.contains(name)) {
                named_exports.emplace(std::move(name), type_queue.emplace(VirtualAddress::from_process_offset(
                    process_page.milestone()
                )));
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, super.super.type)
                );
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, elem_type)
                );
                process_page.emplace_top(
                    TFlag<ArrayType>,
                    Type{{arr_type_t_addr},
                        static_cast<Long>(sizeof(RTTObject) + sizeof(FOverride) * override.size()),
                        arr_constructor_addr, arr_destructor_addr},
                    override_addr,
                    static_cast<Long>(override.size())
                );
            }
            else {
                type_queue.emplace(named_exports.at(name));
            }
            preloaded_overrides.emplace(std::move(override));
        }


        override_base_addr = VirtualAddress::from_process_offset(
            process_page.milestone()
        );

        // phase - 2.3      load overrides (as arr)
        while (!preloaded_overrides.empty()) {
            auto override = std::move(preloaded_overrides.front());
            const auto type_addr = type_queue.front();
            const auto& type = deref<ArrayType>(type_addr);
            preloaded_overrides.pop();
            type_queue.pop();

            process_page.placeholder_push(
                type.super.instance_size
            );
            auto repr = &process_page.ref_top<UByte>();
            *reinterpret_cast<RTTObject*&>(repr) ++ = {type_addr};
            while (!override.empty()) {
                auto& over = *reinterpret_cast<FOverride*&>(repr) ++;
                over = override.front();
                override.pop();
                over.super.type = override_type_base_addr.id_shift(over.super.type.id());
                if (!over.is_native)
                    over.target_addr = static_cast<std::uint64_t>(instruction_base_addr.id_shift(static_cast<VirtualAddress>(over.target_addr).id()));
                else {
                    over.target_addr = reinterpret_cast<std::uintptr_t>(ntv_procs.front());
                    ntv_procs.pop();
                }
            }
        }
    }

    // phase - 3 load class member info list
    VirtualAddress class_member_addr;{

        // phase - 3.1 load member info arr classes
        const auto member_info_type_addr = named_exports.contains(VM_TEXT("::MemberInfo"))
            ? named_exports.at(VM_TEXT("::MemberInfo"))
            : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::MemberInfo")));
        auto type_queue = Queue<VirtualAddress>{&process_memory};

        for (auto [[maybe_unused]]_: std::views::iota(0u, preloaded_member_segments.size())) {
            auto member = std::move(preloaded_member_segments.front());
            preloaded_member_segments.pop();
            if (auto name = typename_arr(&process_memory, VM_TEXT("::MemberInfo"), member.size());
                !named_exports.contains(name)) {
                named_exports.emplace(std::move(name), type_queue.emplace(VirtualAddress::from_process_offset(
                    process_page.milestone()
                )));
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, super.super.type)
                );
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, elem_type)
                );
                process_page.emplace_top(
                    TFlag<ArrayType>,
                    Type{{arr_type_t_addr},
                        static_cast<Long>(sizeof(RTTObject) + sizeof(MemberInfo) * member.size()),
                    arr_constructor_addr, arr_destructor_addr},
                    member_info_type_addr,
                    static_cast<Long>(member.size())
                );
            }
            else {
                type_queue.emplace(named_exports.at(name));
            }
            preloaded_member_segments.emplace(std::move(member));
        }

        class_member_addr = VirtualAddress::from_process_offset(
            process_page.milestone()
        );

        // phase 3.2 load member info
        while (!preloaded_member_segments.empty()) {
            auto member = std::move(preloaded_member_segments.front());
            auto type_addr = type_queue.front();
            auto& type = deref<ArrayType>(type_addr);
            preloaded_member_segments.pop();
            type_queue.pop();


            const auto milestone = process_page.milestone();

            process_page.placeholder_push(type.super.instance_size);

            auto repr = &process_page.ref_top<UByte>();
            *reinterpret_cast<RTTObject*&>(repr) ++ = {type_addr};
            auto offset = sizeof(RTTObject);
            while (!member.empty()) {
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    milestone,
                    static_cast<std::uint16_t>(offset + offsetof(MemberInfo, type))
                );
                *reinterpret_cast<MemberInfo*&>(repr) ++ = member.front();
                member.pop();
                offset += sizeof(MemberInfo);
            }
        }

    }

    auto class_lifetime = Queue<VirtualAddress>{&process_memory};

    // phase - 4 load class ext list
    VirtualAddress class_ext_addr;{

        // phase - 4.1 load addr arr classes
        const auto member_info_type_addr = named_exports.contains(VM_TEXT("::address"))
        ? named_exports.at(VM_TEXT("::address"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::address")));
        auto type_queue = Queue<VirtualAddress>{&process_memory};

        for (auto [[maybe_unused]]_: std::views::iota(0u, preloaded_cls_ext_segments.size())) {
            auto ext = std::move(preloaded_cls_ext_segments.front());
            preloaded_cls_ext_segments.pop();
            if (auto name = typename_arr(&process_memory, VM_TEXT("address"), ext.size());
                !named_exports.contains(name)) {
                named_exports.emplace(std::move(name), type_queue.emplace(VirtualAddress::from_process_offset(
                    process_page.milestone()
                )));
                process_page.emplace_top(
                    TFlag<ArrayType>,
                    Type{{arr_type_t_addr},
                        static_cast<Long>(sizeof(RTTObject) + sizeof(VirtualAddress) * ext.size()),
                    arr_constructor_addr, arr_destructor_addr},
                    member_info_type_addr,
                    static_cast<Long>(ext.size())
                );
            }
            else {
                type_queue.emplace(named_exports.at(name));
            }
            preloaded_cls_ext_segments.emplace(std::move(ext));
        }

        class_ext_addr = VirtualAddress::from_process_offset(
            process_page.milestone()
        );

        auto counter = 0;
        // phase 3.2 load class ext
        while (!preloaded_cls_ext_segments.empty()) {
            auto ext = std::move(preloaded_cls_ext_segments.front());
            auto type_addr = type_queue.front();
            auto& type = deref<ArrayType>(type_addr);
            preloaded_cls_ext_segments.pop();
            type_queue.pop();


            process_page.placeholder_push(type.super.instance_size);

            auto repr = &process_page.ref_top<UByte>();
            *reinterpret_cast<RTTObject*&>(repr) ++ = {type_addr};


            if (counter == 0) {
                // constructor
                class_lifetime.emplace(*reinterpret_cast<VirtualAddress*&>(repr) ++ = ext.front());
                ext.pop();
                // destructor
                class_lifetime.emplace(*reinterpret_cast<VirtualAddress*&>(repr) ++ = ext.front());
                ext.pop();
            }
            while (!ext.empty()) {
                *reinterpret_cast<VirtualAddress*&>(repr) ++ = ext.front();
                ext.pop();
            }
            counter = (counter + 1) % 3;
        }

    }

    // phase - 5 load char arr type
    auto str_type = Queue<VirtualAddress>{&process_memory};{
        const auto char_addr = named_exports.contains(VM_TEXT("::char"))
        ? named_exports.at(VM_TEXT("::char"))
        : VirtualAddress::from_reserve_offset(exports.at<Str>(VM_TEXT("::char")));

        while (!preloaded_str_length.empty()) {
            auto len = preloaded_str_length.front();
            preloaded_str_length.pop();

            if (auto name = typename_arr(&process_memory, VM_TEXT("::char"), len);
                !named_exports.contains(name)) {
                named_exports.emplace(std::move(name), str_type.emplace(VirtualAddress::from_process_offset(
                    process_page.milestone()
                )));
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, super.super.type)
                );
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(ArrayType, elem_type)
                );
                process_page.emplace_top(
                    TFlag<ArrayType>,
                    Type{{arr_type_t_addr},
                        static_cast<Long>(sizeof(RTTObject) + sizeof(Char) * len),
                        arr_constructor_addr, arr_destructor_addr},
                    char_addr,
                    static_cast<Long>(len)
                );
                }
            else {
                str_type.emplace(named_exports.at(name));
            }
        }
    }

    const auto base_object_addr = VirtualAddress::from_process_offset(
        process_page.milestone()
    );

    auto counter = 0ull;
    auto ready_objects = PriQue<LTObject>{&process_memory};
    while (!preloaded.empty()) {
        if (exports.contains(counter)) [[unlikely]]
            named_exports.emplace(
                exports.at(counter),
                VirtualAddress::from_process_offset(process_page.milestone())
            );

        std::visit([&]<typename T>(T& obj){
            if constexpr (TypePack<
                Bool, Char, Byte,
                Int, Long, Float,
                Double>::contains<T>) {
                process_page.emplace_top(TFlag<T>, obj);
            }
            else if constexpr (std::same_as<T, NamedType>) {
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(NamedType, super.super.type)
                );
                process_page.emplace_top(TFlag<NamedType>, NamedType{
                    {   {obj.super.super.type},
                        obj.super.instance_size,
                        base_object_addr.id_shift(obj.super.maker.id()),
                        base_object_addr.id_shift(obj.super.collector.id())},
                    string_base_addr.id_shift(obj.name.id())
                });
            }
            else if constexpr (std::same_as<T, VirtualAddress>) {
                if (obj.is_reserved())
                    process_page.emplace_top(TFlag<VirtualAddress>,
                        base_object_addr.id_shift(obj.id()));
                else
                    process_page.emplace_top(TFlag<VirtualAddress>,
                        obj);
            }
            else if constexpr (std::same_as<T, FFamily>) {
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(FFamily, super.type)
                );
                process_page.emplace_top(TFlag<FFamily>, FFamily{
                    obj.super.type,
                    base_object_addr.id_shift(obj.base_addr.id()) ,
                    override_base_addr.id_shift(obj.overrides.id())
                });
            }
            else if constexpr (std::same_as<T, Class>) {
                const auto cons = class_lifetime.front(); class_lifetime.pop();
                const auto des = class_lifetime.front(); class_lifetime.pop();
                translate_required.emplace(
                    VirtualAddress::process_page_id,
                    process_page.milestone(),
                    u16offsetof(Class, super.super.super.type)
                );
                process_page.emplace_top(TFlag<Class>,
                    NamedType{{obj.super.super.super,
                        obj.super.super.instance_size,
                        cons, des},
                    string_base_addr.id_shift(obj.super.name.id())},
                    class_member_addr.id_shift(obj.members.id()),
                    class_ext_addr.id_shift(obj.methods.id()),
                    class_ext_addr.id_shift(obj.static_members.id()),
                    class_ext_addr.id_shift(obj.static_methods.id())
                );
            }
            else if constexpr (std::same_as<T, LTObject>) {
                process_page.placeholder_push(obj.size);
                ready_objects.emplace(
                    std::move(obj)
                );
            }
            else if constexpr (std::same_as<T, CharArrayRepr>) {
                const auto tp = str_type.front(); str_type.pop();
                const auto& t = deref<ArrayType>(tp);
                process_page.placeholder_push(t.super.instance_size);
                auto repr = &process_page.ref_top<UByte>();
                *reinterpret_cast<RTTObject*&>(repr) ++ = {tp};
                for (const Char ch: obj)
                    *reinterpret_cast<Char*&>(repr) ++ = ch;
            }
            else std::unreachable();
        }, preloaded.front());
        preloaded.pop();
        ++ counter;
    }

    while (!params.empty()) {
        std::visit([&]<typename T>(const T v) {
            if constexpr (std::same_as<T, VirtualAddress>)
                if (v.is_reserved()) {
                    thread.eval_stack.emplace(TFlag<VirtualAddress>, base_object_addr.id_shift(v.id()));
                    return;
                }
            thread.eval_stack.emplace(TFlag<T>, v);
        }, params.top());
        params.pop();
    }


    while (!ready_objects.empty()) {
        auto obj = std::move(const_cast<LTObject&>(ready_objects.top()));
        ready_objects.pop();

        call_init_function(thread, deref<FFamily>(deref<NamedType>(
            obj.type
        ).super.maker), obj.maker_override);

        ready_objects.pop();
    }

    while (!translate_required.empty()) {
        auto& va = deref<VirtualAddress>(translate_required.front());
        translate_required.pop();
        if (va.is_reserved())
            va = base_object_addr.id_shift(va.id());
    }

    for (auto& lib: extern_libs)
        libraries.emplace_back(std::move(lib));
}
