#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <x86intrin.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include "db.h"
#include <signal.h>

static const time_t LOOKUP_MSG_TIME = 1;

static void sigchld_handler(int sig);
static void sigquit_handler(int sig);
static int cli(pid_t *pid, char **msg, pid_t ch_pid);
static int cli_read_pid(pid_t *pid);
static int cli_read_msg(char **msg);
static int lookup_msg(void);
static int send_msg(pid_t receiver_pid, pid_t sender_pid, char *msg);
[[noreturn]] static void child_do(void);
[[noreturn]] static void parent_do(pid_t child_pid);

int main(void)
{
	signal(SIGCHLD, sigchld_handler);
	signal(SIGINT, sigquit_handler);
	signal(SIGQUIT, sigquit_handler);

	pid_t pid = fork();

	if(pid < 0)
		err(EXIT_FAILURE, "fork");
	else if(pid == 0)
		child_do();
	else
		parent_do(pid);

	assert(0 && "Unreachable");
}

[[noreturn]] static void child_do(void)
{
	while(1)
	{
		if(lookup_msg())
			_exit(1);

		sleep(LOOKUP_MSG_TIME);
	}
}

[[noreturn]] static void parent_do(pid_t child_pid)
{
	pid_t receiver_pid = 0;
	char *msg = NULL;

	fprintf(stderr, "[Welcome to tchat! Your pid is %d]\n", getpid());

	/* init readline history */
	using_history();

	while(cli(&receiver_pid, &msg, child_pid))
	{
		if(send_msg(receiver_pid, getpid(), msg))
		{
			kill(child_pid, SIGQUIT);
			errx(1, "Can't send message");
		}
	}

	kill(child_pid, SIGQUIT);
	fprintf(stderr, "[Quit.]\n");
	_exit(0);
}

static void sigchld_handler([[maybe_unused]] int sig)
{
	assert(sig == SIGCHLD);

	int wstatus = 0; waitpid(-1, &wstatus, WNOHANG);
	if(WEXITSTATUS(wstatus))
	{
		fprintf(stderr, "`msg_lookup` crashed\n");
		_exit(1);
	}
}

static void sigquit_handler([[maybe_unused]] int sig)
{
	assert(sig == SIGQUIT || sig == SIGINT);

	_exit(0);
}

static int lookup_msg(void)
{
	int fd = open(DB_FILENAME, O_RDWR);
	if(fd < 0) return 1;

	/* skip daemon header */
	db_next_entry(fd);

	db_entry_t entry = {};
	while(db_peek_entry_header(fd, &entry.header))
	{
		if(entry.header.valid && entry.header.receiver_pid == getppid())
		{
			db_mark_entry(fd, 0);
			db_read_entry(fd, &entry);

			fprintf(stderr, "(%d) ==> %s\n", entry.header.sender_pid, entry.msg);

			free(entry.msg);
			break;
		}

		db_next_entry(fd);
	}

	close(fd);
	return 0;
}

static int send_msg(pid_t receiver_pid, pid_t sender_pid, char *msg)
{
	assert(msg);

	int fd = open(DB_FILENAME, O_WRONLY | O_APPEND);
	if(fd < 0) return 1;

	const db_entry_t entry =
	{
		.header =
		{
			.receiver_pid = receiver_pid,
			.sender_pid = sender_pid,
			.valid = 1,
			.time = time(NULL),
			.id = __rdtsc(),
			.msg_len = strlen(msg)
		},
		.msg = msg
	};

	db_write_entry(&entry);
	close(fd);

	fprintf(stderr, "(%d) <== \n", receiver_pid);
	return 0;
}

static int cli_read_pid(pid_t *pid)
{
	assert(pid);

	char *line = NULL, *line_end = NULL;
	int is_read = 0;

	do
	{
		line = readline("pid: ");
		if(line == NULL) return 0;

		*pid = strtol(line, &line_end, 10);
		if(line_end[0] != '\0')
			fprintf(stderr, "pid must be an integer\n");
		else
			is_read = 1;

		add_history(line);
		free(line);
	}
	while(!is_read);

	return 1;
}

static int cli_read_msg(char **msg)
{
	assert(msg);

	*msg = readline("msg: ");
	if(*msg == NULL) return 0;

	return 1;
}

static int cli(pid_t *pid, char **msg, pid_t ch_pid)
{
	assert(pid); assert(msg); assert(ch_pid > 0);

	/* waiting for input (in read mode) */
	int retval = (getchar() != EOF);

	/* Don't spam input msgs while typing */
	kill(ch_pid, SIGSTOP);
	retval = retval && cli_read_pid(pid) && cli_read_msg(msg);
	kill(ch_pid, SIGCONT);

	return retval;
}

