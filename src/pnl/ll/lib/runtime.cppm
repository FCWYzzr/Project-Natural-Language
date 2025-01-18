//
// Created by FCWY on 25-1-3.
//
/**
 * communicate platform & vm
 */
export module pnl.ll.runtime;
export import pnl.ll.base;
export import pnl.ll.comtime;
export import pnl.ll.string;
export import pnl.ll.meta_prog;
export import pnl.ll.collections;


import <dlfcn.h>;
import <filesystem>;
import <stdexcept>;
import <utility>;
import <algorithm>;
import <ranges>;
import <unordered_map>;
import <iostream>;
import <variant>;
import <thread>;
import <semaphore>;
import <shared_mutex>;
import <mutex>;
import <memory_resource>;

using namespace pnl::ll;

template<typename T>
T ushr(const T a, const T b) noexcept {
    auto ret = a >> b;
    if (a < 0)
        ret &= ~(1 << (sizeof(T) - 1));
    return ret;
}


export namespace pnl::ll::inline runtime{

    struct Process;
    struct Thread;
    struct PNL_LIB_PREFIX VirtualAddress {
        constexpr static std::uint8_t  process_page_id = 0xd;
        constexpr static std::uint8_t  native_page_id = 0xe;
        constexpr static std::uint8_t  invalid_page_id = 0xf;


        // | reversed - 12 |  page - 4 | object id - 32 | body offset - 16 |
        std::uintptr_t content;

        constexpr VirtualAddress() noexcept:
            VirtualAddress(invalid_page_id, UINT32_MAX){}

        explicit VirtualAddress(const std::uintptr_t addr) noexcept:
            content(addr){}

        constexpr VirtualAddress(
            const std::uint8_t page_id,
            const std::uint32_t object_offset,
            const std::uint16_t body_offset=0) noexcept:
            content(
                (static_cast<std::uintptr_t>(page_id) << 48)
                | (static_cast<std::uintptr_t>(object_offset) << 16)
                | body_offset){}

        VirtualAddress(const VirtualAddress &other) noexcept = default;

        VirtualAddress & operator=(const VirtualAddress &other) noexcept {
            if (this == &other)
                return *this;
            content = other.content;
            return *this;
        }

        [[nodiscard]]
        std::uint8_t page() const noexcept {
            return (content  >> 48) % 0xf;
        }
        [[nodiscard]]
        std::uint32_t id() const noexcept {
            return (content >> 16) & 0xffffffff;
        }
        [[nodiscard]]
        std::uint16_t offset() const noexcept {
            return content & 0xffff;
        }

        [[nodiscard]]
        VirtualAddress id_shift(const std::uint32_t shift) const noexcept {
            return {page(), id() + shift, offset()};
        }

        [[nodiscard]]
        VirtualAddress offset_shift(const std::uint16_t shift) const noexcept {
            return {page(), id(), static_cast<std::uint16_t>(offset() + shift)};
        }

        [[nodiscard]]
        bool is_reserved() const noexcept {
            return page() == invalid_page_id;
        }
        [[nodiscard]]
        bool is_process() const noexcept {
            return page() == process_page_id;
        }
        [[nodiscard]]
        std::strong_ordering
        operator <=> (const VirtualAddress other) const noexcept {
            return content <=> other.content;
        }

        static VirtualAddress from_reserve_offset(std::uint32_t offset) noexcept {
            return {invalid_page_id, offset};
        }

        static VirtualAddress from_process_offset(std::uint32_t offset) noexcept {
            return {process_page_id, offset};
        }

        explicit operator std::uint64_t() const noexcept {
            return content;
        }

    };
    constexpr VirtualAddress null_addr;
    struct FuncContext;

    using NtvFunc   = void(*)(Thread&);
    struct PNL_LIB_PREFIX VMLib {
        void*   lib_ref;

        explicit VMLib(void* lib_ref=nullptr) noexcept:
            lib_ref{lib_ref} {}

        explicit VMLib(
            const Path& path
            ) noexcept;

        VMLib(VMLib &&other) noexcept
            : lib_ref(other.lib_ref) {
            other.lib_ref = nullptr;
        }

        VMLib & operator=(VMLib &&other) noexcept {
            if (this == &other)
                return *this;
            std::swap(lib_ref, other.lib_ref);
            return *this;
        }

        [[nodiscard]]
        NtvFunc find_proc(const NStr& name) const noexcept {
            return static_cast<NtvFunc>(dlsym(lib_ref, name.data()));
        }

        ~VMLib() noexcept;
    };

    inline namespace load{
        struct Datapack;
        struct Patch;
        struct LTObject;
    }

