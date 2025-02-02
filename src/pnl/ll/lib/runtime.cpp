//
// Created by FCWY on 25-1-8.
//
// ReSharper disable CppDFANullDereference
// ReSharper disable CppDFAUnreadVariable
// ReSharper disable CppDFAUnusedValue
module;
#include "project-nl.h"
module pnl.ll.runtime;
using namespace pnl::ll::runtime;
using namespace pnl::ll::runtime::load;
using namespace pnl::ll::runtime::execute;



std::uint8_t VirtualAddress::page() const noexcept {
    return (content  >> 48) & 0xf;
}

std::uint32_t VirtualAddress::id() const noexcept {
    return (content >> 16) & 0xffffffff;
}

std::uint16_t VirtualAddress::offset() const noexcept {
    return content & 0xffff;
}

VirtualAddress VirtualAddress::id_shift(const std::uint32_t shift) const noexcept {
    return {page(), id() + shift, offset()};
}

VirtualAddress VirtualAddress::offset_shift(const std::uint16_t shift) const noexcept {
    return {page(), id(), static_cast<std::uint16_t>(offset() + shift)};
}

bool VirtualAddress::is_native() const noexcept {
    return page() == native_page_id;
}

bool VirtualAddress::is_reserved() const noexcept {
    return page() == invalid_page_id;
}

bool VirtualAddress::is_process() const noexcept {
    return page() == process_page_id;
}

std::strong_ordering VirtualAddress::operator<=>(const VirtualAddress other) const noexcept {
    return content <=> other.content;
}

[[nodiscard]]
static Class& vm__top_obj_type(Thread&) noexcept;
static auto&
    cur_func(Thread& thr) noexcept {
    return thr.call_deque.front();
}
static void
pop_func(Thread& thr) noexcept {
    return thr.call_deque.pop_front();
}
static void cast(Thread&, Instruction) noexcept;
static void cmp(Thread&) noexcept;
static void invoke_top(Thread&) noexcept;
static void jump(Thread&, Instruction) noexcept;
static void add(Thread&) noexcept;
static void sub(Thread&) noexcept;
static void mul(Thread&) noexcept;
static void div(Thread&) noexcept;
static void rem(Thread&) noexcept;
static void neg(Thread&) noexcept;
static void shl(Thread&) noexcept;
static void shr(Thread&) noexcept;
static void ushr(Thread&) noexcept;
static void bit_and(Thread&) noexcept;
static void bit_or(Thread&) noexcept;
static void bit_xor(Thread&) noexcept;
static void bit_inv(Thread&) noexcept;
static void p_load(Thread&, Instruction) noexcept;
static void p_store(Thread&, Instruction) noexcept;
static void f_load(Thread&, Instruction) noexcept;
static void f_store(Thread&, Instruction) noexcept;
static void stack_alloc(Thread&) noexcept;
static void stack_new(Thread&) noexcept;
static void end_proc(Thread&) noexcept;
static void wild_alloc(Thread&) noexcept;
static void wild_new(Thread&) noexcept;
static void wild_collect(Thread&) noexcept;
static void p_ref_at(Thread&, Instruction) noexcept;
static void f_ref_at(Thread&, Instruction) noexcept;
static void load_by_ref(Thread&, Instruction) noexcept;
static void store_by_ref(Thread&, Instruction) noexcept;
static void ref_member(Thread&, Instruction) noexcept;
static void ref_static_member(Thread&, Instruction) noexcept;
static void ref_method(Thread&, Instruction) noexcept;
static void ref_static_method(Thread&, Instruction) noexcept;
static void invoke_override(Thread&, Instruction) noexcept;
static void instate(Thread&, Instruction) noexcept;
static void destroy(Thread&) noexcept;


template<typename T>
static T ushr(const T a, const T b) noexcept {
    auto ret = a >> b;
    if (a < 0)
        ret &= ~(1 << (sizeof(T) - 1));
    return ret;
}


Process::Process(MManager* const upstream):
    process_memory{
        upstream
    }{
    // construct main thread
    threads.emplace_back(this, 0);
}

void Process::pause() const noexcept {
    for (auto& thr: threads)
        thr.pause();
}

void Process::resume() const noexcept {
    for (auto& thr: threads)
        thr.resume();
}

void Process::terminate() noexcept {
    for (auto& thr: threads)
        thr.terminate();
}

VirtualAddress Process::get_vr(const std::uint8_t thread_id, const std::uint32_t object_offset) noexcept {
    return VirtualAddress{thread_id, object_offset};
}

