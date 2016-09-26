#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <errno.h>     

#define MAX_LINE_LEN 256
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char * lsh_read_line();
void lsh_loop();
char **lsh_split_line(char *line);
int lsh_launch(char **args);
int lsh_execute(char **args);
void kill_process(char * pid);
void stop_process(char * pid);
void start_process(char * pid);

void *emalloc(int n) 
{
 	void *p;
 	p = malloc(n);
 	if (p == NULL)
	{
	 	fprintf(stderr, "malloc of %u bytes failed", n);
 		exit(1);
 	}
 	return p;
}

typedef struct Proc
{
	pid_t pid;
	char ** cmd;
	struct Proc * next;
} Proc;

Proc *newitem (pid_t pid, char ** cmd)
{
 	Proc *newp;
 	newp = (Proc *) emalloc(sizeof(Proc));
 	newp->pid = pid;
	newp->cmd = cmd;
 	newp->next = NULL;
 	return newp;
}

Proc *addfront(Proc *listp, Proc *newp)
{
 	newp->next = listp;
 	return newp;
}

Proc *delitem (Proc *listp, pid_t pid, int * position)
{
	*position = 0;
 	Proc *curr, *prev;
 	prev = NULL;
	
 	for (curr = listp; curr != NULL; curr = curr-> next) 
	{
 		if (pid == curr->pid)
		{
 			if (prev == NULL)
			{
 				listp = curr->next;
 			}
			else
			{
 				prev->next = curr->next;
 			}
			if(curr->cmd != NULL)
			{
				free(curr->cmd);
			}
 			free(curr);
 			return listp;
 		}
 		prev = curr;
		*position = *position + 1;
 	}
 	return NULL;
}

int lookup_pid(Proc *listp, pid_t pid) {
	Proc * tmp = listp;
 	for (; tmp != NULL; tmp = tmp->next)
	{
 		if (pid == tmp->pid)
		{
 			return 1;
 		}
 	}
 	return 0;
}

void freeall(Proc *listp)
{
 	Proc *next;
 	for ( ; listp != NULL; listp = next ) {
 		next = listp->next;
 		free(listp->cmd);
 		free(listp);
 	}
}

int main (int argc, char **argv) {
	lsh_loop();
	return (0);
}

void lsh_loop()
{
  char *line;
  char **args;
  int status;
	
  do {
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

char * lsh_read_line() {
	char *input = NULL ;
  char *prompt = "PMan:> ";
  input = readline(prompt);

  return input;
}

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = emalloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else {
		// Parent process
		
	}
  return 1;
}

int lsh_execute(char **args)
{
  if (args[0] == NULL) {
		printf("PMan:> Please enter a command\n");
    return 1;
  }

	else if (strcmp(args[0], "bg") == 0) {
		// pass everything after bg
		return lsh_launch(args + 1);
	}

	else if (strcmp(args[0], "bglist") == 0) {
		
	}

	else if (strcmp(args[0], "bgkill") == 0) {
		kill_process((args + 1)[0]);
	}

	else if (strcmp(args[0], "bgstop") == 0) {
		stop_process((args + 1)[0]);
	}

	else if (strcmp(args[0], "bgstart") == 0) {
		start_process((args + 1)[0]);
	}

	else {
		printf("PMan:> %s: command not found\n", args[0]);
	}

  return 1;
}

void kill_process(char * pid) {
	kill((pid_t) atoi(pid), SIGTERM);
}

void stop_process(char * pid) {
	kill((pid_t) atoi(pid), SIGSTOP);
}

void start_process(char * pid) {
	kill((pid_t) atoi(pid), SIGCONT);
}








