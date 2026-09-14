// TODO: remove useless includes
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include "db_defs.h"

// TODO: watch mode
static void print_help(const char *progname)
{
	assert(progname);
	fprintf(stderr,
			"Usage:\t%s <command>\n"
			"command may be one of the following:\n"
			"\thelp\tprint this msg and exit\n"
			"\trun\trun a daemon\n"
			"\tdump\tdump database\n"
			"\tstop\tstop a daemon\n",
			progname);
}

// TODO: db_clear
static int run(void)
{
	if(access(DB_FILENAME, F_OK) == 0)
	{
		fprintf(stderr, "daemon is already running\n");
		return 1;
	}

	int fd = open(DB_FILENAME, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	if(fd < 0)
	{
		warn("Database file couldn't be created");
		return 1;
	}

	close(fd);
	return 0;
}

static void stop(void)
{
	remove(DB_FILENAME);
}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		print_help(argv[0]);
		return EXIT_FAILURE;
	}

	int retval = 0;

	if(!strcmp(argv[1], "help") || !strcmp(argv[1], "--help"))
		print_help(argv[0]);
	else if(!strcmp(argv[1], "run"))
		retval = run();
	else if(!strcmp(argv[1], "dump"))
		db_dump();
	else if(!strcmp(argv[1], "stop"))
		stop();
	else
	{
		retval = 1;
		print_help(argv[0]);
	}

	return retval;
}

