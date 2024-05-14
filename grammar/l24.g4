grammar l24;		

// lexer
// keyword
If : 'if';
Then : 'then';
Else : 'else';
End : 'end';
While : 'while';
Scan : 'scan';
Print : 'print';
Int : 'int';
Return : 'return';
Const : 'const';


// literal
IntLiteral : [0-9]+;
StringLiteral:      '"' .*? '"' ;  
 
// operator
Assign:             '=' ;
Less:               '<';
Greater:            '>';
LessEq:             '<=';
GreaterEq:          '>=';
NotEq:              '!=';
Eq:                 '==';
LogicalAnd:         '&&';
LogicalOr:          '||';
Star:               '*';
Slash:              '/';
Percentage:         '%';
Plus:               '+';
Minus:              '-';
SemiColon:          ';';
Comm:               ',';
LeftBrace:          '{';
RightBrace:         '}';
LeftParen:          '(';
RightParen:         ')';
Not:                '!';

//标识符
Ident :                [a-zA-Z_][a-zA-Z0-9_]*;
 
//空白字符，抛弃
Whitespace:         [ \t]+ -> skip;
Newline:            ( '\r' '\n'?|'\n')-> skip;

// comments, skip
BlockComments:           '/*'.*?'*/' -> skip;
LINECOMMENT:             '//' ~[\r\n]* -> skip;


// parser

program
    : func 
    ;

func
    : 'int' Ident '(' ')' block
    ;


block
    : '{' (blockItem)* '}'
    ;

blockItem
    : decl
    | stmt
    ;

decl
    : constDecl
    | varDecl
    ;

stmt
    : 'return' (exp)? ';'
    | lVal '=' exp ';'
    | (exp)? ';'
    | block
    | 'if' '(' exp ')' 'then' stmt ('else' stmt)? 'end'
    ;

constDecl
    : 'const' bType constDef (',' constDef)* ';'
    ;

varDecl
    : bType varDef (','  varDef)* ';'
    ;

bType
    : 'int'
    ;

constDef
    : Ident '=' constInitVal
    ;
varDef
    : Ident
    | Ident '=' initVal
    ;

constInitVal
    : constExp
    ;
initVal
    : exp
    ;

constExp
    : exp
    ;

lVal
    : Ident
    ;

exp
    : lOrExp
    ;

lOrExp
    : lAndExp
    | lOrExp '||' lAndExp
    ;
lAndExp
    : eqExp
    | lAndExp '&&' eqExp
    ;

eqExp
    : relExp
    | eqExp '==' relExp
    | eqExp '!=' relExp
    ;

relExp
    : addExp
    | relExp '<' addExp
    | relExp '>' addExp
    | relExp '<=' addExp
    | relExp '>=' addExp
    ;

addExp
    : mulExp
    | addExp '+' mulExp
    | addExp '-' mulExp;

mulExp
    : unaryExp
    | mulExp '*' unaryExp
    | mulExp '/' unaryExp
    | mulExp '%' unaryExp;

unaryExp
    : primaryExp
    | unaryOp unaryExp
    ;

unaryOp
    : Plus
    | Minus
    | Not
    ;

primaryExp
    : '(' exp ')'
    | number
    | lVal
    ;


number 
    : IntLiteral
    ;
