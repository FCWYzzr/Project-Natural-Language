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


