/* This file is going to host the symbol tables
It'll have the functions that will be used to check if a symbol
already exists in the symbol table and to add symbols to the 
symbol table.
Max Krishka-Pachal
CPSC 411
Final Project
stable.h
May 10th 2022
*/


#ifndef _STABLE_H_
#define _STABLE_H_

#include "utils.h"

#define SYM_HASH        5381
#define SCOPE_HASH      97
#define INT_SIZE        8

//runtime libraries
#define GETCHAR_RL      "getchar"
#define HALT_RL         "halt"
#define PRINTB_RL       "printb"
#define PRINTC_RL       "printc"
#define PRINTI_RL       "printi"
#define PRINTS_RL       "prints"
#define ERROR_RL        "ERROR"

//runtime library parameters
#define PRINTB_P        "b"
#define PRINTC_P        "c"
#define PRINTI_P        "i"
#define PRINTS_P        "s"
#define ERROR_P         "e"

//beginning
#define PROG_NAME       "BEGINNINGOFTHEPROGRAM"

//buffer
#define LABEL_BUFFER    10

typedef struct {
    NewNode *nodePointer;
    char label[10];
    int save;
} Symbol;

typedef struct FullScopeStack {
    Symbol *symbolTable[SYM_HASH];
    struct FullScopeStack *parentScope;
    char *name;
} ScopeStack;

extern ScopeStack *topOfStack;

Symbol* findSymbol(char *scopeName, char *nodeName);
void addSymbol(char *scopeName, NewNode *newSymbol);
void scopePop();
void scopePush(char *scopeName);
void saveStackSpace(int save, Symbol *symbol);
int getStackSpace(Symbol *symbol);

#endif