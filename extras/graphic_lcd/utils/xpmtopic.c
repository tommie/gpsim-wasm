/*

  xpmtopic

  Convert an XPM formatted file into a Microchip PIC .asm file. 
  The main purpose is to allow graphics to be easily used with 
  gpsim's graphic LCD driver.

  Compile with:

  gcc -g -o  xpmtopic xpmtopic.c


*/

#include <stdio.h>
#include <stdlib.h>


/* PIC */
int nNibbles = 0;
int nBytes = 0;
int nBits  = 0;
int curByte =0;

/*
  Parsing;
 */

int xpmState = 0;
int xpmColor = 0;
int xpmRow   = 0;
int xpmColorDepth = 4; // in bits.


//------------------------------------------------------------------------
// XpmStruct
//
// As the XPM is parsed, the information is stored in the XpmStruct
//
// Pixel data: The XPM is a rectangular image. This program will extract
// the image and store it in an array. The sequence of pixels begins at
// the upper left hand corner of the image, and advances across one row
// at a time.
//

typedef struct 
{
  int width;
  int height;
  int nColors;
  int charsPerPixel;
  int *indices;
  int *colors;
  int *data;
} XpmStruct;

XpmStruct xpm;

//------------------------------------------------------------------------
// Encode formats.
//  The XPM data is converted into a PIC include file. The encoding formats
// supported are illustrated below. Each pair of numbers separated by a comma
// is the byte and bit for the output. The position where these pair of
// numbers is located describes the XPM bit. In other words, the XPM image is
// a grid and at each grid point is a pair of numbers describing the output
// byte and bit that holds the grid point information.
//
// Rectilinear:
//
//    0,0      1,0  ...  W-1,0
//    0,1      1,1  ...  W-1,1
//    0,2      1,2  ...  W-1,2
//    0,3      1,3  ...  W-1,3
//    0,4      1,4  ...  W-1,4
//    0,5      1,5  ...  W-1,5
//    0,6      1,6  ...  W-1,6
//    0,7      1,7  ...  W-1,7
//
//    W+0,0  W+1,0  ...  2W-1,0
//    W+0,1  W+1,1  ...  2W-1,1
//    W+0,2  W+1,2  ...  2W-1,2
//    W+0,3  W+1,3  ...  2W-1,3
//    W+0,4  W+1,4  ...  2W-1,4
//    W+0,5  W+1,5  ...  2W-1,5
//    W+0,6  W+1,6  ...  2W-1,6
//    W+0,7  W+1,7  ...  2W-1,7


// SSD0323Direct:
//
//    0,0  0,1
//    0,2  0,3
//    0,4  0,5
//    0,6  0,7
//    1,0  1,1
//    1,2  1,3
//    1,4  1,5
//    1,6  1,7
//

enum {
  eEncodeRectilinear,
  eEncodeSSD0323Direct,
};

int gEncoding = eEncodeSSD0323Direct;

void xpm_init(XpmStruct *pXpm)
{
  if (!pXpm)
    return;

  pXpm->width   = 0;
  pXpm->height  = 0;
  pXpm->nColors = 0;
  pXpm->charsPerPixel = 0;
  pXpm->indices = NULL;
  pXpm->colors  = NULL;
  pXpm->data    = NULL;

  xpmState = 0;

}

int XpmFindColor(XpmStruct *pXpm, int colorIndex)
{
  int i;

  if (!pXpm)
    return;

  for (i=0; i<pXpm->nColors; i++) {
    if (pXpm->indices[i] == colorIndex)
      return pXpm->colors[i];
  }
  fprintf(stderr,"Warning: could not find color: %X\n", colorIndex);
  return -1;
}

