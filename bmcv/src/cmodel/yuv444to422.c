#include <bmcv_cmodel.h>

// void h2v1_downsample(int width, int height, u8* input_data, u8* output_data)
// {
//   unsigned int outrow;
//   unsigned int outcol;
//   unsigned int output_cols = width / 2;
//   register u8 *inptr, *outptr;
//   register int bias;


//   for (outrow = 0; outrow < height; outrow++) {
//     outptr = output_data + output_cols * outrow;
//   // printf(" 000  outptr %p\n",outptr);

//     inptr = input_data + width * outrow;
//     bias = 0;                   /* bias = 0,1,0,1,... for successive samples */
//     for (outcol = 0; outcol < output_cols; outcol++) {
//       *outptr++ = (u8)(((*inptr) + (inptr[1]) + bias) >> 1);

//    //printf(" 111  outptr %p,outcol %d\n",outptr,outcol);

//       bias ^= 1;                /* 0=>1, 1=>0 */
//       inptr += 2;
//     }
//   }
//   return;
// }

void h2v1_downsample(int width, int height, u8* input_data, u8* output_data) {
    unsigned int outrow;
    unsigned int outcol;
    unsigned int output_cols = width / 2;
    register u8 *inptr, *outptr;

    for (outrow = 0; outrow < height; outrow++) {
        outptr = output_data + output_cols * outrow;
        inptr = input_data + width * outrow;
        for (outcol = 0; outcol < output_cols; outcol++) {
            *outptr++ = inptr[1];
            inptr += 2;
        }
    }
    return;
}