//
// Created by FCWY on 25-1-8.
//
module;
#include <iostream>
#include <iconv.h>
module pnl.ll.base;

void pnl::ll::conditions::assert(const bool condition, const Str &err_desc) noexcept {
    if (!condition) [[unlikely]] {
        std::cerr << cvt(err_desc);
        std::abort();
    }
}

void pnl::ll::conditions::assert(const bool condition, const MBStr &err_desc) noexcept {
    if (!condition) [[unlikely]] {
        std::cerr << err_desc;
        std::abort();
    }
}


pnl::ll::Str pnl::ll::codecvt::cvt(const MBStr &in) noexcept {
    auto s = Str{in.get_allocator()};
    s.reserve(in.size()+1);
    s.resize_and_overwrite(in.size() + 1, [&](Char* buf, const std::size_t buf_size) {
        return code_cvt(
            reinterpret_cast<char*>(buf), buf_size * sizeof(Char),
            in.data(), in.size(),
            ntv_encoding, vm_encoding
        ) / sizeof(Char);
    });
    return s;
}
pnl::ll::MBStr pnl::ll::codecvt::cvt(const Str &in) noexcept {
    auto s = MBStr{in.get_allocator()};
    s.reserve((in.size() + 1) * sizeof(Char));
    s.resize_and_overwrite((in.size() + 1) * sizeof(Char), [&](char* buf, const std::size_t buf_size) {
        return code_cvt(
            buf, buf_size,
            reinterpret_cast<const char*>(in.data()), in.length() * sizeof(Char),
            vm_encoding, ntv_encoding);
    });
    return s;
}

std::size_t pnl::ll::codecvt::code_cvt(char* out, std::size_t out_size, const char* in, std::size_t in_size, const char* code_in, const char* code_out) noexcept {
    const auto codecvt = iconv_open(code_out, code_in);
    const auto out_ori = out_size;

    assert(reinterpret_cast<intptr_t>(codecvt) != -1, "iconv fail to open");
    assert(iconv(codecvt, &const_cast<char*&>(in), &in_size, &out, &out_size) != -1, "iconv fail to cvt");

    iconv_close(codecvt);

    return out_ori - out_size;
}
