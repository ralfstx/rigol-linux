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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/usb/tmc.h>
#include <readline/readline.h>

const int max_command_length = 255;
const int max_response_length = 1024 * 1024 + 1024;
const int normal_response_length = 1024;

int dev_send(int handle, const char *command)
{
	int rc;
	char buf[max_command_length + 2];
	buf[sizeof(buf) - 1] = 0;
	buf[sizeof(buf) - 2] = 0;
	if(strlen(command) > max_command_length) {
		fprintf(stderr, "Could not send command: string too long");
		return -1;
	}
	strncpy(buf, command, max_command_length);
	strcat(buf, "\n");
	rc = write(handle, buf, strlen(buf));
	if (rc < 0)
		perror("Could not send command");
	return rc;
}

int dev_recv(int handle, unsigned char *buf, size_t size)
{
	int rc;
	if (!size)
		return -1;
	buf[0] = 0;
	rc = read(handle, buf, size);
	if ((rc > 0) && (rc < size))
		buf[rc] = 0;
	if (rc < 0)
		perror("Could not receive response");
	return rc;
}

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

int main(int argc, char **argv)
{

	char device[256] = "/dev/usbtmc0";
	if (argc > 2 && strncmp(argv[1], "-D", 2) == 0) {
		strcpy(device, argv[2]);
		argc -= 2;
		*argv += 2;
	}

	int handle;
	int rc;
	int i;
	unsigned char buf[max_response_length];
	handle = open(device, O_RDWR);
	if (handle < 0) {
		fprintf(stderr, "Could not open device '%s': %s\n", device,
				strerror(errno));
		exit(1);
	}

	dev_send(handle, "*IDN?");
	dev_recv(handle, buf, normal_response_length);
	printf("%s\n", buf);
	
	//readline
	rl_instream = stdin;
	rl_outstream = stderr;

	while (1) {
		char *cmd = readline("rigol> ");
		if (!cmd) {
			// unlock scope keypad
			dev_send(handle, ":KEY:LOCK DISABLE");
			printf("\n");
			exit(0);
		}
		add_history(cmd);
		dev_send(handle, cmd);
		if (strchr(cmd, '?') == NULL) {
			rc = 0;
		} else {
			rc = dev_recv(handle, buf, max_response_length);
			if (rc <= 0) {
				printf("[%d]:\n", rc);
			} else if (rc < 512) {
				// assume this is just text
				printf("[%d]:%s\n", rc, buf);
			} else {
				printf("[%d]:\n", rc);
				if (strncmp(buf, "#8", 2) == 0) {
					// header + samples
					int len;
					unsigned char z = buf[10];
					buf[10] = 0;
					if (1 != sscanf(buf + 2, "%d", &len))
						printf("Warning - malformed header\n");
					if (rc != 10 + len)
						printf("Warning - inconsistent read() and header length %s\n", buf);
					buf[10] = z;
					int i;
					printf("Print or Save filename? ");
					fflush(stdout);
					char x[64];
					fgets(x, 64, stdin);
					if (x[strlen(x) - 1] == '\n')
						x[strlen(x) - 1] = 0;
					if (tolower(x[0]) == 'p')
						print_data(buf + 10, rc);
					else
						write_to_file(x + 2, buf + 10, len);
				}
			}
		}
	}
}

