//Max Krishka-Pachal

//This is going to have the header tokens for the scanner
//I made this file earlier :)
/* This is the header file for the scanner that holds
the token values and the variables that go between the lexer and c file
Max Krishka-Pachal
CPSC 411
Final Project
utils.h
May 10th 2022
*/


#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>

#define CHILDNODECOUNT  3
#define LINE_START      100000

typedef enum { //error flags
    FILE_ERR,
    SCAN_ERR,
    PARSE_ERR,
    STABLE_ERR,
    SEM_ERR,
    GEN_ERR,
    PRINT_ERR
} ErrorFlag;

typedef enum {
    PROG_BEGIN,
    //types of delcarations
    VAR_D,
    MAIN_D,
    FUNC_D,
    PARA_D,
    //types of expressions
    OP_E,
    ID_E,
    CONST_E,
    STATE_E,
    ASSIGN_E,
    //types of statements
    IF_S,
    IFELSE_S,
    BREAK_S,
    WHILE_S,
    RETURN_S,
    FUNCCALL_S,
    EMPTY_S,
    SEMICOLON_S,
    //types of parameters/arguments
    FUNC_PARA,
    FUNC_ARG,
    //block (misc)
    BLOCK_M,
    //runtime libraries
    LIBRARY_RT
} NodeType;

typedef enum {
    VOID_T = 0,
    INT_T,
    BOOL_T,
    STR_T,
    NONE
} VariableType;

typedef struct treeNode { //the nodes of each tree
    struct treeNode *childNode[CHILDNODECOUNT], *sisterNode; //children and sisternodes
    NodeType nType; //the type of statement that it is
    VariableType vType; //if it has a return or delcaration type
    char *name; //the name, or string value that it has
    char *scope; 
    int lineStart, lineEnd, argParaSize; //the line it appears on and the number of arguments or parameters it has
} NewNode;

void errorPrint(NewNode *tree, char *message, ErrorFlag SectionFlag);
NewNode *createNewNode(NodeType nodeType, VariableType varType, int lineStart, int lineEnd, char *name); //the declaration of the new nodes
void parsePrint(NewNode *tree); //tree print declaration
void semanticPrint(NewNode *tree);

extern FILE *outputCode;
extern FILE *yyin;   //holds the file that's being read
extern int yylex(); //the lexer!
extern int yylineno; //the line number for the lexer
extern char* yytext; //the return from the lexer
extern int strPtr;   //the pointer for the string so it can be printed properly
extern int lineCounter; //for counting the proper line a token is on
extern int lineStartCount[LINE_START];
extern int lineCountIndex;

#endif

