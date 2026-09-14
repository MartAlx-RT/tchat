// TODO: remove useless includes
#include "db_defs.h"
#include <signal.h>
#include <sys/wait.h>
#include <err.h>
#include <x86intrin.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <readline/readline.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void lookup_received_msg(void)
{
	int fd = open(DB_FILENAME, O_RDWR);
	assert(fd > 0);

	db_entry_t entry = {};
	while(db_peek_entry_header(fd, &entry.header))
	{
		if(entry.header.valid && entry.header.receiver_pid == getppid())
		{
			db_mark_entry(fd, 0);
			db_read_entry(fd, &entry);

			printf("(%d) ==> %s\n", entry.header.sender_pid, entry.msg);

			free(entry.msg);
			break;
		}

		db_next_entry(fd);
	}

	close(fd);
}

static void send_msg(pid_t pid, const char *msg)
{
	assert(msg);

	int fd = open(DB_FILENAME, O_WRONLY | O_APPEND);
	assert(fd > 0);

	const db_entry_t entry =
	{
		.header =
		{
			.receiver_pid = pid,
			.sender_pid = getpid(),
			.valid = 1,
			.time = time(NULL),
			.id = __rdtsc(),
			.msg_len = strlen(msg)
		},
		.msg = msg
	};

	db_write_entry(&entry);
	close(fd);
}

static void cli(void)
{
	char *line = NULL, *line_end = NULL;
	pid_t pid = 0;
	do
	{
		/* waiting for enter */
		if(getchar() == EOF)
			break;

		/* reading pid */
		line = readline("pid: ");
		pid = strtol(line, &line_end, 10);
		if(line_end[0] != '\0')
		{
			printf("pid must be an integer\n");
			continue;
		}
		free(line);

		/* reading msg */
		line = readline("msg: ");
		send_msg(pid, line);
		free(line);
	} while(1);
}

int main(void)
{
	pid_t pid = fork();

	if(pid < 0)
		err(EXIT_FAILURE, "fork");
	else if(pid == 0)
	{
		while(1)
			lookup_received_msg();
	}
	else
	{
		cli();
		kill(pid, SIGKILL);
	}

	return 0;
}

