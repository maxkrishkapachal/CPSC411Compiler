//bison file meant for parsing the tokens and determining how they fit together structurally
//Max Krishka-Pachal
//CPSC 411
//Final Project
//Parser.y
//May 10th 2022

//this file is going to be for the actual parsing
//this is the big guy!
//
%{
#include "parser.h" //this is my header for the tree nodes
//#include "scanner.h"
#include "utils.h"
#include <stdlib.h>

int yyerror(char *message); //catches any errors in the parsed code

static NewNode *treeFull; //the full tree once the parsing is complete
NewNode *tempNode; //for creating sisterNodes
VariableType tempVType; //allows me to grab the type of an identifier or function

int argParaCounter = 0; //counts the number of parameters and arguments during function calls and declarations

%}

%union{ 
  char *name; //for getting the string value of strings, identifiers, numbers
  NewNode *node; //for the other pieces that will create nodes for the ast
  VariableType vType; //used for bool and int 
}

%start	        start //the beginning flag

%token <name>   NUM ID STR COM TRUE FALSE //values that will have a string name
%token <vType>  VOID INT BOOL //values that will have a variable type enum
%token          IF ELSE WHILE BREAK RETURN ERROR
%token <name>   EQ NEQ LT GT LE GE AND OR NOT //values that have string names as well - for operator prints
%token <name>   ADD SUB MULT DIV PERC ASSIGN //also have string names - for operator prints
%token          SEMICOLON COMMA LBRACK RBRACK LCURLY RCURLY

%type <node>    globaldeclarations globaldeclaration variabledeclaration functiondeclaration block
%type <node>    assignment functiondeclarator formalparameterlist formalparameter mainfunctiondeclaration
%type <node>    mainfunctiondeclarator blockstatements blockstatement statement statementexpression
%type <node>    primary argumentlist functioninvocation postfixexpression unaryexpression expression
%type <node>    multiplicativeexpression additiveexpression relationalexpression equalityexpression
%type <node>    conditionalandexpression conditionalorexpression assignmentexpression functionheader 
%type <name>    identifier literal //these ones have to return the string name 
%type <vType>   type //this one returns the variable enum value


%%

start                       : /* empty */
                            | globaldeclarations { //grabs the entire tree
                                tempNode = createNewNode(PROG_BEGIN, VOID_T, 0, 0, "");
                                tempNode -> childNode[0] = $1;
                                treeFull = tempNode;
                            }
                            ;

literal                     : NUM {
                                tempVType = INT_T;
                                $$ = $1;
                            }
                            | STR {
                                tempVType = STR_T;
                                $$ = $1;
                            }
                            | TRUE {
                                tempVType = BOOL_T;
                                $$ = "true";
                            }
                            | FALSE {
                                tempVType = BOOL_T;
                                $$ = "false";
                            }
                            ;

type                        : BOOL {
                                $$ = BOOL_T;
                            }
                            | INT {
                                $$ = INT_T;
                            }
                            ;

globaldeclarations          : globaldeclaration {
                                $$ = $1;
                            }
                            | globaldeclarations globaldeclaration {
                                //checks if there is a sisternode, and as long as there is
                                //it goes to the sisternode of that node to find the next
                                //available node
                                tempNode = $1;
                                if(tempNode != NULL) {
                                    while(tempNode -> sisterNode != NULL) tempNode = tempNode -> sisterNode;
                                    tempNode -> sisterNode = $2;
                                    $$ = $1;
                                }
                                //otherwise, it places it as the first sisternode
                                else $$ = $2;
                            }
                            ;

globaldeclaration           : variabledeclaration {
                                $$ = $1;
                            }
                            | functiondeclaration {
                                $$ = $1;
                            }
                            | mainfunctiondeclaration {
                                $$ = $1;
                            }
                            ;

variabledeclaration         : type identifier SEMICOLON {
                                $$ = createNewNode(VAR_D, $1, lineCounter, 0, $2);
                            }
                            ;

identifier                  : ID {
                                $$ = $1;
                            }
                            ;

functiondeclaration         : functionheader block {
                                $$ = $1;
                                $$ -> childNode[1] = $2;
                            }
                            ;

functionheader              : type functiondeclarator {
                                $$ = $2;
                                $$ -> vType = $1;
                            }
                            | VOID functiondeclarator {
                                $$ = $2;
                                $$ -> vType = VOID_T;
                            }
                            ;

