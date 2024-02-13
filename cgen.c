/* This is where code for MIPS will be generated
We're going to try making a base first and then
decide how to approach it from there
Max Krishka-Pachal
CPSC 411
Final Project
cgen.c
May 14th 2022
*/
#include "stable.h"
#include "cgen.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

//counts global variable labels, counts function labels, counts general labels
int globalCounter = 0, functionCounter = 0, labelCounter = 0, quickCounter = 0;
int breakCounter = 0, breakLoop = 0; //breakLoop counts whiles, breakCounter counts break statements
int stackSpace = 0, stackLoop = 0, saveValueCount = 0; //stackSpace counts allocated space in functions, 
    //stackLoop counts additonal saved space for function calls
int markCounter = -1; //for knowing which registers to flush after assigning arguments
int returnLabel = 0; //the label for a function's return
int assignType = 0, assignRegister = 0; //the register being used in assign statements
//int registerArray[TOTAL_REG] = { 0 }; //all the available registers
RegInfo *allReg[TOTAL_REG];
RegSave *regSave, *tempSave; //for moving around the registers between nodes
NewNode *fakeNode; //for runtime library errors

FILE *code; //the file where the generated code is being sent

void initRegisters(){
    for(int i = 0; i < TOTAL_REG; i++){
        int reg = (i <= TEMP_REG_E ? 0 : 1);
        allReg[i] = malloc(sizeof(RegInfo));
        allReg[i]->registerInfo = malloc(sizeof(RegSave));
        allReg[i]->registerInfo->type = reg;
        allReg[i]->registerInfo->regNum = i - (reg * SAVE_REG_S);
        allReg[i]->active = 0;
        allReg[i]->mark = 0;
        allReg[i]->save = 0;
        //printf("i: %d, type: %d, reg: %d, active: %d, mark: %d, save: %d\n", i,
            // allReg[i]->registerInfo->type, allReg[i]->registerInfo->regNum, allReg[i]->active,
            // allReg[i]->mark, allReg[i]->save);
    }

    // printf("allReg[2] type: %d, reg: %d\n", allReg[2]->registerInfo->type, allReg[2]->registerInfo->regNum);
    // regSave->regNum = allReg[2]->registerInfo->regNum;
    // regSave->type = allReg[2]->registerInfo->type;
    // printf("regSave type: %d, reg: %d\n", regSave->type, regSave->regNum);
    // regSave->type = 2;
    // regSave->regNum = 45;
    // printf("regSave type: %d, reg: %d\n", regSave->type, regSave->regNum);
    // printf("allReg[2] type: %d, reg: %d\n", allReg[2]->registerInfo->type, allReg[2]->registerInfo->regNum);
}

RegSave *allocateRegister(){
    //gives an available register of the type being asked ($t or $s)
    //temp is 0, save is 1
    RegSave *aReg = malloc(sizeof(RegSave));

    for(int i = 0; i < TOTAL_REG; i++){
        if(!allReg[i]->active){
            allReg[i]->active = 1;
            aReg->type = allReg[i]->registerInfo->type;
            aReg->regNum = allReg[i]->registerInfo->regNum;
            //printf("allocate, type: %d, reg: %d\n", aReg->type, aReg->regNum);
            return aReg;
        }
    }
    aReg->type = -1;
    aReg->regNum = -1;
    //printf("allocate, type: %d, reg: %d\n", aReg->type, aReg->regNum);
    return aReg;
}

void deallocateRegister(RegSave *dReg){
    //deallocates registers that have been allocated and are able to be reused again
    //printf("deallocate, type: %d, reg: %d, %d\n", dReg->type, dReg->regNum, (dReg->type * SAVE_REG_E) + dReg->regNum);
    allReg[(dReg->type * SAVE_REG_E) + dReg->regNum]->active = 0;
}

void markRegister(RegSave *mReg, int markCounter){
    //marks specific registers with a given number, used by the argument nodes
    allReg[(mReg->type * SAVE_REG_E) + mReg->regNum]->mark = markCounter;
}

void flushRegister(int markCounter){
    //frees up all the registers that have been marked for deallocation
    for(int i = 0; i < TOTAL_REG; i++){
        if(allReg[i]->mark == markCounter){
            allReg[i]->mark = 0;
            allReg[i]->active = 0;
        }
    }
}

