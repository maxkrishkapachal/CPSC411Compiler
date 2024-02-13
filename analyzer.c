/* This file will be used to actually do the
semantic checking
So it will do the 4 pass throughs
Max Krishka-Pachal
CPSC 411
Final Project
analyzer.c
May 10th 2022
*/

#include "stable.h"
#include "analyzer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

int argPara[5][5] = {
    //           void int bool str none
    /* void */ {  1,   0,  0,   0,  1 },
    /* int  */ {  0,   1,  0,   0,  0 },
    /* bool */ {  0,   0,  1,   0,  0 },
    /* str  */ {  0,   0,  0,   1,  0 },
    /* none */ {  1,   0,  0,   0,  1 }
};

int notOperator[5][5] = { //for !
    //           void    int    bool  str   none
    /* void */ { NONE,   NONE,  NONE, NONE, NONE },
    /* int  */ { NONE,   NONE,  NONE, NONE, NONE },
    /* bool */ { BOOL_T, NONE,  NONE, NONE, BOOL_T },
    /* str  */ { NONE,   NONE,  NONE, NONE, NONE },
    /* none */ { NONE,   NONE,  NONE, NONE, NONE }
};

int negativeOperator[5][5] = { //for -
    //           void   int   bool  str  none
    /* void */ { NONE, NONE,  NONE, NONE, NONE },
    /* int  */ { INT_T, INT_T,NONE, NONE, INT_T },
    /* bool */ { NONE, NONE,  NONE, NONE, NONE },
    /* str  */ { NONE, NONE,  NONE, NONE, NONE },
    /* none */ { NONE, NONE,  NONE, NONE, NONE }
};

int arithOperators[5][5] = { //for +, *, /, %
    //           void   int   bool  str  none
    /* void */ { NONE, NONE,  NONE, NONE, NONE },
    /* int  */ { NONE, INT_T, NONE, NONE, NONE },
    /* bool */ { NONE, NONE,  NONE, NONE, NONE },
    /* str  */ { NONE, NONE,  NONE, NONE, NONE },
    /* none */ { NONE, NONE,  NONE, NONE, NONE }
};

int relationalOperators[5][5] = { //for <, >, <=, >=
    //           void   int    bool  str  none
    /* void */ { NONE, NONE,   NONE, NONE, NONE },
    /* int  */ { NONE, BOOL_T, NONE, NONE, NONE },
    /* bool */ { NONE, NONE,   NONE, NONE, NONE },
    /* str  */ { NONE, NONE,   NONE, NONE, NONE },
    /* none */ { NONE, NONE,   NONE, NONE, NONE }
};

int equalityOperators[5][5] = { //for ==, !=
    //           void   int    bool    str  none
    /* void */ { NONE, NONE,   NONE,   NONE, NONE },
    /* int  */ { NONE, BOOL_T, NONE,   NONE, NONE },
    /* bool */ { NONE, NONE,   BOOL_T, NONE, NONE },
    /* str  */ { NONE, NONE,   NONE,   NONE, NONE },
    /* none */ { NONE, NONE,   NONE,   NONE, NONE }
};

int conditionalOperators[5][5] = { //for &&, ||
    //           void   int  bool    str   none
    /* void */ { NONE, NONE, NONE,   NONE, NONE },
    /* int  */ { NONE, NONE, NONE,   NONE, NONE },
    /* bool */ { NONE, NONE, BOOL_T, NONE, NONE },
    /* str  */ { NONE, NONE, NONE,   NONE, NONE },
    /* none */ { NONE, NONE, NONE,   NONE, NONE }
};

int mainCounter = 0;
int ifFlag = BASEFLAG, elseFlag = BASEFLAG, whileFlag = BASEFLAG, 
    returnFlag = NORETURN, blockFlag = BASEFLAG;
NewNode *returnNode = NULL;

