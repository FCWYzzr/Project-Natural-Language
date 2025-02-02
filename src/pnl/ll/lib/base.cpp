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


pnl::ll::Str pnl::ll::codecvt::cvt(const std::string &in, MManager &mem) noexcept {
    return cvt(MBStr{in.c_str(), &mem});
}
pnl::ll::Str pnl::ll::codecvt::cvt(const MBStr &in) noexcept {
    auto s = Str{in.get_allocator()};
    s.reserve(in.size()+1);
    s.resize_and_overwrite(in.size() + 1, [&](Char* buf, const std::size_t buf_size) {
        return code_cvt(
                   in.data(), in.size(),
                   reinterpret_cast<char*>(buf), buf_size * sizeof(Char),
                   ntv_encoding, vm_encoding,
                   in.get_allocator().resource()
               ) / sizeof(Char);
    });
    return s;
}
pnl::ll::MBStr pnl::ll::codecvt::cvt(const Str &in) noexcept {
    auto s = MBStr{in.get_allocator()};
    s.reserve((in.size() + 1) * sizeof(Char));
    s.resize_and_overwrite((in.size() + 1) * sizeof(Char), [&](char* buf, const std::size_t buf_size) {
        return code_cvt(
            reinterpret_cast<const char*>(in.data()), in.length() * sizeof(Char),
            buf, buf_size,
            vm_encoding, ntv_encoding,
            in.get_allocator().resource()
        );
    });
    return s;
}

std::size_t pnl::ll::codecvt::code_cvt(const char* in, std::size_t in_size, char* out, std::size_t out_size, const char* code_in, const char* code_out, MManager* mem) noexcept {
    const auto codecvt = iconv_open(code_out, code_in);
    const auto out_ori = out_size;

    assert(reinterpret_cast<intptr_t>(codecvt) != -1, {"iconv fail to open", mem});
    iconv(codecvt, &const_cast<char*&>(in), &in_size, &out, &out_size);

    iconv_close(codecvt);

    return out_ori - out_size;
}
