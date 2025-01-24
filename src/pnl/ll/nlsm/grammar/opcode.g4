grammar opcode;



// opcodes

NOP
    : 'NOP'
    ;
WASTE
    : 'WASTE'
    ;
CAST_C2I
    : 'CAST_C2I'
    ;
CAST_B2I
    : 'CAST_B2I'
    ;
CAST_I2B
    : 'CAST_I2B'
    ;
CAST_I2C
    : 'CAST_I2C'
    ;
CAST_I2L
    : 'CAST_I2L'
    ;
CAST_I2F
    : 'CAST_I2F'
    ;
CAST_I2D
    : 'CAST_I2D'
    ;
CAST_L2I
    : 'CAST_L2I'
    ;
CAST_L2F
    : 'CAST_L2F'
    ;
CAST_L2D
    : 'CAST_L2D'
    ;
CAST_F2I
    : 'CAST_F2I'
    ;
CAST_F2L
    : 'CAST_F2L'
    ;
CAST_F2D
    : 'CAST_F2D'
    ;
CAST_D2I
    : 'CAST_D2I'
    ;
CAST_D2L
    : 'CAST_D2L'
    ;
CAST_D2F
    : 'CAST_D2F'
    ;
CMP
    : 'CMP'
    ;
ADD
    : 'ADD'
    ;
SUB
    : 'SUB'
    ;
MUL
    : 'MUL'
    ;
DIV
    : 'DIV'
    ;
REM
    : 'REM'
    ;
NEG
    : 'NEG'
    ;
SHL
    : 'SHL'
    ;
SHR
    : 'SHR'
    ;
USHR
    : 'USHR'
    ;
BIT_AND
    : 'BIT_AND'
    ;
BIT_OR
    : 'BIT_OR'
    ;
BIT_XOR
    : 'BIT_XOR'
    ;
BIT_INV
    : 'BIT_INV'
    ;
INVOKE_FIRST
    : 'INVOKE_FIRST'
    ;
RETURN
    : 'RETURN'
    ;
STACK_ALLOC
    : 'STACK_ALLOC'
    ;
STACK_NEW
    : 'STACK_NEW'
    ;
WILD_ALLOC
    : 'WILD_ALLOC'
    ;
WILD_NEW
    : 'WILD_NEW'
    ;
WILD_COLLECT
    : 'WILD_COLLECT'
    ;
DESTROY
    : 'DESTROY'
    ;
//ARG_FLAG
//    : 'ARG_FLAG'
//    ;
JUMP
    : 'JUMP'
    ;
JUMP_IF_ZERO
    : 'JUMP_IF_ZERO'
    ;
JUMP_IF_NOT_ZERO
    : 'JUMP_IF_NOT_ZERO'
    ;
JUMP_IF_POSITIVE
    : 'JUMP_IF_POSITIVE'
    ;
JUMP_IF_NEGATIVE
    : 'JUMP_IF_NEGATIVE'
    ;
P_LOAD_BOOL
    : 'P_LOAD_BOOL'
    ;
P_LOAD_CHAR
    : 'P_LOAD_CHAR'
    ;
P_LOAD_BYTE
    : 'P_LOAD_BYTE'
    ;
P_LOAD_INT
    : 'P_LOAD_INT'
    ;
P_LOAD_LONG
    : 'P_LOAD_LONG'
    ;
P_LOAD_FLOAT
    : 'P_LOAD_FLOAT'
    ;
P_LOAD_DOUBLE
    : 'P_LOAD_DOUBLE'
    ;
P_LOAD_REF
    : 'P_LOAD_REF'
    ;
F_LOAD_BOOL
    : 'F_LOAD_BOOL'
    ;
F_LOAD_CHAR
    : 'F_LOAD_CHAR'
    ;
F_LOAD_BYTE
    : 'F_LOAD_BYTE'
    ;
F_LOAD_INT
    : 'F_LOAD_INT'
    ;
F_LOAD_LONG
    : 'F_LOAD_LONG'
    ;
F_LOAD_FLOAT
    : 'F_LOAD_FLOAT'
    ;
F_LOAD_DOUBLE
    : 'F_LOAD_DOUBLE'
    ;
F_LOAD_REF
    : 'F_LOAD_REF'
    ;
P_STORE_BOOL
    : 'P_STORE_BOOL'
    ;
P_STORE_CHAR
    : 'P_STORE_CHAR'
    ;
P_STORE_BYTE
    : 'P_STORE_BYTE'
    ;
P_STORE_INT
    : 'P_STORE_INT'
    ;
P_STORE_LONG
    : 'P_STORE_LONG'
    ;
P_STORE_FLOAT
    : 'P_STORE_FLOAT'
    ;
P_STORE_DOUBLE
    : 'P_STORE_DOUBLE'
    ;
P_STORE_REF
    : 'P_STORE_REF'
    ;
F_STORE_BOOL
    : 'F_STORE_BOOL'
    ;
F_STORE_CHAR
    : 'F_STORE_CHAR'
    ;
F_STORE_BYTE
    : 'F_STORE_BYTE'
    ;
F_STORE_INT
    : 'F_STORE_INT'
    ;
F_STORE_LONG
    : 'F_STORE_LONG'
    ;
F_STORE_FLOAT
    : 'F_STORE_FLOAT'
    ;
F_STORE_DOUBLE
    : 'F_STORE_DOUBLE'
    ;
