all : winsuport.o tron0 tron1 

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o

memoria.o : memoria.c memoria.h
	gcc -Wall -c memoria.c -o memoria.o

tron0 : tron0.c winsuport.o winsuport.h
	gcc -Wall tron0.c winsuport.o memoria.o -o tron0 -lcurses

tron1 : tron1.c winsuport.o winsuport.h
	gcc -Wall tron1.c winsuport.o memoria.o -o tron1 -lcurses

clean: 
	rm winsuport.o tron0 tron1
