/*******************************************************************************
** GRADIENT (gradient file generation) - Michael Behrens 2019-2023
*******************************************************************************/

/*******************************************************************************
** main.c
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI      3.14159265358979323846f
#define TWO_PI  6.28318530717958647693f

typedef struct color
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} color;

enum
{
  SOURCE_APPROX_NES = 0,
  SOURCE_APPROX_NES_ROTATED,
  SOURCE_COMPOSITE_04_1X,
  SOURCE_COMPOSITE_04_1X_ROTATED,
  SOURCE_COMPOSITE_08_2X,
  SOURCE_COMPOSITE_08_2p50X,
  SOURCE_COMPOSITE_16_1X,
  SOURCE_COMPOSITE_16_1X_ROTATED,
  SOURCE_COMPOSITE_16_3X,
  SOURCE_COMPOSITE_16_3X_ROTATED,
  SOURCE_COMPOSITE_32_2X,
  SOURCE_COMPOSITE_32_2p50X
};

/* the table step is 1 / (n + 2), where */
/* n is the number of colors per hue    */
#define COMPOSITE_04_TABLE_STEP 0.166666666666667f  /* 1/6  */
#define COMPOSITE_08_TABLE_STEP 0.1f                /* 1/10 */
#define COMPOSITE_16_TABLE_STEP 0.055555555555556f  /* 1/18 */
#define COMPOSITE_32_TABLE_STEP 0.029411764705882f  /* 1/34 */

/* the luma is the average of the low and high voltages */
/* for the 1st half of each table, the low value is 0   */
/* for the 2nd half of each table, the high value is 1  */
/* the saturation is half of the peak-to-peak voltage   */

/* for the nes tables, the numbers were obtained    */
/* from information on the nesdev wiki              */
/* (see the "NTSC video" and "PPU palettes" pages)  */
float S_nes_p_p[4] = {0.399f,   0.684f, 0.692f, 0.285f};
float S_nes_lum[4] = {0.1995f,  0.342f, 0.654f, 0.8575f};
float S_nes_sat[4] = {0.1995f,  0.342f, 0.346f, 0.1425f};

/* note that if we used a "composite 04" table, with  */
/* the table step being 1/(4 + 2) = 1/6, we would end */
/* up with another approximation of these values!     */
float S_approx_nes_p_p[4] = {0.4f, 0.7f,  0.7f,   0.3f};
float S_approx_nes_lum[4] = {0.2f, 0.35f, 0.65f,  0.85f};
float S_approx_nes_sat[4] = {0.2f, 0.35f, 0.35f,  0.15f};

float S_composite_04_lum[4];
float S_composite_04_sat[4];

float S_composite_08_lum[8];
float S_composite_08_sat[8];

float S_composite_16_lum[16];
float S_composite_16_sat[16];

float S_composite_32_lum[32];
float S_composite_32_sat[32];

#define MAX_SHADES    64
#define MAX_GRADIENTS 3

color   G_shades_array[MAX_SHADES];
int     G_num_shades;

int     G_source;

float*  S_luma_table;
float*  S_saturation_table;
int     S_table_length;

/*******************************************************************************
** generate_voltage_tables()
*******************************************************************************/
short int generate_voltage_tables()
{
  int k;

  /* composite 04 tables */
  for (k = 0; k < 2; k++)
  {
    S_composite_04_lum[k] = (k + 1) * COMPOSITE_04_TABLE_STEP;
    S_composite_04_lum[3 - k] = 1.0f - S_composite_04_lum[k];

    S_composite_04_sat[k] = S_composite_04_lum[k];
    S_composite_04_sat[3 - k] = S_composite_04_sat[k];
  }

  /* composite 08 tables */
  for (k = 0; k < 4; k++)
  {
    S_composite_08_lum[k] = (k + 1) * COMPOSITE_08_TABLE_STEP;
    S_composite_08_lum[7 - k] = 1.0f - S_composite_08_lum[k];

    S_composite_08_sat[k] = S_composite_08_lum[k];
    S_composite_08_sat[7 - k] = S_composite_08_sat[k];
  }

  /* composite 16 tables */
  for (k = 0; k < 8; k++)
  {
    S_composite_16_lum[k] = (k + 1) * COMPOSITE_16_TABLE_STEP;
    S_composite_16_lum[15 - k] = 1.0f - S_composite_16_lum[k];

    S_composite_16_sat[k] = S_composite_16_lum[k];
    S_composite_16_sat[15 - k] = S_composite_16_sat[k];
  }

  /* composite 32 tables */
  for (k = 0; k < 16; k++)
  {
    S_composite_32_lum[k] = (k + 1) * COMPOSITE_32_TABLE_STEP;
    S_composite_32_lum[31 - k] = 1.0f - S_composite_32_lum[k];

    S_composite_32_sat[k] = S_composite_32_lum[k];
    S_composite_32_sat[31 - k] = S_composite_32_sat[k];
  }

  return 0;
}

