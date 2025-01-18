//
// Created by FCWY on 25-1-8.
//
// ReSharper disable CppDFANullDereference

module pnl.ll.runtime;

#ifdef WIN32
#define DLL_EXT ".dll"
#else
#define DLL_EXT ".so"
#endif

import <optional>;
import <memory>;
import <stdexcept>;


using namespace pnl::ll::runtime;
using namespace pnl::ll::runtime::load;
using namespace pnl::ll::runtime::execute;

Process::Process(MManager* const upstream, Datapack target, const Path& runtime_path):
    process_memory{
        upstream
    }{
    // construct main thread
    threads.emplace_back(this, 0);

    auto mem = &process_memory;
    // load core
    auto lib_map = Map<Path, VMLib>{mem};
    lib_map.emplace(runtime_path / "NLCore", VMLib(runtime_path / "NLCore"));
    auto pkg_map = Dict<Package>{mem};
    pkg_map.emplace(L"", Package::anonymous_builtin(mem));

    auto datapack = Datapack::from_lib(std::move(lib_map));
    datapack += Datapack::from_pkg(std::move(pkg_map));
    auto patch = compile_datapack(std::move(datapack));
    assert(patch.has_value(), "how did you get here??");
    load_patch(std::move(patch.value()));



    lib_map.clear();
    pkg_map.clear();
    lib_map.emplace(runtime_path / "NL_IO", VMLib(runtime_path / "NL_IO"));
    pkg_map.emplace(L"io", Package::io(mem));
    datapack = Datapack::from_lib(std::move(lib_map));
    datapack += Datapack::from_pkg(std::move(pkg_map));
    patch = compile_datapack(std::move(datapack));
    assert(patch.has_value(), "how did you get here??");
    load_patch(std::move(patch.value()));



    Str cache;

    patch = compile_datapack(std::move(target), &cache);
    assert(patch.has_value(), cache);
    load_patch(std::move(patch.value()));


    assert(named_exports.contains(L"main"), "module not executable(main not found )");

    auto& main = deref<FFamily>(named_exports.at(L"main"));
    threads[0].call_function(main, 0);

    threads[0].resume();
}

void Process::pause() const noexcept {
    for (auto& thr: threads)
        thr.pause();
}

void Process::resume() const noexcept {
    for (auto& thr: threads)
        thr.resume();
}

VirtualAddress Process::get_vr(const std::uint8_t thread_id, const std::uint32_t object_offset) noexcept {
    return VirtualAddress{thread_id, object_offset};
}

Thread & Process::get_available_thread() noexcept {
    for (auto& thr: threads)
        if (thr.call_stack.empty())
            return thr;
    threads.emplace_back(this, static_cast<std::uint16_t>(threads.size()));
    return threads.back();
}

Thread & Process::main_thread() noexcept {
    return threads.at(0);
}

Class& Thread::vm__top_obj_type() noexcept {
    const auto& obj = deref<RTTObject>(std::get<VirtualAddress>(eval_stack.top()));
    return deref<Class>(obj.type);
}


Thread::Thread(
    Process *process,
    const std::uint16_t thread_id) noexcept:
    step_token{0},
    process{process},
    thread_id{thread_id},
    call_stack{&process->process_memory},
    thread_page{&process->process_memory},
    eval_stack{&process->process_memory},
    native_handler{std::function{[this](const std::stop_token &token){
        run(token);
    }}}{}

Thread::Thread(Thread &&other) noexcept:
    step_token{0},
    process(other.process),
    thread_id(other.thread_id),
    call_stack(std::move(other.call_stack)),
    thread_page(std::move(other.thread_page)),
    eval_stack(std::move(other.eval_stack)),
    native_handler(std::move(other.native_handler)) {
}

void Thread::pause() const noexcept {
    step_token.acquire();
}

void Thread::resume() const noexcept {
    step_token.release();
}

void Thread::call_function(const FFamily &family, const std::uint32_t override_id) noexcept {
    if (const auto& override = deref<Array<FOverride>>(family.overrides)[override_id];
        override.is_native) [[unlikely]]
        override.ntv()(*this);
    else
        call_stack.emplace(
            TFlag<FuncContext>,
            thread_page.milestone(),
            family.base_addr,
            override.vm(),
            *process
        );
}

