/*******************************************************************************
** GRADIENT (gradient file generation) - Michael Behrens 2019-2022
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
  SOURCE_COMPOSITE_16_1X,
  SOURCE_COMPOSITE_16_1X_ROTATED,
  SOURCE_COMPOSITE_08_2X,
  SOURCE_COMPOSITE_32_2X,
  SOURCE_COMPOSITE_06_0p75X,
  SOURCE_COMPOSITE_24_0p75X,
  SOURCE_COMPOSITE_12_1p50X,
  SOURCE_COMPOSITE_48_1p50X,
  SOURCE_COMPOSITE_08_2p50X,
  SOURCE_COMPOSITE_32_2p50X,
  SOURCE_EGA_EXTENDED_08,
  SOURCE_EGA_EXTENDED_32,
  SOURCE_CGA0_EXTENDED_16,
  SOURCE_CGA1_EXTENDED_16
};

enum
{
  HUE_MODIFIER_FULL = 0,
  HUE_MODIFIER_LOWER_HALF,
  HUE_MODIFIER_UPPER_HALF
};

/* the table step is 1 / (n + 1), where */
/* n is the number of colors per hue    */
#define COMPOSITE_06_TABLE_STEP 0.142857142857143f  /* 1/7  */
#define COMPOSITE_08_TABLE_STEP 0.111111111111111f  /* 1/9  */
#define COMPOSITE_12_TABLE_STEP 0.076923076923077f  /* 1/13 */
#define COMPOSITE_16_TABLE_STEP 0.058823529411765f  /* 1/17 */
#define COMPOSITE_24_TABLE_STEP 0.04f               /* 1/25 */
#define COMPOSITE_32_TABLE_STEP 0.03030303030303f   /* 1/33 */
#define COMPOSITE_48_TABLE_STEP 0.020408163265306f  /* 1/49 */

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

float S_approx_nes_p_p[4] = {0.4f, 0.7f,  0.7f,   0.3f};
float S_approx_nes_lum[4] = {0.2f, 0.35f, 0.65f,  0.85f};
float S_approx_nes_sat[4] = {0.2f, 0.35f, 0.35f,  0.15f};

float S_composite_06_lum[6];
float S_composite_06_sat[6];

float S_composite_08_lum[8];
float S_composite_08_sat[8];

float S_composite_12_lum[12];
float S_composite_12_sat[12];

float S_composite_16_lum[16];
float S_composite_16_sat[16];

float S_composite_24_lum[24];
float S_composite_24_sat[24];

float S_composite_32_lum[32];
float S_composite_32_sat[32];

float S_composite_48_lum[48];
float S_composite_48_sat[48];

#define MAX_HUES    32
#define MAX_COLORS  64