Thread & Process::get_available_thread() noexcept {
    for (auto& thr: threads)
        if (thr.call_deque.empty())
            return thr;
    threads.emplace_back(this, static_cast<std::uint8_t>(threads.size()));
    return threads.back();
}

Thread & Process::main_thread() noexcept {
    return threads.at(0);
}

VirtualAddress Process::wild_alloc(const Int size) noexcept {
    for (auto [idx, elem]: native_page | std::views::enumerate)
        if (elem.first == nullptr) {
            elem.first = static_cast<UByte*>(process_memory.allocate(size));
            elem.second = size;
            return {VirtualAddress::native_page_id, static_cast<std::uint32_t>(idx)};
        }

    native_page.emplace_back(
        static_cast<UByte*>(process_memory.allocate(size)),
        static_cast<std::uint32_t>(size)
    );
    return {VirtualAddress::native_page_id, static_cast<std::uint32_t>(native_page.size() - 1)};
}

void Process::wild_collect(const VirtualAddress va) noexcept {
    if (va.is_reserved())
        return;
    assert(va.is_native(), "trying to collect a non-wild-alloced address");
    assert(va.id() < native_page.size(), "address invalid");
    auto& [ptr, size] = native_page[va.id()];
    process_memory.deallocate(ptr, size);
    ptr = nullptr;

    shrink_if_necessary(native_page);
}

Class& vm__top_obj_type(Thread& thr) noexcept {
    const auto& [type] = thr.deref<RTTObject>(std::get<VirtualAddress>(thr.eval_deque.front()));
    return thr.deref<Class>(type);
}

Thread::Thread(
    Process *process,
    const std::uint8_t thread_id) noexcept:
    step_token{0},
    process{process},
    thread_id{thread_id},
    call_deque{&process->process_memory},
    eval_deque{&process->process_memory},
    thread_page{&process->process_memory},
    native_handler{std::function{[this](const std::stop_token &token){
        run(token);
    }}}{}

Thread::Thread(Thread &&other) noexcept:
    step_token{0},
    process(other.process),
    thread_id(other.thread_id),
    call_deque(std::move(other.call_deque)),
    eval_deque(std::move(other.eval_deque)),
    thread_page(std::move(other.thread_page)),
    native_handler(std::move(other.native_handler)) {
}

FuncContext& Thread::current_proc() noexcept {
    return std::get<FuncContext>(cur_func(*this));
}

void Thread::pause() const noexcept {
    step_token.acquire();
}

void Thread::resume() const noexcept {
    step_token.release();
}

void Thread::terminate() noexcept {
    if (native_handler.joinable()) {
        native_handler.get_stop_source().request_stop();
        auto [[maybe_unused]]_ = step_token.try_acquire();
        step_token.release();
        native_handler.join();
    }
}

Thread::~Thread() noexcept { // NOLINT(*-use-equals-default)
    terminate();
}

void Thread::call_function(const FFamily &family, const std::uint32_t override_id) noexcept {
    if (const auto& override = deref<Array<FOverride>>(family.overrides)[override_id];
        override.is_native) [[unlikely]]
        override.ntv()(*this);
    else
        call_deque.emplace_front(
            TFlag<FuncContext>,
            thread_page.milestone(),
            family.base_addr,
            override.vm()
        );
}

void Thread::clear() noexcept {
    call_deque.clear();
    eval_deque.clear();
    thread_page.waste_since(0);
}

VirtualAddress Thread::process_memory_base() noexcept {
    return current_proc().base_addr;
}

VirtualAddress Thread::function_memory_base() noexcept {
    return {thread_id, current_proc().milestone};
}

Value Thread::take() noexcept {
    auto top = std::move(eval_deque.front());
    eval_deque.pop_front();
    return std::move(top);
}

void Thread::run(const std::stop_token &token) noexcept {
    step_token.acquire();
    step_token.release();
    while (true){
        if (is_daemon && token.stop_requested())
            break;
        if (call_deque.empty())
            break;

        step_token.acquire();
        if (cur_func(*this).index() != 0) [[unlikely]] {
            const auto ntv_proc = std::get<NtvFunc>(cur_func(*this));
            pop_func(*this);
            ntv_proc(*this);
        }
        else [[likely]]
            step();
        step_token.release();
    }
    is_done = true;
}

#define INTEGRAL(prefix) \
prefix##_INT:        \
case prefix##_LONG

#define COMPUTABLE(prefix)  \
INTEGRAL(prefix):       \
case prefix##_FLOAT:         \
case prefix##_DOUBLE

#define ALL(prefix)  \
prefix##_BOOL:       \
case prefix##_CHAR:       \
case prefix##_BYTE:       \
case COMPUTABLE(prefix):  \
case prefix##_REF

