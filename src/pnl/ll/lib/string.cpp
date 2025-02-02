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

Str & string::append(Str &self, Long v, IntBase base) {
    using enum IntBase;
    constexpr static Char mapping[] = VM_TEXT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    assert(static_cast<int>(base) > 1, {"invalid base", self.get_allocator()});
    assert(static_cast<int>(base) < 36, {"base >= 36, infer digit desc fail", self.get_allocator()});

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

USize string::to_usz(const Str& self, IntBase base) noexcept {
    using enum IntBase;
    assert(static_cast<int>(base) > 1, {"invalid base", self.get_allocator()});
    assert(static_cast<int>(base) < 36, {"base >= 36, infer digit desc fail", self.get_allocator()});

    USize buf = 0;
    for (const Char& ch: self) {
        if (base <= DEC) {
            if (!is_digit(ch) || ch >= VM_TEXT('0' + static_cast<int>(base)))
                return 0;
            buf *= static_cast<int>(base);
            buf += ch - VM_TEXT('0');
        }
        else if  (static_cast<int>(ch) <= 36) {
            buf *= static_cast<int>(base);
            if (is_digit(ch))
                buf += ch - VM_TEXT('0');
            else if (is_upper(ch))
                buf += ch - VM_TEXT('A') + 10;
            else if (is_lower(ch))
                buf += ch - VM_TEXT('a') + 10;
            else
                return 0;

        }
    }
    return buf;
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


bool string::is_digit(const Char ch) noexcept {
    return VM_TEXT('0') <= ch && ch <= VM_TEXT('9');
}
bool string::is_alpha(const Char ch) noexcept {
    return is_upper(ch) || is_lower(ch);
}


bool string::is_upper(const Char ch) noexcept {
    return VM_TEXT('A') <= ch && ch <= VM_TEXT('Z');
}
Char string::to_lower(const Char ch) noexcept {
    if (is_upper(ch))
        return ch - VM_TEXT('A') + VM_TEXT('a');
    return ch;
}

bool string::is_lower(const Char ch) noexcept {
    return VM_TEXT('a') <= ch && ch <= VM_TEXT('z');
}
Char string::to_upper(const Char ch) noexcept {
    if (is_lower(ch))
        return ch - VM_TEXT('a') + VM_TEXT('A');
    return ch;
}