F_STORE_REF
    : 'F_STORE_REF'
    ;
P_REF_AT
    : 'P_REF_AT'
    ;
T_REF_AT
    : 'T_REF_AT'
    ;
FROM_REF_LOAD_BOOL
    : 'FROM_REF_LOAD_BOOL'
    ;
FROM_REF_LOAD_CHAR
    : 'FROM_REF_LOAD_CHAR'
    ;
FROM_REF_LOAD_BYTE
    : 'FROM_REF_LOAD_BYTE'
    ;
FROM_REF_LOAD_INT
    : 'FROM_REF_LOAD_INT'
    ;
FROM_REF_LOAD_LONG
    : 'FROM_REF_LOAD_LONG'
    ;
FROM_REF_LOAD_FLOAT
    : 'FROM_REF_LOAD_FLOAT'
    ;
FROM_REF_LOAD_DOUBLE
    : 'FROM_REF_LOAD_DOUBLE'
    ;
FROM_REF_LOAD_REF
    : 'FROM_REF_LOAD_REF'
    ;
TO_REF_STORE_BOOL
    : 'TO_REF_STORE_BOOL'
    ;
TO_REF_STORE_CHAR
    : 'TO_REF_STORE_CHAR'
    ;
TO_REF_STORE_BYTE
    : 'TO_REF_STORE_BYTE'
    ;
TO_REF_STORE_INT
    : 'TO_REF_STORE_INT'
    ;
TO_REF_STORE_LONG
    : 'TO_REF_STORE_LONG'
    ;
TO_REF_STORE_FLOAT
    : 'TO_REF_STORE_FLOAT'
    ;
TO_REF_STORE_DOUBLE
    : 'TO_REF_STORE_DOUBLE'
    ;
TO_REF_STORE_REF
    : 'TO_REF_STORE_REF'
    ;
REF_MEMBER
    : 'REF_MEMBER'
    ;
REF_STATIC_MEMBER
    : 'REF_STATIC_MEMBER'
    ;
INVOKE_OVERRIDE
    : 'INVOKE_OVERRIDE'
    ;
REF_METHOD
    : 'REF_METHOD'
    ;
REF_STATIC_METHOD
    : 'REF_STATIC_METHOD'
    ;
INSTATE
    : 'INSTATE'
    ;


op  : NOP
    | WASTE
    | CAST_C2I
    | CAST_B2I
    | CAST_I2B
    | CAST_I2C
    | CAST_I2L
    | CAST_I2F
    | CAST_I2D
    | CAST_L2I
    | CAST_L2F
    | CAST_L2D
    | CAST_F2I
    | CAST_F2L
    | CAST_F2D
    | CAST_D2I
    | CAST_D2L
    | CAST_D2F
    | CMP
    | ADD
    | SUB
    | MUL
    | DIV
    | REM
    | NEG
    | SHL
    | SHR
    | USHR
    | BIT_AND
    | BIT_OR
    | BIT_XOR
    | BIT_INV
    | INVOKE_FIRST
    | RETURN
    | STACK_ALLOC
    | STACK_NEW
    | WILD_ALLOC
    | WILD_NEW
    | WILD_COLLECT
    | DESTROY
//    | ARG_FLAG
    | JUMP
    | JUMP_IF_ZERO
    | JUMP_IF_NOT_ZERO
    | JUMP_IF_POSITIVE
    | JUMP_IF_NEGATIVE
    | P_LOAD_BOOL
    | P_LOAD_CHAR
    | P_LOAD_BYTE
    | P_LOAD_INT
    | P_LOAD_LONG
    | P_LOAD_FLOAT
    | P_LOAD_DOUBLE
    | P_LOAD_REF
    | F_LOAD_BOOL
    | F_LOAD_CHAR
    | F_LOAD_BYTE
    | F_LOAD_INT
    | F_LOAD_LONG
    | F_LOAD_FLOAT
    | F_LOAD_DOUBLE
    | F_LOAD_REF
    | P_STORE_BOOL
    | P_STORE_CHAR
    | P_STORE_BYTE
    | P_STORE_INT
    | P_STORE_LONG
    | P_STORE_FLOAT
    | P_STORE_DOUBLE
    | P_STORE_REF
    | F_STORE_BOOL
    | F_STORE_CHAR
    | F_STORE_BYTE
    | F_STORE_INT
    | F_STORE_LONG
    | F_STORE_FLOAT
    | F_STORE_DOUBLE
    | F_STORE_REF
    | P_REF_AT
    | T_REF_AT
    | FROM_REF_LOAD_BOOL
    | FROM_REF_LOAD_CHAR
    | FROM_REF_LOAD_BYTE
    | FROM_REF_LOAD_INT
    | FROM_REF_LOAD_LONG
    | FROM_REF_LOAD_FLOAT
    | FROM_REF_LOAD_DOUBLE
    | FROM_REF_LOAD_REF
    | TO_REF_STORE_BOOL
    | TO_REF_STORE_CHAR
    | TO_REF_STORE_BYTE
    | TO_REF_STORE_INT
    | TO_REF_STORE_LONG
    | TO_REF_STORE_FLOAT
    | TO_REF_STORE_DOUBLE
    | TO_REF_STORE_REF
    | REF_MEMBER
    | REF_STATIC_MEMBER
    | INVOKE_OVERRIDE
    | REF_METHOD
    | REF_STATIC_METHOD
    | INSTATE
    ;