using enum OPCode;
void Thread::step() noexcept {
    const auto inst = current_proc().decode(*process);
    ++ current_proc();
    switch (inst.opcode()) {
        case NOP:
            break;
        case WASTE:
            eval_deque.pop_front();
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
            cast(*this, inst);
            break;
        case CMP:
            cmp(*this);
            break;
        case ADD:
            add(*this);
            break;
        case SUB:
            sub(*this);
            break;
        case MUL:
            mul(*this);
            break;
        case DIV:
            div(*this);
            break;
        case REM:
            rem(*this);
            break;
        case NEG:
            neg(*this);
            break;
        case SHL:
            shl(*this);
            break;
        case SHR:
            shr(*this);
            break;
        case USHR:
            ushr(*this);
            break;
        case BIT_AND:
            bit_and(*this);
            break;
        case BIT_OR:
            bit_or(*this);
            break;
        case BIT_XOR:
            bit_xor(*this);
            break;
        case BIT_INV:
            bit_inv(*this);
            break;
        case INVOKE_FIRST:
            invoke_top(*this);
            break;
        case RETURN:
            end_proc(*this);
            break;
        case STACK_ALLOC:
            stack_alloc(*this);
            break;
        case STACK_NEW:
            stack_new(*this);
            break;
        case WILD_ALLOC:
            wild_alloc(*this);
            break;
        case WILD_NEW:
            wild_new(*this);
            break;
        case WILD_COLLECT:
            wild_collect(*this);
            break;
        case DESTROY:
            destroy(*this);
            break;
        case ARG_FLAG:
            std::unreachable();
        case JUMP:
        case JUMP_IF_ZERO:
        case JUMP_IF_NOT_ZERO:
        case JUMP_IF_POSITIVE:
        case JUMP_IF_NEGATIVE:
            jump(*this, inst);
            break;
        case ALL(P_LOAD):
            p_load(*this, inst);
            break;
        case ALL(F_LOAD):
            f_load(*this, inst);
            break;
        case P_REF_AT:
            p_ref_at(*this, inst);
            break;
        case T_REF_AT:
            f_ref_at(*this, inst);
            break;
        case ALL(P_STORE):
            p_store(*this, inst);
            break;
        case ALL(F_STORE):
            f_store(*this, inst);
            break;
        case ALL(FROM_REF_LOAD):
            load_by_ref(*this, inst);
            break;
        case ALL(TO_REF_STORE):
            store_by_ref(*this, inst);
            break;
        case REF_MEMBER:
            ref_member(*this, inst);
            break;
        case REF_STATIC_MEMBER:
            ref_static_member(*this, inst);
            break;
        case INVOKE_OVERRIDE:
            invoke_override(*this, inst);
            break;
        case REF_METHOD:
            ref_method(*this, inst);
            break;
        case REF_STATIC_METHOD:
            ref_static_method(*this, inst);
            break;
        case INSTATE:
            instate(*this, inst);
            break;
    }
}



Instruction FuncContext::decode(Process& process) const noexcept {
    return process.deref<Array<Instruction>>(instructions)[program_counter];
}

FuncContext & FuncContext::operator ++ () noexcept {
    ++ program_counter;
    return *this;
}

void stack_alloc(Thread& thr) noexcept {
    auto base = VirtualAddress{
        thr.thread_id,
        thr.thread_page.milestone()
    };
    thr.thread_page.placeholder_push(std::get<Int>(thr.take()));
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, base);
}
void stack_new(Thread& thr) noexcept {
    auto base = VirtualAddress{
        thr.thread_id,
        thr.thread_page.milestone()
    };
    thr.thread_page.placeholder_push(std::get<Int>(thr.take()));
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, base);
}



void cast(Thread& thr, const Instruction inst) noexcept {
    Value v = thr.take();
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
    thr.eval_deque.emplace_front(v);
}

void cmp(Thread& thr) noexcept {
    const auto top1 = thr.take();

    // ReSharper disable once CppDFAUnusedValue
    const auto ret = std::visit([top2=thr.take()]<typename T>(const T v1) {
        return std::visit([&v1]<typename U>(const U v2) -> std::partial_ordering {
            if constexpr (TypePack<Int, Long, Float, Double>::contains<T>
                    && std::same_as<T, U>)
                return v1 <=> v2;
            else
                std::unreachable();
        }, top2);
    }, top1);

    if (ret < 0)
        thr.eval_deque.emplace_front(TFlag<Int>, -1);
    else if (ret > 0)
        thr.eval_deque.emplace_front(TFlag<Int>, 1);
    else
        thr.eval_deque.emplace_front(TFlag<Int>, 0);
}

