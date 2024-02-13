/*This is my current compiler code 
This file is for printing the tree node
Max Krishka-Pachal
CPSC 411
Final Project
utils.c
May 10th 2022
*/

#include "utils.h"
#include "stable.h"
#include <stdlib.h>

static int tabNum = 0; //this is for how many indents to add when printing

#define TAB     tabNum += 1 //these little mini functions just move the indent back and forth
#define UNTAB   tabNum -= 1


void errorPrint(NewNode *tree, char *message, ErrorFlag SectionFlag){
    switch(SectionFlag){
        case FILE_ERR:
            fprintf(stderr, "File Error: %s", message);
            break;
        case SCAN_ERR:
            fprintf(stderr, "Scanning Error: %s", message);
            break;
        case PARSE_ERR: 
            fprintf(stderr, "Parsing Error in line %d: %s", tree->lineStart, message);
            break;
        case STABLE_ERR:
            fprintf(stderr, "Symbol Table Error in line %d: %s", tree->lineStart, message);
            break;
        case SEM_ERR:
            fprintf(stderr, "Semantic Analysis Error in line %d: %s", tree->lineStart, message);
            break;
        case GEN_ERR:
            fprintf(stderr, "Code Generation Error in line %d: %s", tree->lineStart, message);
            break;
        case PRINT_ERR:
            fprintf(stderr, "Printing Error: %s", message);
            break;
    }
    exit(EXIT_FAILURE);
}

NewNode *createNewNode(NodeType nodeType, VariableType varType, int lineStart, int lineEnd, char *name){ //this thing creates new nodes for the tree
    NewNode *tempNode = (NewNode *) malloc(sizeof(NewNode)); //it makes enough size for the node to be in the tree
    if(tempNode == NULL) errorPrint(tempNode, "createNewNode failed.\n", PARSE_ERR); //error checking for the node
    else {
        for(int i = 0; i < CHILDNODECOUNT; i++) tempNode->childNode[i] = NULL; //makes sure that all the children are null
        tempNode->sisterNode = NULL; //makes sure the sisternode is null too, this is for printing reasons later
        tempNode->nType = nodeType; //this holds the specified type of node
        tempNode->vType = varType; //VOID_T; //and this is the variable type, this isn't relevant in all cases, so it inits to void
        tempNode->lineStart = lineStart; //0; //inits all line numbers to 0, this will be changed later
        tempNode->lineEnd = lineEnd;
        tempNode->name = name;
    }
    return tempNode; //then it returns the whole node it just made
}

static void tabPrint(){ //this prints al lthe tabs before the actual function statement
    for(int i = 0; i < tabNum; i++) fprintf(stdout, "   ");
}


