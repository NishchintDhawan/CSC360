.phony all:
all: a2 a3

a2: diskinfo.c
	gcc -Wall diskinfo.c -lreadline -o diskinfo -g
a3: disklist.c
	gcc -Wall disklist.c -lreadline -o disklist -g

.PHONY clean:
clean:
	-rm -rf *.o *.exe
