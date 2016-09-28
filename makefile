.phony all:
all: PMan

PMan: PMan.c
	gcc PMan.c process_status.c proc_list.c -o pman -lreadline -lhistory -ggdb

.PHONY clean:
clean:
	-rm -rf *.o *.exe