/*******************************************************************************
** set_voltage_table_pointers()
*******************************************************************************/
short int set_voltage_table_pointers()
{
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if ( (G_source == SOURCE_COMPOSITE_04_1X) || 
            (G_source == SOURCE_COMPOSITE_04_1X_ROTATED))
  {
    S_luma_table = S_composite_04_lum;
    S_saturation_table = S_composite_04_sat;
    S_table_length = 4;
  }
  else if ( (G_source == SOURCE_COMPOSITE_08_2X) || 
            (G_source == SOURCE_COMPOSITE_08_2p50X))
  {
    S_luma_table = S_composite_08_lum;
    S_saturation_table = S_composite_08_sat;
    S_table_length = 8;
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_16_3X)          || 
            (G_source == SOURCE_COMPOSITE_16_3X_ROTATED))
  {
    S_luma_table = S_composite_16_lum;
    S_saturation_table = S_composite_16_sat;
    S_table_length = 16;
  }
  else if ( (G_source == SOURCE_COMPOSITE_32_2X) || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    S_luma_table = S_composite_32_lum;
    S_saturation_table = S_composite_32_sat;
    S_table_length = 32;
  }
  else
  {
    printf("Cannot set voltage table pointers; invalid source specified.\n");
    return 1;
  }

  return 0;
}

/*******************************************************************************
** add_shade()
*******************************************************************************/
short int add_shade(unsigned char r, unsigned char g, unsigned char b)
{
  /* make sure the array indices are valid */
  if ((G_num_shades < 0) || (G_num_shades >= MAX_SHADES))
  {
    printf("Unable to add color: No more available shades.\n");
    return 1;
  }

  /* add color */
  G_shades_array[G_num_shades].r = r;
  G_shades_array[G_num_shades].g = g;
  G_shades_array[G_num_shades].b = b;

  G_num_shades += 1;

  return 0;
}

/*******************************************************************************
** generate_shades_from_source()
*******************************************************************************/
short int generate_shades_from_source()
{
  int   k;

  int   r;
  int   g;
  int   b;

  /* approximate nes */
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    /* add pure black */
    add_shade(0, 0, 0);

    /* add greys */
    for (k = 0; k < S_table_length; k++)
    {
      r = (int) ((S_luma_table[k] * 255) + 0.5f);
      g = (int) ((S_luma_table[k] * 255) + 0.5f);
      b = (int) ((S_luma_table[k] * 255) + 0.5f);

      add_shade(r, g, b);
    }

    /* add pure white */
    add_shade(255, 255, 255);
  }
  /* composite source */
  else if ( (G_source == SOURCE_COMPOSITE_04_1X)          || 
            (G_source == SOURCE_COMPOSITE_04_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_08_2X)          || 
            (G_source == SOURCE_COMPOSITE_08_2p50X)       || 
            (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_16_3X)          || 
            (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_32_2X)          || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    /* add greys */
    for (k = 0; k < S_table_length; k++)
    {
      r = (int) ((S_luma_table[k] * 255) + 0.5f);
      g = (int) ((S_luma_table[k] * 255) + 0.5f);
      b = (int) ((S_luma_table[k] * 255) + 0.5f);

      add_shade(r, g, b);
    }
  }

  return 0;
}