void invoke_top(Thread& thr) noexcept {
    const auto place = std::get<VirtualAddress>(thr.take());
    const auto& ref = thr.deref<FFamily>(place);
    thr.call_function(ref, 0);
}

void end_proc(Thread& thr) noexcept {
    // pop proc
    // ReSharper disable once CppDFAUnusedValue
    const auto last_milestone = thr.current_proc()
            .milestone;
    pop_func(thr);

    // waste data
    thr.thread_page.waste_since(last_milestone);
}

void wild_alloc(Thread& thr) noexcept {
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>,
        thr.process->wild_alloc(
            std::get<Int>(thr.take())
        )
    );
}

void wild_new(Thread& thr) noexcept {
    const auto& tp = thr.deref<Class>(std::get<VirtualAddress>(thr.take()));
    // ReSharper disable once CppDFAUnreadVariable
    // ReSharper disable once CppDFAUnusedValue
    const auto o_id = std::get<Int>(thr.take());

    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, thr.process->wild_alloc(tp.super.super.instance_size));
    thr.call_function(thr.deref<FFamily>(tp.super.super.maker), o_id);
}

void wild_collect(Thread& thr) noexcept {
    thr.process -> wild_collect(std::get<VirtualAddress>(thr.take()));
}

void jump(Thread& thr, const Instruction inst) noexcept {
    Value top;
    switch (inst.opcode()) {
        case JUMP:[[unlikely]]
                    thr.current_proc()
                    .program_counter = inst.arg();
            break;
        case JUMP_IF_ZERO:
            top = thr.take();
            if (std::get<int>(top) == 0)
                thr.current_proc()
                        .program_counter = inst.arg();
            break;
        case JUMP_IF_NOT_ZERO:
            top = thr.take();
            if (std::get<int>(top) != 0)
                thr.current_proc()
                        .program_counter = inst.arg();
            break;
        case JUMP_IF_POSITIVE:
            top = thr.take();
            if (std::get<int>(top) > 0)
                thr.current_proc()
                        .program_counter = inst.arg();
            break;
        case JUMP_IF_NEGATIVE:
            top = thr.take();
            if (std::get<int>(top) < 0)
                thr.current_proc()
                        .program_counter = inst.arg();
            break;
        default:[[unlikely]]
            std::unreachable();
    }
}



#define BI_VAR_METHOD(NAME, ...)\
void NAME(Thread& thr) noexcept {\
    auto top1 = thr.take();\
    auto top2 = thr.take();\
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
    thr.eval_deque.emplace_front(ret);\
}
#define UNI_VAR_METHOD(NAME, ...)\
void NAME(Thread& thr) noexcept {\
    auto top = thr.take();\
    auto ret = std::visit([](const auto v) {\
        if constexpr (!TypePack<__VA_ARGS__>::template contains<decltype(v)>) [[unlikely]] {\
            std::unreachable();\
            return Value{};\
        }\
        else\
            return Value{TFlag<decltype(OP(v))>, OP(v)};\
    }, top);\
    thr.eval_deque.emplace_front(ret);\
}

#define UNI_ALL_METHOD(PREFIX, NAME)\
    void NAME(Thread& thr, Instruction inst) noexcept{\
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

#define OP(v1, v2) ((v1) + (v2))
BI_VAR_METHOD(add, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) ((v1) - (v2))
BI_VAR_METHOD(sub, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) ((v1) * (v2))
BI_VAR_METHOD(mul, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) ((v1) / (v2))
BI_VAR_METHOD(div, Int, Long, Float, Double)
#undef OP
#define OP(v1, v2) ((v1) % (v2))
BI_VAR_METHOD(rem, Int, Long)
#undef OP
#define OP(v) (-(v))
UNI_VAR_METHOD(neg, Int, Long)
#undef OP
#define OP(v1, v2) ((v1) << (v2))
BI_VAR_METHOD(shl, Int, Long)
#undef OP
#define OP(v1, v2) ((v1) >> (v2))
BI_VAR_METHOD(shr, Int, Long)
#undef OP
#define OP(v1, v2) (::ushr(v1, v2))
BI_VAR_METHOD(ushr, Bool, Int, Long)
#undef OP
#define OP(v1, v2) ((v1) & (v2))
BI_VAR_METHOD(bit_and, Bool, Int, Long)
#undef OP
#define OP(v1, v2) ((v1) | (v2))
BI_VAR_METHOD(bit_or, Bool, Int, Long)
#undef OP
#define OP(v1, v2) ((v1) ^ (v2))
BI_VAR_METHOD(bit_xor, Bool, Int, Long)
#undef OP
#define OP(v) (~(v))
UNI_VAR_METHOD(bit_inv, Int, Long)
#undef OP

#define G_OP
#define OP(TYPE) {\
            auto& top = thr.deref<TYPE>(thr.process_memory_base().id_shift(inst.arg()));\
            thr.eval_deque.emplace_front(TFlag<TYPE>, top);\
        }
