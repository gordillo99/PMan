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
#define MAX_PATH_LEN 40
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"

char * prompt_user();
void main_loop();
char **parse_user_input(char * line);
int execute_process(char ** args, char * line);
int handle_user_input(char ** args, char * line);
void change_process_status(char * pid, int option);
void find_and_print_process_info(pid_t target_pid);
void get_updated_state(pid_t target_pid, char ** state);
void update_bg_procss();

void *emalloc(int n) 
{
 	void *p;
 	p = malloc(n);
 	if (p == NULL)
	{
	 	perror("command malloc failed\n");
		exit(EXIT_FAILURE);
 	}
 	return p;
}

typedef struct Proc
{
	pid_t pid;
	char * state;
	int isStop;
	char * cmd;
	struct Proc * next;
} Proc;

Proc * process_list;

Proc *new_item (pid_t pid, char * line)
{
 	Proc *newp;
 	newp = (Proc *) emalloc(sizeof(Proc));
 	newp->pid = pid;
 	newp->state = NULL;
 	newp->isStop = 0;
 	newp->next = NULL;
 	newp->cmd = (char*) emalloc(strlen(line)*sizeof(char));
 	strcpy(newp->cmd, line);
 	return newp;
}

Proc *add_front(Proc *listp, Proc *newp)
{
 	newp->next = listp;
 	return newp;
}

Proc *delete_item (Proc *listp, pid_t pid)
{
 	Proc *curr, *prev, *head;
 	head = listp;
 	prev = NULL;
	
 	for (curr = listp; curr != NULL; curr = curr->next) 
	{
 		if (pid == curr->pid)
		{
 			if (curr->next == NULL && prev == NULL)
			{
				if (curr->cmd != NULL) free(curr->cmd);
	 			free(curr);
	 			return NULL;
 			}
			else if (prev == NULL) listp = curr->next;
			else prev->next = curr->next;
 			
 			if (curr->cmd != NULL) free(curr->cmd);
 			free(curr);
 			return head;
 		}
 		prev = curr;
 	}
 	return head;
}

void update_isStop(Proc *listp, pid_t pid, int val) {
	Proc * tmp = listp;
 	for (; tmp != NULL; tmp = tmp->next)
	{
 		if (pid == tmp->pid)
		{
 			tmp->isStop = val;
 			break;
 		}
 	}
}

int lookup_pid(Proc *listp, pid_t pid) {
	Proc * tmp = listp;
 	for (; tmp != NULL; tmp = tmp->next) if (pid == tmp->pid) return 1;
 	return 0;
}

