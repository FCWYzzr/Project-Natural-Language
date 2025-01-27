//
// Created by FCWY on 24-12-16.
//
module;
#include "project-nl.h"
export module pnl.ll.string;
import pnl.ll.base;


export namespace pnl::ll::inline string{

    enum class IntBase: int {
        BIN=2,
        OCT=8,
        DEC=10,
        HEX=16
    };

    PNL_LIB_PREFIX
    USize length(SSize v, IntBase base=IntBase::DEC) noexcept;

    PNL_LIB_PREFIX
    Str& append(Str& self, Long v, IntBase base=IntBase::DEC);


    PNL_LIB_PREFIX
    Str to_string(
        MManager*
            mem,
        Long v,
        IntBase base=IntBase::DEC) noexcept;





    template<typename ...Args>
    Str build_str(MManager* mem, Args&& ...args) noexcept {
        Str ret{mem};
        ((ret += std::forward<Args>(args)), ...);
        return ret;
    }
    template<typename ...Args>
    MBStr build_n_str(MManager* mem, Args&& ...args) noexcept {
        MBStr ret{mem};
        ((ret += std::forward<Args>(args)), ...);
        return ret;
    }
}