functiondeclarator          : identifier LBRACK formalparameterlist RBRACK {
                                $$ = createNewNode(FUNC_D, VOID_T, lineCounter, 0, $1);
                                
                                $$ -> childNode[0] = createNewNode(FUNC_PARA, VOID_T, lineCounter, 0, "");
                                $$ -> childNode[0] -> argParaSize = argParaCounter;
                                argParaCounter = 0;

                                $$ -> childNode[0] -> childNode[0] = $3;
                            }
                            | identifier LBRACK RBRACK {
                                $$ = createNewNode(FUNC_D, VOID_T, lineCounter, 0, $1);
                                $$ -> childNode[0] = createNewNode(FUNC_PARA, VOID_T, lineCounter, 0, "");
                                $$ -> childNode[0] -> argParaSize = 0;
                            }
                            ;

formalparameterlist         : formalparameter {
                                $$ = $1;
                            }
                            | formalparameterlist COMMA formalparameter {
                                tempNode = $1;
                                if(tempNode != NULL) {
                                    while(tempNode -> sisterNode != NULL) tempNode = tempNode -> sisterNode;
                                    tempNode -> sisterNode = $3;
                                    $$ = $1;
                                }
                                else $$ = $1;
                            }
                            ;

formalparameter             : type identifier {
                                $$ = createNewNode(PARA_D, $1, lineCounter, 0, $2);
                                argParaCounter++;
                            }
                            ;

mainfunctiondeclaration     : mainfunctiondeclarator block {
                                $$ = $1;
                                $$ -> childNode[0] = $2;
                            }
                            ;

mainfunctiondeclarator      : identifier LBRACK RBRACK {
                                $$ = createNewNode(MAIN_D, VOID_T, lineCounter, 0, $1);
                            }
                            ;

block                       : LCURLY blockstatements RCURLY {
                                lineCountIndex--;
                                $$ = createNewNode(BLOCK_M, VOID_T, lineStartCount[lineCountIndex], lineCounter, "");
                                $$ -> childNode[0] = $2;
                            }
                            | LCURLY RCURLY {
                                lineCountIndex--;
                                $$ = createNewNode(EMPTY_S, VOID_T, lineStartCount[lineCountIndex], lineCounter, "");
                            }
                            ;

blockstatements             : blockstatement {
                                $$ = $1;
                            }
                            | blockstatements blockstatement {
                                tempNode = $1;
                                if(tempNode != NULL) {
                                    while(tempNode -> sisterNode != NULL) tempNode = tempNode -> sisterNode;
                                    tempNode -> sisterNode = $2;
                                    $$ = $1;
                                }
                                else $$ = $2;
                            }
                            ;

blockstatement              : variabledeclaration {
                                $$ = $1;
                            }
                            | statement {
                                $$ = $1;
                            }
                            ;

statement                   : block {
                                $$ = $1;
                            }
                            | SEMICOLON {
                                $$ = createNewNode(SEMICOLON_S, VOID_T, lineCounter, 0, "");
                            }
                            | statementexpression SEMICOLON {
                                $$ = $1;
                            }
                            | BREAK SEMICOLON {
                                $$ = createNewNode(BREAK_S, VOID_T, lineCounter, 0, "");
                            }
                            | RETURN expression SEMICOLON {
                                $$ = createNewNode(RETURN_S, VOID_T, lineCounter, 0, "");
                                $$ -> childNode[0] = $2;
                            }
                            | RETURN SEMICOLON {
                                $$ = createNewNode(RETURN_S, VOID_T, lineCounter, 0, "");
                            }
                            | IF LBRACK expression RBRACK statement {
                                lineCountIndex--;
                                $$ = createNewNode(IF_S, VOID_T, lineStartCount[lineCountIndex], lineCounter, "");
                                $$ -> childNode[0] = $3;
                                $$ -> childNode[1] = $5;
                            }
                            | IF LBRACK expression RBRACK statement ELSE statement {
                                lineCountIndex -= 2;
                                $$ = createNewNode(IF_S, VOID_T, lineStartCount[lineCountIndex], lineStartCount[lineCountIndex + 1], "");
                                $$ -> childNode[0] = $3;
                                $$ -> childNode[1] = $5;

                                $$ -> sisterNode = createNewNode(IFELSE_S, VOID_T, lineStartCount[lineCountIndex + 1], lineCounter, "");
                                $$ -> sisterNode -> childNode[0] = $7;
                            }
                            | WHILE LBRACK expression RBRACK statement {
                                lineCountIndex--;
                                $$ = createNewNode(WHILE_S, VOID_T, lineStartCount[lineCountIndex], lineCounter, "");
                                $$ -> childNode[0] = $3;
                                $$ -> childNode[1] = $5;
                            }
                            ;

