grammar nlsm;

import opcode;

EMPTY
    : [ \r\b\t\n] -> skip
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
STRING
    : '"' SChar* '"'
    ;

POSITIVE_DECIMAL
    : '0'
    | [1-9] [0-9]*
    ;

DECIMAL
    : POSITIVE_DECIMAL
    | '-'? [1-9] [0-9]*
    ;

FLOATING
    : '-'? POSITIVE_DECIMAL '.' POSITIVE_DECIMAL
    ;

FIELD_NAME
    : '.' ~[./\\:]+
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

fragment SChar
    : ~["brtn\\]
    | '\\' ["brtn\\]
    ;


package options{root=true;}
    : package_content +
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
    : NamedType STRING EQ BYTE LBK POSITIVE_DECIMAL RBK
    ;

// func "write" [
//     {
//         param [ "::char" ]
//         return "::unit"
//         impl "write::0"
//     }
//     ...
// ]
function_value
    : FUNC STRING LBK override_value+ RBK
    | FUNC STRING override_value
    ;

override_value
    : LB
         PARAM LBK STRING* RBK
         RETURN STRING
         IMPLEMENT LBK command+ RBK
      RB
    | LB
         PARAM LBK STRING* RBK
         RETURN STRING
         IMPLEMENT STRING
      RB
    ;

// class "class" {
//      .members "::address"
//      .methods "::address"
//      .static_members "::address"
//      .static_methods "::address"
// }
class_value
    : CLASS STRING LB class_content+ RB
    ;

class_content
    : member
    | method
    | s_member
    | s_method
    ;

member
    : FIELD_NAME STRING
    ;

method
    : FIELD_NAME EQ FUNC STRING
    ;

s_member
    : STATIC FIELD_NAME EQ STRING
    ;

s_method
    : STATIC FIELD_NAME EQ FUNC STRING
    ;

object_value
    : INSTATE CLASS STRING AT DECIMAL LB instant_value RB
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