void print_all(Proc *listp)
{
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

void free_all(Proc *listp)
{
 	Proc *next;
 	for ( ; listp != NULL; listp = next ) {
 		next = listp->next;
		if (listp->cmd != NULL) free(listp->cmd);
 		free(listp);
 	}
}

int main (int argc, char **argv) {
	process_list = NULL;
	main_loop();
	return (0);
}

void main_loop()
{
  char *user_input;
  char line_copy[MAX_LINE_LEN];
  char **args;
  int status = 1;
	
  while (status) {
    user_input = prompt_user();
    strcpy(line_copy, user_input);
    args = parse_user_input(user_input);
    status = handle_user_input(args, line_copy);
		free(args);
    free(user_input);  
  }
	free_all(process_list);
	printf("Goodbye!\n");
}

char * prompt_user() {
	char *input = NULL ;
  char *prompt = "PMan:> ";
  input = readline(prompt);

  return input;
}

char **parse_user_input(char *line)
{
  int bufsize = TOK_BUFSIZE, position = 0;
  char **tokens = emalloc(bufsize * sizeof(char*));
  char *token;

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        perror("realloc command failed\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int execute_process(char **args, char * line)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("execvp command failed\n");
			exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
		// Parent process
		Proc * new = new_item(pid, line); 
		process_list = add_front(process_list, new);
	} else {
		perror("fork command failed\n");
		exit(EXIT_FAILURE);
	}
  return 1;
}

int handle_user_input(char **args, char * line)
{
	update_bg_procss();
	
  if (args[0] == NULL) { // if user presses enter with no commands, return
    return 1;
  }
	else if (strcmp(args[0], "bg") == 0) {
		return execute_process(args + 1, line); // pass everything after bg
	}
	else if (strcmp(args[0], "bglist") == 0) {
		print_all(process_list);
	}
	else if (strcmp(args[0], "bgkill") == 0) {
		change_process_status((args + 1)[0], 0);
	}
	else if (strcmp(args[0], "bgstop") == 0) {
		change_process_status((args + 1)[0], 1);
	}
	else if (strcmp(args[0], "bgstart") == 0) {
		change_process_status((args + 1)[0], 2);
	}
	else if (strcmp(args[0], "pstat") == 0) {
		pid_t pid_to_find = (pid_t) atoi(args[1]);
		if (lookup_pid(process_list, pid_to_find)) find_and_print_process_info(pid_to_find);
		else printf("PMan:> Error: Process %d does not exist\n", pid_to_find);
	}
  else if (strcmp(args[0], "exit") == 0) { // if user enters exit, return and break main_loop
		return 0;
	}
	else {
		printf("PMan:> %s: command not found\n", args[0]);
	}

  return 1;
}

void change_process_status(char * pid, int option) {
	pid_t pid_to_find = (pid_t) atoi(pid);
	int status;
	if (!lookup_pid(process_list, pid_to_find)) {
		printf("PMan:> Error: Process %d does not exist\n", pid_to_find);
		return;
	}

	if (option == 0) {
		status = kill(pid_to_find, SIGTERM);
	} else if (option == 1) {
		status = kill(pid_to_find, SIGSTOP);
	} else {
		status = kill(pid_to_find, SIGCONT);
	}

	if (status < 0) printf("kill command failed\n");
}

void find_and_print_process_info(pid_t target_pid) {
	char path[MAX_PATH_LEN], line[MAX_LINE_LEN], *text;
  FILE* status_file;

  snprintf(path, MAX_PATH_LEN, "/proc/%d/status", (int) target_pid);

  status_file = fopen(path, "r");
  if(!status_file) {
		perror("Could not open status file.\n");
		return;
	}

 	printf("PID: %d\n", (int) target_pid);

  while(fgets(line, 100, status_file)) {
  	if(strncmp(line, "Name:", 5) == 0) {
			// Ignore "Name:" and whitespace
			text = line + 6;
			while(isspace(*text)) ++text;

			printf("comm: %s", text);
		}

		if(strncmp(line, "State:", 6) == 0) {
			// Ignore "State:" and whitespace
			text = line + 7;
			while(isspace(*text)) ++text;

			printf("state: %s", text);
		}
		
		if(strncmp(line, "VmRSS:", 6) == 0) {
			// Ignore "VmRSS:" and whitespace
			text = line + 7;
			while(isspace(*text)) ++text;

			printf("rss: %s", text);
		}
		
		if(strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
			// Ignore "voluntary_ctxt_switches:" and whitespace
			text = line + 25;
			while(isspace(*text)) ++text;

			printf("voluntary_ctxt_switches: %s", text);
		}
		
		if(strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
			// Ignore "nonvoluntary_ctxt_switches:" and whitespace
			text = line + 28;
			while(isspace(*text)) ++text;

			printf("nonvoluntary_ctxt_switches: %s", text);
		}
  }

  fclose(status_file);
}

void update_bg_procss() {

  int status = 0;
  int id = 0;

  while(1) {
		id = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
		if (id > 0) {
			if (WIFEXITED(status)) {
				printf("Process %d has terminated.\n", id);
				process_list = delete_item (process_list, id);
			}
			if (WIFSIGNALED(status)) {
				printf("Process %d was killed.\n", id);
		  	process_list = delete_item (process_list, id);
		  }
		  if (WIFSTOPPED(status)) {
				printf("Process %d was stopped.\n", id);
		  	update_isStop(process_list, id, 1);
		  }
		  if (WIFCONTINUED(status)) {
				printf("Process %d restarted.\n", id);
		  	update_isStop(process_list, id, 0);
			}
		} else {
			break;
		}
  }
}









