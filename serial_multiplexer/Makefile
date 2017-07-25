.PHONY: all

all: multi
multi.o: multi.c  
	$(CC) -Wall -c -g $<

multi: multi.o
	$(CC) $< -o multi

clean:
	rm -f multi
	rm -f multi.o

