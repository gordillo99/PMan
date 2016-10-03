.phony all:
all: PMan

PMan: PMan.c
	gcc PMan.c proc_list.c process_status.c -o pman -lreadline -lhistory -ggdb

.PHONY clean:
clean:
	-rm -rf *.o *.exe
