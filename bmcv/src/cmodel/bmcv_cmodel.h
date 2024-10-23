#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "bmcv_internal.h"
#include "bmcv_a2_vpss_internal.h"
#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ \
    && defined __SSE2__ && !defined __APPLE__)) && !defined(__CUDACC__)
#include <emmintrin.h>
#endif

static const int INTER_RESIZE_COEF_BITS = 11;
static const int INTER_RESIZE_COEF_SCALE = 1 << 11;
static const int MAX_ESIZE = 16;

unsigned char saturate_cast_uchar(int a);

#define CV_ENABLE_UNROLLED 1

struct image_struct{
  unsigned int input_image_width;
  unsigned int input_image_height;
  unsigned int output_image_width;
  unsigned int output_image_height;
};

typedef struct resize_image_{
  unsigned char* data;
  unsigned int height;
  unsigned int width;
  unsigned int step;
  unsigned int elemSize;
}resize_image;

typedef struct Size_{
	int width;
	int height;
}Size;

typedef struct Range_{
	int start;
	int end;
}Range;

typedef unsigned char uchar;
#define GETJSAMPLE(value)  ((int)(value))

typedef struct {
 unsigned int downsampled_width;
 unsigned int downsampled_height;
}image_component_info;

typedef struct image_struct *image_struct_ptr;

int alignSize(int a, int b);

int cvFloor( double value );

int cvround( double value );

int cvmin(int a, int b);

int cvmax(int a, int b);

//template<typename unsigned char, typename int, typename short>;
int clip(int x, int a, int b);

void h2v2_fancy_upsample(int width, int height, u8* input_data, u8* output_data);
void h2v1_fancy_upsample(int width, int height, u8* input_data, u8* output_data);
void h2v2_upsample(int width, int height, u8* input_data, u8* output_data);
void h2v1_upsample(int width, int height, u8* input_data, u8* output_data);
void h2v2_downsample(int width, int height, u8* input_data, u8* output_data);
void h2v1_downsample(int width, int height, u8* input_data, u8* output_data);
int resize_cubic(resize_image src, resize_image dst);
void Resize_linear(resize_image src, resize_image dst);
void Resize_NN(resize_image src, resize_image dst);
