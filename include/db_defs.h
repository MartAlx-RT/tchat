#pragma once

#include <time.h>
#include <stddef.h>
#include <stdint.h>

typedef struct db_entry_header_t
{
	time_t time;
	int valid;
	size_t id;
	pid_t sender_pid, receiver_pid;
	size_t msg_len;
} db_entry_header_t;

typedef struct db_entry_t
{
	db_entry_header_t header;
	char *msg;
} db_entry_t;

static const char *DB_FILENAME = "/tmp/terminals_db";

// TODO: comments
int db_peek_entry_header(int fd, db_entry_header_t *header);
void db_next_entry(int fd);
int db_read_entry(int fd, db_entry_t *entry);
void db_clear(void);
void db_flush(void);
void db_write_entry(const db_entry_t *entry);
void db_mark_entry(int fd, int valid);
void db_dump(void);

