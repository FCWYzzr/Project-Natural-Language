//
// Created by FCWY on 25-1-8.
//
module;
#include "project-nl.h"
module pnl.ll.string;
using namespace pnl::ll;



USize string::length(const SSize v, const IntBase base) noexcept {
    if (v == 0)
        return 0;
    if (v < 0)
        return length(-v) + 1;
    return size_cast(floor(log2(v) / log2(static_cast<int>(base)))) + 1;
}

Str & string::append(Str &self, Long v, IntBase base) noexcept {
    using enum IntBase;
    constexpr static Char mapping[] = VM_TEXT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    assert(static_cast<int>(base) > 1, "invalid base");
    assert(static_cast<int>(base) < 36, "base >= 36, infer digit desc fail");

    if (v == 0)
        return self += mapping[0];

    if (v < 0) {
        self.push_back(VM_TEXT('-'));
        v = -v;
    }
    const auto sz = length(v);
    const auto target=self.size() + sz;
    self.resize_and_overwrite(target, [&](Char* buf, USize) noexcept {
        buf += target;

        while (v) {
            * --buf = mapping[v % static_cast<int>(base)];
            v /= static_cast<int>(base);
        }
        return target;
    });
    return self;
}

Str string::to_string(MManager * const mem, const Long v, const IntBase base) noexcept {
    auto s = Str(mem);
    s.reserve(length(v, base));
    append(
        s,
        v,
        base
    );
    return std::move(s);
}

Long string::to_num(const Str &v, IntBase base) noexcept {
    Long num = 0;
    const auto e = v.begin() - 1;
    auto b = v.end() - 1;

    while (b != e) {
        if (*b >= VM_TEXT('0') && *b <= VM_TEXT('9'))
            num = num * 10 + (*b - VM_TEXT('0'));
        else if (*b >= VM_TEXT('a') && *b <= VM_TEXT('f'))
            num = num * 10 + 10 + (*b - VM_TEXT('a'));
        else if (*b >= VM_TEXT('A') && *b <= VM_TEXT('F'))
            num = num * 10 + 10 + (*b - VM_TEXT('A'));
        else
            return -1;
    }

    return num;
}

Str string::typename_arr(MManager * const mem, const Str &type, const USize size) noexcept {

    auto ret = Str{mem};
    ret += type;
    ret += VM_TEXT('[');
    append(ret, static_cast<Long>(size));
    ret += VM_TEXT(']');
    return std::move(ret);
}

Str string::typename_fun(MManager *mem, const List<Str> &param_sigs, const Str &ret_sig) noexcept {
    USize buf_size = 6; // "() -> "

    if (!param_sigs.empty())
        buf_size += size_cast(param_sigs[0].length());


    for (auto& p_sig: param_sigs | std::views::drop(1))
        buf_size += size_cast(p_sig.length() + 2);    // , ti

    buf_size += size_cast(ret_sig.length());  // ti

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
