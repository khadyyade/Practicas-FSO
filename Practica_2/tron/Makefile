all : winsuport.o winsuport2.o tron0 tron1 tron2 tron3 tron4 tron5 tron6 oponent3 oponent4 oponent5 oponent6

winsuport.o : winsuport.c winsuport.h
	gcc -Wall -c winsuport.c -o winsuport.o

memoria.o : memoria.c memoria.h
	gcc -Wall -c memoria.c -o memoria.o

winsuport2.o : winsuport2.c winsuport2.h
	gcc -Wall -c winsuport2.c -o winsuport2.o

missatge.o : missatge.c missatge.h
	gcc -Wall -c missatge.c -o missatge.o

semafor.o : semafor.c semafor.h
	gcc -Wall -c semafor.c -o semafor.o

tron0 : tron0.c winsuport.o winsuport.h
	gcc -Wall tron0.c winsuport.o memoria.o -o tron0 -lcurses

tron1 : tron1.c winsuport.o winsuport.h
	gcc -Wall tron1.c winsuport.o memoria.o -o tron1 -lcurses

tron2 : tron2.c winsuport2.o memoria.o semafor.o
	gcc -Wall tron2.c winsuport2.o memoria.o semafor.o -o tron2 -lcurses

tron3 : tron3.c winsuport2.o memoria.o semafor.o
	gcc -Wall tron3.c winsuport2.o memoria.o semafor.o -o tron3 -lcurses

oponent3 : oponent3.c winsuport2.o memoria.o semafor.o
	gcc -Wall oponent3.c winsuport2.o memoria.o semafor.o -o oponent3 -lcurses

tron4 : tron4.c winsuport2.o memoria.o semafor.o
	gcc -Wall tron4.c winsuport2.o memoria.o semafor.o -o tron4 -lcurses -lpthread

oponent4 : oponent4.c winsuport2.o memoria.o semafor.o
	gcc -Wall oponent4.c winsuport2.o memoria.o semafor.o -o oponent4 -lcurses

tron5 : tron5.c winsuport2.o memoria.o missatge.o
	gcc -Wall tron5.c winsuport2.o memoria.o missatge.o -o tron5 -lcurses -lpthread

oponent5 : oponent5.c winsuport2.o memoria.o missatge.o
	gcc -Wall oponent5.c winsuport2.o memoria.o missatge.o -o oponent5 -lcurses -lpthread

tron6 : tron6.c winsuport2.o memoria.o missatge.o
	gcc -Wall tron6.c winsuport2.o memoria.o missatge.o -o tron6 -lcurses -lpthread

oponent6 : oponent6.c winsuport2.o memoria.o missatge.o
	gcc -Wall oponent6.c winsuport2.o memoria.o missatge.o -o oponent6 -lcurses -lpthread

clean: 
	rm -f *.o tron0 tron1 tron2 tron3 tron4 tron5 tron6 oponent3 oponent4 oponent5 oponent6