/*******************************************************************************
** write_gradient_svg()
*******************************************************************************/
short int write_gradient_svg( char* filename, 
                              char* source_name, 
                              char* gradient_name, 
                              int   number_of_stops, 
                              int   index_start)
{
  int   i;

  int   k;

  FILE* fp_out;

  float interval_low;
  float interval_high;
  float interval_mid;

  int   qs_and_rs[6];
  char  color_string[7];

  /* make sure filename is valid */
  if (filename == NULL)
  {
    printf("Export to .svg failed: No filename specified.\n");
    return 1;
  }

  /* make sure source name is valid */
  if (source_name == NULL)
  {
    printf("Export to .svg failed: No source name specified.\n");
    return 1;
  }

  /* make sure gradient name is valid */
  if (gradient_name == NULL)
  {
    printf("Export to .svg failed: No gradient name specified.\n");
    return 1;
  }

  /* make sure index variables are valid */
  if ((index_start < 0) || (index_start >= G_num_shades))
  {
    printf("Export to .svg failed: Invalid start index.\n");
    return 1;
  }

  /* make sure number of stops is valid */
  if ((number_of_stops <= 0) || (index_start + number_of_stops - 1 >= G_num_shades))
  {
    printf("Export to .svg failed: Invalid number of stops.\n");
    return 1;
  }

  /* open file */
  fp_out = fopen(filename, "wb");

  /* if file did not open, return error */
  if (fp_out == NULL)
  {
    printf("Export to .svg failed: Unable to open output file.\n");
    return 1;
  }

  /* write first lines of file */
  fprintf(fp_out, "<svg>\n");
  fprintf(fp_out, "    <linearGradient id=\"%s %s\" ", source_name, gradient_name);
  fprintf(fp_out, "gradientUnits=\"objectBoundingBox\" spreadMethod=\"pad\">\n");

  /* initialize variables */
  interval_low = 0.0f;
  interval_high = 0.0f;

  for (i = 0; i < 7; i++)
    color_string[i] = '\0';

  /* write gradient to file */
  for ( i = index_start; 
        i < index_start + number_of_stops; 
        i += 1)
  {
    interval_low = interval_high;
    interval_high += 1.0f / number_of_stops;
    interval_mid = (interval_low + interval_high) / 2.0f;

    qs_and_rs[0] = G_shades_array[i].r / 16;
    qs_and_rs[1] = G_shades_array[i].r % 16;

    qs_and_rs[2] = G_shades_array[i].g / 16;
    qs_and_rs[3] = G_shades_array[i].g % 16;

    qs_and_rs[4] = G_shades_array[i].b / 16;
    qs_and_rs[5] = G_shades_array[i].b % 16;

    for (k = 0; k < 6; k++)
    {
      if (qs_and_rs[k] == 0)
        color_string[k] = '0';
      else if (qs_and_rs[k] == 1)
        color_string[k] = '1';
      else if (qs_and_rs[k] == 2)
        color_string[k] = '2';
      else if (qs_and_rs[k] == 3)
        color_string[k] = '3';
      else if (qs_and_rs[k] == 4)
        color_string[k] = '4';
      else if (qs_and_rs[k] == 5)
        color_string[k] = '5';
      else if (qs_and_rs[k] == 6)
        color_string[k] = '6';
      else if (qs_and_rs[k] == 7)
        color_string[k] = '7';
      else if (qs_and_rs[k] == 8)
        color_string[k] = '8';
      else if (qs_and_rs[k] == 9)
        color_string[k] = '9';
      else if (qs_and_rs[k] == 10)
        color_string[k] = 'a';
      else if (qs_and_rs[k] == 11)
        color_string[k] = 'b';
      else if (qs_and_rs[k] == 12)
        color_string[k] = 'c';
      else if (qs_and_rs[k] == 13)
        color_string[k] = 'd';
      else if (qs_and_rs[k] == 14)
        color_string[k] = 'e';
      else if (qs_and_rs[k] == 15)
        color_string[k] = 'f';
    }

    fprintf(fp_out, "        <stop stop-color=\"#%s\" offset=\"%f\" stop-opacity=\"1\"/>\n", color_string, interval_mid);
  }

  /* write last lines of file */
  fprintf(fp_out, "    </linearGradient>\n");
  fprintf(fp_out, "</svg>\n");

  /* close file */
  fclose(fp_out);

  return 0;
}