statementexpression         : assignment {
                                $$ = $1;
                            }
                            | functioninvocation {
                                $$ = $1;
                            }
                            ;

primary                     : literal {
                                $$ = createNewNode(CONST_E, tempVType, lineCounter, 0, $1);
                            }
                            | LBRACK expression RBRACK {
                                $$ = $2;
                            }
                            | functioninvocation {
                                $$ = $1;
                            }
                            ;

argumentlist                : expression {
                                argParaCounter++;
                                $$ = $1;
                            }
                            | argumentlist COMMA expression {
                                tempNode = $1;
                                if(tempNode != NULL) {
                                    while(tempNode -> sisterNode != NULL) tempNode = tempNode -> sisterNode;
                                    argParaCounter++;
                                    tempNode -> sisterNode = $3;
                                    $$ = $1;
                                }
                                else {
                                    argParaCounter++;
                                    $$ = $3;
                                }
                                $$ -> lineStart = lineCounter;
                            }
                            ;

functioninvocation          : identifier LBRACK argumentlist RBRACK {
                                $$ = createNewNode(FUNCCALL_S, VOID_T, lineCounter, 0, $1);
                                
                                $$ -> childNode[0] = createNewNode(FUNC_ARG, VOID_T, lineCounter, 0, "");
                                $$ -> childNode[0] -> childNode[0] = $3;
                                $$ -> childNode[0] -> argParaSize = argParaCounter;
                                
                                argParaCounter = 0;
                            }
                            | identifier LBRACK RBRACK {
                                $$ = createNewNode(FUNCCALL_S, VOID_T, lineCounter, 0, $1);
                                $$ -> childNode[0] = createNewNode(FUNC_ARG, VOID_T, lineCounter, 0, "");
                            }
                            ;

postfixexpression           : primary {
                                $$ = $1;
                            }
                            | identifier {
                                $$ = createNewNode(ID_E, VOID_T, lineCounter, 0, $1);
                            }
                            ;

unaryexpression             : SUB unaryexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $1);
                                $$ -> childNode[0] = $2;
                            }
                            | NOT unaryexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $1);
                                $$ -> childNode[0] = $2;
                            }
                            | postfixexpression {
                                $$ = $1;
                            }
                            ;

multiplicativeexpression    : unaryexpression {
                                $$ = $1;
                            }
                            | multiplicativeexpression MULT unaryexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | multiplicativeexpression DIV unaryexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | multiplicativeexpression PERC unaryexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            ;

additiveexpression          : multiplicativeexpression {
                                $$ = $1;
                            }
                            | additiveexpression ADD multiplicativeexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | additiveexpression SUB multiplicativeexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            ;

relationalexpression        : additiveexpression {
                                $$ = $1;
                            }
                            | relationalexpression LT additiveexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | relationalexpression GT additiveexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | relationalexpression LE additiveexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | relationalexpression GE additiveexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            ;

equalityexpression          : relationalexpression {
                                $$ = $1;
                            }
                            | equalityexpression EQ relationalexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            | equalityexpression NEQ relationalexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            ;

conditionalandexpression    : equalityexpression {
                                $$ = $1;
                            }
                            | conditionalandexpression AND equalityexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            ;

conditionalorexpression     : conditionalandexpression {
                                $$ = $1;
                            }
                            | conditionalorexpression OR conditionalandexpression {
                                $$ = createNewNode(OP_E, VOID_T, lineCounter, 0, $2);
                                $$ -> childNode[0] = $1;
                                $$ -> childNode[1] = $3;
                            }
                            ;

assignmentexpression        : conditionalorexpression {
                                $$ = $1;
                            }
                            | assignment {
                                $$ = $1;
                            }
                            ;

assignment                  : identifier ASSIGN assignmentexpression {
                                $$ = createNewNode(ASSIGN_E, VOID_T, lineCounter, 0, $2);

                                $$ -> childNode[0] = createNewNode(STATE_E, VOID_T, lineCounter, 0, $1);
                                $$ -> childNode[1] = $3;
                            }
                            ;

expression                  : assignmentexpression {
                                $$ = $1;
                            }
                            ;

%%


int yyerror(char *message){ //standard error message for token patterns that don't match the parser grammar
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "Syntax error at line %d with token %s: %s\n", lineCounter, yytext, message);
  errorPrint(NULL, buffer, SCAN_ERR);
  //includes the line, token, and message
  return 0;
}

NewNode *parse(){ //the call to parse
    yyparse(); //begins the parsing process
    return treeFull; //and returns the fully parsed tree for printing
}
    

