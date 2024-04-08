#include "../SFT.h"
#include "../Common.h"

#define TRANSLATE_FLAG          1
#define SCALE_FLAG              2
#define ROTATE_FLAG             4
#define PERSPECTIVE_FLAG        8

typedef enum function {
    draw,
    blit,
    pattern,
    gradient,
} function_t;

static int32_t path_data1[] = {
    2,  35, 50,
    4,  75, 15,
    4, 110, 35,
    4, 100, 50,
    4, 110, 65,
    4,  75, 85,
    0,
};

static int32_t path_data2[] =
{
    2, 155, 155,
    5,  80,   0,
    5,   0,  80,
    5, -80,   0,
    5,   0, -80,
    2, 165, 165,
    5,  60,   0,
    5,   0,  60,
    5, -60,   0,
    5,   0, -60,
    0,
};

/* A simple triangle */
static int32_t path_data3[] = {
    5, 170,  0,
    5, -85, 85,
    0
};
/* An hourglass shape */
static int32_t path_data4[] = {
    5, 100,   0,
    5, -50, 100,
    5, -50, 100,
    5, 100,   0,
    0
};
/* A simple square */
static int32_t path_data5[] = {
    5, 100,   0,
    5,   0, 100,
    5,-100,   0,
    0
};
static int32_t path_data6[] = {
    2, -5, -10,
    4,  5, -10,
    4, 10,  -5,
    4,  0,   0,
    4, 10,   5,
    4,  5,  10,
    4, -5,  10,
    4,-10,   5,
    4,-10,  -5,
    0,
};
/* A square shape with triangular holes */
static int32_t path_data7[] = {
    4,  80,  40,
    4,  40,  80,
    4,   0,   0,
    4, 200,   0,
    4, 160,  80,
    4, 120,  40,
    4, 200,   0,
    4, 200, 200,
    4, 120, 160,
    4, 160, 120,
    4, 200, 200,
    4,   0, 200,
    4,  40, 120,
    4,  80, 160,
    4,   0, 200,
    0
};

/* A spheric triangle */
static int32_t path_data8[] = {
    7,   85, -60, 170,-40,
    7,  -50,  50, -70,210,
    7,  -90, -30,-100,-170,
    0
};

/* A distorsioned shape with a square hole */
static int32_t path_data9[] = {
    VLC_OP_QUAD,     8,     22,     45,     0,
    VLC_OP_QUAD,    37,     26,     45,     53,
    VLC_OP_QUAD,    30,     45,     15,     53,
    VLC_OP_LINE,    15,     18,
    VLC_OP_LINE,    35,     18,
    VLC_OP_LINE,    35,     40,
    VLC_OP_LINE,    0,      40,
    VLC_OP_QUAD,    7,      20,     0,      0,
    VLC_OP_END
};

static int8_t path_data10[] = {
    2,  35, 50,
    4,  75, 15,
    4, 110, 35,
    4, 100, 50,
    4, 110, 65,
    4,  75, 85,
    0,
};
static int16_t path_data11[] = {
    2,  35, 50,
    4,  75, 15,
    4, 110, 35,
    4, 100, 50,
    4, 110, 65,
    4,  75, 85,
    0,
};
static int32_t path_data12[] = {
    2,  35, 50,
    4,  75, 15,
    4, 110, 35,
    4, 100, 50,
    4, 110, 65,
    4,  75, 85,
    0,
};
static int32_t *test_path[9] =
{
    path_data1,
    path_data2,
    path_data3,
    path_data4,
    path_data5,
    path_data6,
    path_data7,
    path_data8,
    path_data9,
};
static int32_t length[]=
{
    sizeof(path_data1),
    sizeof(path_data2),
    sizeof(path_data3),
    sizeof(path_data4),
    sizeof(path_data5),
    sizeof(path_data6),
    sizeof(path_data7),
    sizeof(path_data8),
    sizeof(path_data9),
};

/* Init buffers. */
static vg_lite_error_t gen_buffer(int type, vg_lite_buffer_t *buf, vg_lite_buffer_format_t format, uint32_t width, uint32_t height)
{
    void *data = NULL;
    uint8_t *pdata8, *pdest8;
    uint32_t bpp = get_bpp(format);
    int i;

    /* Randomly generate some images. */
    data = gen_image(type, format, width, height);
    pdata8 = (uint8_t*)data;
    pdest8 = (uint8_t*)buf->memory;
    if (data != NULL)
    {
        for (i = 0; i < height; i++)
        {
            memcpy (pdest8, pdata8, bpp * width / 8 );
            pdest8 += buf->stride;
            pdata8 += bpp * width / 8 ;
        }
        free(data);
    }
    else
    {
        printf("Image data generating failed.\n");
    }

    return VG_LITE_SUCCESS;
}

