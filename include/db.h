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

/*
 * Peeks entry header.
 * Return:
 * 0 - EOF reached
 * 1 - header read
 */
int db_peek_entry_header(int fd, db_entry_header_t *header);

/*
 * Goes to the next entry.
 * Return:
 * 0 - EOF reached
 * 1 - next entry choosen
 */
int db_next_entry(int fd);

/*
 * Reads current entry.
 * Return:
 * 0 - EOF reached
 * 1 - entry read
 */
int db_read_entry(int fd, db_entry_t *entry);

/*
 * Clears database entries that are older
 * than `timeout`.
 */
void db_clear(time_t timeout);

/*
 * Flushes database (clears all entries).
 */
void db_flush(void);

/*
 * Writes entry to the database.
 * Return:
 * 0 - error
 * 1 - entry written
 */
int db_write_entry(const db_entry_t *entry);

/*
 * Sets `valid` field of `db_entry_header_t` structure
 * to 'valid' for current entry.
 * Return:
 * 0 - error
 * 1 - entry marked
 */
int db_mark_entry(int fd, int valid);

/*
 * Prints all entries in database.
 */
void db_dump(void);

/*
 * Gets daemon pid.
 * Return:
 * > 0 - daemon pid
 * < 0 - error
 */
pid_t db_get_daemon_pid(void);

