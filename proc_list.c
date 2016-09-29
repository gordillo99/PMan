#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <errno.h>

typedef struct Proc { // define struct to hold info about process
	pid_t pid;
	char * state;
	int isStop;
	char * cmd;
	struct Proc * next;
} Proc;

void *emalloc(int n) { // custom malloc function with error checking
 	void *p;
 	p = malloc(n);
 	if (p == NULL) { // error handling
	 	perror("command malloc failed\n");
		exit(EXIT_FAILURE);
 	}
 	return p;
}

// creates a new node for the process list
Proc *new_item (pid_t pid, char * line) {
 	Proc *newp;
 	newp = (Proc *) emalloc(sizeof(Proc));
 	newp->pid = pid;
 	newp->state = NULL;
 	newp->isStop = 0;
 	newp->next = NULL;
 	newp->cmd = (char*) emalloc(strlen(line)*sizeof(char)); // allocates space for cmd
 	strcpy(newp->cmd, line); // gets a copy of what the user entered
 	return newp;
}

// adds a node at the front of the list
Proc *add_front(Proc *listp, Proc *newp) {
 	newp->next = listp;
 	return newp;
}

// deletse node with the given pid
Proc *delete_item (Proc *listp, pid_t pid) {
 	Proc *curr, *prev, *head;
 	head = listp;
 	prev = NULL;
	
 	for (curr = listp; curr != NULL; curr = curr->next) {
 		if (pid == curr->pid) {
 			if (curr->next == NULL && prev == NULL) {
				// deallocate relevant memory
				if (curr->cmd != NULL) free(curr->cmd);
	 			free(curr);
	 			return NULL;
 			}
			else if (prev == NULL) listp = curr->next;
			else prev->next = curr->next;
 			
 			if (curr->cmd != NULL) free(curr->cmd);
			// deallocate relevant memory
 			free(curr);
 			return head;
 		}
 		prev = curr;
 	}
 	return head;
}

// updates the isStop prop of the struct
void update_isStop(Proc *listp, pid_t pid, int val) {
	Proc * tmp = listp;
 	for (; tmp != NULL; tmp = tmp->next) {
 		if (pid == tmp->pid) {
 			tmp->isStop = val;
 			break;
 		}
 	}
}

// checks to see if process with given pid is in process list
int lookup_pid(Proc *listp, pid_t pid) {
	Proc * tmp = listp;
 	for (; tmp != NULL; tmp = tmp->next) if (pid == tmp->pid) return 1;
 	return 0;
}

// prints all the bg processes
void print_all(Proc *listp) {
	int i = 0;
 	Proc *next;
 	while (listp != NULL) {
 		if (!listp->isStop) {
 			printf("PMan:> %d: %s\n", (int) listp->pid, listp->cmd);
			i++;
 		}
		listp = listp->next;
 	}
	printf("PMan:> Total background jobs: %d\n", i);
}

// deallocates all the nodes in the process list
void free_all(Proc *listp) {
 	Proc *next;
 	for ( ; listp != NULL; listp = next ) {
 		next = listp->next;
		if (listp->cmd != NULL) free(listp->cmd);
 		free(listp);
 	}
}
