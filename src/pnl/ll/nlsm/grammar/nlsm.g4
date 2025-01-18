grammar nlsm;

import opcode;

EMPTY
    : [ \r\b\t\n] -> skip
    ;


// slots:
EXPORT
    : 'export'
    ;

OBJECT
    : 'object'
    ;

VALUE
    : 'value'
    ;


// name finding

BUILTINS
    : 'builtins'
    ;

TRUE
    : 'true'
    ;

FALSE
    : 'false'
    ;

NULLPTR
    : 'nullptr'
    ;

BYTE_FUN
    : 'byte'
    ;

CHAR
    // normal char
    : '\'' ~['] '\''
    // escape char
    | '\'\\' ['abnrt] '\''
    ;

U32_CHAR_FUN
    : 'char'
    ;

INT_LITERAL
    : '0'
    | [1-9][0-9]*
    ;

FLOAT_LITERAL
    : INT_LITERAL '.' INT_LITERAL?
    | INT_LITERAL? '.' INT_LITERAL
    ;

INT_FUN
    : 'int'
    ;

LONG_FUN
    : 'long'
    ;

FLOAT_FUN
    : 'float'
    ;

DOUBLE_FUN
    : 'double'
    ;



LP  : '('
    ;
RP  : ')'
    ;
LB  : '{'
    ;
RB  : '}'
    ;

package options{ root=true; }
    : STRING_LITERAL LB pkg_content* RB
    ;


pkg_content
    : imports
    | constants
    | packages
    ;

IMPORT
    : 'import'
    ;
LBK : '['
    ;
STRING_LITERAL
    : '"' BARE_NAME '"'
    | '""'
    ;
RBK : ']'
    ;

imports
    : IMPORT LBK STRING_LITERAL* RBK
    ;


CONSTANT
    : 'constant'
    ;


constants
    : CONSTANT LBK ((EXPORT STRING_LITERAL)? constant)* RBK
    ;

PACKAGE
    : 'package'
    ;

packages
    : PACKAGE LBK package* RBK
    ;

constant
    : boolV
    | byteV
    | charV
    | intV
    | longV
    | floatV
    | doubleV
    | refV
    | objectV
    ;

refV
    : NULLPTR
    ;

objectV
    : class_literial
    | f_family_literial
    | instance_literial
    ;

CLASS
    : 'class'
    ;

class_literial
    : CLASS STRING_LITERAL LB class_member* RB
    ;


class_member
    : data_member
    | func_member
    ;

f_family_literial
    : FUN LBK override+ RBK
    | FUN override
    ;

DOT : '.'
    ;
TYPE_NAME
    : '<' BARE_NAME '>'
    ;
OREF
    : '::'
    ;
DOT_BARE_NAME
    : DOT BARE_NAME
    ;

OREF_BARE_NAME
    : OREF BARE_NAME
    ;

data_member
    : (DOT BARE_NAME | DOT_BARE_NAME) TYPE_NAME
    | (OREF BARE_NAME | OREF_BARE_NAME) TYPE_NAME LB constant RB
    ;



instance_literial
    : TYPE_NAME LB instance_member* RB
    ;

instance_member
    : (DOT BARE_NAME | DOT_BARE_NAME) constant
    ;


FUN
    : 'func'
    ;

func_member
    : (DOT BARE_NAME | DOT_BARE_NAME) f_family_literial
    | (OREF BARE_NAME | OREF_BARE_NAME) f_family_literial
    ;


override
    : LB args ret (proc | native) RB
    ;

ARGS: 'args'
    ;

args: ARGS LBK TYPE_NAME* RBK
    ;

RET : 'ret'
    ;
ret : RET TYPE_NAME
    ;


PROC: 'proc'
    ;
proc: PROC LBK instruction* RBK
    ;

NATIVE
    : 'native'
    ;
native
    : NATIVE STRING_LITERAL
    ;



boolV
    : TRUE
    | FALSE
    ;

byteV
    : BYTE_FUN LP INT_LITERAL RP
    ;

charV
    : CHAR
    | U32_CHAR_FUN LP INT_LITERAL RP
    ;

intV
    : INT_LITERAL
    | INT_FUN LP CHAR RP
    ;

longV
    : LONG_FUN LP INT_LITERAL RP
    ;

floatV
    : FLOAT_LITERAL
    | FLOAT_FUN LP INT_LITERAL RP
    ;

doubleV
    : DOUBLE_FUN LP (INT_LITERAL | FLOAT_LITERAL) RP
    ;


instruction
    : op INT_LITERAL?
    ;
