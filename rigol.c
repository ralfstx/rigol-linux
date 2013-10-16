// rigol.c - a simple terminal program to control Rigol DS1000 series scopes (might work with higher ones too) 
// Copyright 2010 by Mark Whitis, 2012 by Jiri Pittner <jiri@pittnerovi.com>
// Improvements by Jiri Pittner:
// readline with history, avoiding timeouts, unlock scope at CTRL-D, read large sample memory, specify device at the command line
// !!! needs a patched usbtmc.c kernel driver for transmission of more than 1024 samples, otherwise the scope's firmware crashes 
// compile and link as gcc -O rigol.c -lreadline

/*
 *      This program is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 * 
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      You should have received a copy of the GNU General Public License
 *      along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <linux/usb/tmc.h>
#include <readline/readline.h>

/*
 example
*RST
:AUTO
:TRIG:MODE EDGE
:TRIG EDGE:SWEEP SING
:ACQ:MEMD LONG
:CHAN1:MEMD?
:TIM:SCAL 0.0001
:ACQ:SAMP?
:CHAN1:SCAL 1.
:RUN
:STOP
:WAV:POINTS:MODE RAW
:WAV:DATA? CHAN1
 */


int rigol_write(int handle, unsigned char *string)
{
int rc;
unsigned char buf[256];
strncpy(buf,string,sizeof(buf));
buf[sizeof(buf)-1]=0;
strncat(buf,"\n",sizeof(buf));
buf[sizeof(buf)-1]=0;
//printf("rigol_write(): \"%s\"\n",buf);
rc=write(handle, buf, strlen(buf));
if(rc<0) perror("write error");
return(rc);
}

int rigol_read(int handle, unsigned char *buf, size_t size)
{
int rc;
if(!size) return(-1);
buf[0]=0;
rc=read(handle, buf, size);
if((rc>0) && (rc<size)) buf[rc]=0;
if(rc<0) {
if(errno==ETIMEDOUT) {
printf("No response\n");
} else {
perror("read error");
}
}
// printf("rc=%d\n",rc);
return(rc);
}

const int max_response_length=1024*1024+1024;
const int normal_response_length = 1024;

int main(int argc, char **argv)
{
char device[256]="/dev/usbtmc0";
if(argc > 2 && strncmp(argv[1],"-D",2)==0)
	{
	strcpy(device,argv[2]);
	argc -=2;
	*argv +=2;
	}

int handle;
int rc;
int i;
unsigned char buf[max_response_length];
handle=open(device, O_RDWR);
if(handle<0) {
perror("error opening device");
exit(1);
}

rigol_write(handle, "*IDN?");
rigol_read(handle, buf, normal_response_length);
printf("%s\n", buf);

//readline
rl_instream=stdin;
rl_outstream=stderr;


while(1) 
{
char *cmd = readline("rigol> ");
if(!cmd) //unlock scope keypad
	{
	rigol_write(handle, ":KEY:LOCK DISABLE");
	printf("\n");
	exit(0);
	}
add_history(cmd);

rigol_write(handle, cmd);
if(strchr(cmd,'?')==NULL) rc=0; 
else 
	{
	rc=rigol_read(handle, buf,max_response_length);
	if(rc<=0) printf("[%d]:\n",rc);
	else if(rc<512) //assume this is just text
		printf("[%d]:%s\n", rc,buf);
	else
		{
		printf("[%d]:\n",rc);
		if(strncmp(buf,"#8",2)==0) //header+samples
			{
			int len;
			unsigned char z=buf[10];
			buf[10]=0;
 		        if(1!=sscanf(buf+2,"%d",&len)) printf("Warning - malformed header\n");
			if(rc!=10+len) printf("Warning - inconsistent read() and header length %s\n",buf);
			buf[10]=z;
			int i;
			printf("Print or Save filename? "); fflush(stdout);
			char x[64];
			fgets(x,64,stdin);
			if(x[strlen(x)-1]=='\n') x[strlen(x)-1]=0;
			
        		if(tolower(x[0])=='p') 
			   for(i=10;i<rc;i++)
                		{
                		printf("%02X",buf[i]);
                		if( (i%32)==31 ) printf("\n");
                		}
			else
				{
				int fd=open(x+2,O_WRONLY|O_CREAT,0777);
				if(fd<0) perror("cannot open ");
				if(len!=write(fd,buf+10,len)) perror("cannot write");
				if(close(fd)) perror("cannot close");;
				}
			}
		}
	} 
//printf("\n");
}
}
