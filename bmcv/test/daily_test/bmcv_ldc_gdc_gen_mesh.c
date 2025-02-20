#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmcv_api_ext_c.h"
#include <unistd.h>
#include <inttypes.h>

#define align_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#define HW_MESH_SIZE 8

#define TILESIZE 64 // HW: data Tile Size
#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE
#define LDC_ALIGN 64
typedef unsigned int         u32;
typedef unsigned char        u8;

typedef struct COORD2D_INT_HW {
    u8 xcor[3]; // s13.10, 24bit
} __attribute__((packed)) COORD2D_INT_HW;

void test_mesh_gen_get_1st_size(u32 u32Width, u32 u32Height, u32 *mesh_1st_size)
{
    if (!mesh_1st_size)
        return;

    u32 ori_src_width = u32Width;
    u32 ori_src_height = u32Height;

    // In LDC Processing, width & height aligned to TILESIZE
    u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
    u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

    // modify frame size
    u32 dst_height_s1 = src_height_s1;
    u32 dst_width_s1 = src_width_s1;
    u32 num_tilex_s1 = dst_width_s1 / TILESIZE;
    u32 num_tiley_s1 = dst_height_s1 / TILESIZE;

    // 4 = 4 knots in a mesh
    *mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 * 4;
}

void test_mesh_gen_get_2nd_size(u32 u32Width, u32 u32Height, u32 *mesh_2nd_size)
{
    if (!mesh_2nd_size)
        return;

    u32 ori_src_width = u32Width;
    u32 ori_src_height = u32Height;

    // In LDC Processing, width & height aligned to TILESIZE
    u32 src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
    u32 src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

    // modify frame size
    u32 dst_height_s1 = src_height_s1;
    u32 dst_width_s1 = src_width_s1;
    u32 src_height_s2 = dst_width_s1;
    u32 src_width_s2 = dst_height_s1;
    u32 dst_height_s2 = src_height_s2;
    u32 dst_width_s2 = src_width_s2;
    u32 num_tilex_s2 = dst_width_s2 / TILESIZE;
    u32 num_tiley_s2 = dst_height_s2 / TILESIZE;

    // 4 = 4 knots in a mesh
    *mesh_2nd_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE * MESH_NUM_ATILE * num_tilex_s2 * num_tiley_s2 * 4;
}

void test_mesh_gen_get_size(u32 u32Width,
                            u32 u32Height,
                            u32 *mesh_1st_size,
                            u32 *mesh_2nd_size)
{
    if (!mesh_1st_size || !mesh_2nd_size)
        return;

    test_mesh_gen_get_1st_size(u32Width, u32Height, mesh_1st_size);
    test_mesh_gen_get_2nd_size(u32Width, u32Height, mesh_2nd_size);
}


int main() {
    int dev_id = 0;
    int height = 1080, width = 1920;
    bm_image_format_ext src_fmt = FORMAT_GRAY, dst_fmt = FORMAT_GRAY;
    bmcv_gdc_attr stLDCAttr = {0};
    char *src_name = "/home/linaro/A2test/bmcv/test/res/1920x1080_gray.bin";
    bm_handle_t handle = NULL;
    int ret = (int)bm_dev_request(&handle, dev_id);
    if (ret != 0) {
        printf("Create bm handle failed. ret = %d\n", ret);
        exit(-1);
    }

    bm_image src, dst;
    int src_stride[4];
    int dst_stride[4];

    // align
    int align_height = (height + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);
    int align_width  = (width  + (LDC_ALIGN - 1)) & ~(LDC_ALIGN - 1);

    // calc image stride
    int data_size = 1;
    src_stride[0] = align_up(width, 64) * data_size;
    dst_stride[0] = align_up(width, 64) * data_size;
    // create bm image
    bm_image_create(handle, height, width, src_fmt, DATA_TYPE_EXT_1N_BYTE, &src, src_stride);
    bm_image_create(handle, align_height, align_width, dst_fmt, DATA_TYPE_EXT_1N_BYTE, &dst, dst_stride);

    ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
    ret = bm_image_alloc_dev_mem(dst, BMCV_HEAP1_ID);

    int byte_size = width * height;
    unsigned char *input_data = (unsigned char *)malloc(byte_size);
    FILE *fp_src = fopen(src_name, "rb");
    if (fread((void *)input_data, 1, byte_size, fp_src) < (unsigned int)byte_size) {
      printf("file size is less than required bytes%d\n", byte_size);
    };
    fclose(fp_src);
    void *in_ptr[1] = {(void *)input_data};
    bm_image_copy_host_to_device(src, in_ptr);

    bm_device_mem_t dmem;
    u32 mesh_1st_size = 1, mesh_2nd_size = 1;
    test_mesh_gen_get_size(width, height, &mesh_1st_size, &mesh_2nd_size);
    u32 mesh_size = mesh_1st_size + mesh_2nd_size;
    ret = bm_malloc_device_byte(handle, &dmem, mesh_size);

    ret = bmcv_ldc_gdc_gen_mesh(handle, src, dst, stLDCAttr, dmem);
    unsigned char *buffer = (unsigned char *)malloc(mesh_size);
    memset(buffer, 0, mesh_size);
    ret = bm_memcpy_d2s(handle, (void*)buffer, dmem);

    char mesh_name[128];
    snprintf(mesh_name, 128, "./test_mesh_%dx%d_%d_%d_%d_%d_%d_%d_%d.mesh"
        , width, height, stLDCAttr.bAspect, stLDCAttr.s32XRatio, stLDCAttr.s32YRatio
        , stLDCAttr.s32XYRatio, stLDCAttr.s32CenterXOffset, stLDCAttr.s32CenterYOffset, stLDCAttr.s32DistortionRatio);

    FILE *fp = fopen(mesh_name, "wb");
    if (fwrite((void *)(unsigned long int)buffer, mesh_size, 1, fp) != 1) {
        printf("fwrite mesh data error\n");
        free(buffer);
    }
    fclose(fp);
    free(buffer);

    bm_image_destroy(&src);
    bm_image_destroy(&dst);

    bm_dev_free(handle);

    return 0;
}
