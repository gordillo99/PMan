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

