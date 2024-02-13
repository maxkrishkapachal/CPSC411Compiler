/* This file will be used to actually do the
semantic checking
So it will do the 2 pass throughs
Max Krishka-Pachal
CPSC 411
Final Project 
analyzer.h
May 10th 2022

*/
#ifndef _ANALYZER_H_
#define _ANALYZER_H_

#include "utils.h"

#define IS_MAIN          1
#define NOT_MAIN         0

#define CLEANRETURN     1
#define NORETURN        0
#define BASEFLAG        0 


void analyzeSemantics(NewNode *currTree);

#endif