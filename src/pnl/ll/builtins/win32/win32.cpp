//
// Created by FCWY on 25-1-27.
//
#include <project-NL.h>
#include <Windows.h>
import pnl.ll;

using namespace pnl::ll;

namespace FMessageBox{
    void standalone_override_0(Thread& thr) noexcept {
        const auto title_addr = std::get<VirtualAddress>(thr.take());
        const auto content_addr = std::get<VirtualAddress>(thr.take());
        const auto flags = static_cast<std::uint32_t>(std::get<Long>(thr.take()));
        auto title = MBStr{&thr.process->process_memory};{
            auto& title_v = thr.deref<Array<Char>>(title_addr);
            const auto size = thr.deref<ArrayType>(title_v.super.type).length + 1;
            title.resize_and_overwrite(size * sizeof(Char), [&](char* buf, USize buf_size) {
                return code_cvt(
                    reinterpret_cast<char*>(title_v.data()), size * sizeof(Char),
                    buf, buf_size,
                    vm_encoding,
                    ntv_encoding,
                    &thr.process->process_memory
                );
            });
        }
        auto content = MBStr{&thr.process->process_memory};{
            auto& content_v = thr.deref<Array<Char>>(content_addr);
            const auto size = thr.deref<ArrayType>(content_v.super.type).length + 1;
            content.resize_and_overwrite(size * sizeof(Char), [&](char* buf, USize buf_size) {
                return code_cvt(
                    reinterpret_cast<char*>(content_v.data()), size * sizeof(Char),
                    buf, buf_size,
                    vm_encoding,
                    ntv_encoding,
                    &thr.process->process_memory
                );
            });
        }

        thr.eval_deque.emplace_front(
            TFlag<Int>, MessageBoxA(
                nullptr,
                title.data(),
                content.data(),
                flags
            )
        );
    }
}
