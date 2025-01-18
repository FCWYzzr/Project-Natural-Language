parser grammar LTShell;
import LangTexParser;
options { tokenVocab=LangTexLexer; }


input options{root=true;}
    : expression
    | content
    ;