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


// literal
IntLiteral : [0-9]+;
StringLiteral:      '"' .*? '"' ;  
 
// operator
AssignmentOP:       '=' ;    
RelationalOP:       '>'|'>='|'<' |'<=' | '!=' | '==';    
Star:               '*';
Slash:              '/';
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
    : Int Ident LeftParen RightParen block 
    ;


block
    : LeftBrace stmt RightBrace
    ;


stmt
    : Return exp SemiColon
    ;

exp
    : unaryExp
    ;

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
    : LeftParen exp RightParen
    | number
    ;


number 
    : IntLiteral
    ;
