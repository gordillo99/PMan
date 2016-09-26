.phony all:
all: PMan

PMan: PMan.c
	gcc PMan.c -o pman -lreadline -lhistory

.PHONY clean:
clean:
	-rm -rf *.o *.exe
