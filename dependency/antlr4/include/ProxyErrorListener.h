/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

#pragma once

#include "ANTLRErrorListener.h"
#include "Exceptions.h"

namespace antlr4 {

  /// This implementation of ANTLRErrorListener dispatches all calls to a
  /// collection of delegate listeners. This reduces the effort required to support multiple
  /// listeners.
  class ANTLR4CPP_PUBLIC ProxyErrorListener : public ANTLRErrorListener {
  private:
    std::set<ANTLRErrorListener *> _delegates; // Not owned.

  public:
    void addErrorListener(ANTLRErrorListener *listener);
    void removeErrorListener(ANTLRErrorListener *listener);
    void removeErrorListeners();

    void syntaxError(Recognizer *recognizer, Token *offendingSymbol, std::size_t line, std::size_t charPositionInLine,
                     const std::string &msg, std::exception_ptr e) override;

    virtual void reportAmbiguity(Parser *recognizer, const dfa::DFA &dfa, std::size_t startIndex, std::size_t stopIndex, bool exact,
                                 const antlrcpp::BitSet &ambigAlts, atn::ATNConfigSet *configs) override;

    virtual void reportAttemptingFullContext(Parser *recognizer, const dfa::DFA &dfa, std::size_t startIndex, std::size_t stopIndex,
      const antlrcpp::BitSet &conflictingAlts, atn::ATNConfigSet *configs) override;

    virtual void reportContextSensitivity(Parser *recognizer, const dfa::DFA &dfa, std::size_t startIndex, std::size_t stopIndex,
                                          std::size_t prediction, atn::ATNConfigSet *configs) override;
  };

} // namespace antlr4