void XpmParseString(XpmStruct *pXpm, const char * pstr)
{
  int i;

  if (!pstr || !pXpm)
    return;

  printf("xpmState=%d:%s\n",xpmState, pstr);
  switch (xpmState) {
  case 0:   /* Uninitialized */

    if (sscanf(pstr, "%d %d %d %d", &pXpm->width, &pXpm->height, &pXpm->nColors, &pXpm->charsPerPixel) == 4) {
      xpmState = 1;
      xpmColor = 0;
      xpmRow   = 0;
      pXpm->indices = (int *) malloc(sizeof(int) * pXpm->nColors);
      pXpm->colors  = (int *) malloc(sizeof(int) * pXpm->nColors);
      pXpm->data    = (int *) malloc(sizeof(int) * pXpm->width * pXpm->height);
    } else {
      fprintf(stderr,"ERROR: couldn't parse xpm configuration line: \n", pstr);
    }
    break;

  case 1:
    pXpm->colors[xpmColor] = 0;
    for (i=0; i<pXpm->charsPerPixel; i++)
      pXpm->indices[xpmColor] |=  (((int)(*pstr++))&0xff) << (i*8);

    while (*pstr && *pstr != '#')
      pstr++;

    if (sscanf(pstr,"#%X",&pXpm->colors[xpmColor]) == 1)
      xpmColor++;
    else 
      fprintf(stderr,"ERROR: couldn't parse color: \n", pstr);

    if (xpmColor >= pXpm->nColors)
      xpmState = 2;

    break;

  case 2:
    {
      char c;
      int colorIndex=0;
      int xpmCol = pXpm->width * xpmRow;
      
      i = 0;

      while ((c=*pstr++)!=0) {
        colorIndex |= (((int)c)&0xff) << (8*i);
        i++;
        if (i >= pXpm->charsPerPixel) {
          int color = XpmFindColor(pXpm, colorIndex);
          pXpm->data[xpmCol++] = color;

          if (xpmColorDepth == 4) 
            printf("%X",(color>>4)&0x0f);
          else
            printf("%c%c", ((color&0x80) ? '[' : ' '),((color&0x80) ? ']' : ' '));
          colorIndex = 0;
          i=0;

        }
      }
      xpmRow++;
      printf("\n");
    }

    break;
  }
}

void XpmParse(XpmStruct *pXpm, char **ppXpm)
{
  int i;
  if (!pXpm || !ppXpm)
    return;

  xpm_init(pXpm);

  while (xpmState != 2) 
    XpmParseString(pXpm, *ppXpm++);
  for (i=0; i<pXpm->height; i++)
    XpmParseString(pXpm, *ppXpm++);

}

void XpmParseFile(XpmStruct *pXpm, FILE *pf)
{
  enum { 
    eNothing,
    eOpenBrace,
    eComment,
    eQuote
  };

  int prevState = eNothing;
  int parseState = eNothing;

  int ch;
  int prevCh = 0;
  char buffer[1024];
  int buffPtr = 0;

  if (!pXpm || !pf)
    return;

  xpm_init(pXpm);

  while ((ch = fgetc(pf)) != EOF) {
      
    if (parseState == eQuote) {
      if (ch == '"') {
        buffer[buffPtr] = 0;
        if (buffPtr) {
          printf("buffPtr=%d\n",buffPtr);
          buffPtr = 0;
          XpmParseString(pXpm, buffer);
          parseState = eOpenBrace;
        }
      } else
        if (buffPtr < sizeof(buffer))
          buffer[buffPtr++] = ch;
    } else {

      switch (ch) {

      case '*':
        if (prevCh=='/') { 
          prevState = parseState;
          parseState = eComment;
        }
        break;
      case '/':
        if (prevCh == '*' && parseState == eComment)
          parseState = prevState;
        break;
      case '{':
        if (parseState == eNothing)
          parseState = eOpenBrace;
        break;
      case '}':
        if (parseState == eOpenBrace)
          return;
        break;
      case '"':
        if (parseState == eOpenBrace) 
          parseState = eQuote;
        break;
      default:
        break;
      }
    }
  }

}

int XpmValid(XpmStruct *pXpm)
{
  return ! (!pXpm || !pXpm->data || !pXpm->width || !pXpm->height);
}

void PICFinishNibble(void);
void PICPutByte(int byte)
{
  printf( (nBytes ? ",0x%02X" : "    db 0x%02X"), byte&0xff);
  if (++nBytes >= 16)
    PICFinishNibble();
}

void PICPutNibble(int nibble)
{
  //curByte = (curByte <<4) | (nibble & 0x0f);
  curByte = (curByte >>4) | ((nibble & 0x0f)<<4);
  if (++nNibbles >= 2) {
    nNibbles = 0;
    int invert = 0xff;
    PICPutByte(curByte^invert);
    curByte=0;
  }

}

void PICFinishNibble(void)
{
  if (nNibbles) {
    PICPutNibble(0);
    printf(" ; Warning... not a byte boundary");
  }
  nBytes=0;
  printf("\n");
  
}


void PICPutBit(int bit)
{
  curByte = (curByte << 1) | ((bit&1) ? 1 : 0);
  if (++nBits >= 8) {
    nBits = 0;
    int invert = 0xff;
    PICPutByte(curByte^invert);
    curByte=0;
  }

}

void PICFinishBit(void)
{
  if (nBits && nBits<8) {
    PICPutByte(curByte << (8-nBits));
    printf(" ; Warning... not a byte boundary");
  }
  nBits = 0;
  nBytes=0;
  printf("\n");
  
}

