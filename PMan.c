#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <errno.h>
#include "proc_list.h"
#include "process_status.h"

#define MAX_LINE_LEN 256
#define TOK_BUF_SIZE 64
#define MAX_PATH_LEN 40

// prototypes for all the functions in this file
char * prompt_user();
void main_loop();
char **parse_user_input(char * line);
int execute_process(char ** args, char * line);
int handle_user_input(char ** args, char * line);
void change_process_status(char * pid, int option);
void update_bg_procss();
void print_utime_and_stime(pid_t target_pid);

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








