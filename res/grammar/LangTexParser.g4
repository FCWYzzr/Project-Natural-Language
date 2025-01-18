parser grammar LangTexParser;
options { tokenVocab=LangTexLexer; }

module
    : content *
    ;

content
    : name_typing
    | name_assignment
    ;

name_typing
    : NAME COLON set_literal RIGHT_ARROW set_literal EOL
    ;

set_literal
    : everything_set
    | nothing_set
    | builtin_sets
    // todo
    ;

everything_set
    : LB ELLIPSIS RB
    ;

nothing_set
    : LB RB
    ;

builtin_sets
    : INT_SETS
    | FLOAT_SETS
    | CHAR_SETS
    ;

name_assignment
    : NAME (LP params RP)? assign expression
    ;

params
    : plain_params_list
    | unpacking_params_list
    ;

plain_params_list
    : NAME (COMMA NAME)*
    ;

unpacking_params_list
    : (plain_params_list COMMA?)?
        ELLIPSIS
        (COMMA? plain_params_list )?
    ;


expression
    : value
    | NAME LP value (COMMA value)* RP
    | LP expression RP
    | expression l0_bi_op expression
    | expression l1_bi_op expression
    | expression l2_bi_op expression
    | expression l3_bi_op expression
    |<assoc=right> uni_op expression
    | conditional_expression_array
    ;

conditional_expression_array
    : BEGIN LB CASES RB
          conditional_expression (EOL conditional_expression)*
      END LB CASES RB
    ;

conditional_expression
    : expression COMMA expression
    ;


value
    : set_literal
    | NAME
    | INT_LITERAL
    | LOGIC_LITERAL
    // todo
    ;

l0_bi_op
    : MUL
    | DIV
    ;

l1_bi_op
    : ADD
    | SUB
    ;

l2_bi_op
    : GT
    | GE
    | equal
    | NE
    | LE
    | LT
    ;

l3_bi_op
    : AND
    | OR
    | THEN
    | SAME_AS
    ;

uni_op
    : ADD
    | SUB

    | NOT
    ;


assign
    : ASSIGN
    | ASSIGN_OR_EQUALS
    ;

equal
    : EQ
    | ASSIGN_OR_EQUALS
    ;