    inline namespace execute {
        struct RTUObject;
        struct RTTObject;
        struct Type;
        struct NamedType;
        struct Class;
        struct ArrayType;
        struct OverrideType;
        struct FFamily;
        struct FOverride;
        struct MemberInfo;

        using Value     = std::variant<
            Bool, Byte, Char,
            Int, Long, Float, Double,
            VirtualAddress
        >;
    }
}

export namespace pnl::ll::runtime::execute {
    // everything is trivially copiable
    struct alignas(Long) RTUObject {};
    static_assert(Trivial<RTUObject>);


    struct alignas(Long) RTTObject{
        // refer to a type
        VirtualAddress type;
    };
    static_assert(Trivial<RTTObject>);

    struct alignas(Long) MemberInfo{
        // refer to a char array
        VirtualAddress name;

        // refer to a class
        VirtualAddress type;

        // obj offset
        Long  offset;
    };
    static_assert(Trivial<MemberInfo>);

    struct alignas(Long) Type: RTTObject {
        Bool
            rtti_support;
        Int
            instance_size;
        VirtualAddress
            maker;
        VirtualAddress
            collector;
    };
    static_assert(Trivial<Type>);


    struct alignas(Long) NamedType: Type {
        // refer to a string
        VirtualAddress
            name;
    };
    static_assert(Trivial<NamedType>);


    struct alignas(Long) Class: NamedType {

        // refer to an array of member info
        VirtualAddress
            members;

        // refer to an array of func refers
        VirtualAddress
            methods;

        // refer to an array of obj refers
        VirtualAddress
            static_members;

        // refer to an array of func refers
        VirtualAddress
            static_methods;

    };
    static_assert(Trivial<Class>);


    struct alignas(Long) ArrayType: Type {
        VirtualAddress
            elem_type;
        Long
            length;
    };
    static_assert(Trivial<ArrayType>);

    template<typename T>
    struct alignas(Long) Array: RTTObject{
        // Array should only use by reinterpret cast
        Array()=delete;
        // we already alloc & init enough elements, so normally no UB may exist here
        T& operator [] (const USize index) noexcept {
            auto begin = reinterpret_cast<T*>(reinterpret_cast<UByte*>(this) + sizeof(RTTObject));
            return begin[index];
        }
        const T& operator [] (const USize index) const noexcept {
            auto begin = reinterpret_cast<const T*>(reinterpret_cast<const UByte*>(this) + sizeof(RTTObject));
            return begin[index];
        }

        ListV<T> view(Process& proc) noexcept;
    };




    // auto erasable type type
    struct alignas(Long) OverrideType: Type {
        VirtualAddress
            param_type_array_addr;
        VirtualAddress
            return_type;
    };
    static_assert(Trivial<OverrideType>);


    struct PNL_LIB_PREFIX alignas(Long) FOverride: RTTObject {
        bool is_native;
        // if native, refer to NtvFunc
        // else, refer to Instruction[]
        std::uintptr_t target_addr;


        [[nodiscard]]
        NtvFunc ntv() const noexcept;

        [[nodiscard]]
        VirtualAddress vm() const noexcept;
    };
    static_assert(Trivial<FOverride>);


    struct alignas(Long) FFamily: RTTObject {
        // base addr ptr
        VirtualAddress
            base_addr;

        // array of overrides
        VirtualAddress
            overrides;
    };
    static_assert(Trivial<FFamily>);

}


export namespace pnl::ll::runtime::load {
    using LTValue = std::variant<
            Bool, Byte, Char,
            Int, Long, Float, Double,
            NamedType,

            VirtualAddress,
            FFamily,
            Class,
            LTObject,
            CharArrayRepr
        >;

    // merged service pack
    struct PNL_LIB_PREFIX Datapack {
        Map<Path, VMLib>
                extern_libs;
        Dict<Package>
                package_pack;

        Datapack(Map<Path, VMLib> extern_libs, Dict<Package> package_pack) noexcept;

        Datapack(Datapack &&other) noexcept;

        Datapack & operator=(Datapack &&other) noexcept {
            if (this == &other)
                return *this;
            extern_libs = std::move(other.extern_libs);
            package_pack = std::move(other.package_pack);
            return *this;
        }


        Datapack& operator += (Datapack&& pack) noexcept;

        static Datapack
            from_lib(Map<Path, VMLib> extern_libs) noexcept;

        static Datapack
            from_pkg(Dict<Package> pkgs) noexcept;
    };

    struct PNL_LIB_PREFIX LTObject {
        USize init_rank;
        USize size;
        VirtualAddress type;
        USize maker_override;

        LTObject(const USize init_rank, const USize size, const VirtualAddress& type, const USize maker_override)
            : init_rank(init_rank),
              size(size),
              type(type),
              maker_override(maker_override) {
        }

        bool operator < (const LTObject& o) const noexcept {
            return init_rank > o.init_rank;
        }

    };