void Thread::clear() noexcept {
    while (!eval_stack.empty())
        eval_stack.pop();
    while (!call_stack.empty())
        call_stack.pop();
    thread_page.waste_since(0);
}

static std::variant<FuncContext, NtvFunc>&
cur_func(Thread& thr) noexcept {
    if (thr.init_queue.empty())
        return thr.call_stack.top();
    return thr.init_queue.front();
}

static void
pop_func(Thread& thr) noexcept {
    if (thr.init_queue.empty())
        return thr.call_stack.pop();
    return thr.init_queue.pop();
}


void Thread::run(const std::stop_token &token) noexcept {
    step_token.acquire();
    step_token.release();
    while (true){
        if (call_stack.empty() && init_queue.empty())
            return;
        if (is_daemon && token.stop_requested())
            return;

        step_token.acquire();
        if (cur_func(*this).index() != 0) [[unlikely]] {
            const auto ntv_proc = std::get<NtvFunc>(cur_func(*this));
            pop_func(*this);
            ntv_proc(*this);
        }
        else [[likely]]
            step();
        step_token.release();
    };
}

using enum OPCode;
void Thread::step() noexcept {
    const auto inst = *current_proc();
    ++ current_proc();
    switch (inst.opcode()) {
        case NOP:
            nop();
            break;
        case WASTE:
            waste();
            break;
        case CAST_C2I:
        case CAST_B2I:
        case CAST_I2B:
        case CAST_I2C:
        case CAST_I2L:
        case CAST_I2F:
        case CAST_I2D:
        case CAST_L2I:
        case CAST_L2F:
        case CAST_L2D:
        case CAST_F2I:
        case CAST_F2L:
        case CAST_F2D:
        case CAST_D2I:
        case CAST_D2L:
        case CAST_D2F:
            cast(inst);
            break;
        case CMP:
            cmp();
            break;
        case ADD:
            add();
            break;
        case SUB:
            sub();
            break;
        case MUL:
            mul();
            break;
        case DIV:
            div();
            break;
        case REM:
            rem();
            break;
        case NEG:
            neg();
            break;
        case SHL:
            shl();
            break;
        case SHR:
            shr();
            break;
        case USHR:
            ushr();
            break;
        case BIT_AND:
            bit_and();
            break;
        case BIT_OR:
            bit_or();
            break;
        case BIT_XOR:
            bit_xor();
            break;
        case BIT_INV:
            bit_inv();
            break;
        case INVOKE_FIRST:
            invoke_top();
            break;
        case RETURN:
            end_proc();
            break;
        case ARG_FLAG:
            std::unreachable();
            // break;
        case JUMP:
        case JUMP_IF_ZERO:
        case JUMP_IF_NOT_ZERO:
        case JUMP_IF_POSITIVE:
        case JUMP_IF_NEGATIVE:
            jump(inst);
            break;
        case LOAD_BOOL:
        case LOAD_CHAR:
        case LOAD_BYTE:
        case LOAD_INT:
        case LOAD_LONG:
        case LOAD_FLOAT:
        case LOAD_DOUBLE:
        case LOAD_REF:
            load(inst);
            break;
        case REF_AT:
            ref_at(inst);
            break;
        case STORE_BOOL:
        case STORE_CHAR:
        case STORE_BYTE:
        case STORE_INT:
        case STORE_LONG:
        case STORE_FLOAT:
        case STORE_DOUBLE:
        case STORE_REF:
            store(inst);
            break;
        case FROM_REF_LOAD_BOOL:
        case FROM_REF_LOAD_CHAR:
        case FROM_REF_LOAD_BYTE:
        case FROM_REF_LOAD_INT:
        case FROM_REF_LOAD_LONG:
        case FROM_REF_LOAD_FLOAT:
        case FROM_REF_LOAD_DOUBLE:
        case FROM_REF_LOAD_REF:
            load_by_ref(inst);
            break;
        case TO_REF_STORE_BOOL:
        case TO_REF_STORE_CHAR:
        case TO_REF_STORE_BYTE:
        case TO_REF_STORE_INT:
        case TO_REF_STORE_LONG:
        case TO_REF_STORE_FLOAT:
        case TO_REF_STORE_DOUBLE:
        case TO_REF_STORE_REF:
            store_by_ref(inst);
            break;
        case REF_MEMBER:
            ref_member(inst);
            break;
        case REF_STATIC_MEMBER:
            ref_static_member(inst);
            break;
        case INVOKE_OVERRIDE:
            invoke_override(inst);
            break;
        case REF_METHOD:
            ref_method(inst);
            break;
        case REF_STATIC_METHOD:
            ref_static_method(inst);
            break;
    }
}

