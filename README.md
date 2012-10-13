Conversion program to go from the raw output from a MT9P031 sensor on an
LI-5M03 camera module to a DNG file that can be processed by dcraw.

This requires a patched libtiff 3.8.2. To patch, download and unzip
libtiff-3.8.2. Then run:

cd tiff-3.8.2
patch -p1 < pathto/libtiff.patch

## Example usage

This assumes that a 1280x720 sized frame was captured from the MT9P031
and saved to input.bin

    li5m03_dng -w 1280 -h 720 -g 100 input.bin 
    dcraw out.dng 
    display out.ppm
    
## TODO

  1. Fix color conversion matrix stored in DNG file. It is clearly
     wrong, but not so bad as to be unusable for debug
  2. Fix white balance
