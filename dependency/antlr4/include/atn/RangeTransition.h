/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "atn/Transition.h"

namespace antlr4 {
namespace atn {

  class ANTLR4CPP_PUBLIC RangeTransition final : public Transition {
  public:
    static bool is(const Transition &transition) { return transition.getTransitionType() == TransitionType::RANGE; }

    static bool is(const Transition *transition) { return transition != nullptr && is(*transition); }

    const std::size_t from;
    const std::size_t to;

    RangeTransition(ATNState *target, std::size_t from, std::size_t to);

    virtual misc::IntervalSet label() const override;
    virtual bool matches(size_t symbol, std::size_t minVocabSymbol, std::size_t maxVocabSymbol) const override;

    virtual std::string toString() const override;
  };

} // namespace atn
} // namespace antlr4
