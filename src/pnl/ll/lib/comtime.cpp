//
// Created by FCWY on 25-1-8.
//
module;
#include "project-nl.h"
module pnl.ll.comtime;
import pnl.ll.base;
import pnl.ll.string;

using namespace pnl::ll;


ClassRepr::operator ClassRepr::Delegated() noexcept {
    return std::tie(
        name,
        maker,
        collector,
        member_list,
        method_list,
        static_member_list,
        static_method_list
    );
}

ObjectRepr::operator ObjectRepr::Delegated () noexcept {
    return std::tie(
        type,
        constructor_override_id,
        constructor_params
    );
}

Package::Package(MManager *mem) noexcept:
    data{mem}, exports{mem} {}

Package::Package(List<Content> data, Map<USize, ReferenceRepr> exports) noexcept:
    data(std::move(data)),
    exports(std::move(exports)) {}

Package::Package(Package &&other) noexcept:
    data(std::move(other.data)),
    exports(std::move(other.exports)) {}

Package & Package::operator=(const Package &other) noexcept {
    if (this == &other)
        return *this;
    data = other.data;
    exports = other.exports;
    return *this;
}

Package & Package::operator=(Package &&other) noexcept {
    if (this == &other)
        return *this;
    data = std::move(other.data);
    exports = std::move(other.exports);
    return *this;
}


Package::operator Package::Delegated () noexcept {
    return std::tie(
        data,
        exports
    );
}

Str comtime::typename_arr(MManager * const mem, const Char* const type, const USize size) {
    auto ret = Str{mem};
    ret += VM_TEXT("(");
    ret += type;
    ret += VM_TEXT(")");
    ret += VM_TEXT('[');
    append(ret, static_cast<Long>(size));
    ret += VM_TEXT(']');
    ret.shrink_to_fit();
    return std::move(ret);
}

bool comtime::parse_typename_arr(const ReferenceRepr& type, ReferenceRepr& name, USize &size) {
    if (type.v.back() != VM_TEXT(']'))
        return false;

    {
        constexpr auto beg = 1;
        const auto end = type.v.find_last_of(VM_TEXT(')'));
        name = type.v.substr(beg, end - beg - 1);
    }
    {
        const auto beg = type.v.find_last_of(VM_TEXT('[')) + 1;
        const auto end = type.v.find_last_of(VM_TEXT(']'));
        size = to_usz(type.v.substr(beg, end - beg - 1), IntBase::DEC);
    }


    return true;
}

Str comtime::typename_fun(MManager *mem, const List<ReferenceRepr> &param_sigs, const ReferenceRepr &ret_sig) noexcept {
    auto ret = Str(mem);

    ret += VM_TEXT('(');
    if (!param_sigs.empty())
        ret += param_sigs[0];

    for (auto& p_sig: param_sigs | std::views::drop(1)) {
        ret += VM_TEXT(", ");
        ret += p_sig;
    }


    ret += VM_TEXT(") -> ");

    ret += ret_sig;

    ret.shrink_to_fit();
    return ret;
}

bool comtime::parse_typename_fun(const ReferenceRepr& type, List<ReferenceRepr> &param_sigs, ReferenceRepr &ret_sig) noexcept {
    param_sigs.clear();
    {
        auto beg = 1ull;
        auto cur = type.v.find_first_of(VM_TEXT(','), beg + 1);
        while (cur != Str::npos) {
            param_sigs.emplace_back(type.v.substr(beg, cur - beg - 1));
            beg = cur + 2;
            cur = type.v.find_first_of(VM_TEXT(','), beg + 1);
        }
        cur = type.v.find_first_of(VM_TEXT(')'), beg + 1);
        if (cur == Str::npos)
            return false;
        if (cur != beg + 1)
            param_sigs.emplace_back(type.v.substr(beg, cur - beg - 1));
    }
    {
        const auto beg = type.v.find_first_of(VM_TEXT(") -> "));
        if (beg == Str::npos)
            return false;
        ret_sig = type.v.substr(beg);
    }
    return true;
}