void parsePrint(NewNode *tree){ //this prints the tree statements, it's recursive 
    TAB; //the initial one is also indented, just to keep it separate from the rest of the code
    while(tree != NULL){ //this cycles around through the whole tree 
        tabPrint(); //prints the tabs out
        switch(tree->nType){ //each of these is a type of node 
            case PROG_BEGIN: //for the beginning of the program
                fprintf(stdout, " { Program Begins }\n");
                break;
            case VAR_D: //this is for declared variables 
                fprintf(stdout, " { Declared Variable: %s, Variable Type: %s, Line: %d }\n", 
                        tree->name, tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->lineStart);
                break;
            case MAIN_D:
                fprintf(stdout, " { Main Function: %s(), Line: %d }\n", 
                        tree->name, tree->lineStart);
                break;
            case FUNC_D: //this is for declared functions 
                fprintf(stdout, " { Declared Function: %s(), Return Type: %s, Line: %d }\n", 
                        tree->name, tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->lineStart);
                break;
            case PARA_D: //this is for declared parameters inside newly declared functions
                fprintf(stdout, " { Function Parameter: %s, Variable Type: %s, Line: %d }\n", 
                        tree->name, tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->lineStart);
                break;
            case OP_E: //this is for all operators
                fprintf(stdout, " { Operator: %s, Line: %d }\n", 
                        tree->name, tree->lineStart);
                break;
            case ID_E: //all identifiers are here
                fprintf(stdout, " { Identifier: %s, Line: %d }\n", 
                        tree->name, tree->lineStart);
                break;
            case CONST_E: //this is for all constants or literals
                fprintf(stdout, " { Literal: %s, Line: %d }\n", 
                        tree->name, tree->lineStart);
                break;
            case STATE_E: //this is for assignment statements
                fprintf(stdout, " { Assignment Variable: %s, Line: %d }\n", 
                        tree->name, tree->lineStart);
                break;
            case ASSIGN_E: //this is for the actual = sign
                fprintf(stdout, " { Equals: %s, Line: %d } \n", 
                        tree->name, tree->lineStart);
                break;
            case IF_S: //all if statements
                fprintf(stdout, " { If Statement, Starting At Line %d, Ending At Line %d }\n", tree->lineStart, tree->lineEnd);
                break;
            case IFELSE_S: //this is the else part of the if statement
                fprintf(stdout, " { Else Statement, Starting At Line %d, Ending At Line %d }\n", tree->lineStart, tree->lineEnd);
                break;
            case BREAK_S: //this is for break statements
                fprintf(stdout, " { Break Statement, Line: %d }\n", tree->lineStart);
                break;
            case WHILE_S: //this is for while loops
                fprintf(stdout, " { While Statement, Starting At Line %d, Ending At Line %d }\n", tree->lineStart, tree->lineEnd);
                break;
            case RETURN_S: //this is for return statements
                fprintf(stdout, " { Return Statement, Line: %d }\n", tree->lineStart);
                break;
            case FUNCCALL_S: //this is for function calls, as opposed to declarations 
                fprintf(stdout, " { Function Call: %s(), Argument Count: %d, Line: %d }\n", 
                    tree->name, tree->childNode[0] != NULL ? tree->childNode[0]->argParaSize : 0, tree->lineStart);
                break;
            case EMPTY_S: //empty statements get the same treatment as semicolon statements
            case SEMICOLON_S: //they're both empty
                fprintf(stdout, " { Empty Statement, Line: %d }\n", tree->lineStart);
                break;
            case FUNC_PARA: //this is for when a function is created and has parameters, just to count them
                fprintf(stdout, " { Function Parameter Count, Line: %d }\n", tree->lineStart);
                break;
            case FUNC_ARG: //this is for counting the arguments when a function is called
                fprintf(stdout, " { Function Argument Count, Line: %d }\n", tree->lineStart);
                break;
            case BLOCK_M: //this is for block statements 
                fprintf(stdout, " { Block, Starting At Line %d, Ending At Line %d }\n", tree->lineStart, tree->lineEnd);
                break;
            default: //this is for situations where the node can't be read, which shouldn't happen.
                errorPrint(tree, "Unknown node type.\n", PRINT_ERR);
                break;
        }
        for(int i = 0; i < CHILDNODECOUNT; i++) { //then it goes through all the children of a node
            parsePrint(tree->childNode[i]); //if there's a child, we do all this again but with the child
        }
        tree = tree->sisterNode; //once the children have been printed, it's the sisternode's turn
    }
    UNTAB; //every time it gets to the end, it untabs to take the indents away
}