void allocateStack(){
    //this is for saving variables that need to be used between function calls
    //stack loop keeps track of how many layers deep 
    stackLoop++;

    char swCodeToAdd[SECT_BUFFER] = "\0";
    char buffer[LINE_BUFFER];
    int howManyRegisters = 0;

    for(int i = 0; i < TOTAL_REG; i++){
        if(allReg[i]->active == 1 && allReg[i]->save == 0){
            allReg[i]->save = stackLoop;
            snprintf(buffer, sizeof(buffer), "%ssw%s%s%d, %d($sp)\n\n", 
                INSTRUCT_TAB, TWO_TAB, (allReg[i]->registerInfo->type == 0 ? "$t" : "$s"), 
                allReg[i]->registerInfo->regNum, howManyRegisters);
            strcat(swCodeToAdd, buffer);
            howManyRegisters += 4;
        }
    }

    saveValueCount += howManyRegisters;

    if(howManyRegisters > 0){
        fprintf(code, "\n%ssubu%s$sp, $sp, %d\n", INSTRUCT_TAB, FOUR_TAB, howManyRegisters);
        fprintf(code, swCodeToAdd);
    }
}

void deallocateStack(){
    int howManyRegisters = 0;

    for(int i = 0; i < TOTAL_REG; i++){
        if(allReg[i]->save == stackLoop){
            allReg[i]->save = 0;
            fprintf(code, "\n%slw%s%s%d, %d($sp)\n", INSTRUCT_TAB, TWO_TAB, 
                (allReg[i]->registerInfo->type == 0 ? "$t" : "$s"), 
                allReg[i]->registerInfo->regNum, howManyRegisters);
            howManyRegisters += 4;
        }
    }

    if(howManyRegisters > 0)
        fprintf(code, "%saddu%s$sp, $sp, %d\n\n", INSTRUCT_TAB, FOUR_TAB, howManyRegisters);

    saveValueCount -= howManyRegisters;
    stackLoop--;
}

void getCharRuntimeLibrary(){
    fprintf(code, "\n\nR0:\n");
    fprintf(code, "%ssubu%s$sp, $sp, 4\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    
    fprintf(code, "%s.data\n", INSTRUCT_TAB);
    fprintf(code, "RGCS:\n");
    fprintf(code, "%s.space 4\n", INSTRUCT_TAB);
    fprintf(code, "%s.text\n", INSTRUCT_TAB);
    
    fprintf(code, "%sli%s$v0, 8\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sla%s$a0, RGCS\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sli%s$a1, 2\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n", INSTRUCT_TAB);

    fprintf(code, "%smove%s$t0, $a0\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%slb%s$t1, 0($t0)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sbeqz%s$t1, RGC1\n", INSTRUCT_TAB, FOUR_TAB);

    fprintf(code, "%smove%s$v0, $t1\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sj%sRGC2\n", INSTRUCT_TAB, ONE_TAB);

    fprintf(code, "RGC1:\n");
    fprintf(code, "%sli%s$t1, 1\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%snegu%s$t1, $t1\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%smove%s$v0, $t1\n", INSTRUCT_TAB, FOUR_TAB);

    //closing the function and returning
    fprintf(code, "RGC2:\n");
    fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%saddu%s$sp, $sp, 4\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);
}

void haltRuntimeLibrary(){
    fprintf(code, "\n\nR1:\n");
    fprintf(code, "%sli%s$v0, 10\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall", INSTRUCT_TAB);
}

void printbRuntimeLibrary(){
    //initializing the function
    fprintf(code, "\n\nR2:\n");
    fprintf(code, "%ssubu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssw%s$a0, 4($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%slw%s$t0, 4($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sblez%s$t0, RBL1\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%s.data\n", INSTRUCT_TAB);
    
    //the if statement for if b is true
    fprintf(code, "RBS1:\n");
    fprintf(code, "%s%s\n", INSTRUCT_TAB, TRUE_PRINT);
    fprintf(code, "%s.text\n", INSTRUCT_TAB);
    fprintf(code, "%sli%s$v0, 4\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sla%s$a0, RBS1\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n\n", INSTRUCT_TAB);
    fprintf(code, "%sj%sRBL2\n", INSTRUCT_TAB, ONE_TAB);

    //the else statement for if b is false
    fprintf(code, "RBL1:\n");
    fprintf(code, "%s.data\n", INSTRUCT_TAB);

    fprintf(code, "RBS2:\n");
    fprintf(code, "%s%s\n", INSTRUCT_TAB, FALSE_PRINT);
    fprintf(code, "%s.text\n", INSTRUCT_TAB);
    fprintf(code, "%sli%s$v0, 4\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sla%s$a0, RBS2\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n\n", INSTRUCT_TAB);

    //closing the function and returning
    fprintf(code, "RBL2:\n");
    fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%saddu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);
}

