//
// Created by FCWY on 25-1-2.
//
// ReSharper disable CppDFAUnreadVariable
// ReSharper disable CppDFAUnusedValue
#include "project-nl.h"
import pnl.ll;



using namespace pnl::ll;


namespace RTUObject {
    void maker(Thread& thr) noexcept {
        // pop RTUObject cls
        thr.eval_deque.pop_front();
        // pop RTUObject this
        thr.eval_deque.pop_front();
    }
    void collector(Thread& thr) noexcept {
        // pop RTUObject this
        thr.eval_deque.pop_front();
    }
}

namespace RTTObject {
    void maker(Thread& thr) noexcept {
        const auto rtt_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto rtt_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(rtt_this_addr);

        new (&self) runtime::RTTObject {rtt_cls_addr};
    }
    void collector(Thread& thr) noexcept {
        // pop RTTObject this
        thr.eval_deque.pop_front();
    }
}

namespace Type {
    void maker(Thread& thr) noexcept {
        const auto type_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto type_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(type_this_addr);

        const auto instance_size = std::get<Int>(thr.take());
        const auto maker = std::get<VirtualAddress>(thr.take());
        const auto collector = std::get<VirtualAddress>(thr.take());

        new (&self) runtime::Type {
            {type_cls_addr},
            instance_size,
            maker,
            collector
        };
    }
    void collector(Thread& thr) noexcept {
        // pop Type this
        thr.eval_deque.pop_front();
    }
}

namespace NamedType {
    void maker(Thread& thr) noexcept {
        const auto n_type_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto n_type_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(n_type_this_addr);

        const auto instance_size = std::get<Int>(thr.take());
        const auto maker = std::get<VirtualAddress>(thr.take());
        const auto collector = std::get<VirtualAddress>(thr.take());
        const auto name = std::get<VirtualAddress>(thr.take());

        new (&self) runtime::NamedType {
                {{n_type_cls_addr},
                instance_size,
                maker,
                collector},
                name
            };
    }
    void collector(Thread& thr) noexcept {
        // pop NamedType this
        thr.eval_deque.pop_front();
    }
}

namespace Array {
    void maker(Thread& thr) noexcept {
        const auto n_type_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto n_type_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<runtime::NamedType>(n_type_this_addr);

        const auto instance_size = std::get<Int>(thr.take());
        const auto maker = std::get<VirtualAddress>(thr.take());
        const auto collector = std::get<VirtualAddress>(thr.take());
        const auto elem_type = std::get<VirtualAddress>(thr.take());
        const auto length = std::get<Long>(thr.take());

        new (&self) runtime::ArrayType {{
            {n_type_cls_addr},
            instance_size,
            maker,
            collector},
            elem_type,
            length
        };
    }
    void collector(Thread& thr) noexcept {
        thr.eval_deque.pop_front();
    }

    void inst_maker(Thread& thr) noexcept {
        const auto this_addr = std::get<VirtualAddress>(thr.take());
        const auto& arr_type = thr.deref<runtime::ArrayType>(this_addr);
        const auto& elem_type = thr.deref<runtime::Type>(arr_type.elem_type);

        const auto place = std::get<VirtualAddress>(thr.take());
        thr.deref<runtime::RTTObject>(place) = {this_addr};

        for (const auto i: std::views::iota(0, arr_type.length)) {
            thr.eval_deque.emplace_front(TFlag<VirtualAddress>,
                place.offset_shift(
                    sizeof(runtime::RTTObject) + i * elem_type.instance_size
                )
            );
            thr.call_function(thr.deref<FFamily>(elem_type.maker), 0);
        }
    }
    void inst_collector(Thread& thr) noexcept {
        const auto place = std::get<VirtualAddress>(thr.take());
        const auto& inst = thr.deref<runtime::RTTObject>(place);
        const auto& arr_type = thr.deref<runtime::ArrayType>(inst.type);
        const auto& elem_type = thr.deref<runtime::Type>(arr_type.elem_type);

        for (const auto i: std::views::iota(0, arr_type.length)) {
            thr.eval_deque.emplace_front(TFlag<VirtualAddress>,
                place.offset_shift(
                    sizeof(runtime::RTTObject) + i * elem_type.instance_size
                )
            );
            thr.call_function(thr.deref<FFamily>(elem_type.collector), 0);
        }
    }
}

