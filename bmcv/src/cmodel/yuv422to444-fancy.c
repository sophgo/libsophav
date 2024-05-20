#include <bmcv_cmodel.h>

void h2v1_upsample(int width, int height, u8* input_data, u8* output_data)
{
  register u8* inptr, *outptr;
  register unsigned int invalue;
  u8* outend;
  int inrow;

  for (inrow = 0; inrow < height; inrow++) {
    inptr = input_data + width/2 * inrow;
    outptr = output_data + width * inrow;
    outend = outptr + width;
    while (outptr < outend) {
      invalue = *inptr++;       /* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }
  }
}

void h2v1_fancy_upsample(int width, int height, u8* input_data, u8* output_data)
{
  register u8* inptr, *outptr;
  register int invalue;
  register unsigned int colctr;
  int inrow;

  for (inrow = 0; inrow < height; inrow++) {
    inptr = input_data + width/2 * inrow;
    outptr = output_data + width * inrow;
    /* Special case for first column */
    invalue = (*inptr++);
    *outptr++ = invalue;
    *outptr++ = ((invalue * 3 + (*inptr) + 2) >> 2);

    for (colctr = width/2 - 2; colctr > 0; colctr--) {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      invalue = (*inptr++) * 3;
      *outptr++ = ((invalue + (inptr[-2]) + 1) >> 2);
      *outptr++ = ((invalue + (*inptr) + 2) >> 2);
    }

    /* Special case for last column */
    invalue = (*inptr);
    *outptr++ = ((invalue * 3 + (inptr[-1]) + 1) >> 2);
    *outptr++ = invalue;
  }
}