void Thread::waste() noexcept {
    eval_stack.pop();
}

void Thread::cast(const Instruction inst) noexcept {
    Value v = eval_stack.top();
    eval_stack.pop();
    // ReSharper disable CppDFAUnusedValue
    switch (inst.opcode()) {
        case CAST_B2I:
            v = std::get<Bool>(v) ? 1 : 0;
            break;



        case CAST_C2I:
            v = static_cast<Int>(std::get<Char>(v));
            break;



        case CAST_I2B:
            v = std::get<Int>(v) == 1;
            break;
        case CAST_I2C:
            v = static_cast<Char>(std::get<Int>(v));
            break;
        case CAST_I2L:
            v = static_cast<Long>(std::get<Int>(v));
            break;
        case CAST_I2F:
            v = static_cast<Float>(std::get<Int>(v));
            break;
        case CAST_I2D:
            v = static_cast<Double>(std::get<Int>(v));
            break;



        case CAST_L2I:
            v = static_cast<Int>(std::get<Long>(v));
            break;
        case CAST_L2F:
            v = static_cast<Float>(std::get<Long>(v));
            break;
        case CAST_L2D:
            v = static_cast<Double>(std::get<Long>(v));
            break;



        case CAST_F2I:
            v = static_cast<Int>(std::get<Float>(v));
            break;
        case CAST_F2L:
            v = static_cast<Long>(std::get<Float>(v));
            break;
        case CAST_F2D:
            v = static_cast<Double>(std::get<Float>(v));
            break;



        case CAST_D2I:
            v = static_cast<Int>(std::get<Double>(v));
            break;
        case CAST_D2L:
            v = static_cast<Long>(std::get<Double>(v));
            break;
        case CAST_D2F:
            v = static_cast<Float>(std::get<Double>(v));
            break;
        default:
            std::unreachable();
    }
    // ReSharper restore CppDFAUnusedValue
    eval_stack.push(v);
}

void Thread::cmp() noexcept {
    auto top1 = eval_stack.top();
    eval_stack.pop();
    auto top2 = eval_stack.top();
    eval_stack.pop();

    // ReSharper disable once CppDFAUnusedValue
    const auto ret = std::visit([&top2]<typename T>(const T v1) {
        return std::visit([&v1]<typename U>(const U v2) -> std::partial_ordering {
            if constexpr (TypePack<Int, Long, Float, Double>::contains<T>
                    && std::same_as<T, U>)
                return v1 <=> v2;
            else
                std::unreachable();
        }, top2);
    }, top1);

    if (ret < 0)
        eval_stack.emplace(TFlag<Int>, -1);
    else if (ret > 0)
        eval_stack.emplace(TFlag<Int>, 1);
    else
        eval_stack.emplace(TFlag<Int>, 0);
}

void Thread::invoke_top() noexcept {
    const auto top = eval_stack.top();
    eval_stack.pop();
    auto& ref = deref<FFamily>(std::get<VirtualAddress>(top));
    call_function(ref, 0);
}

void Thread::end_proc() noexcept {
    // pop proc
    // ReSharper disable once CppDFAUnusedValue
    const auto last_milestone = current_proc()
            .milestone;
    pop_func(*this);

    // waste data
    thread_page.waste_since(last_milestone);
}

