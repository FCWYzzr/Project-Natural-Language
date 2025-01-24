//
// Created by FCWY on 25-1-21.
//
module;
#include <antlr4-runtime.h>
#include <nlsmLexer.h>
#include <nlsmParser.h>
#include <nlsmBaseVisitor.h>
#include <any>
#include <functional>
#include "project-nl.h"
#ifdef assert
#undef assert
#endif
module nlsm.grammar;
using namespace nlsm::grammar;
using namespace antlr4;
using namespace antlrcpp;
using namespace pnl::ll;
using namespace std::views;
using namespace std::ranges;

struct NLSMBuilder final : nlsmBaseVisitor {
    using Repr = Pair<std::optional<Str>, Package::Content>;
    using ClsMemSlot = Pair<Int, std::variant<Pair<Str, ObjRefRepr>, ObjRefRepr>>;
    MManager* const mem;
    const Str prefix;
    Str cur_cls;

    explicit NLSMBuilder(MManager& res, const Str& prefix) noexcept:
        mem{&res}, prefix{build_str(&res, prefix, VM_TEXT("::"))}, cur_cls{&res} {}

    std::any visitPackage(nlsmParser::PackageContext *ctx) override {
        auto data = List<Package::Content>{mem};
        auto exports = Map<USize, ObjRefRepr>{mem};

        for (auto idx = 0;
            const auto content_ctx:
            ctx->package_content()) {
            auto [maybe_name, content] = std::any_cast<Repr>(visitPackage_content(content_ctx));
            if (maybe_name.has_value()) {
                const auto& name = maybe_name.value();
                if (name.empty()) [[unlikely]] {
                    if (content.index() == 7) [[unlikely]]
                        exports.emplace(
                            idx,
                            std::get<7>(content).name
                        );
                    else if (content.index() == 10) [[likely]]
                        exports.emplace(
                            idx,
                            std::get<10>(content).name
                        );
                    else
                        assert(false, "only types can be anonymously exported");
                }
                else
                    exports.emplace(idx, std::move(name));
            }

            data.emplace_back(std::move(content));
            idx ++;
        }

        return std::any{Package{std::move(data), std::move(exports)}};
    }

    std::any visitPackage_content(nlsmParser::Package_contentContext *ctx) override {
        if (ctx->EXPORT() == nullptr && ctx->STRING() == nullptr) [[likely]]
            return std::any{Repr{std::nullopt, std::any_cast<Package::Content>(visitValue(ctx->value()))}};

        if (ctx->STRING() == nullptr)
            return std::any{Repr{{Str{mem}}, std::any_cast<Package::Content>(visitValue(ctx->value()))}};

        const auto export_name = cvt(MBStr{ctx->STRING()->getText(), mem});
        return std::any{Repr{
            {export_name.substr(1, export_name.size() - 2)},
            std::any_cast<Package::Content>(visitValue(ctx->value()))
        }};
    }