color   G_colors_array[MAX_HUES][MAX_COLORS];
int     G_num_colors[MAX_HUES];
int     G_num_hues;

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

  /* composite 06 tables */
  for (k = 0; k < 3; k++)
  {
    S_composite_06_lum[k] = (k + 1) * COMPOSITE_06_TABLE_STEP;
    S_composite_06_lum[5 - k] = 1.0f - S_composite_06_lum[k];

    S_composite_06_sat[k] = S_composite_06_lum[k];
    S_composite_06_sat[5 - k] = S_composite_06_sat[k];
  }

  /* composite 08 tables */
  for (k = 0; k < 4; k++)
  {
    S_composite_08_lum[k] = (k + 1) * COMPOSITE_08_TABLE_STEP;
    S_composite_08_lum[7 - k] = 1.0f - S_composite_08_lum[k];

    S_composite_08_sat[k] = S_composite_08_lum[k];
    S_composite_08_sat[7 - k] = S_composite_08_sat[k];
  }

  /* composite 12 tables */
  for (k = 0; k < 6; k++)
  {
    S_composite_12_lum[k] = (k + 1) * COMPOSITE_12_TABLE_STEP;
    S_composite_12_lum[11 - k] = 1.0f - S_composite_12_lum[k];

    S_composite_12_sat[k] = S_composite_12_lum[k];
    S_composite_12_sat[11 - k] = S_composite_12_sat[k];
  }

  /* composite 16 tables */
  for (k = 0; k < 8; k++)
  {
    S_composite_16_lum[k] = (k + 1) * COMPOSITE_16_TABLE_STEP;
    S_composite_16_lum[15 - k] = 1.0f - S_composite_16_lum[k];

    S_composite_16_sat[k] = S_composite_16_lum[k];
    S_composite_16_sat[15 - k] = S_composite_16_sat[k];
  }

  /* composite 24 tables */
  for (k = 0; k < 12; k++)
  {
    S_composite_24_lum[k] = (k + 1) * COMPOSITE_24_TABLE_STEP;
    S_composite_24_lum[23 - k] = 1.0f - S_composite_24_lum[k];

    S_composite_24_sat[k] = S_composite_24_lum[k];
    S_composite_24_sat[23 - k] = S_composite_24_sat[k];
  }

  /* composite 32 tables */
  for (k = 0; k < 16; k++)
  {
    S_composite_32_lum[k] = (k + 1) * COMPOSITE_32_TABLE_STEP;
    S_composite_32_lum[31 - k] = 1.0f - S_composite_32_lum[k];

    S_composite_32_sat[k] = S_composite_32_lum[k];
    S_composite_32_sat[31 - k] = S_composite_32_sat[k];
  }

  /* composite 48 tables */
  for (k = 0; k < 24; k++)
  {
    S_composite_48_lum[k] = (k + 1) * COMPOSITE_48_TABLE_STEP;
    S_composite_48_lum[47 - k] = 1.0f - S_composite_48_lum[k];

    S_composite_48_sat[k] = S_composite_48_lum[k];
    S_composite_48_sat[47 - k] = S_composite_48_sat[k];
  }

  return 0;
}

/*******************************************************************************
** set_voltage_table_pointers()
*******************************************************************************/
short int set_voltage_table_pointers()
{
  if (G_source == SOURCE_APPROX_NES)
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
  {
    S_luma_table = S_composite_06_lum;
    S_saturation_table = S_composite_06_sat;
    S_table_length = 6;
  }
  else if ( (G_source == SOURCE_COMPOSITE_08_2X)    || 
            (G_source == SOURCE_COMPOSITE_08_2p50X) || 
            (G_source == SOURCE_EGA_EXTENDED_08))
  {
    S_luma_table = S_composite_08_lum;
    S_saturation_table = S_composite_08_sat;
    S_table_length = 8;
  }
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
  {
    S_luma_table = S_composite_12_lum;
    S_saturation_table = S_composite_12_sat;
    S_table_length = 12;
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_CGA0_EXTENDED_16)         || 
            (G_source == SOURCE_CGA1_EXTENDED_16))
  {
    S_luma_table = S_composite_16_lum;
    S_saturation_table = S_composite_16_sat;
    S_table_length = 16;
  }
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
  {
    S_luma_table = S_composite_24_lum;
    S_saturation_table = S_composite_24_sat;
    S_table_length = 24;
  }
  else if ( (G_source == SOURCE_COMPOSITE_32_2X)    || 
            (G_source == SOURCE_COMPOSITE_32_2p50X) || 
            (G_source == SOURCE_EGA_EXTENDED_32))
  {
    S_luma_table = S_composite_32_lum;
    S_saturation_table = S_composite_32_sat;
    S_table_length = 32;
  }
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
  {
    S_luma_table = S_composite_48_lum;
    S_saturation_table = S_composite_48_sat;
    S_table_length = 48;
  }
  else
  {
    printf("Cannot set voltage table pointers; invalid source specified.\n");
    return 1;
  }

  return 0;
}

