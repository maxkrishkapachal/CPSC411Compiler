objects = y.tab.c lex.yy.c analyzer.c cgen.c stable.c utils.c main.c 

compiler: $(objects)
	gcc -Wall -o compiler $(objects)
lex.yy.c: scanner.l utils.h 
	flex scanner.l 
y.tab.c: parser.y utils.h parser.h 
	bison -y -d parser.y

.PHONY : clean
clean :
	-rm lex.yy.c y.tab.h y.tab.c compiler 