void lineTraverse(NewNode *node){
    switch(node->nType){
        case PROG_BEGIN:
            /*push a new scope, this will be the overall parent scope
            so when we push the new scope, we want it to take the parent
            scope so it can save it in this scope. It's going to hash
            the name of the current function which will be the key for
            the new scope.
            Since this is the program beginning, we're not going to have 
            a parent scope. We will just push this one and then that'll be
            the new current scope
            I think we should initialize the libraries here, but I'll
            worry about that later.
            Then, we want to call this function again with the child node
            and the new scope
            */
            scopePush(PROG_NAME);
            lineTraverse(node->childNode[0]);
            scopePop();
            if(mainCounter == 0) errorPrint(node, "No main function detected.\n", SEM_ERR);

            break;
        case VAR_D:
            /* Check the if flag and the while flag to make sure nothing is
            being declared inside them
            find the symbol, if it already exists, error
            if it doesn't exist, add symbol
            */
            node->scope = topOfStack->name;

            if(node->vType != INT_T && node->vType != BOOL_T){
                char buffer[60];
                snprintf(buffer, sizeof(buffer), "Variable cannot be declared as type %s.\n", 
                    (node->vType == VOID_T) ? "void" : "string");
                errorPrint(node, buffer, STABLE_ERR);
            }
            
            if(ifFlag > BASEFLAG) errorPrint(node, "Variable cannot be declared within if statement.\n", SEM_ERR);
            if(whileFlag > BASEFLAG) errorPrint(node, "Varible cannot be declared within while loop.\n", SEM_ERR);
            
            Symbol *decVar = findSymbol(topOfStack->name, node->name);
            if(decVar != NULL) errorPrint(node, "Variable cannot be declared twice in the same scope.\n", STABLE_ERR);
            addSymbol(topOfStack->name, node);

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case MAIN_D:
            /* Check the main counter when declaring main, too many mains is an error
            Also increase the main counter after declaring main
            Make sure there are no parameters - I think that can't happen
                because it's a parser error in that case
            find main symbol, if it already exists, error.
            if it doesn't exist, add symbol
            scope push
            line traverse with the child and the new top of stack
            scope pop
            */
            node->scope = topOfStack->name;

            if(mainCounter == 1) errorPrint(node, "Too many main functions declared. Only one can be declared.\n", SEM_ERR);
            mainCounter++;

            Symbol *decMain = findSymbol(topOfStack->name, node->name);
            if(decMain != NULL) errorPrint(node, "Name of main function cannot be used to declare other structures.\n", STABLE_ERR);
            addSymbol(topOfStack->name, node);

            scopePush(node->name);
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }
            scopePop();
            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case FUNC_D:
            /* find the symbol, if it already exists, error
            if it doesn't exist, add symbol
            scope push
            line traverse with the child and the new top of stack
            scope pop
            */
            node->scope = topOfStack->name;

            Symbol *decFunc = findSymbol(topOfStack->name, node->name);
            if(decFunc != NULL) errorPrint(node, "Name of function cannot be used to declare other structures.\n", STABLE_ERR);
            addSymbol(topOfStack->name, node);

            scopePush(node->name);
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }
            scopePop();

            if(node->vType != VOID_T && node->vType != NONE){
                if(returnFlag == NORETURN){
                    char buffer[60];
                    snprintf(buffer, sizeof(buffer), "No return statement for function returning type %s.\n", 
                        (node->vType == INT_T) ? "integer" : (node->vType == BOOL_T ? "boolean" : "string"));
                    errorPrint(node, buffer, SEM_ERR);
                }
                else if(returnFlag != CLEANRETURN)
                    errorPrint(returnNode, "Function not able to reach return statements in all circumstances.\n", SEM_ERR);
            }
            returnFlag = NORETURN;
            returnNode = NULL;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case PARA_D:
            /* this lists the exact parameters, so it shows up after the parameter count
            so, we go
            find the symbol, if it already exists, error
            if it doesn't exist, add symbol 
            */
            node->scope = topOfStack->name;

            Symbol *decParam = findSymbol(topOfStack->name, node->name);
            if(decParam != NULL) errorPrint(node, "Name of parameter cannot be used to declare other structures, including other parameters.\n", STABLE_ERR);
            addSymbol(topOfStack->name, node);

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case ID_E:
            /* I think we'll check that something was declared later in the type match run
            So, here we have an identifier which isn't part of a declaration. 
            This we want to check with such an instance is its type and if it's declared already
            Both of those would be dealt with in the type match run.
            So here, we don't need to do anything.
            */
            node->scope = topOfStack->name;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case STATE_E:
            /* This is an assignment variable specifically. It doesn't have a new scope.
            It doesn't have a new instance of a variable. It needs to be checked for type
            matching and for being previously declared, both of which can happen in the type 
            match run. So nothing is happening here.
            */
            node->scope = topOfStack->name;

            break;
        case IF_S:
            /* We need to increment the if flag here
            Then we need to go deeper, with the traverseLine
            Then when we come back out we can decrement the if flag
            */
            node->scope = topOfStack->name;

            ifFlag++;
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }
            ifFlag--;
            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case IFELSE_S: 
            /* We need to increment the else flag
            Recurively visit this function again
            Decrement the else flag
            */
            node->scope = topOfStack->name;

            elseFlag++;
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }
            elseFlag--;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case BREAK_S:
            /* Check that the while flag is up. If it isn't, error
            */
            node->scope = topOfStack->name;

            if(whileFlag <= BASEFLAG) errorPrint(node, "Break outside of while loop.\n", SEM_ERR);

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case WHILE_S:
            /* Increment the while flag
            recursion of this function
            decrement the while flag
            */
            node->scope = topOfStack->name;

            whileFlag++;
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }
            whileFlag--;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case FUNC_PARA:
            /* This is the count before the actual parameter list. It tells you how
            many elements there are. We have to do this in a unique way somehow because
            there can be functions called inside function calls 
            So the child of this will be a function parameter, and the sister of that node
            will be the next parameter, and so on.
            Hmm... Maybe we can make some way to hold onto the node count, or we can just
            cycle through them all until there is a mismatch 
            For now, this case does nothing.
            It's going to recursively travel down though
            */
            node->scope = topOfStack->name;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case FUNCCALL_S:
            /* This is a tricky one because when a function is called, we need to do a 
            lot of stuff.
                - Check that the function exists
                - Check the functions parameter count against this argument count
                - Check the types of the parameters against the types of the arguments
                - Check the return type
                - Check that the function being called isn't main
            Most of this is going to happen in the type match run
            In fact. I think all of it will.
                - To check that the function exists, we need to have established all the 
                functions that are going to be defined
                - To check the function parameter count against the argument count, we have
                to know that the function has been established with all its symbols
                - To check the types, we need to have all the symbols
                - To check the return type, we need to have all the symbols
                - To check that main isn't being called, we have to have already established
                main, and maybe we didn't
            So this function is going to do a bunch of nothing. 
            This will call the traverseLine though
            */
            node->scope = topOfStack->name;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case FUNC_ARG:
            /* This is the list of arguments
            Because the parameters have their own little checker thing but the arguments don't
            we might have to check some sort of flag when we enter the arguments
            I'm not sure if that's necessary yet
            This will recursively go down though
            */
            node->scope = topOfStack->name;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case RETURN_S:
            /* 
            A Breakdown:
                - If there is a Return inside an If, there needs to be a Return inside an Else
                right after or there needs to be a Return after the If inside the function block
                - If there is a Return inside a While, there needs to be a Return right after 
                the While inside the function block
                - If there is a Return, there needs to be no code after it inside the same block
                - If there is a Return, the function cannot be void, otherwise it's an error
                - If there is a Return, the top of the stack cannot be the program

            So, to combat this, the code we'll have is this:
            If the top of the stack is the program, error
            If the top of the stack function has a void return type, error
            If child is null, error
            If sister node isn't null, error
            If there is a Return inside a While loop, return flag = -1
            If there is a Return inside an If statement, return flag = -2
            If there is a Return inside an Else and return flag isn't -2, return flag = -3
            If there is a Return inside an Else and return flag is -2, return flag = 0
            If there is a Return at the end of the block, return flag = 0
            Then, at the end of the checking, if the return flag == 0, it's good
            If the return flag is < 0, error depending on the number.
            We can do a switch statement with the error messages.
            Then, traverseLine with the child node

            */
            node->scope = topOfStack->name;

            if(topOfStack->parentScope == NULL) errorPrint(node, "Return outside of function.\n", SEM_ERR);
            
            Symbol *function = findSymbol(PROG_NAME, topOfStack->name);
            
            if(function->nodePointer->vType == VOID_T && node->childNode[0] != NULL)
                errorPrint(node, "Non-empty return statement not allowed in void functions.\n", SEM_ERR);
        
            if(function->nodePointer->vType != VOID_T && node->childNode[0] == NULL) 
                errorPrint(node, "Non-void function must return matching type.\n", SEM_ERR);

            returnNode = node;
            returnFlag = CLEANRETURN;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case OP_E:
            /* This will be the end of whatever node string to the bottom it is
            Nothing needs to be done here.
            */
            node->scope = topOfStack->name;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case CONST_E: 
            /* This will be the end of whatever node string to the bottom it is
            Nothing needs to be done here.
            */
            node->scope = topOfStack->name;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case EMPTY_S: 
            /* This will be the end of whatever node string to the bottom it is
            Nothing needs to be done here.
            */
            node->scope = topOfStack->name;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case SEMICOLON_S:
            /* This will be the end of whatever node string to the bottom it is
            Nothing needs to be done here.
            */
            node->scope = topOfStack->name;

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case ASSIGN_E:
            /* Just traverseLine here 
            */
            node->scope = topOfStack->name;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        case BLOCK_M:
            /* This is a block, which will be important because it is where all the 
            sister nodes are.
            traverseLine, but then you have to traverse through all the sister nodes here
            So, it'll come back, and you need to go to the next sister
            */
            node->scope = topOfStack->name;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) lineTraverse(node->childNode[i]);
            }

            if(node->sisterNode != NULL) lineTraverse(node->sisterNode);

            break;
        default:
            /* unknown symbol error
            */
           errorPrint(node, "Unknown node.\n", STABLE_ERR);
           break;
    }
}