static void output_string(char *str)
{
    strcat (LogString, str);
    printf("%s", str);
}

static vg_lite_error_t Allocate_Buffer(vg_lite_buffer_t *buffer,
                                       vg_lite_buffer_format_t format,
                                       int32_t width, int32_t height)
{
    vg_lite_error_t error;
    /* There need to test multiple formats,so align with the mininum format width */
    memset(buffer,0,sizeof(vg_lite_buffer_t));
    buffer->width = ALIGN(width, 128);
    buffer->height = height;
    buffer->format = format;
    buffer->stride = 0;
    buffer->handle = NULL;
    buffer->memory = NULL;
    buffer->address = 0;
    buffer->tiled   = 0;

    CHECK_ERROR(vg_lite_allocate(buffer));

ErrorHandler:
    return error;
}

static void Free_Buffer(vg_lite_buffer_t *buffer)
{
    vg_lite_free(buffer);
}

/*  test matrix operation   */
vg_lite_error_t Matrix_Operation(int32_t pathdata[],int32_t length,function_t func,int8_t operationFlag)
{
    vg_lite_buffer_t src_buf,dst_buf;
    int32_t    src_width, src_height, dst_width, dst_height;
    vg_lite_error_t error = VG_LITE_SUCCESS;
    vg_lite_color_t color;
    uint8_t    r, g, b, a;
    vg_lite_path_t path;
    vg_lite_matrix_t matrix,*matrix1;
    vg_lite_float_t sx, sy, tx, ty, degrees, w0, w1;
    vg_lite_point4_t src;
    vg_lite_point4_t dst;

    vg_lite_linear_gradient_t grad;
    uint32_t ramps[] = {0xff000000, 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff};
    uint32_t stops[] = {0, 66, 122, 200, 255};

    memset(&src_buf,0,sizeof(vg_lite_buffer_t));
    memset(&dst_buf,0,sizeof(vg_lite_buffer_t));
    matrix1 = (vg_lite_matrix_t*)malloc(sizeof(vg_lite_matrix_t));

    if(func == gradient) {
        memset(&grad, 0, sizeof(grad));
        if (VG_LITE_SUCCESS != vg_lite_init_grad(&grad)) {
            printf("Linear gradient is not supported.\n");
            vg_lite_close();
        }

        vg_lite_set_grad(&grad, 5, ramps, stops);
        vg_lite_update_grad(&grad);
        matrix1 = vg_lite_get_grad_matrix(&grad);
    }
    if(func == gradient || func == pattern || func == draw) {
        if( func == draw) {
            r = (uint8_t)Random_r(0.0f, 255.0f);
            g = (uint8_t)Random_r(0.0f, 255.0f);
            b = (uint8_t)Random_r(0.0f, 255.0f);
            a = (uint8_t)Random_r(0.0f, 255.0f);
            color = r | (g << 8) | (b << 16) | (a << 24);
        }
        memset(&path, 0, sizeof(path));
        vg_lite_init_path(&path, VG_LITE_S32, VG_LITE_HIGH, length, pathdata, -WINDSIZEX, -WINDSIZEY, WINDSIZEX, WINDSIZEY);
    }

    if(func == blit || func == pattern) {
        /* allocate src buffer */
        src_width = WINDSIZEX;
        src_height = WINDSIZEY;
        CHECK_ERROR(Allocate_Buffer(&src_buf, VG_LITE_RGBA8888, src_width, src_height));
        /* Regenerate src buffers. Check is good for transformation. */
        CHECK_ERROR(gen_buffer(0, &src_buf, VG_LITE_RGBA8888, src_buf.width, src_buf.height));
    }

    dst_width = WINDSIZEX;
    dst_height = WINDSIZEY;
    /* allocate dst buffer */
    CHECK_ERROR(Allocate_Buffer(&dst_buf, VG_LITE_BGRA8888, dst_width, dst_height));

    if(operationFlag & SCALE_FLAG) {
        sx = (vg_lite_float_t)Random_r(-2.0f, 2.0f);
        sy = (vg_lite_float_t)Random_r(-2.0f, 2.0f);
    }
    if(operationFlag & TRANSLATE_FLAG) {
        tx = (vg_lite_float_t)dst_buf.width / 2.0f;
        ty = (vg_lite_float_t)dst_buf.height/ 2.0f;
    }
    if(operationFlag & ROTATE_FLAG) {
        degrees = (vg_lite_float_t)Random_r(-360.0f, 360.0f);
    }

    vg_lite_identity(&matrix);
    vg_lite_translate(tx, ty, &matrix);
    vg_lite_rotate(degrees, &matrix);
    vg_lite_scale(sx, sy, &matrix);
    if(func == blit) {
        if(operationFlag & ROTATE_FLAG) {
            w0 = (vg_lite_float_t)Random_r(0.001f, 0.01f);
            w1 = (vg_lite_float_t)Random_r(0.001f, 0.01f);
        }

        src[0].x = 0.0;
        src[0].y = 0.0;
        src[1].x = src_buf.width;
        src[1].y = 0.0;
        src[2].x = src_buf.width;
        src[2].y = src_buf.height;
        src[3].x = 0.0;
        src[3].y = src_buf.height;

        dst[0].x = 0;
        dst[0].y = 0;
        dst[1].x = (int)(src_buf.width / (w0 * src_buf.width + 1) + 0.5);
        dst[1].y = 0;
        dst[2].x = (int)(src_buf.width / (w0 * src_buf.width + w1 * src_buf.height + 1) + 0.5);
        dst[2].y = (int)(src_buf.height / (w0 * src_buf.width + w1 * src_buf.height + 1) + 0.5);
        dst[3].x = 0;
        dst[3].y = (int)(src_buf.height / (w1 * src_buf.height + 1) + 0.5);

        //vg_lite_perspective(w0, w1, &matrix);
        vg_lite_get_transform_matrix(src, dst, &matrix);
    }

    if(func == pattern || func == gradient) {
        if(operationFlag & SCALE_FLAG) {
            sx = (vg_lite_float_t)Random_r(-2.0f, 2.0f);
            sy = (vg_lite_float_t)Random_r(-2.0f, 2.0f);
        }
        if(operationFlag & ROTATE_FLAG) {
            degrees = (vg_lite_float_t)Random_r(-360.0f, 360.0f);
        }
        if(operationFlag & ROTATE_FLAG) {
            w0 = (vg_lite_float_t)Random_r(0.001f, 0.01f);
            w1 = (vg_lite_float_t)Random_r(0.001f, 0.01f);
        }

        vg_lite_identity(matrix1);
        vg_lite_translate(tx, ty, matrix1);
        vg_lite_rotate(degrees, matrix1);
        vg_lite_scale(sx, sy, matrix1);

        src[0].x = 0.0;
        src[0].y = 0.0;
        src[1].x = src_buf.width;
        src[1].y = 0.0;
        src[2].x = src_buf.width;
        src[2].y = src_buf.height;
        src[3].x = 0.0;
        src[3].y = src_buf.height;

        dst[0].x = 0;
        dst[0].y = 0;
        dst[1].x = (int)(src_buf.width / (w0 * src_buf.width + 1) + 0.5);
        dst[1].y = 0;
        dst[2].x = (int)(src_buf.width / (w0 * src_buf.width + w1 * src_buf.height + 1) + 0.5);
        dst[2].y = (int)(src_buf.height / (w0 * src_buf.width + w1 * src_buf.height + 1) + 0.5);
        dst[3].x = 0;
        dst[3].y = (int)(src_buf.height / (w1 * src_buf.height + 1) + 0.5);

        //vg_lite_perspective(w0, w1, matrix1);
        vg_lite_get_transform_matrix(src, dst, &matrix);
    }

    CHECK_ERROR(vg_lite_clear(&dst_buf, NULL, 0xFFFFFFFF));
    switch (func)
    {
    case draw:
        vg_lite_draw(&dst_buf,&path, VG_LITE_FILL_EVEN_ODD,&matrix,VG_LITE_BLEND_NONE,color);
        if (error)
        {
            printf("[%s: %d]vg_lite_draw failed.error type is %s\n", __func__, __LINE__,error_type[error]);
            Free_Buffer(&dst_buf);
            return error;
        }
        CHECK_ERROR(vg_lite_finish());
        SaveBMP_SFT("Draw_Matrix_Operation", &dst_buf, TRUE);
        break;
    case blit:
        CHECK_ERROR(vg_lite_blit(&dst_buf, &src_buf, &matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT));
        CHECK_ERROR(vg_lite_finish());
        SaveBMP_SFT("Blit_Matrix_Operation", &dst_buf, TRUE);
        Free_Buffer(&src_buf);
        break;
    case gradient:
        vg_lite_draw_grad(&dst_buf, &path, VG_LITE_FILL_EVEN_ODD, &matrix, &grad, VG_LITE_BLEND_NONE);
        if (error)
        {
            printf("[%s: %d]vg_lite_draw_grad failed.error type is %s\n", __func__, __LINE__,error_type[error]);
            Free_Buffer(&dst_buf);
            vg_lite_clear_grad(&grad);
            return error;
        }
        CHECK_ERROR(vg_lite_finish());
        SaveBMP_SFT("Gradient_Matrix_Operation", &dst_buf, TRUE);
        vg_lite_clear_grad(&grad);
        break;
    case pattern:
        CHECK_ERROR(vg_lite_draw_pattern(&dst_buf, &path, VG_LITE_FILL_EVEN_ODD, &matrix, &src_buf, matrix1, VG_LITE_BLEND_NONE, VG_LITE_PATTERN_COLOR, 0xffaabbcc, VG_LITE_FILTER_POINT));
        if (error)
        {
            printf("[%s: %d]vg_lite_blit failed.error type is %s\n", __func__, __LINE__,error_type[error]);
            Free_Buffer(&dst_buf);
            Free_Buffer(&src_buf);
            return error;
        }
        CHECK_ERROR(vg_lite_finish());
        SaveBMP_SFT("Pattern_Matrix_Operation", &dst_buf, TRUE);
        Free_Buffer(&src_buf);
    default:
        break;
    }
    Free_Buffer(&dst_buf);
    return VG_LITE_SUCCESS;

ErrorHandler:
    if(src_buf.handle != NULL)
        Free_Buffer(&src_buf);
    if(dst_buf.handle != NULL)
        Free_Buffer(&dst_buf);
    vg_lite_clear_grad(&grad);
    return error;
}
/*  test path matrix operation   */
vg_lite_error_t Path_Matrix_Operation()
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    int i;
    int flag;
    flag |= TRANSLATE_FLAG;
    flag |= ROTATE_FLAG;
    flag |= SCALE_FLAG;
    for (i = 0; i < OPERATE_COUNT; i++) {
        CHECK_ERROR(Matrix_Operation(test_path[i],length[i],draw,flag));
    }

