//
// Created by FCWY on 25-1-17.
//
import pnl.ll;
#include "project-nl.h"
using namespace pnl::ll;

namespace write {
    void override_0_char(Thread& thr) noexcept {
        unsigned char in[5] = {};
        char out[5] = {};
        reinterpret_cast<Char&>(in) = std::get<Char>(thr.take());
        code_cvt(
            reinterpret_cast<char*>(in),
            5,
            out,
            5,
            vm_encoding,
            ntv_encoding,
            &thr.process->process_memory
        );
        fputs(out, stdout);
    }
}

namespace println {
    void override_0_str_address(Thread& thr) noexcept {
        auto& in = thr.deref<Array<Char>>(std::get<VirtualAddress>(thr.take()));
        auto out = MBStr{&thr.process->process_memory};
        const auto i_sz = thr.deref<ArrayType>(in.super.type).length;
        out.resize_and_overwrite(i_sz * sizeof(Char), [&](char* buf, USize sz) {
            return code_cvt(
                reinterpret_cast<const char*>(in.data()), i_sz * sizeof(Char),
                buf, sz,
                vm_encoding,
                ntv_encoding,
                &thr.process->process_memory
            );
        });
        fputs(out.c_str(), stdout);
        fputc('\n', stdout);
    }
}