UNI_ALL_METHOD(P_LOAD, p_load)
#undef OP
#define OP(TYPE) {\
            auto v = std::get<TYPE>(thr.take());\
            thr.deref<TYPE>(thr.process_memory_base().id_shift(inst.arg())) = v;\
        }
UNI_ALL_METHOD(P_STORE, p_store)
#undef OP
#define OP(TYPE) {\
            auto& top = thr.deref<TYPE>(thr.function_memory_base().id_shift(inst.arg()));\
            thr.eval_deque.emplace_front(TFlag<TYPE>, top);\
        }
UNI_ALL_METHOD(F_LOAD, f_load)
#undef OP
#define OP(TYPE) {\
            auto v = std::get<TYPE>(thr.take());\
            thr.deref<TYPE>(thr.function_memory_base().id_shift(inst.arg())) = v;\
        }
UNI_ALL_METHOD(F_STORE, f_store)
#undef OP
#undef G_OP


void p_ref_at(Thread& thr, const Instruction inst) noexcept {
    const auto tgt = thr.process_memory_base().id_shift(inst.arg());
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, tgt);
}


void f_ref_at(Thread& thr, const Instruction inst) noexcept {
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, thr.function_memory_base().id_shift(inst.arg()));
}


#define G_OP auto target = std::get<VirtualAddress>(thr.take());
#define OP(TYPE) thr.eval_deque.emplace_front(TFlag<TYPE>, thr.deref<TYPE>(target));
UNI_ALL_METHOD(FROM_REF_LOAD, load_by_ref)
#undef OP
#define OP(TYPE) thr.deref<TYPE>(target) = std::get<TYPE>(thr.take());
UNI_ALL_METHOD(TO_REF_STORE, store_by_ref)
#undef OP
#undef G_OP


void ref_member(
    Thread& thr, const Instruction inst
) noexcept {
    const auto offset = thr.deref<Array<MemberInfo>>(vm__top_obj_type(thr).members)[inst.arg()].offset;
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>,
        std::get<VirtualAddress>(thr.take())
        .offset_shift(static_cast<std::uint16_t>(offset)));
}

void ref_static_member(
    Thread& thr, const Instruction inst
) noexcept {
    const auto& cls = vm__top_obj_type(thr);
    const auto& member_offset = thr.deref<Array<VirtualAddress>>(cls.members)[inst.arg()].offset();
    const auto base = std::get<VirtualAddress>(thr.take());
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, base.offset_shift(member_offset));
}

void ref_method(Thread& thr, const Instruction inst) noexcept {
    const auto& cls = vm__top_obj_type(thr);
    const auto& method_ref = thr.deref<Array<VirtualAddress>>(cls.methods)[inst.arg()];
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, method_ref);
}

void ref_static_method(Thread& thr, const Instruction inst) noexcept {
    const auto& cls = vm__top_obj_type(thr);
    const auto& method_ref = thr.deref<Array<VirtualAddress>>(cls.methods)[inst.arg()];
    // pop 'this', so static method won't receive it
    thr.eval_deque.pop_front();
    thr.eval_deque.emplace_front(TFlag<VirtualAddress>, method_ref);
}

void invoke_override(Thread& thr, const Instruction inst) noexcept {
    const auto& family = thr.deref<FFamily>(std::get<VirtualAddress>(thr.take()));
    thr.call_function(family, inst.arg());
}

void instate(Thread& thr, const Instruction inst) noexcept {
    const auto& cls = thr.deref<Class>(std::get<VirtualAddress>(
        thr.take()
    ));
    thr.call_function(thr.deref<FFamily>(cls.super.super.maker), inst.arg());
}

void destroy(Thread& thr) noexcept {
    const auto& cls = vm__top_obj_type(thr);
    thr.call_function(thr.deref<FFamily>(cls.super.super.collector), 0);
}


NtvFunc FOverride::ntv() const noexcept {
    return reinterpret_cast<NtvFunc>(
        target_addr
    );
}

VirtualAddress FOverride::vm() const noexcept {
    return static_cast<VirtualAddress>(target_addr);
}
