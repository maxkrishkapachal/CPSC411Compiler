// /* This file is going to host the symbol tables
// It'll have the functions that will be used to check if a symbol
// already exists in the symbol table and to add symbols to the 
// symbol table.
// Max Krishka-Pachal
// CPSC 411
// Final Project
// stable.c
// May 10th 2022


#include "stable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCOPE_H     0
#define SYMBOL_H    1

int scopeCount = 0;
ScopeStack *topOfStack = NULL;
ScopeStack scopeStack[SCOPE_HASH];

int hash(char *idToHash, int scopeOrSymbol){
    int hashLocation = 0, i = 0;
    while (idToHash[i] != '\0'){ 
        hashLocation = ((hashLocation << INT_SIZE) + idToHash[i]);
        scopeOrSymbol == SCOPE_H ? (hashLocation %= SCOPE_HASH) : (hashLocation %= SYM_HASH);
        i++;
    }
    return hashLocation;
}

void addSymbol(char *scopeName, NewNode *newSymbol){
    Symbol *tempSymbol;

    tempSymbol = malloc(sizeof(Symbol));
    if(tempSymbol == NULL) errorPrint(newSymbol, "Couldn't add symbol.", STABLE_ERR);
    
    tempSymbol->nodePointer = newSymbol;
    
    int symbolHash = hash(newSymbol->name, SYMBOL_H);
    int scopeHash = hash(scopeName, SCOPE_H);

    scopeStack[scopeHash].symbolTable[symbolHash] = malloc(sizeof(Symbol));

    scopeStack[scopeHash].symbolTable[symbolHash] = tempSymbol;
}

Symbol* findSymbol(char *scopeName, char *nodeName){
    int symbolHash = hash(nodeName, SYMBOL_H);
    int scopeHash = hash(scopeName, SCOPE_H);
    return scopeStack[scopeHash].symbolTable[symbolHash];
}

void scopePop(){
    topOfStack = topOfStack->parentScope;
}

void scopePush(char *scopeName){
    ScopeStack *tempStack = malloc(sizeof(ScopeStack));
    if(tempStack == NULL) errorPrint(NULL, "scope stack error", STABLE_ERR);

    int scopeHash = hash(scopeName, SCOPE_H);

    if(scopeStack[scopeHash].name == NULL && scopeStack[scopeHash].parentScope == NULL){
        tempStack->name = scopeName;
        tempStack->parentScope = topOfStack;
        scopeStack[scopeHash] = *tempStack;
    }

    topOfStack = &scopeStack[scopeHash];
}

void saveStackSpace(int save, Symbol *symbol){
    symbol->save = save;
}

int getStackSpace(Symbol *symbol){
    return symbol->save;
}



