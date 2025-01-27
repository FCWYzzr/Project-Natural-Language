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
    ret += type;
    ret += VM_TEXT('[');
    append(ret, static_cast<Long>(size));
    ret += VM_TEXT(']');
    return std::move(ret);
}

Str comtime::typename_fun(MManager *mem, const List<ReferenceRepr> &param_sigs, const ReferenceRepr &ret_sig) noexcept {
    USize buf_size = 6; // "() -> "

    if (!param_sigs.empty())
        buf_size += size_cast(param_sigs[0].v.length());


    for (auto& p_sig: param_sigs | std::views::drop(1))
        buf_size += size_cast(p_sig.v.length() + 2);    // , ti

    buf_size += size_cast(ret_sig.v.length());  // ti

    auto ret = Str(mem);
    ret.reserve(buf_size);

    ret += VM_TEXT('(');
    if (!param_sigs.empty())
        ret += param_sigs[0];

    for (auto& p_sig: param_sigs | std::views::drop(1)) {
        ret += VM_TEXT(", ");
        ret += p_sig;
    }


    ret += VM_TEXT(") -> ");

    ret += ret_sig;

    return ret;
}