    // compiled patch for process
    struct PNL_LIB_PREFIX Patch {
        Map<Path, VMLib>
            extern_libs;

        Queue<Str>
            preloaded_strings;
        // preloaded functions' class
        Queue<OverrideType>
            preloaded_override_type;
        Queue<Queue<VirtualAddress>>
            arg_types;
        Queue<Queue<Instruction>>
            instructions;
        Queue<NtvFunc>
            ntv_funcs;
        Queue<Queue<FOverride>>
            preloaded_overrides;


        Queue<Queue<MemberInfo>>
            preloaded_member_segments;
        // pack: {method, static_member, static_method}
        Queue<Queue<VirtualAddress>>
            preloaded_cls_ext_segments;

        Queue<LTValue>
            preloaded;

        BiMap<Str, USize>
            exports;

        Stack<Value>
            preloaded_params;

        Queue<USize>
            preloaded_str_len;

        Patch(Map<Path, VMLib> extern_libs, Queue<Str> preloaded_strings, Queue<OverrideType> preloaded_override_type,
            Queue<Queue<VirtualAddress>> arg_types, Queue<Queue<Instruction>> instructions, Queue<NtvFunc> ntv_funcs,
            Queue<Queue<FOverride>> preloaded_overrides, Queue<Queue<MemberInfo>> preloaded_member_segments,
            Queue<Queue<VirtualAddress>> preloaded_cls_ext_segments, Queue<LTValue> preloaded,
            BiMap<Str, USize> exports, Stack<Value> preloaded_params, Queue<USize> preloaded_str_len)
            noexcept: extern_libs(std::move(extern_libs)),
              preloaded_strings(std::move(preloaded_strings)),
              preloaded_override_type(std::move(preloaded_override_type)),
              arg_types(std::move(arg_types)),
              instructions(std::move(instructions)),
              ntv_funcs(std::move(ntv_funcs)),
              preloaded_overrides(std::move(preloaded_overrides)),
              preloaded_member_segments(std::move(preloaded_member_segments)),
              preloaded_cls_ext_segments(std::move(preloaded_cls_ext_segments)),
              preloaded(std::move(preloaded)),
              exports(std::move(exports)),
              preloaded_params(std::move(preloaded_params)),
              preloaded_str_len(std::move(preloaded_str_len)) {
        }

        Patch(Patch &&other) noexcept
            : extern_libs(std::move(other.extern_libs)),
              preloaded_strings(std::move(other.preloaded_strings)),
              preloaded_override_type(std::move(other.preloaded_override_type)),
              arg_types(std::move(other.arg_types)),
              instructions(std::move(other.instructions)),
              ntv_funcs(std::move(other.ntv_funcs)),
              preloaded_overrides(std::move(other.preloaded_overrides)),
              preloaded_member_segments(std::move(other.preloaded_member_segments)),
              preloaded_cls_ext_segments(std::move(other.preloaded_cls_ext_segments)),
              preloaded(std::move(other.preloaded)),
              exports(std::move(other.exports)),
              preloaded_params(std::move(other.preloaded_params)),
              preloaded_str_len(std::move(other.preloaded_str_len)) {
        }

        Patch & operator=(Patch &&other) noexcept {
            if (this == &other)
                return *this;
            extern_libs = std::move(other.extern_libs);
            preloaded_strings = std::move(other.preloaded_strings);
            preloaded_override_type = std::move(other.preloaded_override_type);
            arg_types = std::move(other.arg_types);
            instructions = std::move(other.instructions);
            ntv_funcs = std::move(other.ntv_funcs);
            preloaded_overrides = std::move(other.preloaded_overrides);
            preloaded_member_segments = std::move(other.preloaded_member_segments);
            preloaded_cls_ext_segments = std::move(other.preloaded_cls_ext_segments);
            preloaded = std::move(other.preloaded);
            exports = std::move(other.exports);
            preloaded_params = std::move(other.preloaded_params);
            preloaded_str_len = std::move(other.preloaded_str_len);
            return *this;
        }
    };




    struct PNL_LIB_PREFIX LTMemberInfo {
        Str
            name;
        VirtualAddress
            type;
        USize
            offset;
    };

}



export namespace pnl::ll::runtime {

    struct PNL_LIB_PREFIX FuncContext {
        const USize
            milestone;
        const VirtualAddress
            base_addr;
        const VirtualAddress
            instructions;
        USize
            program_counter{0};
        Process& process;

        FuncContext(const USize milestone, const VirtualAddress &base_addr, const VirtualAddress &instructions,
            Process &process)
            : milestone(milestone),
              base_addr(base_addr),
              instructions(instructions),
              process(process) {
        }

        Instruction operator * () const noexcept;

        FuncContext& operator ++ () noexcept;
    };