void printcRuntimeLibrary(){
    fprintf(code, "\n\nR3:\n");
    fprintf(code, "%ssubu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sli%s$v0, 11\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n", INSTRUCT_TAB);

    //closing the function and returning
    fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%saddu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);
}

void printiRuntimeLibrary(){
    fprintf(code, "\n\nR4:\n");
    fprintf(code, "%ssubu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sli%s$v0, 1\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n", INSTRUCT_TAB);

    //closing the function and returning
    fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%saddu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);
}

void printsRuntimeLibrary(){
    //initializing the function
    fprintf(code, "\n\nR5:\n");
    fprintf(code, "%ssubu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sli%s$v0, 4\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n", INSTRUCT_TAB);

    //closing the function and returning
    fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%saddu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);
}

void errorRuntimeLibrary(){
    //initializing the function
    fprintf(code, "\n\nR6:\n");
    fprintf(code, "%ssubu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%sli%s$v0, 4\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n\n", INSTRUCT_TAB);

    fprintf(code, "%sli%s$v0, 10\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%ssyscall\n\n", INSTRUCT_TAB);

    //closing the function and returnings
    fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
    fprintf(code, "%saddu%s$sp, $sp, 8\n", INSTRUCT_TAB, FOUR_TAB);
    fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);
}


void labelTraverse(NewNode *node){
    switch(node->nType){
        case PROG_BEGIN:
            for(int i = 0; i < CHILDNODECOUNT; i++)
                if(node->childNode[i] != NULL) labelTraverse(node->childNode[i]);

            break;
        case VAR_D:
            Symbol *var = findSymbol(node->scope, node->name);
            snprintf(var->label, sizeof(var->label), "G%d", globalCounter);
            globalCounter++;

            if(node->sisterNode != NULL) labelTraverse(node->sisterNode);

            break;
        case MAIN_D:
            strcpy(findSymbol(node->scope, node->name)->label, "M0");
            
            NewNode *nextMainVar = NULL;
            int stackMainCount = 4;

            if(node->childNode[0] != NULL && node->childNode[0]->childNode[0] != NULL)
                nextMainVar = node->childNode[0]->childNode[0];

            while(nextMainVar != NULL){
                if(nextMainVar->nType == VAR_D || nextMainVar->nType == PARA_D)
                    stackMainCount += 4;
                nextMainVar = nextMainVar->sisterNode;
            }

            saveStackSpace(stackMainCount, findSymbol(PROG_NAME, node->name));

            if(node->sisterNode != NULL) labelTraverse(node->sisterNode);

            break;
        case FUNC_D:
            Symbol *func = findSymbol(node->scope, node->name);
            snprintf(func->label, sizeof(func->label), "F%d", functionCounter);
            functionCounter++;  

            NewNode *nextFuncVar = NULL;
            int stackFuncCount = 4;
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL && node->childNode[i]->childNode[0] != NULL)
                    nextFuncVar = node->childNode[i]->childNode[0];
                while(nextFuncVar != NULL){
                    if(nextFuncVar->nType == VAR_D || nextFuncVar->nType == PARA_D)
                        stackFuncCount += 4;
                    nextFuncVar = nextFuncVar->sisterNode;
                }
            }

            saveStackSpace(stackFuncCount, findSymbol(PROG_NAME, node->name));
            
            if(node->sisterNode != NULL) labelTraverse(node->sisterNode);
            
            break;
        case LIBRARY_RT:
            int libraryNumber = -1;
            
            if(!strcmp(node->name, GETCHAR_RL)) libraryNumber = 0;
            else if(!strcmp(node->name, HALT_RL)) libraryNumber = 1;
            else if(!strcmp(node->name, PRINTB_RL)) libraryNumber = 2;
            else if(!strcmp(node->name, PRINTC_RL)) libraryNumber = 3;
            else if(!strcmp(node->name, PRINTI_RL)) libraryNumber = 4;
            else if(!strcmp(node->name, PRINTS_RL)) libraryNumber = 5;
            else if(!strcmp(node->name, ERROR_RL)) libraryNumber = 6;
            
            Symbol *runtime = findSymbol(node->scope, node->name);
            snprintf(runtime->label, sizeof(runtime->label), "R%d", libraryNumber);
            
            if(node->sisterNode != NULL) labelTraverse(node->sisterNode);

            break;
        default:
            
    }
}

