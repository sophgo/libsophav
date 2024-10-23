#include <bmcv_cmodel.h>

void h2v2_upsample(int width, int height, u8* input_data, u8* output_data)
{
  register u8* inptr, *outptr;
  register unsigned int invalue;
  u8* outend;
  int inrow, outrow;

  inrow = outrow = 0;
  while (outrow < height) {
    inptr = input_data + width/2 * inrow;
    outptr = output_data +  width * outrow;
    outend = outptr +  width;
    while (outptr < outend) {
      invalue = *inptr++;       /* don't need GETJSAMPLE() here */
      *outptr++ = invalue;
      *outptr++ = invalue;
    }

    memcpy((void *)(output_data +  width * (outrow + 1)), (void *)(output_data + width * outrow), width);

    inrow++;
    outrow += 2;
  }
}
void h2v2_fancy_upsample(int width, int height, u8 *input_data, u8 *output_data)
{

  register u8* inptr0, *inptr1, *outptr;

  register int thiscolsum, lastcolsum, nextcolsum;

  register unsigned int colctr;
  int inrow, outrow, v;

  inrow = outrow = 0;
  while (outrow < height) {
    for (v = 0; v < 2; v++) {
      /* inptr0 points to nearest input row, inptr1 points to next nearest */
      inptr0 = input_data + width/2 * inrow;
      if (v == 0)               /* next nearest is row above */
        inptr1 = inptr0 - width/2;
      else                      /* next nearest is row below */
        inptr1 = inptr0 + width/2;
      outptr = output_data + width * outrow;
      outrow++;

    // printf(" inptr0 %p,inptr1 %p\n",inptr0,inptr1);

      /* Special case for first column */
      thiscolsum = (*inptr0++) * 3 + (*inptr1++);
      nextcolsum = (*inptr0++) * 3 + (*inptr1++);
      *outptr++ = (u8)((thiscolsum * 4 + 8) >> 4);
      *outptr++ = (u8)((thiscolsum * 3 + nextcolsum + 7) >> 4);
      lastcolsum = thiscolsum;  thiscolsum = nextcolsum;

      for (colctr = width/2 - 2; colctr > 0; colctr--) {
        /* General case: 3/4 * nearer pixel + 1/4 * further pixel in each */
        /* dimension, thus 9/16, 3/16, 3/16, 1/16 overall */
        nextcolsum = (*inptr0++) * 3 + (*inptr1++);
        *outptr++ = (u8)((thiscolsum * 3 + lastcolsum + 8) >> 4);
        *outptr++ = (u8)((thiscolsum * 3 + nextcolsum + 7) >> 4);
        lastcolsum = thiscolsum;  thiscolsum = nextcolsum;
      }

      /* Special case for last column */
      *outptr++ = (u8)((thiscolsum * 3 + lastcolsum + 8) >> 4);
      *outptr++ = (u8)((thiscolsum * 4 + 7) >> 4);
    }
    inrow++;
  }
}
