#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
  FILE* filename;
  FILE* outfile;
  char tmpChar;
  char tmpStr[256];
  float angle = 0.0;
  float time = 0.0;


  if (argc != 2) {
    fprintf(stderr,"usage: parsedata filename\n");
    exit(1);
  }

  filename = fopen(argv[1],"r");
  if (filename == NULL) {
    fprintf(stderr,"\n%s could not be opened!\n", argv[1]);
    exit(1);
  }

  outfile = fopen("lab5_formatted","w");

  while ( fscanf(filename,"%c",&tmpChar) != EOF )
  {
    fscanf(filename,"%c",&tmpChar);
    fscanf(filename,"%f",&angle);
    fgets(tmpStr, 225, filename);
    fprintf(outfile, "%.2f  %.2f\n", time, angle);
    time = time + 0.100000;
  }


  fclose(outfile);
  fclose(filename);
  return 0;
}