/*******************************************************************************
** add_color()
*******************************************************************************/
short int add_color(unsigned char r, unsigned char g, unsigned char b)
{
  /* make sure the array indices are valid */
  if ((G_num_hues < 0) || (G_num_hues >= MAX_HUES))
  {
    printf("Unable to add color: No more available hues.\n");
    return 1;
  }

  if ((G_num_colors[G_num_hues] < 0) || (G_num_colors[G_num_hues] >= MAX_COLORS))
  {
    printf("Unable to add color: No more available colors in this hue.\n");
    return 1;
  }

  /* add color */
  G_colors_array[G_num_hues][G_num_colors[G_num_hues]].r = r;
  G_colors_array[G_num_hues][G_num_colors[G_num_hues]].g = g;
  G_colors_array[G_num_hues][G_num_colors[G_num_hues]].b = b;

  G_num_colors[G_num_hues] += 1;

  return 0;
}

/*******************************************************************************
** generate_greys()
*******************************************************************************/
short int generate_greys()
{
  int k;

  int r;
  int g;
  int b;

  /* generate greys */
  for (k = 0; k < S_table_length; k++)
  {
    r = (int) ((S_luma_table[k] * 255) + 0.5f);
    g = (int) ((S_luma_table[k] * 255) + 0.5f);
    b = (int) ((S_luma_table[k] * 255) + 0.5f);

    add_color(r, g, b);
  }

  return 0;
}

/*******************************************************************************
** generate_hue()
*******************************************************************************/
short int generate_hue(int hue, int modifier)
{
  int   k;

  float y;
  float i;
  float q;

  int   r;
  int   g;
  int   b;

  int   start_index;
  int   end_index;

  /* make sure hue is valid */
  if ((hue < 0) || (hue >= 360))
  {
    printf("Cannot create palette hue; invalid hue specified.\n");
    return 1;
  }

  /* apply modifer */
  if (modifier == HUE_MODIFIER_FULL)
  {
    start_index = 0;
    end_index = S_table_length;
  }
  else if (modifier == HUE_MODIFIER_LOWER_HALF)
  {
    start_index = 0;
    end_index = S_table_length / 2;
  }
  else if (modifier == HUE_MODIFIER_UPPER_HALF)
  {
    start_index = S_table_length / 2;
    end_index = S_table_length;
  }
  else
  {
    printf("Cannot create palette hue; invalid modifier specified.\n");
    return 1;
  }

  /* generate hue */
  for (k = start_index; k < end_index; k++)
  {
    y = S_luma_table[k];
    i = S_saturation_table[k] * cos(TWO_PI * hue / 360.0f);
    q = S_saturation_table[k] * sin(TWO_PI * hue / 360.0f);

    r = (int) (((y + (i * 0.956f) + (q * 0.619f)) * 255) + 0.5f);
    g = (int) (((y - (i * 0.272f) - (q * 0.647f)) * 255) + 0.5f);
    b = (int) (((y - (i * 1.106f) + (q * 1.703f)) * 255) + 0.5f);

    /* bound rgb values */
    if (r < 0)
      r = 0;
    else if (r > 255)
      r = 255;

    if (g < 0)
      g = 0;
    else if (g > 255)
      g = 255;

    if (b < 0)
      b = 0;
    else if (b > 255)
      b = 255;

    /* add color to the palette */
    add_color(r, g, b);
  }

  return 0;
}

