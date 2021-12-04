.phony all:
all: a2

a2: diskinfo.c
	gcc -Wall diskinfo.c -lreadline -o diskinfo -g

.PHONY clean:
clean:
	-rm -rf *.o *.exe
