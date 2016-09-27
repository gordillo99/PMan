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
void find_and_print_process_info(pid_t target_pid);
void get_updated_state(pid_t target_pid, char ** state);

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
	char * state;
	struct Proc * next;
} Proc;

typedef struct ProcInfo
{
	pid_t pid;
	char * comm;
	char * state;
	int utime;
	int stime;
	int rss;
	int voluntary_ctxt_switches;
	int nonvoluntary_ctxt_switches;
} ProcInfo;

Proc * process_list;

Proc *new_item (pid_t pid)
{
 	Proc *newp;
 	newp = (Proc *) emalloc(sizeof(Proc));
 	newp->pid = pid;
 	newp->state = NULL;
 	newp->next = NULL;
 	return newp;
}

Proc *add_front(Proc *listp, Proc *newp)
{
 	newp->next = listp;
 	return newp;
}

Proc *update_process_list (Proc *listp)
{
 	Proc *curr, *prev, *head;
 	head = listp;
 	prev = NULL;
	
 	for (curr = listp; curr != NULL; curr = curr-> next) 
	{
		get_updated_state(curr->pid, &(curr->state));
 		if (!(*(curr->state) == 'S' || *(curr->state) == 'R'))
		{
 			if (curr->next == NULL && prev == NULL)
			{
				if (curr->state != NULL) free(curr->state); 
	 			free(curr);
	 			return NULL;
 			}
			else if (prev == NULL) 
			{
				listp = curr->next;
			}
			else
			{
 				prev->next = curr->next;
 			}
 			if (curr->state != NULL) free(curr->state); 
 			free(curr);
 		}
 		prev = curr;
 	}
 	return head;
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

void print_all(Proc *listp)
{
	int i = 0;
 	Proc *next;
 	while (listp != NULL) {
 		printf("%d: \n", (int) listp->pid);
		listp = listp->next;
		i++;
 	}
	printf("Total background jobs: %d\n", i);
}

void free_all(Proc *listp)
{
 	Proc *next;
 	for ( ; listp != NULL; listp = next ) {
 		next = listp->next;
 		free(listp);
 	}
}

int main (int argc, char **argv) {
	process_list = NULL;
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
		Proc * new = new_item(pid); 
		process_list = add_front(process_list, new);
		do {
      wpid = waitpid(pid, &status, WNOHANG);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
  return 1;
}

int lsh_execute(char **args)
{
	process_list = update_process_list(process_list);
	
  if (args[0] == NULL) {
		printf("PMan:> command not found\n");
    return 1;
  }
	else if (strcmp(args[0], "bg") == 0) {
		// pass everything after bg
		return lsh_launch(args + 1);
	}
	else if (strcmp(args[0], "bglist") == 0) {
		print_all(process_list);
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
	else if (strcmp(args[0], "pstat") == 0) {
		pid_t pid_to_find = (pid_t) atoi(args[1]);
		if (lookup_pid(process_list, pid_to_find)) find_and_print_process_info(pid_to_find);
		else printf("PMan:> Error: Process %d does not exist\n", pid_to_find);
	}
	else {
		printf("PMan:> %s: command not found\n", args[0]);
	}

  return 1;
}

void kill_process(char * pid) {
	pid_t pid_to_find = (pid_t) atoi(pid);
	if (lookup_pid(process_list, pid_to_find)) kill(pid_to_find, SIGTERM);
	else printf("PMan:> Error: Process %d does not exist\n", pid_to_find);
}

void stop_process(char * pid) {
	pid_t pid_to_find = (pid_t) atoi(pid);
	if (lookup_pid(process_list, pid_to_find)) kill((pid_t) atoi(pid), SIGSTOP);
	else printf("PMan:> Error: Process %d does not exist\n", pid_to_find);
}

void start_process(char * pid) {
	pid_t pid_to_find = (pid_t) atoi(pid);
	if (lookup_pid(process_list, pid_to_find)) kill((pid_t) atoi(pid), SIGCONT);
	else printf("PMan:> Error: Process %d does not exist\n", pid_to_find);
}

void find_and_print_process_info(pid_t target_pid) {
	char path[40], line[100], *p;
  FILE* statusf;

  snprintf(path, 40, "/proc/%ld/status", (long) target_pid);

  statusf = fopen(path, "r");
  if(!statusf)
  	return;
 	printf("PID: %d", (int) target_pid);

  while(fgets(line, 100, statusf)) {
		if(strncmp(line, "State:", 6) == 0) {
			// Ignore "State:" and whitespace
			p = line + 7;
			while(isspace(*p)) ++p;

			printf("state: %s", p);
		}
		
		if(strncmp(line, "VmRSS:", 6) == 0) {
			// Ignore "VmRSS:" and whitespace
			p = line + 7;
			while(isspace(*p)) ++p;

			printf("rss: %s", p);
		}
		
		if(strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
			// Ignore "voluntary_ctxt_switches:" and whitespace
			p = line + 25;
			while(isspace(*p)) ++p;

			printf("voluntary_ctxt_switches: %s", p);
		}
		
		if(strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
			// Ignore "nonvoluntary_ctxt_switches:" and whitespace
			p = line + 28;
			while(isspace(*p)) ++p;

			printf("voluntary_ctxt_switches: %s", p);
		}
  }

  fclose(statusf);
}

void get_updated_state(pid_t target_pid, char ** state) {
	char path[40], line[100], *p, *temp_ptr;
  FILE* statusf;
  int i = 0;

  snprintf(path, 40, "/proc/%ld/status", (long) target_pid);

  statusf = fopen(path, "r");
  if(!statusf) {
  	state = NULL;
  	return;
  }

 	while(fgets(line, 100, statusf)) {
		if(strncmp(line, "State:", 6) == 0) {
			p = line + 7;
			while(isspace(*p)) ++p;
			temp_ptr = p;
			while(*temp_ptr != '\n') ++temp_ptr;
			if (*state != NULL) free(*state);
			*state = (char*) emalloc(2 * sizeof(char));
			**state = *p;
			*(*state + 1) = '\0';
			break;
		}
  }

  fclose(statusf);
}









