
all: myshell

myshell: line_parser.o job_control.o myshell.o 
	gcc -g -Wall -o myshell line_parser.o job_control.o myshell.o

line_parser.o: line_parser.c
	gcc -g -Wall -c -o line_parser.o line_parser.c 
	
job_control.o: job_control.c
	gcc -g -Wall -c -o job_control.o job_control.c 
	
myshell.o: myshell.c
	gcc -g -Wall -c -o myshell.o myshell.c 


.PHONY: clean

clean: 
	rm -f *.o myshell