    std::any visitValue(nlsmParser::ValueContext *ctx) override {
#define OP(NAME, UP_NAME) \
        if (ctx-> NAME##_value())\
            return visit##UP_NAME##_value(ctx-> NAME##_value())

        OP(bool, Bool);
        OP(byte, Byte);
        OP(char, Char);
        OP(int, Int);
        OP(long, Long);
        OP(float, Float);
        OP(double, Double);
        OP(reference, Reference);
        OP(char_array, Char_array);
        OP(named_type, Named_type);
        OP(function, Function);
        OP(class, Class);
        OP(object, Object);
#undef OP
        std::unreachable();
    }

    std::any visitBool_value(nlsmParser::Bool_valueContext *ctx) override {
        return {Package::Content{TFlag<Bool>, ctx->FALSE() == nullptr}};
    }

    std::any visitByte_value(nlsmParser::Byte_valueContext *ctx) override {
        const auto parsed = strtol(ctx->DECIMAL()->getText().c_str(), nullptr, 10);
        assert(-128 <= parsed && parsed <= 127, "byte literal out of range");
        return {Package::Content{TFlag<Byte>, static_cast<Byte>(parsed)}};
    }

    std::any visitChar_value(nlsmParser::Char_valueContext *ctx) override {
        const auto parsed = cvt(ctx->CHAR()->getText(), *mem);
        return {Package::Content{TFlag<Char>, parsed[1]}};
    }

    std::any visitInt_value(nlsmParser::Int_valueContext *ctx) override {
        const auto parsed = strtoll(ctx->DECIMAL()->getText().c_str(), nullptr, 10);
        assert(-2147483648 <= parsed && parsed <= 2147483647, "int literal out of range");
        return {Package::Content{TFlag<Int>, static_cast<Int>(parsed)}};
    }

    std::any visitLong_value(nlsmParser::Long_valueContext *ctx) override {
        const auto parsed = strtoll(ctx->DECIMAL()->getText().c_str(), nullptr, 10);
        return {Package::Content{TFlag<Long>, static_cast<Long>(parsed)}};
    }

    std::any visitFloat_value(nlsmParser::Float_valueContext *ctx) override {
        if (ctx->DECIMAL() != nullptr) [[unlikely]]{
            const auto parsed = strtoll(ctx->DECIMAL()->getText().c_str(), nullptr, 10);
            return {Package::Content{TFlag<Float>, static_cast<Float>(parsed)}};
        }
        const auto parsed = strtof(ctx->FLOATING()->getText().c_str(), nullptr);
        return {Package::Content{TFlag<Float>, parsed}};
    }

    std::any visitDouble_value(nlsmParser::Double_valueContext *ctx) override {
        if (ctx->DECIMAL() != nullptr) [[unlikely]]{
            const auto parsed = strtoll(ctx->DECIMAL()->getText().c_str(), nullptr, 10);
            return {Package::Content{TFlag<Double>, static_cast<Double>(parsed)}};
        }
        const auto parsed = strtod(ctx->FLOATING()->getText().c_str(), nullptr);
        return {Package::Content{TFlag<Double>, parsed}};
    }

    std::any visitReference_value(nlsmParser::Reference_valueContext *ctx) override {
        const auto parsed = cvt(ctx->STRING()->getText(), *mem);
        return {Package::Content{TFlag<CharArrayRepr>, parsed.substr(1, parsed.length() - 2)}};
    }

    std::any visitChar_array_value(nlsmParser::Char_array_valueContext *ctx) override {
        const auto parsed = cvt(ctx->STRING()->getText(), *mem);
        return {Package::Content{TFlag<CharArrayRepr>, parsed.substr(1, parsed.length() - 2)}};
    }

    std::any visitNamed_type_value(nlsmParser::Named_type_valueContext *ctx) override {
        const auto size = strtol(ctx->DECIMAL()->getText().data(), nullptr, 10);
        assert(size >= 0, build_n_str(mem, "size must be non-negative: ", ctx->STRING()->getText()));
        auto name = cvt(ctx->STRING()->getText(), *mem);
        name = name.substr(1, name.size() - 2);
        auto maker = (prefix + name) += VM_TEXT("::()");
        auto collector = (prefix + name) += VM_TEXT("::~()");
        return {Package::Content{TFlag<NamedTypeRepr>,
            std::move(name),
            static_cast<Long>(size),
            std::move(maker),
            std::move(collector)
        }};
    }

    std::any visitFunction_value(nlsmParser::Function_valueContext *ctx) override {
        auto func = FFamilyRepr{mem};

        for (const auto over: ctx->override_value())
            func.emplace_back(std::any_cast<FOverrideRepr>(visitOverride_value(over)));

        return {Package::Content{TFlag<FFamilyRepr>, std::move(func)}};
    }

    std::any visitOverride_value(nlsmParser::Override_valueContext *ctx) override {
        auto param = std::any_cast<List<ObjRefRepr>>(visitParams(ctx->params()));
        auto ret = std::any_cast<ObjRefRepr>(visitRet(ctx->ret()));
        if (ctx->STRING() != nullptr) {
            const auto id = ctx->STRING()->getText();
            return {FOverrideRepr{
                std::move(param),
                std::move(ret),
                FOverrideRepr::ImplRepr{TFlag<NtvId>, id.substr(1, id.size() - 2), mem}
            }};
        }


        auto insts = List<Instruction>{mem};
        for (const auto cmd: ctx->command())
            insts.emplace_back(
                std::any_cast<Instruction>(visitCommand(cmd))
            );

        return {FOverrideRepr{
            std::move(param),
            std::move(ret),
            FOverrideRepr::ImplRepr{TFlag<List<Instruction>>,
                std::move(insts)
            }
        }};

    }
    std::any visitParams(nlsmParser::ParamsContext *context) override {
        auto param = List<ObjRefRepr>{mem};
        if (context != nullptr)
            for (const auto tp: context->STRING()) {
                const auto quoted = cvt(tp->getText(), *mem);
                param.emplace_back(quoted.substr(1, quoted.size() - 2));
            }
        return {std::move(param)};
    }

    std::any visitRet(nlsmParser::RetContext *context) override {
        if (context == nullptr)
            return {Str{VM_TEXT("::unit"), mem}};
        const auto name = cvt(context->STRING()->getText(), *mem);
        return {name.substr(1, name.size() - 2)};
    }

    std::any visitClass_value(nlsmParser::Class_valueContext *ctx) override {
        auto name = cvt(ctx->STRING()->getText(), *mem);
        name = name.substr(1, name.size() - 2);
        cur_cls = (prefix + name) += VM_TEXT("::");
        auto maker = cur_cls + VM_TEXT("()");
        auto collector = cur_cls + VM_TEXT("~()");
        auto members = List<Pair<Str, ObjRefRepr>>{mem};
        auto methods = List<ObjRefRepr>{mem};
        auto s_members = List<ObjRefRepr>{mem};
        auto s_methods = List<ObjRefRepr>{mem};

        if (ctx->super() != nullptr)
            members.emplace_back(VM_TEXT("__super"), std::any_cast<ObjRefRepr>(visitSuper(ctx->super())));
        else if (ctx->INVISIBLE() == nullptr)
            members.emplace_back(VM_TEXT("__super"), VM_TEXT("::RTTObject"));

        for (const auto mbr: ctx->class_content())
            switch (auto [idx, v] = std::any_cast<ClsMemSlot>(visitClass_content(mbr));
                idx) {
                case 0:
                    members.emplace_back(std::move(std::get<0>(v)));
                break;
                case 1:
                    methods.emplace_back(std::move(std::get<1>(v)));
                break;
                case 2:
                    s_members.emplace_back(std::move(std::get<1>(v)));
                break;
                case 3:
                    s_methods.emplace_back(std::move(std::get<1>(v)));
                break;
                default:
                    std::unreachable();
            }

        return {Package::Content{TFlag<ClassRepr>,
            std::move(name),
            std::move(maker),
            std::move(collector),
            std::move(members),
            std::move(methods),
            std::move(s_members),
            std::move(s_methods)
        }};
    }

    std::any visitSuper(nlsmParser::SuperContext *context) override {
        const auto name = cvt(context->STRING()->getText(), *mem);
        return {name.substr(1, name.size() - 2)};
    }

    std::any visitClass_content(nlsmParser::Class_contentContext *ctx) override {
        if (ctx->member() != nullptr)
            return ClsMemSlot{0, std::variant<Pair<Str, ObjRefRepr>, ObjRefRepr>{TFlag<Pair<Str, ObjRefRepr>>, std::any_cast<Pair<Str, ObjRefRepr>>(visitMember(ctx->member()))}};
        if (ctx->method() != nullptr)
            return ClsMemSlot{1, std::variant<Pair<Str, ObjRefRepr>, ObjRefRepr>{TFlag<ObjRefRepr>, std::any_cast<ObjRefRepr>(visitMethod(ctx->method()))}};
        if (ctx->s_member() != nullptr)
            return ClsMemSlot{2, std::variant<Pair<Str, ObjRefRepr>, ObjRefRepr>{TFlag<ObjRefRepr>, std::any_cast<ObjRefRepr>((visitS_member(ctx->s_member())))}};
        if (ctx->s_method() != nullptr)
            return ClsMemSlot{3, std::variant<Pair<Str, ObjRefRepr>, ObjRefRepr>{TFlag<ObjRefRepr>, std::any_cast<ObjRefRepr>(visitS_method(ctx->s_method()))}};
        std::unreachable();
    }

    std::any visitMember(nlsmParser::MemberContext *ctx) override {
        if (ctx -> PADDING() != nullptr)
            return {Pair{Str{mem}, cvt(ctx -> PADDING()->getText(), *mem)}};
        const auto tp = cvt(ctx->STRING()->getText(), *mem);
        return {Pair{cvt(ctx -> FIELD_NAME()->getText(), *mem).substr(1), tp.substr(1, tp.size() - 2)}};
    }

    std::any visitMethod(nlsmParser::MethodContext *ctx) override {
        const auto link = cvt(ctx->STRING()->getText(), *mem);
        return {cur_cls + link.substr(1, link.size() - 2)};
    }

    std::any visitS_member(nlsmParser::S_memberContext *ctx) override {
        const auto link = cvt(ctx->STRING()->getText(), *mem);
        return {cur_cls + link.substr(1, link.size() - 2)};
    }

    std::any visitS_method(nlsmParser::S_methodContext *ctx) override {
        const auto link = cvt(ctx->STRING()->getText(), *mem);
        return {cur_cls + link.substr(1, link.size() - 2)};
    }

    std::any visitObject_value(nlsmParser::Object_valueContext *ctx) override {
        auto name = cvt(ctx->STRING()->getText(), *mem);
        name = name.substr(0, name.size() - 2);
        std::uint32_t id = ctx->DECIMAL() == nullptr
            ? 0ul
            : strtoul(ctx->DECIMAL()->getText().data(), nullptr, 10);
        auto params = List<CTValue>{mem};
        for (const auto par: ctx->instant_value())
            params.emplace_back(std::any_cast<CTValue>(visitInstant_value(par)));

        return {ObjectRepr{std::move(name), id, std::move(params)}};
    }

    std::any visitInstant_value(nlsmParser::Instant_valueContext *ctx) override {
        auto content = CTValue{};

#define OP(NAME, UP_NAME) \
        if (ctx-> NAME##_value() != nullptr)\
            content = std::get<UP_NAME>(std::any_cast<Package::Content>(visit##UP_NAME##_value(ctx-> NAME##_value())))
        OP(bool, Bool);
        OP(byte, Byte);
        OP(char, Char);
        OP(int, Int);
        OP(long, Long);
        OP(float, Float);
        OP(double, Double);
        if (ctx-> reference_value())\
            content = std::get<ObjRefRepr>(std::any_cast<Package::Content>(visitReference_value(ctx -> reference_value())));
#undef OP
        return {std::move(content)};
    }

    std::any visitCommand(nlsmParser::CommandContext *ctx) override {
        using enum OPCode;

#define OP(CMD) \
        if (ctx -> op() -> CMD() != nullptr)\
            code = CMD;\
        else

        OPCode code;
        {
            OP(NOP)
            OP(WASTE)
            OP(CAST_C2I)
            OP(CAST_B2I)
            OP(CAST_I2B)
            OP(CAST_I2C)
            OP(CAST_I2L)
            OP(CAST_I2F)
            OP(CAST_I2D)
            OP(CAST_L2I)
            OP(CAST_L2F)
            OP(CAST_L2D)
            OP(CAST_F2I)
            OP(CAST_F2L)
            OP(CAST_F2D)
            OP(CAST_D2I)
            OP(CAST_D2L)
            OP(CAST_D2F)
            OP(CMP)
            OP(ADD)
            OP(SUB)
            OP(MUL)
            OP(DIV)
            OP(REM)
            OP(NEG)
            OP(SHL)
            OP(SHR)
            OP(USHR)
            OP(BIT_AND)
            OP(BIT_OR)
            OP(BIT_XOR)
            OP(BIT_INV)
            OP(INVOKE_FIRST)
            OP(RETURN)
            OP(STACK_ALLOC)
            OP(STACK_NEW)
            OP(WILD_ALLOC)
            OP(WILD_NEW)
            OP(WILD_COLLECT)
            OP(DESTROY)
            // OP(ARG_FLAG)
            OP(JUMP)
            OP(JUMP_IF_ZERO)
            OP(JUMP_IF_NOT_ZERO)
            OP(JUMP_IF_POSITIVE)
            OP(JUMP_IF_NEGATIVE)
            OP(P_LOAD_BOOL)
            OP(P_LOAD_CHAR)
            OP(P_LOAD_BYTE)
            OP(P_LOAD_INT)
            OP(P_LOAD_LONG)
            OP(P_LOAD_FLOAT)
            OP(P_LOAD_DOUBLE)
            OP(P_LOAD_REF)
            OP(F_LOAD_BOOL)
            OP(F_LOAD_CHAR)
            OP(F_LOAD_BYTE)
            OP(F_LOAD_INT)
            OP(F_LOAD_LONG)
            OP(F_LOAD_FLOAT)
            OP(F_LOAD_DOUBLE)
            OP(F_LOAD_REF)
            OP(P_STORE_BOOL)
            OP(P_STORE_CHAR)
            OP(P_STORE_BYTE)
            OP(P_STORE_INT)
            OP(P_STORE_LONG)
            OP(P_STORE_FLOAT)
            OP(P_STORE_DOUBLE)
            OP(P_STORE_REF)
            OP(F_STORE_BOOL)
            OP(F_STORE_CHAR)
            OP(F_STORE_BYTE)
            OP(F_STORE_INT)
            OP(F_STORE_LONG)
            OP(F_STORE_FLOAT)
            OP(F_STORE_DOUBLE)
            OP(F_STORE_REF)
            OP(P_REF_AT)
            OP(T_REF_AT)
            OP(FROM_REF_LOAD_BOOL)
            OP(FROM_REF_LOAD_CHAR)
            OP(FROM_REF_LOAD_BYTE)
            OP(FROM_REF_LOAD_INT)
            OP(FROM_REF_LOAD_LONG)
            OP(FROM_REF_LOAD_FLOAT)
            OP(FROM_REF_LOAD_DOUBLE)
            OP(FROM_REF_LOAD_REF)
            OP(TO_REF_STORE_BOOL)
            OP(TO_REF_STORE_CHAR)
            OP(TO_REF_STORE_BYTE)
            OP(TO_REF_STORE_INT)
            OP(TO_REF_STORE_LONG)
            OP(TO_REF_STORE_FLOAT)
            OP(TO_REF_STORE_DOUBLE)
            OP(TO_REF_STORE_REF)
            OP(REF_MEMBER)
            OP(REF_STATIC_MEMBER)
            OP(INVOKE_OVERRIDE)
            OP(REF_METHOD)
            OP(REF_STATIC_METHOD)
            OP(INSTATE);
        }

        if (code < ARG_FLAG && ctx->DECIMAL() != nullptr)
            assert(false, build_str(mem, VM_TEXT("op code do not need argument: OPCode#"), to_string(mem, static_cast<std::uint8_t>(code))));
        if (code > ARG_FLAG && ctx->DECIMAL() != nullptr)
            assert(false, build_str(mem, VM_TEXT("op code need an argument: OPCode#"), to_string(mem, static_cast<std::uint8_t>(code))));

        std::int64_t arg = ctx->DECIMAL() == nullptr
            ? 0ll
            : strtoll(ctx->DECIMAL()->getText().c_str(), nullptr, 10);

        assert(0ll <= arg && arg <= 0xffffffll, "OPCode arg out of range: [0, 0xffffff]");

#undef OP
        return {Instruction{code, static_cast<std::uint32_t>(arg)}};
    }
};


Package nlsm::compile(std::istream& in, MManager& mem, const Str& name) {
    auto input = ANTLRInputStream{in};
    auto lexer = nlsmLexer{&input};
    auto tokens = CommonTokenStream{&lexer};

    auto parser = nlsmParser{&tokens};


    auto builder = NLSMBuilder{mem, name};
    return std::any_cast<Package>(builder.visitPackage(parser.package()));
}