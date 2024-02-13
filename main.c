/*This is my current compiler code 
It includes my scanner and my parser thus far
This is the c file that runs the printing and the file reading
Max Krishka-Pachal
CPSC 411
Final Project
main.c
May 10th 2022
*/

#include "utils.h"
#include "parser.h"
#include "analyzer.h"
#include "cgen.h"
#include <stdlib.h>
#include <string.h>

#define FILE_SIZE   200

FILE *outputCode;

int main(int argc, char * argv[]){
    //gets source file for checking
    NewNode *ASTTree;
    char fileName[FILE_SIZE];

    //make sure there are actually enough arguments to collect the name of the file
    if(argc != 2) errorPrint(NULL, "Please include file name.\n", FILE_ERR);

    //put the name of the file to be read into file name variable
    strcpy(fileName, argv[1]);
    yyin = fopen(fileName, "r");

    //check that it's a real file name
    if(yyin == NULL) errorPrint(NULL, "Please include valid file name, file not found.\n", FILE_ERR);

    //print to the screen to show that it's being scanned and parsed
    fprintf(stdout, "Scanning, Parsing, Analyzing, Printing: %s\n", fileName);

    //call parse
    ASTTree = parse();

    //semantic analysis
    analyzeSemantics(ASTTree);

    char *outputCodeName = (char *) malloc(sizeof(fileName) + 4);
    strcpy(outputCodeName, fileName);
    strcat(outputCodeName, ".out"); //creating the new out file
    printf("Output file name: %s\n", outputCodeName);

    outputCode = fopen(outputCodeName, "w"); //open it for code generation

    fprintf(stdout, "\nGenerating Code...\n");
    generateCode(ASTTree, outputCode);
    fprintf(stdout, "Finished Generating...\n\n");

    fclose(yyin); //then I close the file
    fclose(outputCode);
    return 0;
}