void PICPutColor(int color)
{
  if (xpmColorDepth == 4) 
    PICPutNibble(color>>4);
  else
    PICPutBit(color>>8);

}

void PICFinishColor()
{
  if (xpmColorDepth == 4) 
    PICFinishNibble();
  else
    PICFinishBit();
}


void XpmToPIC(XpmStruct *pXpm, int nChars)
{
  int i,j,k;
  int xpmCol=0;
  int xpmSize;
  int xpmIndex=0;
  int charWidth;

  if (!XpmValid(pXpm)) {
    fprintf(stderr,"ERROR: xpm is invalid\n");
    return;
  }

  xpmSize = pXpm->width * pXpm->height;

  // Generate an ASCII image:
  printf("\n");
  for (i=0; i<pXpm->height; i++) {
    printf("; ");
    for (j=0; j<pXpm->width; j++) {
      int color = (pXpm->data[xpmIndex++]>>4) & 0x0f;
      if (color == 0x0F) 
        printf(" ");
      else if(xpmColorDepth == 4)
        printf("%X", color);
      else
        printf("%c%c", ((color<8) ? '[' : ' '),((color<8) ? ']' : ' '));

    }
    printf("\n");
  }

  // Generate a PIC include file

  xpmIndex = 0;

  printf("    db 0x%02x,0x%02x     ; Width  and height\n",pXpm->width, pXpm->height);
  nChars = nChars ? nChars : 1;
  charWidth = pXpm->width/nChars;
  if (charWidth * nChars != pXpm->width) 
    printf("XPM width can not be evenly subdivided into %d xpms\n", nChars);

  for (k=0; k<nChars; k++) {

    printf("; char %d\n", k);

    switch (gEncoding) {

    case eEncodeRectilinear:
      for (i=0; i<pXpm->height; i++) {
        xpmIndex = i * pXpm->width + k*charWidth;
        for (j=0; j<charWidth; j++)
          PICPutColor(pXpm->data[xpmIndex++]);

        PICFinishColor();
      }
      break;

    case eEncodeSSD0323Direct:
      for (j=0; j<charWidth; j+=2) {
        for (i=0; i<pXpm->height; i++) {
          xpmIndex = i * pXpm->width + k*charWidth + j;

          PICPutColor(pXpm->data[xpmIndex++]);
          PICPutColor(pXpm->data[xpmIndex++]);
        }
      }
      PICFinishColor();

      break;

    }
  }

  printf("\n");
}

#define ERROR 0xffff
void usage(void)
{
  printf("xpmtopic [-s split] [-i] [-p] [-d depth] file.xpm \n");
  printf("  file.xpm -- name of the xpm file to process\n");
  printf("  split -- number of characters\n");
  printf("  -i -- invert\n");
  printf("  -p -- preview\n");
  printf("  depth -- 4=>4-bit depth, 1=>1-bit depth\n");
}

void help(void)
{
  usage();
  printf("\nA program for converting XPM's into Microchip PIC include files\n\n");
  exit(0);
}

void error(const char *cPerrmsg)
{
  printf("%s\n",cPerrmsg);
  usage();
  exit(1);
}

int main(int argc, char **argv)
{

  int split = 1;

  if (argc < 2) {
    usage();
    return ERROR;
  }

  int bInvert=0;     /* Command line argument for inverting the image */
  int bPreview=0;    /* Command line option for getting an ASCII image */

  int argCounter = 1;
  while (argCounter < argc) {

    if (*argv[argCounter] == '-') {
      switch (*(argv[argCounter]+1)) {
      case 'i':
	bInvert = 1;
	break;
      case 'h':
	help();
	return 0;
      case 'p':
	bPreview = 1;
	break;
      case 's':
        argCounter++;
        split = (sscanf(argv[argCounter], "%d", &split) == 1) ? split : 1;
	break;
      case 'd':
        argCounter++;
        xpmColorDepth = (sscanf(argv[argCounter], "%d", &xpmColorDepth) == 1) ? xpmColorDepth : 4;
	break;
      }

      argCounter++;

    } else 

      break;
  }

  /* Open the file containing the image */
  /* A few preliminary checks are made to ensure the file is valid. */

  const char *file_name = argv[argc-1];
  FILE *fp = fopen(file_name, "rb");
  
  if (!fp)
    error ("File not found");


  /*

  if (argc > 2) 
    split = (sscanf(argv[2], "%d", &split) == 1) ? split : 1;

  FILE *fp = fopen(argv[1],"r");
  */

  XpmParseFile(&xpm,fp);

  XpmToPIC(&xpm,split);

  return 0;
}
