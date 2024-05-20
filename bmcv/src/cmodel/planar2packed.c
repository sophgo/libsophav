#include <bmcv_cmodel.h>

int u8_plan2pack(struct image_struct cinfo, unsigned char * p_in, unsigned char * p_out){
    cinfo.output_image_width  = cinfo.input_image_width;
    cinfo.output_image_height = cinfo.input_image_height;

    int w = cinfo.input_image_width;
    int h = cinfo.input_image_height;
    int offset = w * h;
    unsigned char * data1 = p_in + offset;
    unsigned char * data2 = data1 + offset;
    for(int i = 0; i < h; i++){
      for(int j = 0; j < w; j++){
        p_out[i * w * 3 + j * 3] = p_in[i * w + j];
        p_out[i * w * 3 + j * 3 + 1] = data1[i * w + j];
        p_out[i * w * 3 + j * 3 + 2] = data2[i * w + j];
      }
    }
    return 0;
}

int u16_plan2pack(struct image_struct cinfo, unsigned short * p_in, unsigned short * p_out){
    cinfo.output_image_width  = cinfo.input_image_width;
    cinfo.output_image_height = cinfo.input_image_height;

    int w = cinfo.input_image_width;
    int h = cinfo.input_image_height;
    int offset = w * h;
    unsigned short * data1 = p_in + offset;
    unsigned short * data2 = data1 + offset;
    for(int i = 0; i < h; i++){
      for(int j = 0; j < w; j++){
        p_out[i * w * 3 + j * 3] = p_in[i * w + j];
        p_out[i * w * 3 + j * 3 + 1] = data1[i * w + j];
        p_out[i * w * 3 + j * 3 + 2] = data2[i * w + j];
      }
    }
    return 0;
}

int u32_plan2pack(struct image_struct cinfo, float * p_in, float * p_out){
    cinfo.output_image_width  = cinfo.input_image_width;
    cinfo.output_image_height = cinfo.input_image_height;

    int w = cinfo.input_image_width;
    int h = cinfo.input_image_height;
    int offset = w * h;
    float * data1 = p_in + offset;
    float * data2 = data1 + offset;
    for(int i = 0; i < h; i++){
      for(int j = 0; j < w; j++){
        p_out[i * w * 3 + j * 3] = p_in[i * w + j];
        p_out[i * w * 3 + j * 3 + 1] = data1[i * w + j];
        p_out[i * w * 3 + j * 3 + 2] = data2[i * w + j];
      }
    }
    return 0;
}