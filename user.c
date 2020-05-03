/*
 *  user.c
 *  Contains implementation of user process accessing proc node
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


#define DRIVER_NAME "Dev Page Fault Driver"
#define DRIVER_PATH "/proc/pf_probe_B"
#define PROBE_LOG_NAME "./pf_probe_B.log"

#define USER_SLEEP 5

#define USER_DEBUG 0


void exit_handler(int signal) {
	printf("You have presses Ctrl-C\n");
	exit(0);
}


int main(int argc, char const *argv[]) {

	ssize_t read;
	size_t len = 0;
	int count = 0;
	FILE *file;
	FILE *log_file;
	char *line = NULL;

	printf("This is a simple program to interact with %s\n", DRIVER_NAME);

	file = fopen(DRIVER_PATH, "r");
	if (file == NULL) {
		fprintf(stderr, "Failed to open path %s, of %s\n", DRIVER_PATH, DRIVER_NAME);
		return errno;
	}
	else {
		if (USER_DEBUG) {
			printf("Reading from the %s\n", DRIVER_PATH);
		}
		log_file = fopen(PROBE_LOG_NAME, "w");
		if (log_file == NULL) {
			fprintf(stderr, "Failed to create log path %s\n", PROBE_LOG_NAME);
		}
		signal(SIGINT, exit_handler);
		while (1) {
			read = getline(&line, &len, file);
			if (read < 0){
				fprintf(stderr, "Failed to read the message from the %s\n", DRIVER_PATH);
				return errno;
			}
			else {
				pid_t pid = getpid();
				if (strcmp(line, "EXIT_CODE\n")!=0) {
					printf("%4d:: %s", count, line);
					fprintf(log_file, "%4d:: %s", count, line);
					count += 1;
					if (USER_DEBUG) {
						printf("Process %d sleeping for %d msec.\n", pid, USER_SLEEP);
					}
					usleep(USER_SLEEP * 10000);
				}
				else{
					printf("Reading from the %s Completed\n", DRIVER_PATH);
					break;
				}
			}
		}
		fclose(file);
		fclose(log_file);
		if (line) {
			free(line);
		}
	}
	return 0;
}
