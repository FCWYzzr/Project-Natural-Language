//
// Created by FCWY on 25-1-3.
//
/**
 * communicate platform & vm
 */
module;
#include "project-nl.h"

export module pnl.ll.runtime;
import pnl.ll.base;
import pnl.ll.comtime;
import pnl.ll.string;
import pnl.ll.collections;




using namespace pnl::ll;

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

        VirtualAddress & operator=(const VirtualAddress &) noexcept = default;

        [[nodiscard]]
        std::uint8_t page() const noexcept;

        [[nodiscard]]
        std::uint32_t id() const noexcept;

        [[nodiscard]]
        std::uint16_t offset() const noexcept;

        [[nodiscard]]
        VirtualAddress id_shift(std::uint32_t shift) const noexcept;

        [[nodiscard]]
        VirtualAddress offset_shift(std::uint16_t shift) const noexcept;

        [[nodiscard]]
        bool is_native() const noexcept;

        [[nodiscard]]
        bool is_reserved() const noexcept;

        [[nodiscard]]
        bool is_process() const noexcept;


        [[nodiscard]]
        std::strong_ordering
        operator <=> (VirtualAddress other) const noexcept;

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
        std::filesystem::path path;
        void*   lib_ref;

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
        NtvFunc find_proc(const MBStr& name) const noexcept {
            return static_cast<NtvFunc>(dlsym(lib_ref, name.data()));
        }

        ~VMLib() noexcept;
    };

    inline namespace load{
        struct Patch;
        struct LTObject;
    }

    inline namespace execute {
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

    struct alignas(Long) Type {
        RTTObject
            super;
        Long
            instance_size;
        VirtualAddress
            maker;
        VirtualAddress
            collector;
    };
    static_assert(Trivial<Type>);


    struct alignas(Long) NamedType {
        Type
            super;
        // refer to a string
        VirtualAddress
            name;
    };
    static_assert(Trivial<NamedType>);


    struct alignas(Long) Class {
        NamedType
            super;

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


    struct alignas(Long) ArrayType {
        Type
            super;
        VirtualAddress
            elem_type;
        Long
            length;
    };
    static_assert(Trivial<ArrayType>);

    template<typename T>
    struct alignas(Long) Array{
        RTTObject
            super;
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

        T* data() noexcept {
            return reinterpret_cast<T*>(reinterpret_cast<RTTObject*>(this) + 1);
        }
        const T* data() const noexcept {
            return reinterpret_cast<const T*>(reinterpret_cast<const RTTObject>(this) + 1);
        }

        ListV<T> view(Process& proc) noexcept;
        ListV<const T> view(Process& proc) const noexcept;
    };




    // auto erasable type type
    struct alignas(Long) OverrideType {
        Type
            super;
        VirtualAddress
            param_type_array_addr;
        VirtualAddress
            return_type;
    };
    static_assert(Trivial<OverrideType>);


    struct PNL_LIB_PREFIX alignas(Long) FOverride {
        RTTObject
            super;
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


    struct alignas(Long) FFamily {
        RTTObject
            super;
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

    using Datapack = Dict<Package>;

    struct PNL_LIB_PREFIX LTObject {
        USize init_rank;
        USize size;
        VirtualAddress type;
        USize maker_override;

        LTObject() noexcept = default;

        LTObject(const USize init_rank, const USize size, const VirtualAddress& type, const USize maker_override)
            : init_rank(init_rank),
              size(size),
              type(type),
              maker_override(maker_override) {
        }

        bool operator < (const LTObject& o) const noexcept {
            return init_rank < o.init_rank;
        }

    };

    // compiled patch for process
    struct PNL_LIB_PREFIX Patch {
        Queue<Str>
            preloaded_strings;
        // preloaded functions' class
        Queue<OverrideType>
            preloaded_override_type;
        Queue<Queue<VirtualAddress>>
            arg_types;
        Queue<Queue<Instruction>>
            instructions;
        Queue<NtvId>
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

        Queue<Pair<ReferenceRepr, USize>>
            preloaded_arr_tp_repr;

        Patch(MManager* mem) noexcept
            : preloaded_strings(mem),
              preloaded_override_type(mem),
              arg_types(mem),
              instructions(mem),
              ntv_funcs(mem),
              preloaded_overrides(mem),
              preloaded_member_segments(mem),
              preloaded_cls_ext_segments(mem),
              preloaded(mem),
              exports(mem),
              preloaded_params(mem),
              preloaded_arr_tp_repr(mem) {}

        Patch(Queue<Str> preloaded_strings, Queue<OverrideType> preloaded_override_type,
            Queue<Queue<VirtualAddress>> arg_types, Queue<Queue<Instruction>> instructions, Queue<NtvId> ntv_funcs,
            Queue<Queue<FOverride>> preloaded_overrides, Queue<Queue<MemberInfo>> preloaded_member_segments,
            Queue<Queue<VirtualAddress>> preloaded_cls_ext_segments, Queue<LTValue> preloaded,
            BiMap<Str, USize> exports, Stack<Value> preloaded_params, Queue<Pair<ReferenceRepr, USize>> preloaded_str_len)
            noexcept:
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
              preloaded_arr_tp_repr(std::move(preloaded_str_len)) {
        }

        Patch(Patch &&other) noexcept
            :
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
              preloaded_arr_tp_repr(std::move(other.preloaded_arr_tp_repr)) {
        }

        Patch & operator=(Patch &&other) noexcept {
            if (this == &other)
                return *this;

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
            preloaded_arr_tp_repr = std::move(other.preloaded_arr_tp_repr);
            return *this;
        }

        using Delegated = Tuple<
            Queue<Str>&,
            Queue<OverrideType>&,
            Queue<Queue<VirtualAddress>>&,
            Queue<Queue<Instruction>>&,
            Queue<NtvId>&,
            Queue<Queue<FOverride>>&,
            Queue<Queue<MemberInfo>>&,
            Queue<Queue<VirtualAddress>>&,
            Queue<LTValue>&,
            BiMap<Str, USize>&,
            Stack<Value>&,
            Queue<Pair<ReferenceRepr, USize>>&
        >;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator Delegated () noexcept {
            return std::tie(
            preloaded_strings,
            preloaded_override_type,
            arg_types,
            instructions,
            ntv_funcs,
            preloaded_overrides,
            preloaded_member_segments,
            preloaded_cls_ext_segments,
            preloaded,
            exports,
            preloaded_params,
            preloaded_arr_tp_repr
            );
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
        std::uint32_t
            milestone;
        std::uint32_t
            program_counter{0};
        VirtualAddress
            base_addr;
        VirtualAddress
            instructions;

        FuncContext(const std::uint32_t milestone,
            const VirtualAddress base_addr,
            const VirtualAddress instructions)
            noexcept: milestone(milestone),
              base_addr(base_addr),
              instructions(instructions) {
        }

        FuncContext& operator=(const FuncContext&) noexcept = default;

        Instruction decode (Process&) const noexcept;

        FuncContext& operator ++ () noexcept;
    };



    struct PNL_LIB_PREFIX Thread {
        std::pmr::unsynchronized_pool_resource
                thread_memory;
        mutable std::binary_semaphore
                step_token;

        Process*
                process;
        const std::uint8_t
                thread_id;
        bool
                is_daemon{false};
        std::atomic_bool
                is_done{false};
        Deque<std::variant<FuncContext, NtvFunc>>
                call_deque{&thread_memory};
        Deque<Value>
                eval_deque{&thread_memory};
        pmr::Stack
                thread_page{&thread_memory};
        std::jthread
                native_handler;

        Thread(Process* process, std::uint8_t) noexcept;

        Thread(Thread &&other) noexcept;

        Thread & operator=(Thread &&other) noexcept {
            if (this == &other)
                return *this;
            process = other.process;
            is_daemon = other.is_daemon;
            is_done.exchange(other.is_done);
            call_deque = std::move(other.call_deque);
            eval_deque = std::move(other.eval_deque);
            thread_page = std::move(other.thread_page);
            native_handler = std::move(other.native_handler);
            return *this;
        }

        void run(const std::stop_token &) noexcept;
        void step() noexcept;
        void pause() const noexcept;
        void resume() const noexcept;
        void terminate() noexcept;

        ~Thread() noexcept;

        void call_function(const FFamily& family, std::uint32_t override_id) noexcept;

        void clear() noexcept;

        [[nodiscard]]
        FuncContext& current_proc() noexcept;

        [[nodiscard]]
        VirtualAddress process_memory_base() noexcept;
        [[nodiscard]]
        VirtualAddress function_memory_base() noexcept;

        Value take() noexcept;

        template<typename T=UByte>
        [[nodiscard]]
        T& deref(const VirtualAddress& vr) noexcept;




    };

    struct PNL_LIB_PREFIX Process {
        std::pmr::synchronized_pool_resource
                process_memory;

        // keep libs alive
        List<VMLib>         libraries{&process_memory};

        // stores native func
        pmr::Stack          process_page{&process_memory};
        List<Thread>        threads{&process_memory};

        std::shared_mutex   native_page_mutex;
        List<std::pair<
            UByte*,
            USize
        >>                  native_page{&process_memory};

        // name mapping for dynamic linking
        Dict<VirtualAddress>
                            named_exports{&process_memory};

        explicit Process(MManager*);

        ~Process() noexcept { // NOLINT(*-use-equals-default)
            terminate();
        }

        void pause() const noexcept;
        void resume() const noexcept;
        void terminate() noexcept;

        static VirtualAddress get_vr(
            std::uint8_t thread_id,
            std::uint32_t object_offset
            ) noexcept;


        Thread& get_available_thread() noexcept;

        Thread& main_thread() noexcept;

        [[nodiscard]]
        VirtualAddress wild_alloc(Int) noexcept;
        void wild_collect(VirtualAddress) noexcept;

        template<typename T>
        T& deref(const VirtualAddress vr) noexcept {
            assert(!vr.is_reserved(), {"trying to deref a reserve ptr", &process_memory});
            // if (vr.is_native()) todo
            if (vr.is_process())
                return *reinterpret_cast<T*>(process_page[vr.id()] + vr.offset());
            return *reinterpret_cast<T*>(threads[vr.page()].thread_page[vr.id()] + vr.offset());
        }


        std::optional<Patch> compile_datapack(Datapack, Str* =nullptr) noexcept;


        void load_library(VMLib&&) noexcept;
        void load_patch(Patch&&, Str*);
    };
}


template<typename T>
T & Thread::deref(const VirtualAddress &vr) noexcept {
    return process->deref<T>(vr);
}

template<typename T>
ListV<T> Array<T>::view(Process &proc) noexcept {
    const auto& arr_t = proc.deref<ArrayType>(super.type);
    const auto beg = &(*this)[0];
    const auto end = &(*this)[arr_t.length];
    return ListV<T>(beg, end);
}

template<typename T>
ListV<const T> Array<T>::view(Process &proc) const noexcept {
    const auto& arr_t = proc.deref<ArrayType>(super.type);
    const auto beg = &(*this)[0];
    const auto end = &(*this)[arr_t.length];
    return ListV<const T>(beg, end);
}
