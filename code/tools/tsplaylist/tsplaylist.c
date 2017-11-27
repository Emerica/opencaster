#define MULTICAST
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#define TS_PACKET_SIZE 188

char * remove_newline_ch(char *line)
{
    int new_line = strlen(line) -1;
    if (line[new_line] == '\n')
        line[new_line] = '\0';
		return line;
}
int split (const char *str, char c, char ***arr)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
        exit(1);

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
        {
            (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*arr)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
        if (*p != c && *p != '\0')
        {
            *t = *p;
            t++;
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }

    return count;
}


int main(int argc, char *argv[])
{
	FILE * playlist;
  FILE * cfile;
	int byte_read;
	unsigned char choice;
	unsigned char packet[TS_PACKET_SIZE];
	char * line = NULL;
  char * fline = NULL;
  size_t len = 0;
  ssize_t isread;
	fprintf(stderr, "Loop time'\n");
	int pusi = 0;
	int clk;
	int pclk;
	int lpclk;
	struct stat attr;
	int filestart;
	int fileend;
  char **arr = NULL;
  int c;
  int clkdiff;
  int offset = 3;

	if (argc >= 2) {
		playlist = fopen(argv[1], "r");
		stat(argv[1], &attr);
		lpclk = (int)attr.st_mtime;
		if (playlist == NULL) {
			fprintf(stderr, "Can't open file %s\n", argv[1]);
			return 0;
		}
	} else {
		fprintf(stderr, "Usage: 'tsplaylist playlist'\n");
		return 0;
	}


	while ((isread = getline(&line, &len, playlist)) != -1) {
    //trim
	  line = remove_newline_ch(line);
    //Split
    c = split(line, ',', &arr);
    //TODO - CHECK FOR LENGTH

    //Debug
    fprintf(stderr, "found %d tokens.\n", c);
    fprintf(stderr, "start: %d\n",  atoi(arr[0]));
    fprintf(stderr, "end: %s\n",  arr[1]);
    fprintf(stderr, "file: %s\n",  arr[2]);

    //clock
    clk = (int)time(NULL)+offset;
    fprintf(stderr, "timenow: %d\n",  clk);

    //See if this item should be playing
    if( clk >= atoi(arr[0]) && clk < atoi(arr[1]) ){
      clkdiff = clk-atoi(arr[0]);
      fprintf(stderr, "Seek %d seconds in file, skip %d bytes.", clkdiff, (4334000/8 * clkdiff));

      cfile = fopen(arr[2], "r");
  		if (cfile == NULL) {
  			fprintf(stderr, "Can't open file %s", arr[2]);
  			return 0;
  		}
  		byte_read = 1;
      fseek( cfile, (round(4334000/TS_PACKET_SIZE)*TS_PACKET_SIZE * clkdiff)/8, SEEK_SET );
  		while(byte_read) {

        byte_read = fread(packet, TS_PACKET_SIZE, 1, cfile);
  			//byte_read = read(cfile, packet, TS_PACKET_SIZE);
  			if (byte_read > 0) {
  				pusi = (packet[1] & 0x40);
  				if(pusi){
  					stat(argv[1], &attr);
  					pclk = (int)attr.st_mtime;
  					clk = (int)time(NULL);
  					if(pclk > lpclk){
  						fprintf(stderr, "Playlist Changed - Timestamp: %d , \n", pclk);
  						lpclk = pclk;
  					}
  				}
  				write(STDOUT_FILENO, packet, TS_PACKET_SIZE);
  			}
  			choice = 0;
    	}
  		fclose(cfile);
    }
	}
	fclose(playlist);
	return 0;
}
