/*
  pngtopic

  Convert a png formatted graphics file into a format that may be 
  inserted into a Microchip PIC .asm file. The main purpose is to
  allow graphics to be easily used with gpsim's graphic LCD driver.

  Compile with:

  gcc -g -o  pngtopic pngtopic.c -lpng

*/

#include <png.h>

#define ERROR 0xffff
//#define DEBUG

/* Globals */

png_bytep *gRowPointers;   /* Dynamically allocated by libpng. .png file is stored here*/
png_uint_32 gWidth;        /* Image width (in pixels) */
png_uint_32 gHeight;       /* Image height (in pixels) */
int gBitDepth;             /* Color depth for each pixel */
int gColorType;            /* 3-bit field describing png color type */


int byte_to_nibble(int byte);

void usage()
{
  printf ("usage:  pngtopic [-i] filename.png\n");
  printf ("  options:\n");
  printf ("     -i  - invert pixels\n");
  printf ("     -p  - preview ASCII image\n");

}

void help()
{
  usage();

  printf("\n  pngtopic converts png files to gpasm include files.\n");
  printf("  These files are compatible with gpsim's graphics LCD driver\n");
  printf("  Copyright - T. Scott Dattalo\n");

}

void error(const char *cPerrmsg)
{
  printf("%s\n",cPerrmsg);
  usage();
  exit(1);
}

/*
  getPixel(unsigned int row, unsigned int col)

  Extract a single pixel from the image.

  
*/
int getPixel(unsigned int row, unsigned int col)
{
  if (row < gHeight && col < gWidth) {

    if (gColorType & PNG_COLOR_MASK_ALPHA) {
      /* This probably is not exactly correct.
       */
      int pixel_byte_index = col * 4;
      int byte = gRowPointers[row][pixel_byte_index];
      return (byte>127);

    } else {
      int pixels_per_byte = 8/gBitDepth;
      if (pixels_per_byte) {
	int pixel_byte_index = col / pixels_per_byte;
	int pixel_bit_index  = col % pixels_per_byte;
	int byte = gRowPointers[row][pixel_byte_index];
	int pixel = byte >> ((pixels_per_byte - pixel_bit_index-1)*gBitDepth);
	int mask = (1<<gBitDepth)-1;
	if (row == 700) {
	  printf("row=%d,col=%d, pixels_per_byte=%d pixel_byte_index=%d pixel_bit_index%d\n",
		 row, col,
		 pixels_per_byte,
		 pixel_byte_index,pixel_bit_index);
	  printf("byte=0x%x pixel=%d mask=0x%x\n ",byte,pixel&mask,mask);
	}
	return ((pixel & mask) > (mask/2)) ? 1 : 0;
      }
    }
  }

  return 0;
}


/*************************************************************************/

int main(int argc, char **argv)
{
  char header[8];
  int number = 8;
  int is_png = 0;

  if (argc < 2) {
    usage();
    return ERROR;
  }

  int bInvert=0;     /* Command line argument for inverting the image */
  int bPreview=0;    /* Command line option for getting an ASCII image */

  int argCounter = 1;
  while (argCounter < (argc-1)) {

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

  fread(header, 1, number, fp);
  is_png = !png_sig_cmp(header, 0, number);
  if (!is_png)
    error("File is not png compatible\n");

  /* Much of what follows comes straight from the png documentation.
   */
  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    return (ERROR);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct(&png_ptr,
			      (png_infopp)NULL, (png_infopp)NULL);
      return (ERROR);
    }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
    {
      png_destroy_read_struct(&png_ptr, &info_ptr,
			      (png_infopp)NULL);
      return (ERROR);
    }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, number);

  png_read_info(png_ptr, info_ptr);
  int interlace_type;
  int compression_type;
  int filter_method;

  png_get_IHDR(png_ptr, info_ptr, &gWidth, &gHeight,
	       &gBitDepth, &gColorType, &interlace_type,
	       &compression_type, &filter_method);

  if (!(gWidth <= 100  && gHeight <= 32)) {
    printf (" Image is too large (%dX%d)\n",gWidth,gHeight);
    return (ERROR);
  }

  /* One way to handle png files is by allocating memory for holding
     the image. This is recommended for small files like the ones 
     we're dealing with.
   */
  gRowPointers = png_malloc(png_ptr,
			    gHeight*png_sizeof(png_bytep));
  int i,j;
  int pixel_size = 1;
  int pixel_depth = (gColorType & PNG_COLOR_MASK_ALPHA) ? 4 : 1;

  for (i=0; i<gHeight; i++)
    gRowPointers[i]=png_malloc(png_ptr,
			       gWidth*pixel_size*pixel_depth);
  png_set_rows(png_ptr, info_ptr, gRowPointers);

  /* Finally, we can read in the image.*/

  png_read_image(png_ptr, gRowPointers);


  /* Now that image is in memory, we can extract the pixels in any
   * convenient order. For ASCII previews, we'll just read the pixels
   * straight off a row at a time. For the PIC file format, a column
   * of 8 pixels are combined into a single byte. 
   */
  int bytes_per_row = gWidth * gBitDepth / 8 * pixel_depth;
  int mask = (1<<gBitDepth) - 1;
  int threshold = mask / 2; //(1<<(gBitDepth-1))-1;


  printf ("; pngtopic - png to gpasm PIC include file\n");
  printf ("; for use with gpsim's graphic's LCD driver\n");
  printf (";    Copyright 2005 - Scott Dattalo\n");
  printf ("; Automatically converted from %s\n", file_name);
  printf (";\n");

#if defined(DEBUG)
  printf (";BitDepth=%d gColorType=%d interlace_type=%d\n",
	  gBitDepth,gColorType,interlace_type);
  printf (";bytes_per_row=%d\n",bytes_per_row);
  printf (";mask=%d threshold=%d\n",mask, threshold);
#endif /* defined (DEBUG)*/




  if (bPreview) {
    printf(";");
    for (i=0; i<gHeight; i++) {
      printf ("\n; ");
#if defined(DEBUG)
      printf ("row:%2d ",i);
      for (j=0; j<gWidth/8*gBitDepth; j++) {
	printf ("%02X",gRowPointers[i][j]);
      }
      printf ("  .");

      for (j=0; j<gWidth; j++) {
	printf("%d",getPixel(i,j));
      }
#endif

      for (j=0; j<gWidth; j++) {
	printf("%s",  (getPixel(i,j) ^ bInvert) ? "  " : "[]");
      }

      printf (".");
    }

    printf("\n;\n");
  }

  /* Convert the image into the PIC format. */
  printf ("        db 0x%02x,0x%02x   ;width in pixels and height in bytes\n", gWidth, gHeight/8);

  /* The 'db' directive is used to define bytes in gpasm/mpasm. Limit to 8 bytes: */
#define DBS_PER_ROW  8

  int dbcount = 0;

  for (i=0; i<gHeight; i+=8) {

    for (j=0; j<gWidth; j++) {
      int db = 0;
      int k;
      for (k=0; k<8; k++) {
	db >>= 1;
	db |= (getPixel(i+k,j) ^ bInvert) ? 0 : 0x80;
      }
      
      if (dbcount == 0)
	printf ("        db 0x%02x",db);
      else
	printf (",0x%02x",db);

      if (++dbcount >= DBS_PER_ROW) {
	printf("\n");
	dbcount = 0;
      }

    }

  }

  return 0;
}