namespace MemberInfo {
    void maker(Thread& thr) noexcept {
        const auto this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(this_addr);

        const auto name = std::get<VirtualAddress>(thr.take());
        const auto type = std::get<VirtualAddress>(thr.take());
        const auto offset = std::get<Long>(thr.take());

        new (&self) runtime::MemberInfo {
            name,
            type,
            offset
        };
    }
    void collector(Thread& thr) noexcept {
        thr.eval_deque.pop_front();
    }
}

namespace Class {
    void maker(Thread& thr) noexcept {
        const auto cls_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto cls_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(cls_this_addr);

        const auto instance_size = std::get<Int>(thr.take());
        const auto maker = std::get<VirtualAddress>(thr.take());
        const auto collector = std::get<VirtualAddress>(thr.take());
        const auto name = std::get<VirtualAddress>(thr.take());
        const auto members = std::get<VirtualAddress>(thr.take());
        const auto methods = std::get<VirtualAddress>(thr.take());
        const auto static_members = std::get<VirtualAddress>(thr.take());
        const auto static_methods = std::get<VirtualAddress>(thr.take());



        new (&self) runtime::Class {{{{
            cls_cls_addr},
            instance_size,
            maker,
            collector},
            name},
            members,
            methods,
            static_members,
            static_methods
        };
    }
    void collector(Thread& thr) noexcept {
        // pop NamedType this
        thr.eval_deque.pop_front();
    }
}

template<typename T>
static void maker(Thread& thr) noexcept {
    const auto this_addr = std::get<VirtualAddress>(thr.take());

    new (&thr.deref<>(this_addr)) T {std::get<T>(thr.take())};
}

template<typename T>
static void collector(Thread& thr) noexcept {
    thr.eval_deque.pop_front();
}

#define OP(TYPE) \
namespace TYPE {\
    void maker(Thread& thr) noexcept {\
        ::maker<base::TYPE>(thr);\
    }\
    void collector(Thread& thr) noexcept{\
        ::collector<base::TYPE>(thr);\
    }\
}

OP(Bool)
OP(Char)
OP(Byte)
OP(Int)
OP(Long)
OP(Float)
OP(Double)


namespace Unit {
    void maker(Thread&) noexcept {
        std::unreachable();
    }
    void collector(Thread&) noexcept {
        std::unreachable();
    }
}

namespace Address {
    void maker(Thread& thr) noexcept {
        ::maker<VirtualAddress>(thr);
    }
    void collector(Thread& thr) noexcept{
        ::collector<VirtualAddress>(thr);
    }
}

namespace Instruction {
    void maker(Thread& thr) noexcept {
        const auto this_addr = std::get<VirtualAddress>(thr.take());

        const auto code = static_cast<OPCode>(static_cast<UByte>(std::get<base::Byte>(thr.take())));
        const auto arg = static_cast<std::uint32_t>(std::get<base::Int>(thr.take()));

        new (&thr.deref<>(this_addr)) vm::Instruction {code, arg};

    }
    void collector(Thread& thr) noexcept{
        thr.eval_deque.pop_front();
    }
}


namespace FOverride {
    void maker(Thread& thr) noexcept {
        const auto type_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto type_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(type_this_addr);

        const auto is_native = std::get<base::Bool>(thr.take());
        const auto target = std::get<VirtualAddress>(thr.take());

        new (&self) runtime::FOverride {
                    {type_cls_addr},
                    is_native,
                    target.content
                };
    }
    void collector(Thread& thr) noexcept {
        // pop FFamily this
        thr.eval_deque.pop_front();
    }
    void inst_maker(Thread& thr) noexcept {
        const auto type_inst_addr = std::get<VirtualAddress>(thr.take());
        auto place_addr = std::get<VirtualAddress>(thr.take());
        auto native = std::get<base::Bool>(thr.take());
        auto proc = std::get<VirtualAddress>(thr.take());

        new (&thr.deref<>(place_addr)) runtime::FOverride {
            runtime::RTTObject{type_inst_addr},
            native,
            proc.content
        };


    }
    void inst_collector(Thread& thr) noexcept {
        thr.eval_deque.pop_front();
    }
}

namespace FFamily {
    void maker(Thread& thr) noexcept {
        const auto type_cls_addr = std::get<VirtualAddress>(thr.take());
        const auto type_this_addr = std::get<VirtualAddress>(thr.take());

        auto& self = thr.deref<>(type_this_addr);

        const auto base_addr = std::get<VirtualAddress>(thr.take());
        const auto overrides = std::get<VirtualAddress>(thr.take());

        new (&self) runtime::FFamily {
                {type_cls_addr},
                base_addr,
                overrides
            };
    }
    void collector(Thread& thr) noexcept {
        // pop FFamily this
        thr.eval_deque.pop_front();
    }
}