ErrorHandler:
    return error;
}

/*  test blit matrix operation   */
vg_lite_error_t Blit_Matrix_Operation()
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    int i;
    int flag;
    flag |= TRANSLATE_FLAG;
    flag |= ROTATE_FLAG;
    flag |= SCALE_FLAG;
    flag |= PERSPECTIVE_FLAG;
    for (i = 0; i < OPERATE_COUNT; i++) {
        CHECK_ERROR(Matrix_Operation((int32_t*)NULL,(int32_t)NULL,blit,flag));
    }

ErrorHandler:
    return error;
}

/*  test  Gradient matrix operation   */
vg_lite_error_t Gradient_Matrix_Operation()
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    int i;
    int flag;
    flag |= TRANSLATE_FLAG;
    flag |= ROTATE_FLAG;
    flag |= SCALE_FLAG;
    flag |= PERSPECTIVE_FLAG;
    for (i = 0; i < OPERATE_COUNT; i++) {
        CHECK_ERROR(Matrix_Operation(test_path[i],length[i],gradient,flag));
    }

ErrorHandler:
    return error;
}

/*  test  Pattern matrix operation   */
vg_lite_error_t Pattern_Matrix_Operation()
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    int i;
    int flag;
    flag |= TRANSLATE_FLAG;
    flag |= ROTATE_FLAG;
    flag |= SCALE_FLAG;
    flag |= PERSPECTIVE_FLAG;
    for (i = 0; i < OPERATE_COUNT; i++) {
        CHECK_ERROR(Matrix_Operation(test_path[i],length[i],pattern,flag));
    }

ErrorHandler:
    return error;
}

vg_lite_error_t Matrix()
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    output_string("\nCase: Path_Matrix_Operation:::::::::Started\n");
    CHECK_ERROR(Path_Matrix_Operation());
    output_string("\nCase: Path_Matrix_Operation:::::::::Ended\n");

    output_string("\nCase: Blit_Matrix_Operation:::::::::Started\n");
    CHECK_ERROR(Blit_Matrix_Operation());
    output_string("\nCase: Blit_Matrix_Operation:::::::::Ended\n");

    output_string("\nCase: Gradient_Matrix_Operation:::::::::Started\n");
    CHECK_ERROR(Gradient_Matrix_Operation());
    output_string("\nCase: Gradient_Matrix_Operation:::::::::Ended\n");

    output_string("\nCase: Pattern_Matrix_Operation:::::::::Started\n");
    CHECK_ERROR(Pattern_Matrix_Operation());
    output_string("\nCase: Pattern_Matrix_Operation:::::::::Ended\n");

ErrorHandler:
    return error;
}

/* ***
* Logging entry
*/
void Matrix_Log()
{

}