    struct PNL_LIB_PREFIX Thread {
        mutable std::binary_semaphore
                step_token;
        std::pmr::unsynchronized_pool_resource
                thread_memory;

        Process* const
                process;
        const std::uint16_t
                thread_id;
        bool
                is_daemon{false};
        Stack<std::variant<FuncContext, NtvFunc>>
                call_stack{&thread_memory};
        Queue<std::variant<FuncContext, NtvFunc>>
                init_queue{&thread_memory};
        pmr::Stack
                thread_page{&thread_memory};
        Stack<Value>
                eval_stack{&thread_memory};
        std::jthread
                native_handler;

        Thread(Process* process, std::uint16_t) noexcept;

        Thread(Thread &&other) noexcept;

        void pause() const noexcept;
        void resume() const noexcept;
        void terminate() noexcept {
            if (native_handler.joinable()) {
                native_handler.get_stop_source().request_stop();
                native_handler.join();
            }
        }

        ~Thread() noexcept {
            terminate();
        }

        void call_function(const FFamily& family, std::uint32_t override_id) noexcept;

        void clear() noexcept;


        [[nodiscard]]
        FuncContext& current_proc() noexcept;

        [[nodiscard]]
        VirtualAddress mem_spec_base() noexcept;

        [[nodiscard]]
        Class& vm__top_obj_type() noexcept;


        Value vm__pop_top() noexcept {
            auto top = std::move(eval_stack.top());
            eval_stack.pop();
            return std::move(top);
        }

        template<typename T=UByte>
        [[nodiscard]]
        T& deref(const VirtualAddress& vr) noexcept;



        void run(const std::stop_token &) noexcept;


        void step() noexcept;


        static void nop() noexcept {}


        void waste() noexcept;


        void cast(Instruction inst) noexcept;


        void cmp() noexcept;


        void invoke_top() noexcept;


        void end_proc() noexcept;



        void jump(Instruction inst) noexcept;


        void add() noexcept;

        void sub() noexcept;

        void mul() noexcept;

        void div() noexcept;

        void rem() noexcept;


        void neg() noexcept;

        void shl() noexcept;

        void shr() noexcept;

        void ushr() noexcept;


        void bit_and() noexcept;

        void bit_or() noexcept;

        void bit_xor() noexcept;

        void bit_inv() noexcept;



        void load(Instruction) noexcept;

        void store(Instruction) noexcept;
        void ref_at(Instruction) noexcept;

        void load_by_ref(Instruction) noexcept;

        void store_by_ref(Instruction) noexcept;

        void ref_member(Instruction) noexcept;

        void ref_static_member(Instruction) noexcept;

        void ref_method(Instruction) noexcept;

        void ref_static_method(Instruction) noexcept;


        void invoke_override(Instruction) noexcept;


        void instate(Instruction) noexcept;

        void destroy() noexcept;
    };

    struct PNL_LIB_PREFIX Process {
        std::pmr::synchronized_pool_resource
                process_memory;

        // keep libs alive
        List<VMLib>         libraries{&process_memory};
        BiMap<Str, Path>    plugin_provider{&process_memory};

        // stores native func
        pmr::Stack          process_page{&process_memory};
        List<Thread>        threads{&process_memory};

        std::shared_mutex   native_page_mutex;
        List<std::tuple<
            UByte*,
            std::function<void(const UByte*)>,
            USize>>         native_page{&process_memory};

        // name mapping for dynamic linking
        Dict<VirtualAddress>
                            named_exports{&process_memory};

        explicit Process(MManager*, Datapack, const Path& ="./");
        ~Process() noexcept {
            for (auto& thr: threads)
                thr.terminate();
        }

        void pause() const noexcept;
        void resume() const noexcept;

        static VirtualAddress get_vr(
            std::uint8_t thread_id,
            std::uint32_t object_offset
            ) noexcept;


        Thread& get_available_thread() noexcept;

        Thread& main_thread() noexcept;

        template<typename T>
        T& deref(const VirtualAddress vr) noexcept {
            // if (vr.is_native()) todo
            if (vr.is_process())
                return *reinterpret_cast<T*>(process_page[vr.id()] + vr.offset());
            return *reinterpret_cast<T*>(threads[vr.page()].thread_page[vr.id()] + vr.offset());
        }


        std::optional<Patch> compile_datapack(Datapack, Str* =nullptr) noexcept;



        void load_patch(Patch&&) noexcept;
    };
}


template<typename T>
T & Thread::deref(const VirtualAddress &vr) noexcept {
    return process->deref<T>(vr);
}

template<typename T>
ListV<T> Array<T>::view(Process &proc) noexcept {
    const auto& arr_t = proc.deref<ArrayType>(type);
    const auto beg = &(*this)[0];
    const auto end = &(*this)[arr_t.length];
    return ListV<T>(beg, end);
}