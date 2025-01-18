lexer grammar LangTexLexer;



/* region [builtin sets] */

INT_SETS
    : 'int'
    | 'long'
    ;

FLOAT_SETS
    : 'float'
    | 'double'
    ;

CHAR_SETS
    : 'char'
    ;

/* endregion [builtin sets] */

/* region [chars] */

/* region [chars.Parentheses] */

LB
    : '{'
    ;
RB
    : '}'
    ;
LBK
    : '['
    ;
RBK
    : ']'
    ;
LP
    : '('
    ;
RP
    : ')'
    ;

/* endregion [chars.Parentheses] */

/* region [chars.Comparators] */

GT
    : '>'
    | '\\gt'
    ;
GE
    : '>='
    | '\\ge'
    ;
EQ
    : '=='
    ;
NE
    : '!='
    | '\\ne'
    ;
LE
    : '<='
    | '\\le'
    ;
LT
    : '<'
    | '\\lt'
    ;

/* endregion [chars.Comparators] */

/* region [chars.Operators] */

ADD
    : '+'
    ;
SUB
    : '-'
    ;
MUL
    : '*'
    ;
DIV
    : '/'
    ;

NOT
    : '!'
    | '\\neg'
    ;

AND
    : '&&'
    | '\\wedge'
    ;

OR
    : '||'
    | '\\vee'
    ;

THEN
    : '\\rightarrow'
    ;

SAME_AS
    : '\\leftrightarrow'
    ;

/* endregion [chars.Operators] */

ASSIGN
    : '<-'
    ;

ASSIGN_OR_EQUALS
    : '='
    ;

RIGHT_ARROW
    : '->'
    ;

COLON
    : ':'
    ;
COMMA
    : ','
    ;
ELLIPSIS
    : '...'
    ;

/* endregion [chars] */

/* region [keywords] */

BEGIN
    : '\\begin'
    ;
END
    : '\\end'
    ;
META
    : '\\meta'
    ;

CASES
    : 'cases'
    ;

/* endregion */

/* region [entities] */

NAME
    : [a-zA-Z][a-zA-Z0-9]*
    ;

INT_LITERAL
    : Decimal
    // todo
    ;

LOGIC_LITERAL
    : 'true'
    | 'false'
    ;

EOL
        : '\\\\'
        ;

/* endregion */

/* region [empty segemnts] */

EMPTY
    : [\t\r\n\b ] -> skip
    ;
COMMENT
    :'%' .+? '\n' -> skip
    ;
COMMENTS
    : CommentCmd LB .*? RB -> skip
    ;

/* endregion */



/* region [fragments] */

fragment CommentCmd
    : '\\comment'
    ;

fragment Int0
    : '0'
    ;
fragment IntN0
    : [1-9]
    ;
fragment Decimal
    : Int0
    | IntN0 Int0 *
    ;

/* endregion */