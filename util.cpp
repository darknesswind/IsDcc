/* 
   isDcc
   (c) 1998 Andrew de Quincey
   adq@tardis.ed.ac.uk
   See README.TXT for copying/distribution/modification details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <stdarg.h>
#include <string.h>
#include "util.h"


void printIndent(int value)
{
  int i;
  for(i=0; i < (value*4); i++)
    printf(" ");
}

char* filterLF(char* buffer)
{
  int i=0;

  while(buffer[i] != 0)
  {
    if ((buffer[i] == 0x0a) || (buffer[i] == 0x0d))
      buffer[i] = ' ';
    i++;
  }

  return(buffer);
}


uint get4Byte(int file)
{
  uint tmp;

  if (_read(file, &tmp, 4) != 4)
    error("Read past EOF!\n");
  return(tmp);
}

ushort get2Byte(int file)
{
  ushort tmp;

  if (_read(file, &tmp, 2) != 2)
    error("Read past EOF!\n");
  return(tmp);
}
  
uchar get1Byte(int file)
{
  uchar tmp;

  if (_read(file, &tmp, 1) != 1)
    error("Read past EOF!\n");
  return(tmp);
}
  
char* getString(int file, char* buffer, int length)
{
  getLength(file, buffer, length);
  buffer[length] =0;

  return(buffer);
}


  
char* getLength(int file, char* buffer, int length)
{
  if (_read(file, buffer, length) != length)
    error("Read past EOF!\n");
  return(buffer);
}



void error(char* fmt, ...)
{
  va_list ap;
  
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  assert(false);
  exit(1);
}



void warning(char* fmt, ...)
{
  va_list ap;
  
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}


char* escapeString(char* buffer)
{
  char tmpBuf[4096];
  int srcPos;
  int destPos;
  int c;

  srcPos = 0;
  destPos = 0;
  while((c = buffer[srcPos++]) != 0)
    switch(c)
    {
    case '\n':
      tmpBuf[destPos++] = '\\';
      tmpBuf[destPos++] = 'n';
      break;

    case '\r': // ignore
      tmpBuf[destPos++] = '\\';
      tmpBuf[destPos++] = 'r';
      break;

    case '\'':
      tmpBuf[destPos++] = '\\';
      tmpBuf[destPos++] = '\'';
      break;

    case '\"':
      tmpBuf[destPos++] = '\\';
      tmpBuf[destPos++] = '\"';
      break;

	case '\\':
      tmpBuf[destPos++] = '\\';
      tmpBuf[destPos++] = '\\';
      break;

    default:
      if ((c < 32) || (c > 126))
      {
	sprintf(tmpBuf+destPos, "\\x%02x", c);
	destPos+=4;
      }
      else
	tmpBuf[destPos++] = c;
    }

  tmpBuf[destPos] = 0;

  strcpy(buffer, tmpBuf);
  return(buffer);
}

void* mallocX(long size)
{
  void* buf;

  buf = malloc(size);

  if (buf == NULL)
    error("Memory allocation error!\n");

  memset(buf,0,size);

  return(buf);
}

int readX(int fd, char* buffer, int len)
{
  if (_read(fd, buffer, len) != len)
    error("Read past EOF!\n");

  return(len);
}

long tell(int fd)
{
  return(lseek(fd, 0, 1));
}