void Thread::jump(const Instruction inst) noexcept {
    Value top;
    switch (inst.opcode()) {
        case JUMP:[[unlikely]]
                    current_proc()
                    .program_counter = inst.arg();
            break;
        case JUMP_IF_ZERO:
            top = eval_stack.top();
            eval_stack.pop();
            if (std::get<int>(top) == 0)
                current_proc()
                        .program_counter = inst.arg();
            break;
        case JUMP_IF_NOT_ZERO:
            top = eval_stack.top();
            eval_stack.pop();
            if (std::get<int>(top) != 0)
                current_proc()
                        .program_counter = inst.arg();
            break;
        case JUMP_IF_POSITIVE:
            top = eval_stack.top();
            eval_stack.pop();
            if (std::get<int>(top) > 0)
                current_proc()
                        .program_counter = inst.arg();
            break;
        case JUMP_IF_NEGATIVE:
            top = eval_stack.top();
            eval_stack.pop();
            if (std::get<int>(top) < 0)
                current_proc()
                        .program_counter = inst.arg();
            break;
        default:[[unlikely]]
                    std::unreachable();
    }
}

FuncContext& Thread::current_proc() noexcept {
    return std::get<FuncContext>(cur_func(*this));
}

VirtualAddress Thread::mem_spec_base() noexcept {
    return current_proc().base_addr;
}


Instruction FuncContext::operator * () const noexcept {
    return process.deref<Array<Instruction>>(instructions)[program_counter];
}

FuncContext & FuncContext::operator ++ () noexcept {
    ++ program_counter;
    return *this;
}



#define BI_VAR_METHOD(NAME, ...)\
void Thread::NAME() noexcept {\
    auto top1 = eval_stack.top();\
    eval_stack.pop();\
    auto top2 = eval_stack.top();\
    eval_stack.pop();\
    auto ret = std::visit([&top2](const auto v1) {\
        return std::visit([v1](const auto v2) -> Value{\
            if constexpr (!TypePack<__VA_ARGS__>::template contains<decltype(v1)>) [[unlikely]] {\
                std::unreachable();\
                return Value{};\
            }\
            else\
                return Value{TFlag<decltype(OP(v1, v2))>, OP(v1, v2)};\
        }, top2);\
    }, top1);\
    eval_stack.push(ret);\
}
#define UNI_VAR_METHOD(NAME, ...)\
void Thread::NAME() noexcept {\
    auto top = eval_stack.top();\
    eval_stack.pop();\
    auto ret = std::visit([](const auto v) {\
        if constexpr (!TypePack<__VA_ARGS__>::template contains<decltype(v)>) [[unlikely]] {\
            std::unreachable();\
            return Value{};\
        }\
        else\
            return Value{TFlag<decltype(OP(v))>, OP(v)};\
    }, top);\
    eval_stack.push(ret);\
}

#define UNI_ALL_METHOD(PREFIX, NAME)\
    void Thread::NAME(Instruction inst) noexcept{\
            G_OP;\
            switch (inst.opcode()) {\
                case PREFIX##_BOOL:OP(Bool);break;\
                case PREFIX##_BYTE:OP(Byte);break;\
                case PREFIX##_CHAR:OP(Char);break;\
                case PREFIX##_INT:OP(Int);break;\
                case PREFIX##_LONG:OP(Long);break;\
                case PREFIX##_FLOAT:OP(Float);break;\
                case PREFIX##_DOUBLE:OP(Double);break;\
                case PREFIX##_REF:OP(VirtualAddress);break;\
                default:[[unlikely]]\
                    std::unreachable();\
            }\
        }

#define OP(v1, v2) (v1 + v2)
BI_VAR_METHOD(add, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) (v1 - v2)
BI_VAR_METHOD(sub, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) (v1 * v2)
BI_VAR_METHOD(mul, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) (v1 / v2)
BI_VAR_METHOD(div, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) (v1 % v2)
BI_VAR_METHOD(rem, Int, Long)
#undef OP
#define OP(v) (-v)
UNI_VAR_METHOD(neg, Int, Long)
#undef OP
#define OP(v1, v2) (v1 << v2)
BI_VAR_METHOD(shl, Int, Long)
#undef OP
#define OP(v1, v2) (v1 >> v2)
BI_VAR_METHOD(shr, Int, Long)
#undef OP
#define OP(v1, v2) (::ushr(v1, v2))
BI_VAR_METHOD(ushr, Bool, Int, Long)
#undef OP
#define OP(v1, v2) (v1 & v2)
BI_VAR_METHOD(bit_and, Bool, Int, Long)
#undef OP
#define OP(v1, v2) (v1 | v2)
BI_VAR_METHOD(bit_or, Bool, Int, Long)
#undef OP
#define OP(v1, v2) (v1 ^ v2)
BI_VAR_METHOD(bit_xor, Bool, Int, Long)
#undef OP
#define OP(v) (~v)
UNI_VAR_METHOD(bit_inv, Int, Long)
#undef OP

