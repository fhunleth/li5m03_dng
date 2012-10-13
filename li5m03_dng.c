/*
   Converts raw output from the LI-5M03 camera module to an
   Adobe DNG. Based on code with the following notice:

   Written by Dave Coffin for Berkeley Engineering and Research.

   Free for all uses.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <tiffio.h>

int todng(const char *inputname,
          const char *outputname,
          unsigned char *buffer,
          int image_width,
          int image_height,
          int bytes_per_line,
          double gam)
{
    static const short CFARepeatPatternDim[] = { 2,2 };
    static const float cam_xyz[] =
    { 2.005,-0.771,-0.269, -0.752,1.688,0.064, -0.149,0.283,0.745 };
    static const float neutral[] = { 0.807133, 1.0, 0.913289 };
    long sub_offset=0, white=0x3fff;
    int status=1, i, row;
    unsigned short curve[256], *out;
    struct stat st;
    struct tm tm;
    char datetime[64];
    TIFF *tif;

    for (i=0; i < 256; i++)
        curve[i] = 0x3fff * pow (i/255.0, 100/gam) + 0.5;

    stat (inputname, &st);
    gmtime_r(&st.st_mtime, &tm);
    sprintf (datetime, "%04d:%02d:%02d %02d:%02d:%02d",
             tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);

    if (!(tif = TIFFOpen(outputname, "w")))
        goto fail;
    out = calloc(image_width, sizeof *out);

    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, 1);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, image_width >> 4);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, image_height >> 4);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_MAKE, "Leopard Imaging");
    TIFFSetField(tif, TIFFTAG_MODEL, "LI-5M03");
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_SOFTWARE, "li5mo3_dng");
    TIFFSetField(tif, TIFFTAG_DATETIME, datetime);
    TIFFSetField(tif, TIFFTAG_SUBIFD, 1, &sub_offset);
    TIFFSetField(tif, TIFFTAG_DNGVERSION, "\001\001\0\0");
    TIFFSetField(tif, TIFFTAG_DNGBACKWARDVERSION, "\001\0\0\0");
    TIFFSetField(tif, TIFFTAG_UNIQUECAMERAMODEL, "Leopard Imaging LI-5M03");
    TIFFSetField(tif, TIFFTAG_COLORMATRIX1, 9, cam_xyz);
    TIFFSetField(tif, TIFFTAG_ASSHOTNEUTRAL, 3, neutral);
    TIFFSetField(tif, TIFFTAG_CALIBRATIONILLUMINANT1, 21);
    TIFFSetField(tif, TIFFTAG_ORIGINALRAWFILENAME, inputname);
    {
        unsigned char *buf = malloc(image_width >> 4);
        memset(buf, 0, image_width >> 4);
        for (row=0; row < image_height >> 4; row++)
            TIFFWriteScanline(tif, buf, row, 0);
    }
    TIFFWriteDirectory (tif);

    TIFFSetField (tif, TIFFTAG_SUBFILETYPE, 0);
    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, image_width);
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, image_height);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_CFA);
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField (tif, TIFFTAG_CFAREPEATPATTERNDIM, CFARepeatPatternDim);
    TIFFSetField (tif, TIFFTAG_CFAPATTERN, 4, "\002\001\001\0");
    TIFFSetField (tif, TIFFTAG_LINEARIZATIONTABLE, 256, curve);
    TIFFSetField (tif, TIFFTAG_WHITELEVEL, 1, &white);

    for (row=0; row < image_height; row++) {
        TIFFWriteScanline (tif, &buffer[row * bytes_per_line], row, 0);
    }
    TIFFClose (tif);
    status = 0;
fail:
    return status;
}

static void usage(const char *name)
{
    fprintf(stderr, "%s [-w <width>] [-h <height>] [-b <bytes per line>] [-g <gamma> [-o <output filename>] <input file>\n", name);
    fprintf(stderr, "Converts a raw image buffer to an Adobe DNG.\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, " %s -w 1280 -h 720 -g 100 -o out.dng image.img\n", name);
}

int main(int argc, char *argv[])
{
    FILE *input_file;
    int opt;
    int width = 0;
    int height = 0;
    int bytes_per_line = 0;
    int image_size;
    double gamma = 100;
    const char *output_filename = "out.dng";
    const char *input_filename = 0;
    unsigned char *buffer;

    while ((opt = getopt(argc, argv, "w:h:b:g:o:")) != -1) {
        switch (opt) {
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;
        case 'b':
            bytes_per_line = atoi(optarg);
            break;
        case 'o':
            output_filename = optarg;
            break;
        case 'g':
            gamma = atof(optarg);
            if (gamma < 0) {
                fprintf(stderr, "ERROR: Gamma must be positive\n");
                exit(-1);
            }
            break;
        default:
            usage(argv[0]);
            exit(-1);
        }
    }

    if (optind < argc)
        input_filename = argv[optind];

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "ERROR: Must specify width and height of input image\n");
        exit(-1);
    }
    if ((width | height) & 15) {
        fprintf (stderr, "ERROR: Dimensions must be multiples of 16!\n");
        exit(-1);
    }
    if (bytes_per_line <= 0)
        bytes_per_line = width;

    if (input_filename == 0 || strcmp(input_filename, "-") == 0)
        input_file = stdin;
    else
        input_file = fopen(input_filename, "rb");
    if (input_file == 0) {
        fprintf(stderr, "Error opening %s\n", input_filename);
        exit(-1);
    }

    image_size = bytes_per_line * height * 3 / 2;
    buffer = (unsigned char *) malloc(image_size);
    if (buffer == 0) {
        fprintf(stderr, "Not enough memory\n");
        exit(-1);
    }

    if (fread(buffer, 1, image_size, input_file) != image_size) {
        fprintf(stderr, "Input file not big enough\n");
        exit(-1);
    }

    fclose(input_file);

    todng(input_filename,
              output_filename,
              buffer,
              width,
              height,
              bytes_per_line,
              gamma);

    free(buffer);
    return 0;

}

