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
#define TOK_BUF_SIZE 64
#define MAX_PATH_LEN 40

typedef struct Proc { // define struct to hold info about process
	pid_t pid;
	char * state;
	int isStop;
	char * cmd;
	struct Proc * next;
} Proc;

// prototypes for all the functions in this file
char * prompt_user();
void main_loop();
char **parse_user_input(char * line);
int execute_process(char ** args, char * line);
int handle_user_input(char ** args, char * line);
void change_process_status(char * pid, int option);
void update_bg_procss();
void find_and_print_process_info(pid_t target_pid);
void print_utime_and_stime(pid_t target_pid);
typedef struct Proc Proc;
void *emalloc(int n);
Proc *new_item (pid_t pid, char * line);
Proc *add_front(Proc *listp, Proc *newp);
Proc *delete_item (Proc *listp, pid_t pid);
void update_isStop(Proc *listp, pid_t pid, int val);
int lookup_pid(Proc *listp, pid_t pid);
void print_all(Proc *listp);
void free_all(Proc *listp);

// global pointer responsible for keeping track of the processes created/removed
Proc * process_list;

// asks user for input, parses it, and executes respective code
int main (int argc, char **argv) {
	char *user_input;
  char line_copy[MAX_LINE_LEN]; // this copy is saved with each Proc struct in cmd
  char **args; // parsed user input
  int status = 1;

	process_list = NULL;
	
  while (status) {
    user_input = prompt_user();
    strcpy(line_copy, user_input);
    args = parse_user_input(user_input);
    status = handle_user_input(args, line_copy);
		if (args != NULL) free(args);
		if (args != NULL) free(user_input);
  }

	free_all(process_list); // avoid memory leaks
	printf("Goodbye!\n");
	return (0); // user entered exit
}


// prompts and collects user input
char * prompt_user() {
	char *input = NULL ;
  char *prompt = "PMan:> ";
  input = readline(prompt);

  return input;
}

// parses the user input, splits string based on separators " \t\r\n\a"
char ** parse_user_input(char * line)
{
  int buffer_size = TOK_BUF_SIZE;
	int position = 0;
  char **tokens = emalloc(buffer_size * sizeof(char*)); // allocate space, if more is needed, it will be reallocated
  char *token;

  token = strtok(line, " \t\r\n\a");
  while (token != NULL) {
    tokens[position] = token; // save token
    position++; // move to next position in tokens

    if (position >= buffer_size) { // if more space is needed, allocate more memory
      buffer_size += TOK_BUF_SIZE; // increase buffer size
      tokens = realloc(tokens, buffer_size * sizeof(char*)); // reallocate memory
      if (!tokens) { // error handling
        perror("realloc command failed\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, " \t\r\n\a");
  }
  tokens[position] = NULL;
  return tokens;
}

// forks and new process runs user's commands
int execute_process(char **args, char * line)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) { // child process code
    if (execvp(args[0], args) == -1) {
			// error handling
      perror("execvp command failed ");
			exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
  } else if (pid > 0) { // parent process code
		Proc * new = new_item(pid, line); // create new struct for process
		process_list = add_front(process_list, new); // add struct to process list
	} else { // error handling
		perror("fork command failed\n");
		exit(EXIT_FAILURE);
	}
  return 1;
}

// updates the process list and reacts according to user input
int handle_user_input(char ** args, char * line)
{
	update_bg_procss(); // update process list
	
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
		if (lookup_pid(process_list, pid_to_find)) { // check if process exists
			find_and_print_process_info(pid_to_find); // print the information from status file
			print_utime_and_stime(pid_to_find); // print the information from the stat file
		}
		else printf("PMan:> Error: Process %d does not exist\n", pid_to_find); // error handling
	}
  else if (strcmp(args[0], "exit") == 0) { // if user enters exit, return and break main_loop
		return 0;
	}
	else {
		printf("PMan:> %s: command not found\n", args[0]);
	}

  return 1;
}

// sends respective signal to process
void change_process_status(char * pid, int option) {
	pid_t pid_to_find = (pid_t) atoi(pid);
	int status;
	if (!lookup_pid(process_list, pid_to_find)) { // verify process is in the process list
		printf("PMan:> Error: Process %d does not exist\n", pid_to_find); // error handling
		return;
	}

	if (option == 0) { // handle different signals the user can send
		status = kill(pid_to_find, SIGTERM);
	} else if (option == 1) {
		status = kill(pid_to_find, SIGSTOP);
	} else {
		status = kill(pid_to_find, SIGCONT);
	}

	if (status < 0) printf("kill command failed\n"); // error handling
}

// this loop determines which processes in the list need to be updated
void update_bg_procss() {

  int status = 0;
  int id = 0;

  while(1) {
		id = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED); 
		if (id > 0) { // the status of the process changed
			if (WIFEXITED(status)) { // the process ended normally
				printf("Process %d has terminated.\n", id);
				process_list = delete_item (process_list, id);
			}
			if (WIFSIGNALED(status)) { // the process was killed
				printf("Process %d was killed.\n", id);
		  	process_list = delete_item (process_list, id);
		  }
		  if (WIFSTOPPED(status)) { // the process was stopped
				printf("Process %d was stopped.\n", id);
		  	update_isStop(process_list, id, 1);
		  }
		  if (WIFCONTINUED(status)) { // the process was restarted
				printf("Process %d restarted.\n", id);
		  	update_isStop(process_list, id, 0);
			}
		} else {
			break;
		}
  }
}