#define G_OP
#define OP(TYPE) {\
            auto& top = deref<TYPE>(mem_spec_base().id_shift(inst.arg()));\
            eval_stack.emplace(TFlag<TYPE>, top);\
        }
UNI_ALL_METHOD(LOAD, load)
#undef OP


#define OP(TYPE) {\
            auto v = std::get<TYPE>(eval_stack.top());\
            eval_stack.pop();\
            deref<TYPE>(mem_spec_base().id_shift(inst.arg())) = v;\
        }
UNI_ALL_METHOD(STORE, store)

void Thread::ref_at(const Instruction inst) noexcept {
    eval_stack.emplace(TFlag<VirtualAddress>, mem_spec_base().id_shift(inst.arg()));
}
#undef OP
#undef G_OP

#define G_OP auto target = std::get<VirtualAddress>(eval_stack.top());\
    eval_stack.pop()
#define OP(TYPE) {\
            eval_stack.emplace(TFlag<TYPE>, deref<TYPE>(target));\
        }
UNI_ALL_METHOD(FROM_REF_LOAD, load_by_ref)
#undef OP
#define OP(TYPE) {\
            deref<TYPE>(target) = std::get<TYPE>(eval_stack.top());\
            eval_stack.pop();\
        }
UNI_ALL_METHOD(TO_REF_STORE, store_by_ref)
#undef OP
#undef G_OP


void Thread::ref_member(
    const Instruction inst
) noexcept {
    const auto offset = deref<Array<MemberInfo>>(vm__top_obj_type().members)[inst.arg()].offset;
    const auto obj_base_addr = std::get<VirtualAddress>(eval_stack.top());
    eval_stack.pop();
    eval_stack.emplace(TFlag<VirtualAddress>, obj_base_addr.offset_shift(static_cast<std::uint16_t>(offset)));
}

void Thread::ref_static_member(
    const Instruction inst
) noexcept {
    const auto member_addr = deref<Array<VirtualAddress>>(vm__top_obj_type().members)[inst.arg()];
    eval_stack.pop();
    eval_stack.emplace(TFlag<VirtualAddress>, member_addr);
}

void Thread::ref_method(const Instruction inst) noexcept {
    const auto& cls = vm__top_obj_type();
    const auto& method_ref = deref<Array<VirtualAddress>>(cls.methods)[inst.arg()];
    eval_stack.emplace(TFlag<VirtualAddress>, method_ref);
}

void Thread::ref_static_method(const Instruction inst) noexcept {
    const auto& cls = vm__top_obj_type();
    const auto& method_ref = deref<Array<VirtualAddress>>(cls.methods)[inst.arg()];
    // pop 'this', so static method won't receive it
    eval_stack.pop();
    eval_stack.emplace(TFlag<VirtualAddress>, method_ref);
}

void Thread::invoke_override(const Instruction inst) noexcept {
    const auto ref = std::get<VirtualAddress>(eval_stack.top());
    eval_stack.pop();
    auto& family = deref<FFamily>(ref);
    call_function(family, inst.arg());
}

void Thread::instate(const Instruction inst) noexcept {
    const auto& cls = deref<Class>(std::get<VirtualAddress>(
        eval_stack.top()
    ));
    eval_stack.pop();
    call_function(deref<FFamily>(cls.maker), inst.arg());

}

void Thread::destroy() noexcept {
    const auto& cls = vm__top_obj_type();
    call_function(deref<FFamily>(cls.collector), 0);
}


NtvFunc FOverride::ntv() const noexcept {
    return reinterpret_cast<NtvFunc>(
        target_addr
    );
}

VirtualAddress FOverride::vm() const noexcept {
    return static_cast<VirtualAddress>(target_addr);
}
