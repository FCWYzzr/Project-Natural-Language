grammar nlsm;

import opcode;

EMPTY
    : [ \r\b\t\n]+ -> skip
    ;

EXPORT
    : 'export'
    ;

AS
    : 'as'
    ;

TRUE
    : 'true'
    ;

FALSE
    : 'false'
    ;

BYTE
    : 'byte'
    ;

INT
    : 'int'
    ;

LONG
    : 'long'
    ;

FLOAT
    : 'float'
    ;

DOUBLE
    : 'double'
    ;

NamedType
    : 'type'
    ;

IMPORT
    : 'import'
    ;

FUNC
    : 'func'
    ;

CLASS
    : 'class'
    ;

LB  : '{'
    ;
RB  : '}'
    ;
LBK : '['
    ;
RBK : ']'
    ;
LP  : '('
    ;
RP  : ')'
    ;
EQ  : '='
    ;
CHAR
    : '\'' ~['brtn\\] '\''
    | '\'\\' ['brtn\\] '\''
    ;
fragment CChar
    : ~[\\']
    | '\\' ['"?\\abfnrtv]
    | '\\o{' [0-7]+ '}'
    | '\\x{' [0-9a-fA-F]+ '}'
    ;
STRING
    : '"' SChar* '"'
    ;
fragment SChar
    : ~[\\"]
    | '\\' ['"?\\abfnrtv]
    | '\\' [oO] '{' [0-7]+ '}'
    | '\\' [xXuU] '{' [0-9a-fA-F]+ '}'
    ;
PADDING
    : '_' +
    ;


DECIMAL
    : '0'
    | '-'? [1-9] [0-9]*
    ;

FLOATING
    : '-'? DECIMAL '.' DECIMAL
    ;

FIELD_NAME
    : '.' ~[ ./\\:"'@]+
    ;

STATIC
    : 'static'
    ;
INSTATE
    : 'instate'
    ;
AT
    : '@'
    ;
PARAM
    : 'param'
    ;
IMPLEMENT
    : 'implement'
    ;
INVISIBLE
    : 'invisible'
    ;
EXTENDS
    : 'extends'
    ;
COMMA
    : ','
    ;
RETURN_TAG
    : 'return'
    ;


package options{root=true;}
    : (package_content COMMA?) +
    ;

package_content
    : EXPORT value (AS STRING)?
    | value
    ;

value
    : bool_value
    | byte_value
    | char_value
    | int_value
    | long_value
    | float_value
    | double_value
    | reference_value
    | char_array_value
    | named_type_value
    | function_value
    | class_value
    | object_value
    ;

bool_value
    : TRUE
    | FALSE
    ;

byte_value
    : BYTE DECIMAL
    ;

char_value
    : CHAR
    ;

int_value
    : INT DECIMAL
    ;

long_value
    : LONG DECIMAL
    ;

float_value
    : FLOAT FLOATING
    | FLOAT DECIMAL
    ;

double_value
    : DOUBLE FLOATING
    | DOUBLE DECIMAL
    ;

reference_value
    : IMPORT STRING
    ;

char_array_value
    : STRING
    ;

// type "unit" = byte[0]
// autogen unit::() & unit::~()
named_type_value
    : NamedType STRING EQ BYTE LBK DECIMAL RBK
    ;

// func "write" [
//     {
//         param [ "core::char" ]
//         return "core::unit"
//         impl "write::0"
//     }
//     ...
// ]
function_value
    : FUNC LBK (override_value COMMA?)+ RBK
    | FUNC override_value
    ;

override_value
    : LB
         (PARAM params)?
         (RETURN_TAG ret)?
         IMPLEMENT ((LBK command+ RBK) | STRING)
      RB
    ;

params
    : LBK (STRING COMMA?)* RBK
    ;
ret
    : STRING
    ;

// class "Class" extends "core::NamedType" {
//      .members "core::address"
//      .methods "core::address"
//      .static_members "core::address"
//      .static_methods "core::address"
// }



class_value
    : CLASS STRING super? LB class_content* RB
    | INVISIBLE CLASS STRING LB class_content* RB
    ;
super
    : EXTENDS STRING
    ;

class_content
    : member
    | method
    | s_member
    | s_method
    ;

member
    : FIELD_NAME STRING
    | PADDING
    ;

method
    : FUNC STRING
    ;

s_member
    : STATIC FIELD_NAME EQ STRING
    ;

s_method
    : STATIC FUNC STRING
    ;

object_value
    : INSTATE CLASS STRING (AT DECIMAL)? LB instant_value* RB
    ;

instant_value
    : bool_value
    | byte_value
    | char_value
    | int_value
    | long_value
    | float_value
    | double_value
    | reference_value
    ;

command
    : op DECIMAL?
    ;

