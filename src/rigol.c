/**
 * rigol.c - a simple terminal program to control Rigol DS1000 series scopes
 *
 * Copyright (C) 2010 Mark Whitis
 * Copyright (C) 2012 Jiri Pittner <jiri@pittnerovi.com>
 * Copyright (C) 2013 Ralf Sternberg <ralfstx@gmail.com>
 *
 * Improvements by Jiri Pittner:
 *   readline with history, avoiding timeouts, unlock scope at CTRL-D,
 *   read large sample memory, specify device at the command line
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see [http://www.gnu.org/licenses/].
 */


#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "connection.h"

void print_data(const char *buf, size_t count)
{
	int i;
	for (i = 0; i < count; i++) {
		if (i % 16 == 0)
			printf("%08x  ", i);
		printf("%02x ", buf[i]);
		if (i % 16 == 7)
			printf(" ");
		if (i % 16 == 15)
			printf("\n");
	}
	if (i % 16 != 0)
		printf("\n");
}

void write_to_file(const char *filename, const char *buf, size_t count)
{
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "Could not open file '%s': %s\n", filename,
				strerror(errno));
		return;
	}
	if (count != write(fd, buf, count))
		fprintf(stderr, "Could not write to file '%s': %s\n", filename,
				strerror(errno));
	if (close(fd))
		fprintf(stderr, "Could not close file '%s': %s\n", filename,
				strerror(errno));
}

void cmd_print(const struct connection *con)
{
	if (con->data_size > 0)
		print_data(con->buffer + 10, con->data_size);
}

void cmd_save(const struct connection *con, const char *filename)
{
	if (con->data_size > 0)
		write_to_file(filename, con->buffer + 10, con->data_size);
}

void _cmd_receive_response(struct connection *con)
{
	int size = con_recv(con);
	if (size <= 0)
		return;
	if (con->data_size >= 0)
		printf("%i bytes of data received\n", con->data_size);
	else
		printf("%s\n", con->buffer);
}

void cmd_send_direct(struct connection *con, const char *cmd)
{
	int size = con_send(con, cmd);
	if (size <= 0)
		return;
	if (strchr(cmd, '?') != NULL) {
		_cmd_receive_response(con);
	}
}

void enter_console(struct connection *con)
{
	// readline
	rl_instream = stdin;
	rl_outstream = stderr;

	cmd_send_direct(con, "*IDN?");
	while (1) {
		char *cmd = readline("rigol> ");
		if (!cmd)
			break;
		add_history(cmd);
		if (strncmp(cmd, "print", 5) == 0) {
			cmd_print(con);
		} else if (strncmp(cmd, "save ", 4) == 0) {
			cmd_save(con, cmd + 5);
		} else {
			cmd_send_direct(con, cmd);
		}
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	char device[256] = "/dev/usbtmc0";
	if (argc > 2 && strncmp(argv[1], "-D", 2) == 0) {
		strcpy(device, argv[2]);
		argc -= 2;
		*argv += 2;
	}

	struct connection con;
	con_open(&con, device);
	if (con.fd < 0) {
		fprintf(stderr, "Could not open device '%s': %s\n", device,
				strerror(errno));
		exit(1);
	}

	enter_console(&con);
	con_close(&con);
	exit(EXIT_SUCCESS);
}