void initRuntimeLibraries(NewNode *node){
    /*get the node for the prog_begin's second child
    cycle through the runtime libraries
    */

    //for ERROR 
    node->childNode[1] = createNewNode(LIBRARY_RT, VOID_T, 0, 0, ERROR_RL);
    node->childNode[1]->scope = PROG_NAME;
    node->childNode[1]->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
    node->childNode[1]->childNode[0]->scope = ERROR_RL;
    node->childNode[1]->childNode[0]->childNode[0] = createNewNode(PARA_D, STR_T, 0, 0, ERROR_P);
    node->childNode[1]->childNode[0]->childNode[0]->scope = ERROR_RL;

    addSymbol(PROG_NAME, node->childNode[1]);
    addSymbol(ERROR_RL, node->childNode[1]->childNode[0]->childNode[0]);
    
    node = node->childNode[1];

    if(findSymbol(PROG_NAME, GETCHAR_RL) == NULL){ //getchar
        NewNode *link = NULL;
        link = createNewNode(LIBRARY_RT, INT_T, 0, 0, GETCHAR_RL);
        link->scope = PROG_NAME;
        link->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
        link->childNode[0]->scope = GETCHAR_RL;

        addSymbol(PROG_NAME, link);

        node->sisterNode = link;
        node = node->sisterNode;
    }
    if(findSymbol(PROG_NAME, HALT_RL) == NULL){ //halt
        NewNode *link;
        link = createNewNode(LIBRARY_RT, VOID_T, 0, 0, HALT_RL);
        link->scope = PROG_NAME;
        link->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
        link->childNode[0]->scope = HALT_RL;

        addSymbol(PROG_NAME, link);

        node->sisterNode = link;
        node = node->sisterNode;
    }
    if(findSymbol(PROG_NAME, PRINTB_RL) == NULL){ //printb
        NewNode *link;
        link = createNewNode(LIBRARY_RT, VOID_T, 0, 0, PRINTB_RL);
        link->scope = PROG_NAME;
        link->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
        link->childNode[0]->scope = PRINTB_RL;
        link->childNode[0]->childNode[0] = createNewNode(PARA_D, BOOL_T, 0, 0, PRINTB_P);
        link->childNode[0]->childNode[0]->scope = PRINTB_RL;

        addSymbol(PROG_NAME, link);
        addSymbol(PRINTB_RL, link->childNode[0]->childNode[0]);

        node->sisterNode = link;
        node = node->sisterNode;
    }
    if(findSymbol(PROG_NAME, PRINTC_RL) == NULL){ //printc
        NewNode *link;
        link = createNewNode(LIBRARY_RT, VOID_T, 0, 0, PRINTC_RL);
        link->scope = PROG_NAME;
        link->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
        link->childNode[0]->scope = PRINTC_RL;
        link->childNode[0]->childNode[0] = createNewNode(PARA_D, INT_T, 0, 0, PRINTC_P);
        link->childNode[0]->childNode[0]->scope = PRINTC_RL;

        addSymbol(PROG_NAME, link);
        addSymbol(PRINTC_RL, link->childNode[0]->childNode[0]);

        node->sisterNode = link;
        node = node->sisterNode;
    }
    if(findSymbol(PROG_NAME, PRINTI_RL) == NULL){ //printi
        NewNode *link;
        link = createNewNode(LIBRARY_RT, VOID_T, 0, 0, PRINTI_RL);
        link->scope = PROG_NAME;
        link->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
        link->childNode[0]->scope = PRINTI_RL;
        link->childNode[0]->childNode[0] = createNewNode(PARA_D, INT_T, 0, 0, PRINTI_P);
        link->childNode[0]->childNode[0]->scope = PRINTI_RL;

        addSymbol(PROG_NAME, link);
        addSymbol(PRINTI_RL, link->childNode[0]->childNode[0]);

        node->sisterNode = link;
        node = node->sisterNode;
    }
    if(findSymbol(PROG_NAME, PRINTS_RL) == NULL){ //printb
        NewNode *link;
        link = createNewNode(LIBRARY_RT, VOID_T, 0, 0, PRINTS_RL);
        link->scope = PROG_NAME;
        link->childNode[0] = createNewNode(FUNC_PARA, VOID_T, 0, 0, "");
        link->childNode[0]->scope = PRINTS_RL;
        link->childNode[0]->childNode[0] = createNewNode(PARA_D, STR_T, 0, 0, PRINTS_P);
        link->childNode[0]->childNode[0]->scope = PRINTS_RL;

        addSymbol(PROG_NAME, link);
        addSymbol(PRINTS_RL, link->childNode[0]->childNode[0]);

        node->sisterNode = link;
        node = node->sisterNode;
    }
}

