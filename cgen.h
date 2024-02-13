/* This is the header file for the code
generation
Max Krishka-Pachal
CPSC 411
Final Project
cgen.h
May 14th 2022
*/

#ifndef _CGEN_H_
#define _CGEN_H_

#include "utils.h"

//buffers
#define SECT_BUFFER     1024
#define LINE_BUFFER     40
#define REG_BUFFER      10
#define STACK_REG       5

//register types
#define TEMP            0
#define SAVE            1

//register names
#define TEMP_REG_S      0
#define TEMP_REG_E      9
#define SAVE_REG_S      10
#define SAVE_REG_E      17
#define TOTAL_REG       18

//register allocation
#define NO_REG          -1
#define NO_TYPE         -1
#define NO_ALLOC        -1

//tabs
#define INSTRUCT_TAB    "        "
#define ONE_TAB         "        "
#define TWO_TAB         "       "
#define THREE_TAB       "      "
#define FOUR_TAB        "     "

//errors
#define DIV_ZERO_ERR    ".byte 68, 105, 118, 105, 115, 105, 111, 110, 32, 98, 121, 32, 122, 101, 114, 111, 10, 0"
    //division by zero
#define NO_RET_ERR_1    ".byte 70, 117, 110, 99, 116, 105, 111, 110, 32, 34, "
#define NO_RET_ERR_2    "34, 32, 109, 117, 115, 116, 32, 104, 97, 118, 101, 32, 97, 32, 114, 101, 116, 117, 114, 110, 32, 118, 97, 108, 117, 101, 46, 10, 0"
    //no return error

//print statements
#define TRUE_PRINT      ".byte 116, 114, 117, 101, 0"
#define FALSE_PRINT     ".byte 102, 97, 108, 115, 101, 0"             

typedef struct {
    int type, regNum;
} RegSave;

typedef struct {
    RegSave *registerInfo;
    int mark, save, active;
} RegInfo;

void generateCode(NewNode *currTree, FILE *outputCode);

#endif