// void semanticPrint(NewNode *tree){ //this prints the tree statements, it's recursive 
//     TAB; //the initial one is also indented, just to keep it separate from the rest of the code
//     while(tree != NULL){ //this cycles around through the whole tree 
//         sTable symbol = findSymbol(hashName(tree->name), tree->name, tree->scope);
//         tabPrint(); //prints the tabs out
//         switch(tree->nType){ //each of these is a type of node 
//             case PROG_BEGIN: //for the beginning of the program
//                 fprintf(stdout, " { Program Begins, Is Scope: %d }\n", tree->scope);
//                 break;
//             case VAR_D: //this is for declared variables 
//                 fprintf(stdout, " { Declared Variable: %s, Variable Type: %s, In Scope: %d, Symbol Table Location: %p, Line: %d }\n", 
//                         symbol->identifier, symbol->vType == INT_T ? "integer" : (symbol->vType == BOOL_T ? "boolean" : "void"), symbol->scopeLevel, &symbol, symbol->lines->lineno);
//                 break;
//             case MAIN_D:
//                 fprintf(stdout, " { Main Function: %s(), Return Type: void, In Scope: %d, Is Scope: %d, Symbol Table Location: %p, Line: %d }\n", 
//                         symbol->identifier, symbol->scopeLevel, symbol->scopeGovern + scopeLayer[symbol->scopeLevel].parentTable, 
//                         &symbol, symbol->lines->lineno);
//                 break;
//             case FUNC_D: //this is for declared functions 
//                 fprintf(stdout, " { Declared Function: %s(), Return Type: %s, In Scope: %d, Is Scope: %d, Symbol Table Location: %p, Line: %d }\n", 
//                         symbol->identifier, symbol->vType == INT_T ? "integer" : (symbol->vType == BOOL_T ? "boolean" : "void"), 
//                         symbol->scopeLevel, symbol->scopeGovern, &symbol, symbol->lines->lineno);
//                 break;
//             case PARA_D: //this is for declared parameters inside newly declared functions
//                 fprintf(stdout, " { Function Parameter: %s, Variable Type: %s, In Scope: %d, Symbol Table Location: %p, Line: %d }\n", 
//                         symbol->identifier, symbol->vType == INT_T ? "integer" : (symbol->vType == BOOL_T ? "boolean" : "void"), 
//                         symbol->scopeLevel, &symbol, symbol->lines->lineno);
//                 break;
//             case OP_E: //this is for all operators
//                 fprintf(stdout, " { Operator: %s, Variable Type: %s, In Scope: %d, Line: %d }\n", 
//                         tree->name,  tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart);
//                 break;
//             case ID_E: //all identifiers are here
//                 fprintf(stdout, " { Identifier: %s, Variable Type: %s, In Scope: %d, Symbol Table Location: %p, Line: %d }\n", 
//                         symbol->identifier, symbol->vType == INT_T ? "integer" : (symbol->vType == BOOL_T ? "boolean" : "void"), 
//                         symbol->scopeLevel, &symbol, tree->lineStart);
//                 break;
//             case CONST_E: //this is for all constants or literals
//                 fprintf(stdout, " { Literal: %s, Variable Type: %s, In Scope: %d, Line: %d }\n", 
//                         tree->name, tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "string"), tree->scope, tree->lineStart);
//                 break;
//             case STATE_E: //this is for assignment statements
//                 fprintf(stdout, " { Assignment Variable: %s, Variable Type: %s, In Scope: %d, Symbol Table Location: %p, Line: %d }\n", 
//                         symbol->identifier, symbol->vType == INT_T ? "integer" : (symbol->vType == BOOL_T ? "boolean" : "void"), 
//                         symbol->scopeLevel, &symbol, tree->lineStart);
//                 break;
//             case ASSIGN_E: //this is for the actual = sign
//                 fprintf(stdout, " { Equals: %s, Variable Type: %s, In Scope: %d, Line: %d } \n", 
//                         tree->name,  tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart);
//                 break;
//             case IF_S: //all if statements
//                 fprintf(stdout, " { If Statement, Variable Type: %s, In Scope: %d, Starting At Line %d, Ending At Line %d }\n", 
//                         tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart, tree->lineEnd);
//                 break;
//             case IFELSE_S: //this is the else part of the if statement
//                 fprintf(stdout, " { Else Statement, Variable Type: %s, In Scope: %d, Starting At Line %d, Ending At Line %d }\n", 
//                     tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart, tree->lineEnd);
//                 break;
//             case BREAK_S: //this is for break statements
//                 fprintf(stdout, " { Break Statement, Variable Type: %s, In Scope: %d, Line: %d }\n", 
//                     tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart);
//                 break;
//             case WHILE_S: //this is for while loops
//                 fprintf(stdout, " { While Statement, Variable Type: %s, In Scope: %d, Starting At Line %d, Ending At Line %d }\n", 
//                     tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart, tree->lineEnd);
//                 break;
//             case RETURN_S: //this is for return statements
//                 fprintf(stdout, " { Return Statement, Variable Type: %s, In Scope: %d, Line: %d }\n", 
//                     tree->vType == INT_T ? "integer" : (tree->vType == BOOL_T ? "boolean" : "void"), tree->scope, tree->lineStart);
//                 break;
//             case FUNCCALL_S: //this is for function calls, as opposed to declarations
//                 fprintf(stdout, " { Function Call: %s(), Return Type: %s, In Scope: %d, Argument Count: %d, Line: %d }\n", 
//                     symbol->identifier, symbol->vType == INT_T ? "integer" : (symbol->vType == BOOL_T ? "boolean" : "void"), 
//                     symbol->scopeLevel, tree->childNode[0] != NULL ? tree->childNode[0]->argParaSize : 0, tree->lineStart);
//                 break;
//             case EMPTY_S: //empty statements get the same treatment as semicolon statements
//             case SEMICOLON_S: //they're both empty
//                 fprintf(stdout, " { Empty Statement, In Scope: %d, Line: %d }\n", tree->scope, tree->lineStart);
//                 break;
//             case FUNC_PARA: //this is for when a function is created and has parameters, just to count them
//                 fprintf(stdout, " { Function Parameter Count, In Scope: %d, Line: %d }\n", tree->scope, tree->lineStart);
//                 break;
//             case FUNC_ARG: //this is for counting the arguments when a function is called
//                 fprintf(stdout, " { Function Argument Count, In Scope: %d, Line: %d }\n", tree->scope, tree->lineStart);
//                 break;
//             case BLOCK_M: //this is for block statements 
//                 fprintf(stdout, " { Block, In Scope: %d, Starting At Line %d, Ending At Line %d }\n", 
//                     tree->scope, tree->lineStart, tree->lineEnd);
//                 break;
//             default: //this is for situations where the node can't be read, which shouldn't happen.
//                 errorPrint(tree, "Unkown node type.\n", PRINT_ERR);
//                 break;
//         }
//         //printf("Z\n");
//         for(int i = 0; i < CHILDNODECOUNT; i++) { //then it goes through all the children of a node
//             semanticPrint(tree->childNode[i]); //if there's a child, we do all this again but with the child
//         }
//         tree = tree->sisterNode; //once the children have been printed, it's the sisternode's turn
//     }
//     UNTAB; //every time it gets to the end, it untabs to take the indents away
// }
