
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

// int main(int argc, char ** argv) {
// 	printf("PID : %ld \t PPID : %ld\n", (long)getpid(), (long)getppid());
// 	int result = 0;
// 	if (argc != 2) {
// 		printf("Help : Usage %s <path of file to read> eg '/usr/bin'\n", argv[0]);
// 		exit(1);
// 	}
// 	else {
// 		int fd = open(argv[1], O_RDONLY);
// 		struct stat stats;
// 		fstat(fd, &stats);
// 		posix_fadvise(fd, 0, stats.st_size, POSIX_FADV_DONTNEED);
// 		char * map = (char *) mmap(NULL, stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
// 		if (map == MAP_FAILED) {
// 			perror("Failed to mmap");
// 			return 1;
// 		}
// 		for (int i = 0; i < stats.st_size; i++) {
// 			result += map[i];
// 		}
// 		munmap(map, stats.st_size);
// 	}
// 	return result;
// }

static volatile int keep_running = 1;


void intHandler(int dummy) {
  keep_running = 0;
}

int main(void) {
	printf("PID : %ld \t PPID : %ld\n", (long)getpid(), (long)getppid());
	char* ptr = NULL;
	signal(SIGINT, intHandler);
	while (keep_running) {
		ptr = (char*)malloc(1 * 1024 * 1024 * 1024);
		for (int i = 0; i < 1024; ++i) {
			ptr[i * 1024 * 1024] = 0;     /* touch the pages */
		}
		sleep(0.5);
		// free(ptr);
	}
	free(ptr);
	printf("Bye ...\n");
	return 0;
}
