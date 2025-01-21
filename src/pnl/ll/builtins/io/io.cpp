//
// Created by FCWY on 25-1-17.
//
import pnl.ll;
#include "project-nl.h"
using namespace pnl::ll;

namespace write {
    void override_0_char(Thread& thr) noexcept {
        putwchar(std::get<Char>(thr.take()));
    }
}

namespace println {
    void override_0_str_address(Thread& thr) noexcept {

        for (const auto& str = thr.deref<Array<Char>>(std::get<VirtualAddress>(thr.take()));
            const auto ch: str.view(*thr.process))
            putwchar(ch);
    }
}