RegSave *intermediateTraverse(NewNode *node){
    switch(node->nType){
        case PROG_BEGIN:
            fprintf(code, "%s.text\n", INSTRUCT_TAB);
            fprintf(code, "%s.globl main\n", INSTRUCT_TAB);
            fprintf(code, "main:\n");
            fprintf(code, "%sjal%sM0\n", INSTRUCT_TAB, THREE_TAB);
            fprintf(code, "%sli%s$v0, 10\n", INSTRUCT_TAB, TWO_TAB);
            fprintf(code, "%ssyscall\n\n", INSTRUCT_TAB);

            NewNode *nextDec = NULL;

            if(node->childNode[0] != NULL){
                nextDec = node->childNode[0];
                while(nextDec != NULL){
                    if(nextDec->nType == VAR_D)
                        regSave = intermediateTraverse(nextDec);
                    nextDec = nextDec->sisterNode;
                }
                
                nextDec = node->childNode[0];
                while(nextDec != NULL){
                    if(nextDec->nType != IFELSE_S && nextDec->nType != VAR_D){
                        regSave = intermediateTraverse(nextDec);
                        if(regSave->regNum > NO_ALLOC) 
                            deallocateRegister(regSave);
                    }
                    nextDec = nextDec->sisterNode;
                }
            }

            if(node->childNode[1] != NULL) nextDec = node->childNode[1];

            while(nextDec != NULL){
                regSave = intermediateTraverse(nextDec);
                nextDec = nextDec->sisterNode;
            }

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case PARA_D:
            saveStackSpace(stackSpace, findSymbol(node->scope, node->name));

            fprintf(code, "%ssw%s$a%d, %d($sp)\n", 
                INSTRUCT_TAB, TWO_TAB, ((stackSpace / 4) - 1), stackSpace);

            stackSpace += 4;

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case VAR_D:
            if(!strcmp(node->scope, PROG_NAME)){ //if it's global
                fprintf(code, "%s.data\n", INSTRUCT_TAB);
                fprintf(code, "%s:\n", findSymbol(node->scope, node->name)->label);
                fprintf(code, "%s.word 0\n", INSTRUCT_TAB);
                fprintf(code, "%s.text\n", INSTRUCT_TAB); 
            }
            else {
                saveStackSpace(stackSpace, findSymbol(node->scope, node->name));
                stackSpace += 4;
            }

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case FUNC_D:
        case MAIN_D:
            stackSpace = 4;

            fprintf(code, "\n%s:\n", findSymbol(node->scope, node->name)->label);
            fprintf(code, "%ssubu%s$sp, $sp, %d\n", INSTRUCT_TAB, FOUR_TAB, 
                getStackSpace(findSymbol(node->scope, node->name)));
            fprintf(code, "%ssw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);

            returnLabel = labelCounter;
            labelCounter++;

            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL){
                    regSave = intermediateTraverse(node->childNode[i]);
                    if(regSave->regNum > NO_ALLOC) 
                        deallocateRegister(regSave);
                }
            }

            if(node->nType == FUNC_D && node->vType != VOID_T && node->vType != NONE){
                fprintf(code, "%s.data\n", INSTRUCT_TAB);
                fprintf(code, "L%d:\n", labelCounter);
                fprintf(code, "%s%s", INSTRUCT_TAB, NO_RET_ERR_1);

                for(int i = 0; i < strlen(node->name); i++)
                    fprintf(code, "%d, ", node->name[i]);

                fprintf(code, "%s\n", NO_RET_ERR_2);
                fprintf(code, "%s.align 2\n", INSTRUCT_TAB);
                fprintf(code, "%s.text\n", INSTRUCT_TAB);
                fprintf(code, "%sla%s$a0, L%d\n", INSTRUCT_TAB, TWO_TAB, labelCounter);
                fprintf(code, "%sj%sR6\n", INSTRUCT_TAB, ONE_TAB);

                labelCounter++;
            }

            fprintf(code, "\nL%d:\n", returnLabel);
            fprintf(code, "%slw%s$ra, 0($sp)\n", INSTRUCT_TAB, TWO_TAB);
            fprintf(code, "%saddu%s$sp, $sp, %d\n", INSTRUCT_TAB, FOUR_TAB, 
                getStackSpace(findSymbol(node->scope, node->name)));
            fprintf(code, "%sjr%s$ra\n", INSTRUCT_TAB, TWO_TAB);

           
            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case LIBRARY_RT:
            if(!strcmp(node->name, GETCHAR_RL)) getCharRuntimeLibrary();
            else if(!strcmp(node->name, HALT_RL)) haltRuntimeLibrary();
            else if(!strcmp(node->name, PRINTB_RL)) printbRuntimeLibrary();
            else if(!strcmp(node->name, PRINTC_RL)) printcRuntimeLibrary();
            else if(!strcmp(node->name, PRINTI_RL)) printiRuntimeLibrary();
            else if(!strcmp(node->name, PRINTS_RL)) printsRuntimeLibrary();
            else if(!strcmp(node->name, ERROR_RL)) errorRuntimeLibrary();

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case ID_E:
            Symbol *findID;

            regSave = allocateRegister();
            if(regSave->type == NO_ALLOC)
                errorPrint(node, "Not enough registers to generate code.\n", GEN_ERR);

            //printf("regSave, type: %d, reg: %d\n", regSave->type, regSave->regNum);

            fprintf(code, "%slw%s%s%d, ", INSTRUCT_TAB, TWO_TAB, 
                (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum);

            //check if global
            if((findID = findSymbol(node->scope, node->name)) != NULL && 
                findID->nodePointer->lineStart < node->lineStart)
                    fprintf(code, "%d($sp)\n", getStackSpace(findID) + saveValueCount);
            else if((findID = findSymbol(PROG_NAME, node->name)) != NULL)
                fprintf(code, "%s\n", findID->label);

            return regSave;
        case STATE_E:
            Symbol *findState;

            regSave->regNum = assignRegister;
            regSave->type = assignType;

            fprintf(code, "%ssw%s%s%d, ", INSTRUCT_TAB, TWO_TAB, 
                (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum);

            if((findState = findSymbol(node->scope, node->name)) != NULL)
                fprintf(code, "%d($sp)\n", getStackSpace(findState));
            else if((findState = findSymbol(PROG_NAME, node->name)) != NULL)
                fprintf(code, "%s\n", findState->label);
            return regSave;
        case IF_S:
            //(1) grabbed the label and saved it in the string to return
            fprintf(code, "L%d:\n", labelCounter);
            labelCounter++;

            //(2) traverse the expression
            if(node->childNode[0] != NULL)
                regSave = intermediateTraverse(node->childNode[0]);
            
            //(3) adding the jump code with the test
            int skipLabelIf = labelCounter;
            int skipLabelElse = skipLabelIf;
            labelCounter++;
            // fprintf(code, "%sbeqz%s%s%d, L%d\n", INSTRUCT_TAB, FOUR_TAB,
            //     (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum, skipLabelIf);

            fprintf(code, "%sbeqz%s%s%d, Q%d\n", INSTRUCT_TAB, FOUR_TAB, 
                (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum, quickCounter);
            fprintf(code, "%sj%sQ%d\n", INSTRUCT_TAB, ONE_TAB, quickCounter + 1);
            fprintf(code, "Q%d:\n", quickCounter);
            fprintf(code, "%sj%sL%d\n", INSTRUCT_TAB, ONE_TAB, skipLabelIf);
            fprintf(code, "Q%d:\n", quickCounter + 1);
            quickCounter += 2;

            deallocateRegister(regSave);
            
            //(4) add the body of the if
            if(node->childNode[1] != NULL){
                regSave = intermediateTraverse(node->childNode[1]);
                if(regSave->regNum > NO_ALLOC) 
                    deallocateRegister(regSave);
            }

            //(5) check if there's an else
            if(node->sisterNode != NULL && node->sisterNode->nType == IFELSE_S){
                skipLabelElse = labelCounter;
                labelCounter++;

                fprintf(code, "%sj%sL%d\n", INSTRUCT_TAB, ONE_TAB, skipLabelElse);
                fprintf(code, "L%d:\n", skipLabelIf);
            
                regSave = intermediateTraverse(node->sisterNode);
                if(regSave->regNum > NO_ALLOC) 
                    deallocateRegister(regSave);
            }

            //(6) add skip label
            fprintf(code, "L%d:\n", skipLabelElse);

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case IFELSE_S:
            for(int i = 0; i < CHILDNODECOUNT; i++){
                if(node->childNode[i] != NULL){
                    regSave = intermediateTraverse(node->childNode[i]);
                    if(regSave->regNum > NO_ALLOC) 
                        deallocateRegister(regSave);
                }
            }

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case BREAK_S:
            fprintf(code, "%sj%sB%d%d\n", INSTRUCT_TAB, ONE_TAB, breakLoop, breakCounter);
            breakCounter++;

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case WHILE_S:
            //(1) get the label for the while loop
            int whileLabel = labelCounter;
            fprintf(code, "L%d:\n", whileLabel);
            labelCounter++;

            //(2) traverse the expression
            if(node->childNode[0] != NULL)
                regSave = intermediateTraverse(node->childNode[0]);

            //(4) adding the jump code with the test
            int skipLabelWhile = labelCounter;
            int breakCountHold = breakCounter;
            breakLoop++;
            labelCounter++;

            // fprintf(code, "%sbeqz%s%s%d, L%d\n", INSTRUCT_TAB, FOUR_TAB,
            //     (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum, skipLabelWhile);

            fprintf(code, "%sbeqz%s%s%d, Q%d\n", INSTRUCT_TAB, FOUR_TAB, 
                (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum, quickCounter);
            fprintf(code, "%sj%sQ%d\n", INSTRUCT_TAB, ONE_TAB, quickCounter + 1);
            fprintf(code, "Q%d:\n", quickCounter);
            fprintf(code, "%sj%sL%d\n", INSTRUCT_TAB, ONE_TAB, skipLabelWhile);
            fprintf(code, "Q%d:\n", quickCounter + 1);
            quickCounter += 2;

            deallocateRegister(regSave);     

            //(5) add the body of the while
            if(node->childNode[1] != NULL){
                regSave = intermediateTraverse(node->childNode[1]);
                if(regSave->regNum > NO_ALLOC) 
                    deallocateRegister(regSave);
            }  

            //(6) add the jump to the top of the while
            fprintf(code, "%sj%sL%d\n", INSTRUCT_TAB, ONE_TAB, whileLabel);

            //(7) check the break counter
            for(int breakNum = breakCountHold; breakNum < breakCounter; breakNum++)
                fprintf(code, "B%d%d:\n", breakLoop, breakNum);

            fprintf(code, "L%d:\n", skipLabelWhile);
            breakLoop--;
            breakCounter = breakCountHold;

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case FUNC_PARA:
            NewNode *nextParam = NULL;

            if(node->childNode[0] != NULL) nextParam = node->childNode[0];

            while(nextParam != NULL){
                regSave = intermediateTraverse(nextParam);
                if(regSave->regNum > NO_ALLOC) 
                    deallocateRegister(regSave);
                nextParam = nextParam->sisterNode;
            }

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case FUNCCALL_S:

            allocateStack();

            //(2) call the traverse for the argument count node and add it
            if(node->childNode[0] != NULL){
                regSave = intermediateTraverse(node->childNode[0]);
                if(regSave->regNum > NO_ALLOC) 
                    deallocateRegister(regSave);
            }

            //(3) add the jump code
            fprintf(code, "%sjal%s%s\n\n", 
                INSTRUCT_TAB, THREE_TAB, findSymbol(PROG_NAME, node->name)->label);

            //(4) check if there is a return value
            if(findSymbol(PROG_NAME, node->name)->nodePointer->vType != VOID_T){
                regSave = allocateRegister();
                if(regSave->type == NO_ALLOC)
                    errorPrint(node, "Not enough registers to generate code.\n", GEN_ERR);


                fprintf(code, "%smove%s%s%d, $v0\n", INSTRUCT_TAB, FOUR_TAB, 
                    (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum);
            }
            else {
                regSave->regNum = NO_REG;
                regSave->type = NO_TYPE;
            }

            deallocateStack();

            return regSave;
        case FUNC_ARG:
            char buffer[LINE_BUFFER];
            char funcArguments[SECT_BUFFER] = "\0";
            NewNode *nextArg = NULL;
            int argumentCounter = 0;
            int markKeep = markCounter;
            markCounter--;

            if(node->childNode[0] != NULL) nextArg = node->childNode[0];

            memset(funcArguments, 0, strlen(funcArguments));

            while(nextArg != NULL){
                regSave = intermediateTraverse(nextArg);

                snprintf(buffer, sizeof(buffer), "%smove%s$a%d, %s%d\n", 
                    INSTRUCT_TAB, FOUR_TAB, argumentCounter, 
                    (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum);
                strcat(funcArguments, buffer);

                markRegister(regSave, markKeep);
                
                argumentCounter++;
                nextArg = nextArg->sisterNode;
            }

            flushRegister(markKeep);
            fprintf(code, funcArguments);

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;

            return regSave;
        case RETURN_S:
            if(node->childNode[0] != NULL){
                regSave = intermediateTraverse(node->childNode[0]);
                fprintf(code, "%smove%s$v0, %s%d\n", INSTRUCT_TAB, FOUR_TAB, 
                    (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum);
            }

            fprintf(code, "%sj%sL%d\n", INSTRUCT_TAB, ONE_TAB, returnLabel);
            if(regSave->regNum > NO_ALLOC)
                deallocateRegister(regSave);

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        case OP_E:
            char *opType = node->name;
            int leftType, leftReg, rightType = NO_TYPE, rightReg = NO_REG;
            int skipError = labelCounter;
            labelCounter++;
            int theError = labelCounter;
            labelCounter++;

            if(node->childNode[0] != NULL){
                regSave = intermediateTraverse(node->childNode[0]);
                leftType = regSave->type;
                leftReg = regSave->regNum;
            }

            if(node->childNode[1] != NULL){
                regSave = intermediateTraverse(node->childNode[1]);
                rightType = regSave->type;
                rightReg = regSave->regNum;
            }

            if(rightType > NO_ALLOC)
                deallocateRegister(regSave);
            
            if(!strcmp(opType, "!"))
                fprintf(code, "%snor%s%s%d, %s%d, %s%d\n", INSTRUCT_TAB, THREE_TAB, 
                    (leftType == TEMP ? "$t" : "$s"), leftReg, 
                    (leftType == TEMP ? "$t" : "$s"), leftReg, 
                    (leftType == TEMP ? "$t" : "$s"), leftReg);
            else if(!strcmp(opType, "-") && node->childNode[1] == NULL)
                fprintf(code, "%snegu%s%s%d, %s%d\n", INSTRUCT_TAB, FOUR_TAB, 
                    (leftType == TEMP ? "$t" : "$s"), leftReg, 
                    (leftType == TEMP ? "$t" : "$s"), leftReg);                    
            else{
                if(!strcmp(opType, "+"))
                    fprintf(code, "%saddu", INSTRUCT_TAB);
                else if(!strcmp(opType, "-"))
                    fprintf(code, "%ssubu", INSTRUCT_TAB);
                else if(!strcmp(opType, "*"))
                    fprintf(code, "%smul ", INSTRUCT_TAB);
                else if(!strcmp(opType, ">"))
                    fprintf(code, "%ssgt ", INSTRUCT_TAB);
                else if(!strcmp(opType, "<"))
                    fprintf(code, "%sslt ", INSTRUCT_TAB);
                else if(!strcmp(opType, ">="))
                    fprintf(code, "%ssge ", INSTRUCT_TAB);
                else if(!strcmp(opType, "<="))
                    fprintf(code, "%ssle ", INSTRUCT_TAB);
                else if(!strcmp(opType, "=="))
                    fprintf(code, "%sseq ", INSTRUCT_TAB);
                else if(!strcmp(opType, "!="))
                    fprintf(code, "%ssne ", INSTRUCT_TAB);
                else if(!strcmp(opType, "&&"))
                    fprintf(code, "%sand ", INSTRUCT_TAB);
                else if(!strcmp(opType, "||"))
                    fprintf(code, "%sor  ", INSTRUCT_TAB);
                else {
                    fprintf(code, "%sbnez%s%s%d, L%d\n", INSTRUCT_TAB, FOUR_TAB, 
                        (rightType == TEMP ? "$t" : "$s"), rightReg, skipError);
                    fprintf(code, "%s.data\n", INSTRUCT_TAB);
                    fprintf(code, "L%d:\n", theError);
                    fprintf(code, "%s%s\n", INSTRUCT_TAB, DIV_ZERO_ERR);
                    fprintf(code, "%s.align 2\n", INSTRUCT_TAB);
                    fprintf(code, "%s.text\n", INSTRUCT_TAB);
                    fprintf(code, "%sla%s$a0, L%d\n", INSTRUCT_TAB, TWO_TAB, theError);
                    fprintf(code, "%sj%sR6\n", INSTRUCT_TAB, ONE_TAB);
                    fprintf(code, "L%d:\n", skipError);
                    
                    if(!strcmp(opType, "/"))
                        fprintf(code, "%sdiv ", INSTRUCT_TAB);
                    else if(!strcmp(opType, "%"))
                        fprintf(code, "%srem ", INSTRUCT_TAB);
                }

                fprintf(code, "%s%s%d, %s%d, %s%d\n", FOUR_TAB,
                    (leftType == TEMP ? "$t" : "$s"), leftReg, 
                    (leftType == TEMP ? "$t" : "$s"), leftReg, 
                    (rightType == TEMP ? "$t" : "$s"), rightReg);
            }
            regSave->type = leftType;
            regSave->regNum = leftReg;

            return regSave;
        case CONST_E:
            char *constValue = node->name;

            regSave = allocateRegister();
            if(regSave->type == NO_ALLOC)
                errorPrint(node, "Not enough registers to generate code.\n", GEN_ERR);

            if(node->vType == STR_T){
                int stringLabel = labelCounter;
                labelCounter++;

                fprintf(code, "%s.data\n", INSTRUCT_TAB);
                fprintf(code, "L%d:\n", stringLabel);
                fprintf(code, "%s.byte ", INSTRUCT_TAB);
                
                for(int i = 1; i < strlen(node->name) - 1; i++){
                    if(node->name[i] == '\\'){
                        switch(node->name[i + 1]){
                            case 'r':
                                fprintf(code, "13, ");
                                break;
                            case 'f':
                                fprintf(code, "12, ");
                                break;
                            case 'n':
                                fprintf(code, "10, ");
                                break;
                            case 't':
                                fprintf(code, "9, ");
                                break;
                            case 'b':
                                fprintf(code, "8, ");
                                break;
                            case '\'':
                                fprintf(code, "39, ");
                                break;
                            case '\"':
                                fprintf(code, "34, ");
                                break;
                            case '\\':
                                fprintf(code, "92, ");
                                break;
                            default: 
                                errorPrint(node, "Incomplete escape character\n", GEN_ERR);
                        }
                        i += 1;
                    }
                    else
                        fprintf(code, "%d, ", node->name[i]);
                }

                fprintf(code, "0\n");
                fprintf(code, "%s.align 2\n", INSTRUCT_TAB);
                fprintf(code, "%s.text\n", INSTRUCT_TAB);
                fprintf(code, "%sla%s%s%d, L%d\n", INSTRUCT_TAB, TWO_TAB, 
                    (regSave->type == TEMP ? "$t" : "$s"), regSave->type, stringLabel);
            }

            else {
                if(!strcmp(node->name, "true"))
                    constValue = "1";
                else if(!strcmp(node->name, "false"))
                    constValue = "0";
                
                fprintf(code, "%sli%s%s%d, %s\n", INSTRUCT_TAB, TWO_TAB, 
                    (regSave->type == TEMP ? "$t" : "$s"), regSave->regNum, constValue);
            }

            return regSave;
        case EMPTY_S:
        case SEMICOLON_S:
            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;

            return regSave;
        case ASSIGN_E:
            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            if(node->childNode[1] != NULL){
                tempSave = intermediateTraverse(node->childNode[1]);
                assignType = tempSave->type;
                assignRegister = tempSave->regNum;
            }
            
            if(node->childNode[0] != NULL)
                regSave = intermediateTraverse(node->childNode[0]);

            
            return regSave;
        case BLOCK_M:
            NewNode *nextStatement = NULL;

            if(node->childNode[0] != NULL) nextStatement = node->childNode[0];

            while(nextStatement != NULL){
                if(nextStatement->nType != IFELSE_S){
                    regSave = intermediateTraverse(nextStatement);
                    if(regSave->regNum > NO_ALLOC) 
                        deallocateRegister(regSave);
                }
                nextStatement = nextStatement->sisterNode;
            }

            regSave->regNum = NO_REG;
            regSave->type = NO_TYPE;
            return regSave;
        default:
            errorPrint(node, "Unknown node in code generation.\n", GEN_ERR);
    }
    return NULL;
}


void generateCode(NewNode *currTree, FILE *outputCode){
    code = outputCode;
    initRegisters();
    regSave = malloc(sizeof(RegSave));
    tempSave = malloc(sizeof(RegSave));
    fakeNode = currTree;
    labelTraverse(currTree);
    intermediateTraverse(currTree);
}