VariableType typeTraverse(NewNode *node){
    switch(node->nType){
        case PROG_BEGIN:
            /* This is just the beginning of the run through
            So, maybe we'll just do 
            node->childNode[0] != NULL ? (return NULL) : (return typeTraverse(node->childNode[0]));
            */
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            break;
        case VAR_D:
            /* 
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case MAIN_D: 
            /* Just move down the child nodes
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case LIBRARY_RT:
        case FUNC_D:
            /* 
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case PARA_D:
            /* 
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            return node->vType;
            break;
        case STATE_E:
        case ID_E:
            /* Here, we want to check that this ID is able to be used
            So, we search for it in the local scope
            If it is there, we return the type from that search
            If it isn't there, we search for it in the global scope
            If it is there, we return the type from that search
            If it isn't there, it hasn't been initialized, so we throw an error
            */
            Symbol *checkId = findSymbol(node->scope, node->name);
            if(checkId == NULL || checkId->nodePointer->lineStart > node->lineStart){
                checkId = findSymbol(PROG_NAME, node->name);
                if(checkId == NULL) errorPrint(node, "Variable has not been initialized.\n", STABLE_ERR);
            }
            return checkId->nodePointer->vType;
        case IF_S:
            /* The first child node is going to be the contents of the if statement
            They need to come back as BOOL
            so we'll go 
            if(typeTraverse(node->childNode[0]) != BOOL) error
            the second child is the block
            we need to traverse it, but the return doesn't matter
            typeTraverse(node->childNode[1]);
            then we visit the sister node if there is one because that'll be the else
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            if(node->childNode[0] != NULL && typeTraverse(node->childNode[0]) != BOOL_T) 
                errorPrint(node, "If statement must be of type boolean.\n", SEM_ERR);

            if(node->childNode[1] != NULL) typeTraverse(node->childNode[1]);

            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case IFELSE_S:
            /* There will only be one child and it'll be the block of whatever is in the 
            else
            we will do this:
            typeTraverse(node->childNode[0]);
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return non
            */
            if(node->childNode[0] != NULL) typeTraverse(node->childNode[0]);
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case BREAK_S: 
            /* traverse sisterNode
            return none here
            */
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case WHILE_S:
            /* if(typeTraverse(node->childNode[0]) != BOOL) error
            the second child is the block
            we need to traverse it, but the return doesn't matter
            typeTraverse(node->childNode[1]);
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            if(node->childNode[0] != NULL && typeTraverse(node->childNode[0]) != BOOL_T) 
                errorPrint(node, "While statement must be of type boolean.\n", SEM_ERR);

            if(node->childNode[1] != NULL) typeTraverse(node->childNode[1]);

            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case FUNC_PARA:
            /* for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return none
            */
            if(node->childNode[0] != NULL) typeTraverse(node->childNode[0]);
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case FUNCCALL_S:
            /* Search the name of the function in the global scope
            If it's the main function, error
            If it's a runtime library, error which will be replaced
            If it otherwise doesn't exist, error
            If it does exist move onto the next part

            Here, we're checking args and params
            We need a check init to 0
            For this node, we'll go to the child of the child (named currArg)
            Then we'll take the node of the found function and grab the child of the child 
            (named currParam)
            this will be a while loop, so while currArg != NULL and currParam != NULL
                if(currArg != NULL && currParam == NULL) too many args error
                else if(currArg == NULL && currParam != NULL) not enough args error
                else if(typeTraverse(currArg) != typeTraverse(currParam)) mismatched type
                    error in argument i
                i++
            if everything goes smooth and there's no error, then traverse sisterNode 
            and return the return type of the function
            */
            Symbol *checkFunc = findSymbol(PROG_NAME, node->name);
            if(checkFunc == NULL) errorPrint(node, "Function has not been initialized.\n", STABLE_ERR);
            else if(checkFunc->nodePointer->nType == MAIN_D) 
                errorPrint(node, "Main function cannot be called.\n", SEM_ERR);
            
            NewNode *currArg = NULL, *currParam = NULL;
            int i = 0;
            if(node->childNode[i] != NULL && node->childNode[i]->childNode[i] != NULL)
                currArg = node->childNode[i]->childNode[i];
            if(checkFunc->nodePointer->childNode[i] != NULL && 
                checkFunc->nodePointer->childNode[i]->childNode[i] != NULL)
                    currParam = checkFunc->nodePointer->childNode[i]->childNode[i];

            while(currArg != NULL || currParam != NULL){
                i++; 
                if(currArg == NULL && currParam != NULL)
                    errorPrint(node, "Not enough arguments for this function.\n", SEM_ERR);
                else if(currArg != NULL && currParam == NULL)
                    errorPrint(node, "Too many arguments for this function.\n", SEM_ERR);
                else if(!argPara[typeTraverse(currArg)][typeTraverse(currParam)]){
                    char buffer[1024];
                    snprintf(buffer, sizeof(buffer), "Argument %d type does not match parameter type.\n", i);
                    errorPrint(node, buffer, SEM_ERR);
                }
                currArg = currArg->sisterNode;
                currParam = currParam->sisterNode;  
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return checkFunc->nodePointer->vType;
            break;
        case FUNC_ARG: 
            /* traverse sisterNode
            empty
            */
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case RETURN_S:
            /* Here, we will take the saved scope value and we will search for it
            in the global scope
            When we find it, we have access to the node and we can check the return type
            So if child node isn't empty, type traverse it
            If the return isn't the same as the function return, error
            If it is the same, good
            If the child node is empty and the function isn't void, error
            otherwise, good
            traverse sisterNode
            */
            Symbol *findFunc = findSymbol(PROG_NAME, node->scope);
            if(findFunc == NULL) errorPrint(node, "Function has not been initialized.\n", STABLE_ERR);
            
            if(node->childNode[0] != NULL && findFunc->nodePointer->vType != typeTraverse(node->childNode[0]))
                errorPrint(node, "Return variable type does not match function return type.\n", SEM_ERR);
            
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case OP_E:
            /* This is where the operator checks come into play
            So, we will traverse down the left side and save it
            Then we will traverse down the right side and save it
            Then we will check them inside the operator arrays depending on what
            type of operator it is
            traverse sisterNode
            And it will give us a value which we will return
            */
            VariableType leftSide = NONE, rightSide = NONE, toReturnOP = NONE;

            if(node->childNode[0] != NULL) leftSide = typeTraverse(node->childNode[0]);
            if(node->childNode[1] != NULL) rightSide = typeTraverse(node->childNode[1]);

            if(!strcmp(node->name, "-")) toReturnOP = negativeOperator[leftSide][rightSide];
            else if(!strcmp(node->name, "+") || !strcmp(node->name, "*") || !strcmp(node->name, "/") || !strcmp(node->name, "%"))
                toReturnOP = arithOperators[leftSide][rightSide];
            else if(!strcmp(node->name, "!"))
                toReturnOP = notOperator[leftSide][rightSide];
            else if(!strcmp(node->name, "<") || !strcmp(node->name, ">") || !strcmp(node->name, "<=") || !strcmp(node->name, ">="))
                toReturnOP = relationalOperators[leftSide][rightSide];
            else if(!strcmp(node->name, "==") || !strcmp(node->name, "!="))
                toReturnOP = equalityOperators[leftSide][rightSide];
            else if(!strcmp(node->name, "&&") || !strcmp(node->name, "||"))
                toReturnOP = conditionalOperators[leftSide][rightSide];
            
            if(toReturnOP == NONE) errorPrint(node, "Operation unable to be performed on incompatible types.\n", SEM_ERR);

            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);

            return toReturnOP;
            break;
        case CONST_E: 
            /* traverse sisterNode
            Just return the v type
            */
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return node->vType;
            break;
        case EMPTY_S: 
            /* traverse sisterNode
            return none
            */
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case SEMICOLON_S:
            /* traverse sisterNode
            return none
            */
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        case ASSIGN_E:
            /* Traverse the right side and save it
            Traverse the left side and save it
            Compare them
            If they're not equal, error
            If they're equal, return what they are equal to
            traverse sisterNode
            */
            VariableType toReturnASSIGN = NONE;
            if(node->childNode[0] != NULL && node->childNode[1] != NULL){
                toReturnASSIGN = typeTraverse(node->childNode[0]);
                if(toReturnASSIGN != typeTraverse(node->childNode[1]))
                    errorPrint(node, "Cannot assign mismatching type to variable.\n", SEM_ERR);
            }   
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            return toReturnASSIGN;
            break;
        case BLOCK_M:
            /* traverse the children and sister
            return none
            */
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL) typeTraverse(node->childNode[i]);
            }
            if(node->sisterNode != NULL) typeTraverse(node->sisterNode);
            break;
        default:
           break;
    }
    return NONE;
}

void analyzeSemantics(NewNode *currTree){
    lineTraverse(currTree);
    initRuntimeLibraries(currTree);
    typeTraverse(currTree);
}


