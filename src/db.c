#include "db_defs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <string.h>

int db_peek_entry_header(int fd, db_entry_header_t *header)
{
	assert(fd > 0); assert(header);

	return pread(fd, header, sizeof(*header), lseek(fd, 0, SEEK_CUR)) > 0;
}

void db_next_entry(int fd)
{
	assert(fd > 0);

	db_entry_header_t header = {};
	db_peek_entry_header(fd, &header);

	lseek(fd, sizeof(header) + header.msg_len, SEEK_CUR);
}

int db_read_entry(int fd, db_entry_t *entry)
{
	assert(fd > 0); assert(entry);

	if(!db_peek_entry_header(fd, &entry->header))
		return 0;

	lseek(fd, sizeof(entry->header), SEEK_CUR);

	entry->msg = (char *)calloc(entry->header.msg_len+1, sizeof(char));
	assert(entry->msg);

	entry->msg[entry->header.msg_len] = '\0';
	return !!read(fd, entry->msg, entry->header.msg_len);
}

void db_clear(void)
{
	int fd = open(DB_FILENAME, O_RDONLY);
	assert(fd > 0);

	const time_t timeout = 30;

	time_t crnt_time = time(NULL);

	db_entry_header_t db_entry_header = {};
	int need_clean = 0;
	while(db_peek_entry_header(fd, &db_entry_header))
	{
		if(crnt_time - db_entry_header.time > timeout)
		{
			need_clean = 1;
			break;
		}

		db_next_entry(fd);
	}

	if(need_clean)
	{
		struct stat f_info = {}; fstat(fd, &f_info);

		int new_fd = open(DB_FILENAME, O_WRONLY | O_TRUNC);
		assert(new_fd > 0);

		sendfile(new_fd, fd, NULL, f_info.st_size - lseek(fd, 0, SEEK_CUR));
		close(new_fd);
	}

	close(fd);
}

void db_flush(void)
{
	truncate(DB_FILENAME, 0);
}

void db_write_entry(const db_entry_t *entry)
{
	assert(entry);

	int fd = open(DB_FILENAME, O_WRONLY | O_APPEND);
	assert(fd > 0);

	write(fd, &entry->header, sizeof(entry->header));
	write(fd, entry->msg, entry->header.msg_len);
}

void db_mark_entry(int fd, int valid)
{
	assert(fd > 0);

	db_entry_header_t header = {};
	if(!db_peek_entry_header(fd, &header))
		return;

	header.valid = valid;
	pwrite(fd, &header, sizeof(header), lseek(fd, 0, SEEK_CUR));
}

void db_dump(void)
{
	int fd = open(DB_FILENAME, O_RDONLY);
	if(fd < 0) return;

	printf("time\t\tok\tid\t\tsndr\trcvr\tmsg_len\tmsg\n");

	db_entry_t entry = {};
	while(db_read_entry(fd, &entry))
	{
		printf("%ld\t%d\t%zu\t%d\t%d\t%zu\t%s\n",
				entry.header.time,
				entry.header.valid,
				entry.header.id,
				entry.header.sender_pid,
				entry.header.receiver_pid,
				entry.header.msg_len,
				entry.msg);
		free(entry.msg);
	}
}