/*******************************************************************************
** main()
*******************************************************************************/
int main(int argc, char *argv[])
{
  int   i;

  char  output_base_filename[24];

  /* 3 gradients, 64 characters per filename */
  char  output_svg_filenames[MAX_GRADIENTS][64];

  char  source_name[24];

  /* initialization */
  G_num_shades = 0;

  output_base_filename[0] = '\0';

  for (i = 0; i < MAX_GRADIENTS; i++)
    output_svg_filenames[i][0] = '\0';

  source_name[0] = '\0';

  /* initialize variables */
  G_source = SOURCE_APPROX_NES;

  S_luma_table = S_approx_nes_lum;
  S_saturation_table = S_approx_nes_sat;
  S_table_length = 4;

  /* generate voltage tables */
  generate_voltage_tables();

  /* read command line arguments */
  i = 1;

  while (i < argc)
  {
    /* source */
    if (!strcmp(argv[i], "-s"))
    {
      i++;

      if (i >= argc)
      {
        printf("Insufficient number of arguments. ");
        printf("Expected source name. Exiting...\n");
        return 0;
      }

      if (!strcmp("approx_nes", argv[i]))
        G_source = SOURCE_APPROX_NES;
      else if (!strcmp("composite_04", argv[i]))
        G_source = SOURCE_COMPOSITE_04_1X;
      else if (!strcmp("composite_08", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2X;
      else if (!strcmp("composite_16", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X;
      else if (!strcmp("composite_32", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2X;
      else
      {
        printf("Unknown source %s. Exiting...\n", argv[i]);
        return 0;
      }

      i++;
    }
    else
    {
      printf("Unknown command line argument %s. Exiting...\n", argv[i]);
      return 0;
    }
  }

  /* determine output filenames */
  if (G_source == SOURCE_APPROX_NES)
    strncpy(output_base_filename, "approx_nes", 16);
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
    strncpy(output_base_filename, "approx_nes", 24);
  else if (G_source == SOURCE_COMPOSITE_04_1X)
    strncpy(output_base_filename, "composite_04", 24);
  else if (G_source == SOURCE_COMPOSITE_04_1X_ROTATED)
    strncpy(output_base_filename, "composite_04", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2X)
    strncpy(output_base_filename, "composite_08", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
    strncpy(output_base_filename, "composite_08", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X)
    strncpy(output_base_filename, "composite_16", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
    strncpy(output_base_filename, "composite_16", 24);
  else if (G_source == SOURCE_COMPOSITE_16_3X)
    strncpy(output_base_filename, "composite_16", 24);
  else if (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)
    strncpy(output_base_filename, "composite_16", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2X)
    strncpy(output_base_filename, "composite_32", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
    strncpy(output_base_filename, "composite_32", 24);

  for (i = 0; i < MAX_GRADIENTS; i++)
  {
    strncpy(output_svg_filenames[i], output_base_filename, 24);

    /* determine gradient type */
    if (i == 0)
      strncat(output_svg_filenames[i], "_shadow.svg", 16);
    else if (i == 1)
      strncat(output_svg_filenames[i], "_mid.svg", 16);
    else if (i == 2)
      strncat(output_svg_filenames[i], "_hilite.svg", 16);
  }

  /* set voltage table pointers */
  if (set_voltage_table_pointers())
  {
    printf("Error setting voltage table pointers. Exiting...\n");
    return 0;
  }

  /* generate shades */
  generate_shades_from_source();

  /* determine source name */
  if (G_source == SOURCE_APPROX_NES)
    strncpy(source_name, "Approx NES", 24);
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
    strncpy(source_name, "Approx NES", 24);
  else if (G_source == SOURCE_COMPOSITE_04_1X)
    strncpy(source_name, "Composite 04", 24);
  else if (G_source == SOURCE_COMPOSITE_04_1X_ROTATED)
    strncpy(source_name, "Composite 04", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2X)
    strncpy(source_name, "Composite 08", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
    strncpy(source_name, "Composite 08", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X)
    strncpy(source_name, "Composite 16", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
    strncpy(source_name, "Composite 16", 24);
  else if (G_source == SOURCE_COMPOSITE_16_3X)
    strncpy(source_name, "Composite 16", 24);
  else if (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)
    strncpy(source_name, "Composite 16", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2X)
    strncpy(source_name, "Composite 32", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
    strncpy(source_name, "Composite 32", 24);

  /* write output files */

  /* approx nes & approx nes rotated */
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    /* 4 tone shadow: 0, 1, 2, 3        */
    /* 4 tone mid:       1, 2, 3, 4     */
    /* 4 tone hilite:       2, 3, 4, 5  */
    write_gradient_svg(output_svg_filenames[0], source_name, "Shadow",  4, 0);
    write_gradient_svg(output_svg_filenames[1], source_name, "Mid",     4, 1);
    write_gradient_svg(output_svg_filenames[2], source_name, "Hilite",  4, 2);
  }
  /* 4 color gradients */
  else if ( (G_source == SOURCE_COMPOSITE_04_1X) || 
            (G_source == SOURCE_COMPOSITE_04_1X_ROTATED))
  {
    /* 3 tone shadow: 0, 1, 2     */
    /* 4 tone mid:    0, 1, 2, 3  */
    /* 3 tone hilite:    1, 2, 3  */
    write_gradient_svg(output_svg_filenames[0], source_name, "Shadow",  3, 0);
    write_gradient_svg(output_svg_filenames[1], source_name, "Mid",     4, 0);
    write_gradient_svg(output_svg_filenames[2], source_name, "Hilite",  3, 1);
  }
  /* 8 color gradients */
  else if ( (G_source == SOURCE_COMPOSITE_08_2X) || 
            (G_source == SOURCE_COMPOSITE_08_2p50X))
  {
    /* 6 tone shadow: 0, 1, 2, 3, 4, 5        */
    /* 6 tone mid:       1, 2, 3, 4, 5, 6     */
    /* 6 tone hilite:       2, 3, 4, 5, 6, 7  */
    write_gradient_svg(output_svg_filenames[0], source_name, "Shadow",  6, 0);
    write_gradient_svg(output_svg_filenames[1], source_name, "Mid",     6, 1);
    write_gradient_svg(output_svg_filenames[2], source_name, "Hilite",  6, 2);
  }
  /* 16 color gradients */
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_16_3X)          || 
            (G_source == SOURCE_COMPOSITE_16_3X_ROTATED))
  {
    /* 12 tone shadow: 0, 1, ..., 11  */
    /* 12 tone mid:    2, 3, ..., 13  */
    /* 12 tone hilite: 4, 5, ..., 15  */
    write_gradient_svg(output_svg_filenames[0], source_name, "Shadow", 12, 0);
    write_gradient_svg(output_svg_filenames[1], source_name, "Mid",    12, 2);
    write_gradient_svg(output_svg_filenames[2], source_name, "Hilite", 12, 4);
  }
  /* 32 color gradients */
  else if ( (G_source == SOURCE_COMPOSITE_32_2X) || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    /* 24 tone shadow: 0, 1, ..., 23  */
    /* 24 tone mid:    4, 5, ..., 27  */
    /* 24 tone hilite: 8, 9, ..., 31  */
    write_gradient_svg(output_svg_filenames[0], source_name, "Shadow", 24, 0);
    write_gradient_svg(output_svg_filenames[1], source_name, "Mid",    24, 4);
    write_gradient_svg(output_svg_filenames[2], source_name, "Hilite", 24, 8);
  }

  return 0;
}
