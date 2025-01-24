//
// Created by FCWY on 24-12-28.
//
module;
#include "project-nl.h"
export module nlsm.grammar;
import pnl.ll;
using namespace pnl::ll;

export namespace nlsm::inline grammar {
    Package compile(std::istream& in, MManager& mem, const Str&);
}


