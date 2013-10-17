/**
 * connection.c - connection to the oscilloscope and buffer for received data
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/usb/tmc.h>

#include "connection.h"

const int max_command_length = 255;
const int max_response_length = 1024 * 1024 + 1024;

void con_open(struct connection *con, const char *device)
{
	con->fd = open(device, O_RDWR);
	con->buffer = malloc(max_response_length + 1);
}

void con_close(struct connection *con)
{
	// unlock scope keypad
	con_send(con, ":KEY:LOCK DISABLE");
	close(con->fd);
	free(con->buffer);
}

int con_send(struct connection *con, const char *command)
{
	int size;
	char buf[max_command_length + 2];
	buf[sizeof(buf) - 1] = 0;
	buf[sizeof(buf) - 2] = 0;
	if(strlen(command) > max_command_length) {
		fprintf(stderr, "Could not send command: string too long");
		return -1;
	}
	strncpy(buf, command, max_command_length);
	strcat(buf, "\n");
	size = write(con->fd, buf, strlen(buf));
	if (size < 0)
		perror("Could not send command");
	return size;
}

int get_data_size(const char *buffer, int size)
{
	int len;
	char head[11];
	if (size < 10)
		return -1;
	if (strncmp(buffer, "#8", 2) != 0)
		return -1;
	strncpy(head, buffer, 10);
	head[10] = 0;
	if (1 != sscanf(head + 2, "%d", &len))
		fprintf(stderr, "Error: received malformed header\n");
	else if (len != size - 10)
		fprintf(stderr, "Error: inconsistent data length in header, %i != %i\n",
				len, size - 10);
	else
		return len;
	return -1;
}

int con_recv(struct connection *con)
{
	int size;
	con->buffer[0] = 0;
	con->data_size = -1;
	size = read(con->fd, con->buffer, max_response_length);
	if ((size > 0) && (size < max_response_length))
		con->buffer[size] = 0;
	if (size < 0)
		perror("Could not receive response");
	else
		con->data_size = get_data_size(con->buffer, size);
	return size;
}