// prints utime and stime from the /proc/stat file
void print_utime_and_stime(pid_t target_pid) {
	char path[MAX_PATH_LEN], line[MAX_LINE_LEN], *text;
  FILE* status_file;

  snprintf(path, MAX_PATH_LEN, "/proc/%d/stat", (int) target_pid); // setup path to file

  status_file = fopen(path, "r"); // read file

  if(!status_file) { // error handling
		perror("Could not open status file.\n");
		return;
	}

  if (fgets(line, MAX_LINE_LEN, status_file)) { // get first 256 chars from the first line
  	char ** proc_stat = parse_user_input(line);
		long utime = atol(*(proc_stat + 14)) / sysconf(_SC_CLK_TCK);
		long stime = atol(*(proc_stat + 15)) / sysconf(_SC_CLK_TCK);
		// print utime and stime
		printf("utime: %ld \n", utime);
		printf("stime: %ld \n", stime);
  }

  fclose(status_file);
}

void find_and_print_process_info(pid_t target_pid) {
	char path[MAX_PATH_LEN], line[MAX_LINE_LEN], *text;
  FILE* status_file;

  snprintf(path, MAX_PATH_LEN, "/proc/%d/status", (int) target_pid); // setup path to file

  status_file = fopen(path, "r"); // read file

  if(!status_file) { // error handling
		perror("Could not open status file.\n");
		return;
	}

 	printf("PID: %d\n", (int) target_pid);

  while(fgets(line, MAX_LINE_LEN, status_file)) { // read line by line

		// get name info 
  	if(strncmp(line, "Name:", 5) == 0) {
			// Ignore "Name:" and whitespace
			text = line + 6;
			while(isspace(*text)) ++text;

			printf("comm: %s", text);
		}

		// get state info
		if(strncmp(line, "State:", 6) == 0) {
			// Ignore "State:" and whitespace
			text = line + 7;
			while(isspace(*text)) ++text;

			printf("state: %s", text);
		}
		
		// get rss info
		if(strncmp(line, "VmRSS:", 6) == 0) {
			// Ignore "VmRSS:" and whitespace
			text = line + 7;
			while(isspace(*text)) ++text;

			printf("rss: %s", text);
		}
		
		// get voluntary cxtx switches info
		if(strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
			// Ignore "voluntary_ctxt_switches:" and whitespace
			text = line + 25;
			while(isspace(*text)) ++text;

			printf("voluntary_ctxt_switches: %s", text);
		}
		
		// get nonvoluntary cxtx switches info
		if(strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
			// Ignore "nonvoluntary_ctxt_switches:" and whitespace
			text = line + 28;
			while(isspace(*text)) ++text;

			printf("nonvoluntary_ctxt_switches: %s", text);
		}
  }

  fclose(status_file);
}

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








