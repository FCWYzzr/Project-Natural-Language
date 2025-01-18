//
// Created by FCWY on 24-12-16.
//

export module pnl.ll.string;
import <cstring>;
import <ranges>;
import <cmath>;
import <string>;
import <stdexcept>;
import <codecvt>;
import <vector>;
import pnl.ll.base;


export namespace pnl::ll::inline string{

    enum class IntBase: int {
        BIN=2,
        OCT=8,
        DEC=10,
        HEX=16,
        INVALID=36
    };

    PNL_LIB_PREFIX
    USize length(const SSize v, const IntBase base=IntBase::DEC) noexcept {
        if (v == 0)
            return 0;
        if (v < 0)
            return length(-v) + 1;
        return size_cast(floor(log2(v) / log2(static_cast<int>(base)))) + 1;
    }

    PNL_LIB_PREFIX
    Str& append(Str& self, Long v, IntBase base=IntBase::DEC) {
        using enum IntBase;
        constexpr static Char mapping[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (static_cast<int>(base) <= 1)
            throw std::invalid_argument("invalid base");
        if (base >= INVALID)
            throw std::invalid_argument("base >= 36, infer digit desc fail");
        if (v == 0)
            return self += mapping[0];

        if (v < 0) {
            self.push_back(L'-');
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


    PNL_LIB_PREFIX
    Str to_string(
        MManager* const
            mem,
        const Long v,
        const IntBase base=IntBase::DEC) {
        using enum IntBase;
        if (base > INVALID)
            throw std::invalid_argument("base > 36, infer desc fail");

        auto s = Str(mem);
        s.reserve(length(v, base));
        append(
            s,
            v,
            base
        );
        return std::move(s);
    }


    // format: Arr<T>
    PNL_LIB_PREFIX
    Str typename_arr(
        MManager* const
            mem,
        const Str& type,
        const USize size) {

        auto ret = Str{mem};
        ret += type;
        ret += L'[';
        append(ret, static_cast<Long>(size));
        ret += L']';
        return std::move(ret);
    }

    // format: (p1t, p2t, <1>, p3t, <2>, ...) -> ret/<i>
    PNL_LIB_PREFIX
    Str typename_fun(
            MManager*
                mem,
            const List<Str>&   param_sigs,
            const Str&          ret_sig) {
        USize buf_size = 6; // "() -> "

        if (!param_sigs.empty())
            buf_size += size_cast(param_sigs[0].length());


        for (auto& p_sig: param_sigs | std::views::drop(1))
            buf_size += size_cast(p_sig.length() + 2);    // , ti

        buf_size += size_cast(ret_sig.length());  // ti

        auto ret = Str(mem);
        ret.reserve(buf_size);

        ret += U'(';
        if (!param_sigs.empty())
            ret += param_sigs[0];

        for (auto& p_sig: param_sigs | std::views::drop(1)) {
            ret += L", ";
            ret += p_sig;
        }


        ret += L") -> ";

        ret += ret_sig;

        return ret;
    }


    PNL_LIB_PREFIX
    Str cvt(const NStr& in);
    PNL_LIB_PREFIX
    NStr cvt(const Str& in);

    template<typename ...Args>
    Str build_str(MManager* mem, Args&& ...args) noexcept {
        Str ret{mem};
        ((ret += std::forward<Args>(args)), ...);
        return ret;
    }
    template<typename ...Args>
    NStr build_n_str(MManager* mem, Args&& ...args) noexcept {
        NStr ret{mem};
        ((ret += std::forward<Args>(args)), ...);
        return ret;
    }
}

#ifdef WIN32
import <Windows.h>;
import <cassert>;
namespace pnl::ll::string {
    Str cvt(const NStr &in) {
        std::vector<wchar_t> buffer(in.length() + 1);
        assert(("code cvt fail", MultiByteToWideChar(
            CP_UTF8, MB_COMPOSITE,
            in.data(),
            in.length(),
            buffer.data(),
            buffer.size()
            ) != 0));
        return {buffer.data(), in.get_allocator()};
    }
    NStr cvt(const Str &in) {
        std::vector<char> buffer(in.length() * sizeof(wchar_t));
        assert(("code cvt fail", WideCharToMultiByte(
            CP_UTF8, 0,
            in.data(),
            in.length(),
            buffer.data(),
            buffer.size(),
            nullptr, nullptr
            ) != 0));
        return {buffer.data(), in.get_allocator()};
    }
}

#endif

