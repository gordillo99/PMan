#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <errno.h>
#include "process_status.h"
#include "proc_list.h"

#define MAX_LINE_LEN 256
#define TOK_BUF_SIZE 64

char * prompt_user();
void main_loop();
char **parse_user_input(char * line);
int execute_process(char ** args, char * line);
int handle_user_input(char ** args, char * line);
void change_process_status(char * pid, int option);
void update_bg_procss();

Proc * process_list;

int main (int argc, char **argv) {
	char *user_input;
  char line_copy[MAX_LINE_LEN];
  char **args;
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

	free_all(process_list);
	printf("Goodbye!\n");
	return (0);
}

char * prompt_user() {
	char *input = NULL ;
  char *prompt = "PMan:> ";
  input = readline(prompt);

  return input;
}

char ** parse_user_input(char *line)
{
  int buffer_size = TOK_BUF_SIZE;
	int position = 0;
  char **tokens = emalloc(buffer_size * sizeof(char*));
  char *token;

  token = strtok(line, " \t\r\n\a");
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= buffer_size) {
      buffer_size += TOK_BUF_SIZE;
      tokens = realloc(tokens, buffer_size * sizeof(char*));
      if (!tokens) {
        perror("realloc command failed\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, " \t\r\n\a");
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
      perror("execvp command failed ");
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

int handle_user_input(char ** args, char * line)
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









