//
// Created by FCWY on 25-1-17.
//
import pnl.ll;
import <cstdio>;
import <ranges>;
using namespace pnl::ll;

namespace write {
    void override_0_char(Thread& thr) noexcept {
        putwchar(std::get<Char>(thr.vm__pop_top()));
    }
}
