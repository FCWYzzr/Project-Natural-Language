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
    Str& append(Str& self, Long v, IntBase base=IntBase::DEC) noexcept;


    PNL_LIB_PREFIX
    Str to_string(
        MManager*
            mem,
        Long v,
        IntBase base=IntBase::DEC) noexcept;

    PNL_LIB_PREFIX
    Long to_num(const Str& v,
        IntBase base=IntBase::DEC) noexcept;




    // format: Arr<T>
    PNL_LIB_PREFIX
    Str typename_arr(
        MManager*
            mem,
        const Str& type,
        USize size) noexcept;

    // format: (p1t, p2t, <1>, p3t, <2>, ...) -> ret/<i>
    PNL_LIB_PREFIX
    Str typename_fun(
            MManager*
                mem,
            const List<Str>&   param_sigs,
            const Str&          ret_sig) noexcept;


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

