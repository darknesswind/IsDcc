/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <conio.h>

#include "common.h"
#include "util.h"
#include "ishield.h"
#include "header.h"
#include "decode.h"
#include "output.h"


char version[] = "3.1";


int Scramble(char *filename)
{
	long size;
	unsigned char *buffer;
	unsigned char *xbuffer;
	int XOR_VAL = 0xF1;
	char txt[256];
	int j;
	FILE *f1;
	FILE *f = fopen(filename,"rb");
	fseek(f,0,SEEK_END);
	size = ftell(f);
	fseek(f,0,SEEK_SET);
	buffer = (unsigned char *)malloc(size);
	xbuffer = (unsigned char *)malloc(size);
	fread(buffer,size,1,f);
	fclose(f);

	for(j=0;j<size;j++)
	{
		int c = buffer[j];
		c += (j % 71);
		c = (( c << 2)&0xFF) | ((c>>6)&0x3);
		c ^= XOR_VAL;			
		xbuffer[j] = c;
	}
	sprintf(txt,"%s.inx",filename);

	f1 = fopen(txt,"wb");
	fwrite(xbuffer,size,1,f1);
	fclose(f1);
	free(xbuffer);
	free(buffer);
	return 0;
}

int unScramble(char *filename)
{
	unsigned char *buffer;
	unsigned char *xbuffer;
	long size;
	int XOR_VAL = 0xF1;
	int b,j;
	char txt[256];
	FILE *f1;
	FILE *f = fopen(filename,"rb");

	fseek(f,0,SEEK_END);
	size = ftell(f);
	fseek(f,0,SEEK_SET);
	buffer = (unsigned char *)malloc(size);
	xbuffer = (unsigned char *)malloc(size);
	fread(buffer,size,1,f);
	fclose(f);
	for(j=0;j<size;j++)
	{
		int c = buffer[j];
		c ^= XOR_VAL;
		
		b = (unsigned char)((c >> 2) | (c << 6)) - (j % 71);
		xbuffer[j] = b;
	}
	sprintf(txt,"%s.dec",filename);

	f1 = fopen(txt,"wb");
	fwrite(xbuffer,size,1,f1);
	fclose(f1);
	return 0;
}

int main(int argc, char* argv[])
{
  int fd;
  ISData isData;
 

  fprintf(stderr, "isDcc v%s, (c) 1998 Andrew de Quincey\n", version);
  fprintf(stderr, "isDcc v2.00, (c) 2000 Mr. Smith\n"); //version2
  fprintf(stderr, "isDcc v2.10, (c) 2001 Mr. Smith\n"); //version2.1
  fprintf(stderr, "isDcc v3.10, (c) 2001 Mr Won't tell:) \n"); //version3.1

  fprintf(stderr, "Syntax : isDcc -u filename.inx //To Unscramble the script file\n"); //version3.1
  fprintf(stderr, "Syntax : isDcc -s filename.inx.dec //To Scramble the script file\n"); //version3.1
  fprintf(stderr, "Syntax : isDcc filename.inx.dec //To desassemble the script file\n"); //version3.1

  // --------------------------------------------------------------------------
  // check params
  // modified for version2
  if ((argc > 3 ) || ( argc < 2))
    exit(1); 

  if ( strcmp(argv[1],"-u") == 0 )
  {
	  unScramble(argv[2]);
	  exit(0);
  }

  if ( strcmp(argv[1],"-s") == 0 )
  {
	  Scramble(argv[2]);
	  exit(0);
  }

  argnum = argc;


  // --------------------------------------------------------------------------
  // open file
  if ((fd = open(argv[argc-1], O_RDONLY | O_BINARY)) == -1)
    error("Cannot open file %s for reading\n", argv[argc-1]); //version2


  // --------------------------------------------------------------------------
  // initialise the opTable stuff
  initDecode();


  // --------------------------------------------------------------------------
  // parse the header
  parseHeader(fd, &isData);

  // decode the code
  decode(fd, &isData);

  // do any extra conversion on the code
  //optimise(&isData);

  // output it
  output(fd, &isData, 1);

  close(fd);
}