/*******************************************************************************
** generate_colors_from_source()
*******************************************************************************/
short int generate_colors_from_source()
{
  int i;

  int hue;
  int step;

  /* approximate nes */
  if ((G_source == SOURCE_APPROX_NES) || (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    /* set hue step & hue start */
    step = 30;

    if (G_source == SOURCE_APPROX_NES_ROTATED)
      hue = 15;
    else
      hue = 0;

    /* add pure black */
    add_color(0, 0, 0);

    /* add greys */
    generate_greys();

    /* add pure white */
    add_color(255, 255, 255);

    G_num_hues += 1;

    /* add hues */
    for (i = 0; i < 360 / step; i++)
    {
      /* add pure black */
      add_color(0, 0, 0);

      /* add colors */
      generate_hue(hue, HUE_MODIFIER_FULL);

      /* add pure white */
      add_color(255, 255, 255);

      G_num_hues += 1;

      /* increment hue */
      hue += step;
      hue = hue % 360;
    }
  }
  /* composite source */
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_08_2X)          || 
            (G_source == SOURCE_COMPOSITE_32_2X)          || 
            (G_source == SOURCE_COMPOSITE_06_0p75X)       || 
            (G_source == SOURCE_COMPOSITE_24_0p75X)       || 
            (G_source == SOURCE_COMPOSITE_12_1p50X)       || 
            (G_source == SOURCE_COMPOSITE_48_1p50X)       || 
            (G_source == SOURCE_COMPOSITE_08_2p50X)       || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    /* determine hue step & hue start */
    if ((G_source == SOURCE_COMPOSITE_16_1X) || 
        (G_source == SOURCE_COMPOSITE_16_1X_ROTATED))
    {
      step = 30;
    }
    else if ( (G_source == SOURCE_COMPOSITE_08_2X) || 
              (G_source == SOURCE_COMPOSITE_32_2X))
    {
      step = 15;
    }
    else if ( (G_source == SOURCE_COMPOSITE_06_0p75X) || 
              (G_source == SOURCE_COMPOSITE_24_0p75X))
    {
      step = 40;
    }
    else if ( (G_source == SOURCE_COMPOSITE_12_1p50X) || 
              (G_source == SOURCE_COMPOSITE_48_1p50X))
    {
      step = 20;
    }
    else if ( (G_source == SOURCE_COMPOSITE_08_2p50X) || 
              (G_source == SOURCE_COMPOSITE_32_2p50X))
    {
      step = 12;
    }
    else
    {
      step = 30;
    }

    if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
      hue = 15;
    else
      hue = 0;

    /* add greys */
    generate_greys();
    G_num_hues += 1;

    /* add hues */
    for (i = 0; i < 360 / step; i++)
    {
      generate_hue(hue, HUE_MODIFIER_FULL);
      G_num_hues += 1;

      /* increment hue */
      hue += step;
      hue = hue % 360;
    }
  }
  /* ega extended */
  else if ( (G_source == SOURCE_EGA_EXTENDED_08) || 
            (G_source == SOURCE_EGA_EXTENDED_32))
  {
    /* add greys */
    generate_greys();
    G_num_hues += 1;

    /* add hues */
    generate_hue(15, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(75, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(135, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(195, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(255, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(345, HUE_MODIFIER_LOWER_HALF);
    generate_hue(315, HUE_MODIFIER_UPPER_HALF);
    G_num_hues += 1;
  }
  /* cga palette 0 extended */
  else if (G_source == SOURCE_CGA0_EXTENDED_16)
  {
    /* green, red, brown/yellow */
    generate_hue(255, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(15, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(345, HUE_MODIFIER_LOWER_HALF);
    generate_hue(315, HUE_MODIFIER_UPPER_HALF);
    G_num_hues += 1;
  }
  /* cga palette 1 extended */
  else if (G_source == SOURCE_CGA1_EXTENDED_16)
  {
    /* cyan, magenta, grey */
    generate_hue(195, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_hue(75, HUE_MODIFIER_FULL);
    G_num_hues += 1;

    generate_greys();
    G_num_hues += 1;
  }

  return 0;
}

/*******************************************************************************
** write_gradient_svg()
*******************************************************************************/
short int write_gradient_svg( char* filename, 
                              char* source_name, 
                              char* hue_name, 
                              char* gradient_name, 
                              int   hue_index,
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

  /* make sure hue name is valid */
  if (hue_name == NULL)
  {
    printf("Export to .svg failed: No hue name specified.\n");
    return 1;
  }

  /* make sure gradient name is valid */
  if (gradient_name == NULL)
  {
    printf("Export to .svg failed: No gradient name specified.\n");
    return 1;
  }

  /* make sure the hue index is valid */
  if ((hue_index < 0) || (hue_index >= G_num_hues))
  {
    printf("Export to .svg failed: Invalid hue index.\n");
    return 1;
  }

  /* make sure index variables are valid */
  if ((index_start < 0) || (index_start >= G_num_colors[hue_index]))
  {
    printf("Export to .svg failed: Invalid start index.\n");
    return 1;
  }

  /* make sure number of stops is valid */
  if ((number_of_stops <= 0) || (index_start + number_of_stops - 1 >= G_num_colors[hue_index]))
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
  fprintf(fp_out, "    <linearGradient id=\"%s %s %s\" ", source_name, hue_name, gradient_name);
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

    qs_and_rs[0] = G_colors_array[hue_index][i].r / 16;
    qs_and_rs[1] = G_colors_array[hue_index][i].r % 16;

    qs_and_rs[2] = G_colors_array[hue_index][i].g / 16;
    qs_and_rs[3] = G_colors_array[hue_index][i].g % 16;

    qs_and_rs[4] = G_colors_array[hue_index][i].b / 16;
    qs_and_rs[5] = G_colors_array[hue_index][i].b % 16;

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
  int   j;

  char  output_base_filename[24];

  /* 32 max hues, 3 gradients per hue, 64 characters per filename */
  char  output_svg_filenames[MAX_HUES][3][64];

  char  source_name[24];
  char  hue_name[8];

  /* initialization */
  G_num_hues = 0;

  for (i = 0; i < MAX_HUES; i++)
    G_num_colors[i] = 0;

  output_base_filename[0] = '\0';

  for (i = 0; i < MAX_HUES; i++)
  {
    for (j = 0; j < 3; j++)
      output_svg_filenames[i][j][0] = '\0';
  }

  source_name[0] = '\0';
  hue_name[0] = '\0';

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
      else if (!strcmp("approx_nes_rotated", argv[i]))
        G_source = SOURCE_APPROX_NES_ROTATED;
      else if (!strcmp("composite_16_1x", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X;
      else if (!strcmp("composite_16_1x_rotated", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X_ROTATED;
      else if (!strcmp("composite_08_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2X;
      else if (!strcmp("composite_32_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2X;
      else if (!strcmp("composite_06_0p75x", argv[i]))
        G_source = SOURCE_COMPOSITE_06_0p75X;
      else if (!strcmp("composite_24_0p75x", argv[i]))
        G_source = SOURCE_COMPOSITE_24_0p75X;
      else if (!strcmp("composite_12_1p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_12_1p50X;
      else if (!strcmp("composite_48_1p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_48_1p50X;
      else if (!strcmp("composite_08_2p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2p50X;
      else if (!strcmp("composite_32_2p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2p50X;
      else if (!strcmp("ega_extended_08", argv[i]))
        G_source = SOURCE_EGA_EXTENDED_08;
      else if (!strcmp("ega_extended_32", argv[i]))
        G_source = SOURCE_EGA_EXTENDED_32;
      else if (!strcmp("cga0_extended_16", argv[i]))
        G_source = SOURCE_CGA0_EXTENDED_16;
      else if (!strcmp("cga1_extended_16", argv[i]))
        G_source = SOURCE_CGA1_EXTENDED_16;
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
    strncpy(output_base_filename, "approx_nes_rotated", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X)
    strncpy(output_base_filename, "composite_16_1x", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
    strncpy(output_base_filename, "composite_16_1x_rotated", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2X)
    strncpy(output_base_filename, "composite_08_2x", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2X)
    strncpy(output_base_filename, "composite_32_2x", 24);
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
    strncpy(output_base_filename, "composite_06_0p75x", 24);
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
    strncpy(output_base_filename, "composite_24_0p75x", 24);
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
    strncpy(output_base_filename, "composite_12_1p50x", 24);
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
    strncpy(output_base_filename, "composite_48_1p50x", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
    strncpy(output_base_filename, "composite_08_2p50x", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
    strncpy(output_base_filename, "composite_32_2p50x", 24);
  else if (G_source == SOURCE_EGA_EXTENDED_08)
    strncpy(output_base_filename, "ega_extended_08", 24);
  else if (G_source == SOURCE_EGA_EXTENDED_32)
    strncpy(output_base_filename, "ega_extended_32", 24);
  else if (G_source == SOURCE_CGA0_EXTENDED_16)
    strncpy(output_base_filename, "cga0_extended_16", 24);
  else if (G_source == SOURCE_CGA1_EXTENDED_16)
    strncpy(output_base_filename, "cga1_extended_16", 24);

  for (i = 0; i < MAX_HUES; i++)
  {
    for (j = 0; j < 3; j++)
    {
      strncpy(output_svg_filenames[i][j], output_base_filename, 24);

      /* determine gradient hue */
      if (i == 0)
        strncat(output_svg_filenames[i][j], "_hue_00", 8);
      else if (i == 1)
        strncat(output_svg_filenames[i][j], "_hue_01", 8);
      else if (i == 2)
        strncat(output_svg_filenames[i][j], "_hue_02", 8);
      else if (i == 3)
        strncat(output_svg_filenames[i][j], "_hue_03", 8);
      else if (i == 4)
        strncat(output_svg_filenames[i][j], "_hue_04", 8);
      else if (i == 5)
        strncat(output_svg_filenames[i][j], "_hue_05", 8);
      else if (i == 6)
        strncat(output_svg_filenames[i][j], "_hue_06", 8);
      else if (i == 7)
        strncat(output_svg_filenames[i][j], "_hue_07", 8);
      else if (i == 8)
        strncat(output_svg_filenames[i][j], "_hue_08", 8);
      else if (i == 9)
        strncat(output_svg_filenames[i][j], "_hue_09", 8);
      else if (i == 10)
        strncat(output_svg_filenames[i][j], "_hue_10", 8);
      else if (i == 11)
        strncat(output_svg_filenames[i][j], "_hue_11", 8);
      else if (i == 12)
        strncat(output_svg_filenames[i][j], "_hue_12", 8);
      else if (i == 13)
        strncat(output_svg_filenames[i][j], "_hue_13", 8);
      else if (i == 14)
        strncat(output_svg_filenames[i][j], "_hue_14", 8);
      else if (i == 15)
        strncat(output_svg_filenames[i][j], "_hue_15", 8);
      else if (i == 16)
        strncat(output_svg_filenames[i][j], "_hue_16", 8);
      else if (i == 17)
        strncat(output_svg_filenames[i][j], "_hue_17", 8);
      else if (i == 18)
        strncat(output_svg_filenames[i][j], "_hue_18", 8);
      else if (i == 19)
        strncat(output_svg_filenames[i][j], "_hue_19", 8);
      else if (i == 20)
        strncat(output_svg_filenames[i][j], "_hue_20", 8);
      else if (i == 21)
        strncat(output_svg_filenames[i][j], "_hue_21", 8);
      else if (i == 22)
        strncat(output_svg_filenames[i][j], "_hue_22", 8);
      else if (i == 23)
        strncat(output_svg_filenames[i][j], "_hue_23", 8);
      else if (i == 24)
        strncat(output_svg_filenames[i][j], "_hue_24", 8);
      else if (i == 25)
        strncat(output_svg_filenames[i][j], "_hue_25", 8);
      else if (i == 26)
        strncat(output_svg_filenames[i][j], "_hue_26", 8);
      else if (i == 27)
        strncat(output_svg_filenames[i][j], "_hue_27", 8);
      else if (i == 28)
        strncat(output_svg_filenames[i][j], "_hue_28", 8);
      else if (i == 29)
        strncat(output_svg_filenames[i][j], "_hue_29", 8);
      else if (i == 30)
        strncat(output_svg_filenames[i][j], "_hue_30", 8);
      else if (i == 31)
        strncat(output_svg_filenames[i][j], "_hue_31", 8);
      else if (i == 32)
        strncat(output_svg_filenames[i][j], "_hue_32", 8);

      /* determine gradient type */
      if (j == 0)
        strncat(output_svg_filenames[i][j], "_shadow.svg", 16);
      else if (j == 1)
        strncat(output_svg_filenames[i][j], "_mid.svg", 16);
      else if (j == 2)
        strncat(output_svg_filenames[i][j], "_highlight.svg", 16);
    }
  }

  /* set voltage table pointers */
  if (set_voltage_table_pointers())
  {
    printf("Error setting voltage table pointers. Exiting...\n");
    return 0;
  }

  /* generate colors */
  generate_colors_from_source();

  /* determine source name */
  if (G_source == SOURCE_APPROX_NES)
    strncpy(source_name, "Approx NES", 24);
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
    strncpy(source_name, "Approx NES Rotated", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X)
    strncpy(source_name, "Composite 16 1x", 24);
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
    strncpy(source_name, "Composite 16 1x Rotated", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2X)
    strncpy(source_name, "Composite 08 2x", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2X)
    strncpy(source_name, "Composite 32 2x", 24);
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
    strncpy(source_name, "Composite 06 0.75x", 24);
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
    strncpy(source_name, "Composite 24 0.75x", 24);
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
    strncpy(source_name, "Composite 12 1.5x", 24);
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
    strncpy(source_name, "Composite 48 1.5x", 24);
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
    strncpy(source_name, "Composite 08 2.5x", 24);
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
    strncpy(source_name, "Composite 32 2.5x", 24);
  else if (G_source == SOURCE_EGA_EXTENDED_08)
    strncpy(source_name, "EGA Extended 08", 24);
  else if (G_source == SOURCE_EGA_EXTENDED_32)
    strncpy(source_name, "EGA Extended 32", 24);
  else if (G_source == SOURCE_CGA0_EXTENDED_16)
    strncpy(source_name, "CGA0 Extended 16", 24);
  else if (G_source == SOURCE_CGA1_EXTENDED_16)
    strncpy(source_name, "CGA1 Extended 16", 24);

  /* write output files */
  for (i = 0; i < G_num_hues; i++)
  {
    /* determine hue name */
    if (i == 0)
      strncpy(hue_name, "Hue 00", 8);
    else if (i == 1)
      strncpy(hue_name, "Hue 01", 8);
    else if (i == 2)
      strncpy(hue_name, "Hue 02", 8);
    else if (i == 3)
      strncpy(hue_name, "Hue 03", 8);
    else if (i == 4)
      strncpy(hue_name, "Hue 04", 8);
    else if (i == 5)
      strncpy(hue_name, "Hue 05", 8);
    else if (i == 6)
      strncpy(hue_name, "Hue 06", 8);
    else if (i == 7)
      strncpy(hue_name, "Hue 07", 8);
    else if (i == 8)
      strncpy(hue_name, "Hue 08", 8);
    else if (i == 9)
      strncpy(hue_name, "Hue 09", 8);
    else if (i == 10)
      strncpy(hue_name, "Hue 10", 8);
    else if (i == 11)
      strncpy(hue_name, "Hue 11", 8);
    else if (i == 12)
      strncpy(hue_name, "Hue 12", 8);
    else if (i == 13)
      strncpy(hue_name, "Hue 13", 8);
    else if (i == 14)
      strncpy(hue_name, "Hue 14", 8);
    else if (i == 15)
      strncpy(hue_name, "Hue 15", 8);
    else if (i == 16)
      strncpy(hue_name, "Hue 16", 8);
    else if (i == 17)
      strncpy(hue_name, "Hue 17", 8);
    else if (i == 18)
      strncpy(hue_name, "Hue 18", 8);
    else if (i == 19)
      strncpy(hue_name, "Hue 19", 8);
    else if (i == 20)
      strncpy(hue_name, "Hue 20", 8);
    else if (i == 21)
      strncpy(hue_name, "Hue 21", 8);
    else if (i == 22)
      strncpy(hue_name, "Hue 22", 8);
    else if (i == 23)
      strncpy(hue_name, "Hue 23", 8);
    else if (i == 24)
      strncpy(hue_name, "Hue 24", 8);
    else if (i == 25)
      strncpy(hue_name, "Hue 25", 8);
    else if (i == 26)
      strncpy(hue_name, "Hue 26", 8);
    else if (i == 27)
      strncpy(hue_name, "Hue 27", 8);
    else if (i == 28)
      strncpy(hue_name, "Hue 28", 8);
    else if (i == 29)
      strncpy(hue_name, "Hue 29", 8);
    else if (i == 30)
      strncpy(hue_name, "Hue 30", 8);
    else if (i == 31)
      strncpy(hue_name, "Hue 31", 8);
    else if (i == 32)
      strncpy(hue_name, "Hue 32", 8);
    else
      strncpy(hue_name, "Unknown", 8);

    /* approx nes & approx nes rotated */
    if ((G_source == SOURCE_APPROX_NES) || 
        (G_source == SOURCE_APPROX_NES_ROTATED))
    {
      /* 4 tone shadow: 0, 1, 2, 3 */
      /* 4 tone mid:    1, 2, 3, 4 */
      /* 4 tone hilite: 2, 3, 4, 5 */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i,  4, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i,  4, 1);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i,  4, 2);
    }
    /* 6 color gradients */
    else if (G_source == SOURCE_COMPOSITE_06_0p75X)
    {
      /* 3 tone shadow: 0 ... 2 */
      /* 4 tone mid:    1 ... 4 */
      /* 3 tone hilite: 3 ... 5 */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i,  3, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i,  4, 1);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i,  3, 3);
    }
    /* 8 color gradients */
    else if ( (G_source == SOURCE_COMPOSITE_08_2X)    || 
              (G_source == SOURCE_COMPOSITE_08_2p50X) || 
              (G_source == SOURCE_EGA_EXTENDED_08))
    {
      /* 4 tone shadow: 0, 1, 2, 3 */
      /* 4 tone mid:    2, 3, 4, 5 */
      /* 4 tone hilite: 4, 5, 6, 7 */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i,  4, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i,  4, 2);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i,  4, 4);
    }
    /* 12 color gradients */
    else if (G_source == SOURCE_COMPOSITE_12_1p50X)
    {
      /* 6 tone shadow: 0 ... 5   */
      /* 6 tone mid:    3 ... 8   */
      /* 6 tone hilite: 6 ... 11  */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i,  6, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i,  6, 3);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i,  6, 6);
    }
    /* 16 color gradients */
    else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
              (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
              (G_source == SOURCE_CGA0_EXTENDED_16)         || 
              (G_source == SOURCE_CGA1_EXTENDED_16))
    {
      /* 8 tone shadow: 0 ... 7   */
      /* 8 tone mid:    4 ... 11  */
      /* 8 tone hilite: 8 ... 15  */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i,  8, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i,  8, 4);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i,  8, 8);
    }
    /* 24 color gradients */
    else if (G_source == SOURCE_COMPOSITE_24_0p75X)
    {
      /* 12 tone shadow:  0 ... 11  */
      /* 12 tone mid:     6 ... 17  */
      /* 12 tone hilite: 12 ... 23  */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i, 12, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i, 12, 6);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i, 12, 12);
    }
    /* 32 color gradients */
    else if ( (G_source == SOURCE_COMPOSITE_32_2X)    || 
              (G_source == SOURCE_COMPOSITE_32_2p50X) || 
              (G_source == SOURCE_EGA_EXTENDED_32))
    {
      /* 16 tone shadow:  0 ... 15  */
      /* 16 tone mid:     8 ... 23  */
      /* 16 tone hilite: 16 ... 31  */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i, 16, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i, 16, 8);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i, 16, 16);
    }
    /* 48 color gradients */
    else if (G_source == SOURCE_COMPOSITE_48_1p50X)
    {
      /* 24 tone shadow:  0 ... 23  */
      /* 24 tone mid:    12 ... 35  */
      /* 24 tone hilite: 24 ... 47  */
      write_gradient_svg(output_svg_filenames[i][0], source_name, hue_name, "Shadow",     i, 24, 0);
      write_gradient_svg(output_svg_filenames[i][1], source_name, hue_name, "Mid",        i, 24, 12);
      write_gradient_svg(output_svg_filenames[i][2], source_name, hue_name, "Highlight",  i, 24, 24);
    }
  }

  return 0;
}
