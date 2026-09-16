#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "db.h"

/*
 * timeouts in seconds
 */
const time_t WATCH_SLEEP_TIME = 1;
const time_t DB_CLEAR_TIMEOUT = 10;
const time_t DB_CLEAR_SLEEP_TIME = 5;

static void print_help(void);
[[noreturn]] static void quit_handler(int sig);
[[noreturn]] static void watch(void);
[[noreturn]] static void run(void);
[[noreturn]] static int stop(void);

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		print_help();
		return 1;
	}

	const char *opt = argv[1];

	if(!strcmp(opt, "run"))
		run();
	else if(!strcmp(opt, "stop"))
		stop();
	else if(!strcmp(opt, "watch"))
		watch();
	else if(!strcmp(opt, "dump"))
	{
		db_dump();
		return 0;
	}
	else if(!strcmp(opt, "help") || !strcmp(opt, "--help"))
	{
		print_help();
		return 0;
	}
	else
	{
		print_help();
		return 1;
	}

	assert(0 && "Unreachable");
}

static void print_help(void)
{
	warnx(
			"Usage:\t <command>\n"
			"command may be one of the following:\n"
			"\thelp\tprint this msg and exit\n"
			"\trun\trun a daemon\n"
			"\tdump\tdump database\n"
			"\tstop\tstop a daemon\n"
			"\twatch\trun dump in loop\n"
	     );
}

[[noreturn]] static void quit_handler([[maybe_unused]] int sig)
{
	assert(sig == SIGQUIT || sig == SIGINT);

	fputc('\n', stderr);
	_exit(0);
}

[[noreturn]] static void watch(void)
{
	signal(SIGINT, quit_handler);
	signal(SIGQUIT, quit_handler);

	fprintf(stderr,
			"[Watch mode activated."
			"Send SIGINT (Ctrl+C) or SIGQUIT to exit]\n"
	       );
	while(1)
	{
		db_dump();
		putchar('\n');
		sleep(WATCH_SLEEP_TIME);
	}
}

[[noreturn]] static void run(void)
{
	/* checking for database not existence and creating it */
	if(access(DB_FILENAME, F_OK) == 0)
		errx(1, "daemon is already running, or database exists\n");
	int fd = open(DB_FILENAME, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	if(fd < 0)
		err(1, "Check your FS permissions: open");

	/* 1st fork (runner exits here) */
	pid_t pid = fork();
	if(pid < 0)
		err(1, "fork");
	else if(pid > 0)
		_exit(0);

	/*
	 * ignoring SIGHUP,
	 * setup SIGQUIT as daemon
	 * exit signal
	 */
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, quit_handler);

	/* creating new session */
	if(setsid() < 0)
		err(1, "setsid");

	/* second fork to avoid terminal capturing */
	pid = fork();
	if(pid < 0)
		err(1, "fork");
	else if(pid > 0)
		_exit(0);

	/* writing daemon entry */
	db_write_entry(&(const db_entry_t)
			{
				.header =
				{
					.time = time(NULL),
					.sender_pid = getpid(),
					.receiver_pid = -1,
					.valid = 1,
					.id = 0,
					.msg_len = 0
				},
				.msg = NULL
			});

	/*
	 * runing main daemon task -
	 * database clearing
	 */
	while(1)
	{
		db_clear(DB_CLEAR_TIMEOUT);
		sleep(DB_CLEAR_SLEEP_TIME);
	}

	assert(0 && "Unreachable");
}

[[noreturn]] static int stop(void)
{
	if(access(DB_FILENAME, F_OK))
		errx(1, "daemon is not running");

	if(kill(db_get_daemon_pid(), SIGQUIT))
		err(1, "kill");

	if(remove(DB_FILENAME))
		err(1, "remove");

	_exit(0);
}

