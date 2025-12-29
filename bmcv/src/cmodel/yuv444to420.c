#include <bmcv_cmodel.h>

// void h2v2_downsample(int width, int height, u8* input_data, u8* output_data)
// {
//   int inrow, outrow;
//   unsigned int outcol;
//   unsigned int output_cols = width / 2;
//   register u8* inptr0, *inptr1, *outptr;
//   register int bias;

//   inrow = 0;
//   for (outrow = 0; outrow < height / 2; outrow++) {
//     outptr = output_data + output_cols * outrow;
//     inptr0 = input_data + width * inrow;
//     inptr1 = input_data + width * (inrow+1);
//     bias = 1;                   /* bias = 1,2,1,2,... for successive samples */
//     for (outcol = 0; outcol < output_cols; outcol++) {
//       *outptr++ = (u8)(((*inptr0) + (inptr0[1]) + (*inptr1) + (inptr1[1]) + bias) >> 2);
//       bias ^= 3;                /* 1=>2, 2=>1 */
//       inptr0 += 2;  inptr1 += 2;
//     }
//     inrow += 2;
//   }
// }

void h2v2_downsample(int width, int height, u8* input_data, u8* output_data) {
    int inrow, outrow;
    unsigned int outcol;
    unsigned int output_cols = width / 2;
    u8 *inptr0, *inptr1, *outptr;

    inrow = 0;
    for (outrow = 0; outrow < height / 2; outrow++) {
        outptr = output_data + output_cols * outrow;
        inptr0 = input_data + width * inrow;
        inptr1 = input_data + width * (inrow + 1);

        for (outcol = 0; outcol < output_cols; outcol++) {
            *outptr++ = inptr1[1];
            inptr0 += 2;
            inptr1 += 2;
        }
        inrow += 2;
    }
}
