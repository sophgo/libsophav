//-----------------------------------------------------------------------------
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//                    (C) COPYRIGHT 2003 - 2020 CHIPS&MEDIA INC.
//                           ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//     copies.
//
// File         : host_dec_stream.c
// Author       : Jeff Oh
// Description  : 
//
// Revision     : 0.9 07/15, 2021        initial
//-----------------------------------------------------------------------------

#include "com.h"
#include "host_dec.h"
#include "../../misc/debug.h"
#include "vpuconfig.h"
#include "vpuapi.h"

typedef enum
{
    AV1_OBU_RESERVED_00 = 0,
    AV1_OBU_SEQUENCE_HEADER = 1,
    AV1_OBU_TEMPORAL_DELIMITER = 2,
    AV1_OBU_FRAME_HEADER = 3,
    AV1_OBU_TILE_GROUP = 4,
    AV1_OBU_METADATA = 5,
    AV1_OBU_FRAME = 6,
    AV1_OBU_REDUNDANT_FRAME_HEADER = 7,
    AV1_OBU_TILE_LIST = 8,
    AV1_OBU_RESERVED_09 = 9,
    AV1_OBU_RESERVED_10 = 10,
    AV1_OBU_RESERVED_11 = 11,
    AV1_OBU_RESERVED_12 = 12,
    AV1_OBU_RESERVED_13 = 13,
    AV1_OBU_RESERVED_14 = 14,
    AV1_OBU_PADDING = 15,
    AV1_OBU_DROP = 16,
} av1_nal_type_t;


static const unsigned int  bitMap[32] = {
0x1         , 0x3           , 0x7           , 0xf,
0x1f        , 0x3f          , 0x7f          , 0xff, 
0x1ff       , 0x3ff         , 0x7ff         , 0xfff, 
0x1fff      , 0x3fff        , 0x7fff        , 0xffff,
0x1ffff     , 0x3ffff       , 0x7ffff       , 0xfffff, 
0x1fffff    , 0x3fffff      , 0x7fffff      , 0xffffff,   
0x1ffffff   , 0x3ffffff     , 0x7ffffff     , 0xfffffff,
0x1fffffff  , 0x3fffffff    , 0x7fffffff    , 0xffffffff,      
};

//static int  data        = 0;
//static int  remain_bits = 0;

//static void s_consume_byte(int n);
static int  s_get_byte(FILE* fp);
static int  s_get_bits(host_dec_internal_data* dec_data, FILE* fp, int bits);
static int  s_show_byte(FILE* fp);
static int  s_get_av1_obu_size(host_dec_internal_data* dec_data, FILE* fp, int *p_used_byte);
static void s_get_av1_sps(host_dec_internal_data* dec_data, FILE* fp);

//static FILE *s_fp_stream;
//static int  s_end_of_stream; --> eos in host_bsfeeder_framesize_impl.c

//static int s_vp9_superframe_en; // --> context data in host_bsfeeder_framesize_impl.c
//static int s_vp9_superframe_idx;
//static int s_vp9_superframe_num;
//static int s_vp9_superframe_size[8];
//static int s_consume_superframe_header_flag;
//static int s_super_frame_header_size;

//static int remain_size = 0;
//static int temporal_id = 0;
//static int spatial_id = 0;
//static int reduced_still_pic_header = 0;
//static int operatingPointIdc = 0;

//static int  s_av1_temp_unit_size;           // for ANNEX B

#define IS_VP9_SUPERFRAME(__header)     ((__header & 0xe0) == 0xc0)

int  host_dec_start_code(host_dec_internal_data* dec_data, BYTE frame_cpb_buf[], int base_addr_cpb, FILE *fp)
{
    int sta_pos, size = 0;
    int temp_data, byte_data, byte_data_3;
    //int  nal_ref_idc;
    int nal_unit_type, nal_layer_id;
    int lead_zero_byte_cnt;                    // AVC, HEVC
    int prev_pic_end_pos = 0;                      // for HEVC only
    int frame_header_found;
    //int  prev_nal_unit_type;
    int obu_type, obu_size = 0;
    //int  obu_size_byte;
    int show_existing_frame;
    int obu_extension_flag, obu_has_size_field;
    int obu_drop_flag;
    int skip_frame;
    int leb128byte, size_byte_cnt, i;
    int frame_size;            // annex_b
    int codec_std;
    int stream_format;
#ifdef CNM_DPI_SIM
    int addr_128;
    int addr_32;
    int wdata32[4];
    int result;
#endif

    if (!dec_data) {
        VLOG(ERR, "%s:%d Null pointer exception \n", __FUNCTION__, __LINE__);
        return -1;
    }
    
    if (dec_data->end_of_stream == 1)
        return 0;

    codec_std = dec_data->host_std_idx;
    stream_format = dec_data->stream_format;
    //s_fp_stream = fp;
    
    skip_frame      = 0;
    obu_drop_flag   = 0;
    frame_size = 0;

    if (dec_data->init_flag == 1)
        fseek(fp, 0, SEEK_SET);

    frame_header_found = 0;
    lead_zero_byte_cnt = 0;
    byte_data_3 = 0;
    sta_pos = ftell(fp);

    if (codec_std == STD_AVC_DEC || codec_std == STD_HEVC_DEC)
        byte_data_3 = (s_get_byte(fp) << 8) | s_get_byte(fp);

    if ((codec_std == STD_VP9_DEC || codec_std == STD_AV1_DEC) && dec_data->init_flag == 1 && stream_format == STREAM_FORMAT_IVF)
    {
        sta_pos += 32;
        fseek(fp, 32, SEEK_SET);   // IVF FILE header
    }

    if (dec_data->consume_superframe_header_flag)
    {
        sta_pos += (dec_data->super_frame_header_size);
        fseek(fp, dec_data->super_frame_header_size, SEEK_CUR);   // IVF FILE header
    }

    while (1) {        
        byte_data = s_get_byte(fp);
        if (codec_std == STD_AV1_DEC && stream_format == STREAM_FORMAT_IVF)
        {
            if (dec_data->remain_size != 0)
                dec_data->remain_size -= 1;
        }
        if (byte_data == EOF) {
            if (frame_header_found == 1 )//|| obu_drop_flag == 1)
                break;
            else
                return 0;
        }

        // AV1
        if (codec_std == STD_AV1_DEC) {
            if (stream_format == STREAM_FORMAT_IVF || stream_format == STREAM_FORMAT_AV1_ANNEXB)
            {
                if (dec_data->remain_size == 0)
                {
                    if (stream_format == STREAM_FORMAT_IVF) {
                        if(skip_frame == 0)
                            sta_pos += 12;
                        obu_size = byte_data;//(s_get_byte() & 0x7F) <<  0;
                        obu_size |= (s_get_byte(fp) & 0xFF) << 8;
                        obu_size |= (s_get_byte(fp) & 0xFF) << 16;
                        obu_size |= (s_get_byte(fp) & 0xFF) << 24;
                        dec_data->remain_size = obu_size;
                        fseek(fp, 8, SEEK_CUR);
                        continue;
                    } else {
                        // temp unit size
                        fseek(fp, -1, SEEK_CUR);       // 1st byte data
                        dec_data->remain_size = s_get_av1_obu_size(dec_data, fp, &size_byte_cnt);
                        // frame unit unit size
                        frame_size = s_get_av1_obu_size(dec_data, fp, &size_byte_cnt);
                        if (frame_size == 1687)
                            frame_size = frame_size;
                        continue;
                    }
                }
                else
                {
                    if (stream_format == STREAM_FORMAT_AV1_ANNEXB) {
                        fseek(fp, -1, SEEK_CUR);       // 1st byte data
                        if (frame_size == 0)
                            frame_size = s_get_av1_obu_size(dec_data, fp, &size_byte_cnt);
                        obu_size = s_get_av1_obu_size(dec_data, fp, &size_byte_cnt);
                        frame_size -= (size_byte_cnt + obu_size);
                        byte_data = s_get_byte(fp);
                        obu_size -= 1;
                        if (frame_size < 0)
                            VLOG(ERR, "Annex B frame_unit_size err\n");
                        dec_data->remain_size -= 1;
                    }
                    if ((byte_data & 0x81) != 0)
                        continue;
                    
                    obu_type = (byte_data >> 3) & 0xF;
                    //obu_size_byte = (obu_type == 6) ? 4 : 1;        // OBU_FRAME --> 4byte
                    obu_extension_flag = (byte_data >> 2) & 0x1;
                    obu_has_size_field = (byte_data >> 1) & 0x1;
                    dec_data->temporal_id = 0;
                    dec_data->spatial_id = 0;
                    if (obu_extension_flag)
                    {
                        byte_data = s_get_byte(fp); // obu extension header                        
                        dec_data->temporal_id = ((byte_data >> 5) & 0x7);
                        dec_data->spatial_id  = ((byte_data >> 3) & 0x3);
                        dec_data->remain_size -= 1;
                        if (stream_format == STREAM_FORMAT_AV1_ANNEXB)
                            obu_size -= 1;
                    }

                    if (stream_format == STREAM_FORMAT_AV1_ANNEXB && obu_has_size_field == 1) {
                        int  new_obu_size;
                        // obu size
                        new_obu_size = s_get_av1_obu_size(dec_data, fp, &size_byte_cnt);
                        obu_size -= size_byte_cnt;      // should be same with new_obu_size
                        if (new_obu_size) {}
                    }

                    if (stream_format == STREAM_FORMAT_IVF)
                        obu_size = s_get_av1_obu_size(dec_data, fp, &size_byte_cnt);                                        

                    if (obu_type != AV1_OBU_SEQUENCE_HEADER && obu_type != AV1_OBU_TEMPORAL_DELIMITER && dec_data->operatingPointIdc != 0 && obu_extension_flag == 1)
                    {
                        if (!((dec_data->operatingPointIdc >> dec_data->temporal_id) & 0x1) || !((dec_data->operatingPointIdc >> (dec_data->spatial_id + 8)) & 0x1))
                            obu_drop_flag = 1;     
                        else
                            obu_drop_flag = 0;                                                    
                    }
                    else if (obu_drop_flag == 1)
                    {
                        obu_drop_flag = 0;
                    }

                    if (obu_drop_flag == 1 && stream_format == STREAM_FORMAT_IVF) {
                        fseek(fp, obu_size, SEEK_CUR);
                        dec_data->remain_size -= obu_size;
                        sta_pos = ftell(fp);
                        //print_msg("OBU DROP in host\n");
                        continue;
                    }
                    if (obu_drop_flag == 1) {
                        //printf("ANNEX B OBU DROP in host [type=%d, size=%d]\n", obu_type, obu_size);
                        if (obu_size == 324)
                            obu_size =obu_size;
                        obu_type = AV1_OBU_DROP;
                    }

                    if (obu_type == AV1_OBU_FRAME)
                    {
                        fseek(fp, obu_size, SEEK_CUR);
                        dec_data->remain_size -= obu_size;
                        frame_header_found = 1;                        
                        break;
                    }
                    else if (obu_type == AV1_OBU_FRAME_HEADER)
                    {
                        temp_data = s_get_byte(fp);
                        dec_data->remain_size -= 1;
                        show_existing_frame = (temp_data & 0x80) >> 7;
                        obu_size -= 1;
                        dec_data->remain_size -= obu_size;                  
                        fseek(fp, obu_size, SEEK_CUR);
                        if (stream_format == STREAM_FORMAT_AV1_ANNEXB && dec_data->remain_size == 0) {
                            frame_header_found = 1;
                            break;
                        }

                        show_existing_frame = dec_data->reduced_still_pic_header == 1 ? 0 : show_existing_frame;                       
                        if (show_existing_frame == 1)
                        {
                            if (obu_drop_flag == 1)                            
                                skip_frame = 1;                            
                            else
                            {
                                frame_header_found = 1;
                                break;
                            }
                        }
                        else
                            continue;
                    }
                    else if (obu_type == AV1_OBU_SEQUENCE_HEADER)
                    {
                        s_get_av1_sps(dec_data, fp);
                        fseek(fp, obu_size, SEEK_CUR);
                        dec_data->remain_size -= obu_size;                        
                        continue;
                    }
                    else if (obu_type == AV1_OBU_TILE_GROUP)
                    {
                        fseek(fp, obu_size, SEEK_CUR);
                        dec_data->remain_size -= obu_size;

                        temp_data = 0;
                        if (dec_data->remain_size != 0)
                        {

                            if (stream_format == STREAM_FORMAT_IVF) {
                                temp_data = s_show_byte(fp);
                                temp_data = (temp_data >> 3) & 0xF;
                            } else
                                temp_data = 0;               // ANNEX B
                        }

                        if ((dec_data->remain_size == 0 || temp_data == AV1_OBU_TEMPORAL_DELIMITER || temp_data == AV1_OBU_FRAME || temp_data == AV1_OBU_FRAME_HEADER))
                        {
                            if (obu_drop_flag == 1)
                                skip_frame = 1;
                            else
                            {
                                frame_header_found = 1;
                                break;
                            }
                        }
                        if (stream_format == STREAM_FORMAT_AV1_ANNEXB && frame_size == 0) {
                            frame_header_found = 1;
                            break;
                        }
                        continue;                        
                    }                
                    else
                    {
                        fseek(fp, obu_size, SEEK_CUR);
                        dec_data->remain_size -= obu_size;
                        continue;
                    }
                }
            }
            else
            {
                if ((byte_data & 0x81) != 0)
                    continue;
                obu_type = (byte_data >> 3) & 0xF;
                //obu_size_byte = (obu_type == 6) ? 4 : 1;        // OBU_FRAME --> 4byte
                if (obu_type == 6) {
                    obu_size = 0;
                    for (i = 0; i < 8; i++)
                    {
                        leb128byte = s_get_byte(fp);
                        obu_size |= ((leb128byte & 0x7f) << (i * 7));
                        if (!(leb128byte & 0x80))
                            break;
                    }
                    fseek(fp, obu_size, SEEK_CUR);
                    frame_header_found = 1;
                    break;
                }
                else if (obu_type == 3)
                {
                    obu_size = (s_get_byte(fp) & 0x7F) << 0;
                    fseek(fp, obu_size, SEEK_CUR);
                    frame_header_found = 1;
                    break;
                }
                else if (obu_type == 1) {
                    obu_size = s_get_byte(fp);
                    fseek(fp, obu_size, SEEK_CUR);
                }
                continue;
            }
        }
        else if (codec_std == STD_VP9_DEC)
        {
            long long time_stamp; 
            //const int size_byte = 4;
            const int time_byte = 8;
            int frame_start_pos;
            //static int s_vp9_superframe_marker_len;

            if (dec_data->vp9_superframe_en == 1) 
            {
                fseek(fp, -1, SEEK_CUR);
                size = dec_data->vp9_superframe_size[dec_data->vp9_superframe_idx];

                (dec_data->vp9_superframe_idx)++;
                if (dec_data->vp9_superframe_idx == dec_data->vp9_superframe_num) 
                {
                    dec_data->vp9_superframe_en = 0;
                    dec_data->consume_superframe_header_flag = 1;
                }
                break;
            }

            dec_data->consume_superframe_header_flag = 0;

            obu_size  = byte_data;//(s_get_byte() & 0x7F) <<  0;
            obu_size |= (s_get_byte(fp) & 0xFF) <<  8;
            obu_size |= (s_get_byte(fp) & 0xFF) << 16;
            obu_size |= (s_get_byte(fp) & 0xFF) << 24;

            fread( &time_stamp, 1, time_byte, fp);
            sta_pos = ftell(fp);             // ivf frame header
            size = obu_size;

            // check superframe
            frame_start_pos = ftell(fp);
            fseek(fp, (obu_size) -1 , SEEK_CUR);
            temp_data = s_get_byte(fp);
            if (IS_VP9_SUPERFRAME(temp_data))
            {
                int  marker;
                int  frame_size_len, num_frame;
                int  vp9_frame_size, vp9_idx;
                int  total_superframe_size = 0; 
                marker = temp_data;
                frame_size_len = ((temp_data >> 3) & 0x3) + 1;
                num_frame      = ((temp_data >> 0) & 0x7) + 1;
                fseek(fp, -(frame_size_len * num_frame + 2), SEEK_CUR);
                temp_data = s_get_byte(fp);        // marker
                if (marker != temp_data)
                    VLOG(ERR,"Super frame marker error\n");
                for (vp9_idx=0; vp9_idx<num_frame; vp9_idx++) {
                    vp9_frame_size = s_get_byte(fp);
                    if (frame_size_len >= 2)
                        vp9_frame_size |= s_get_byte(fp) << 8;
                    if (frame_size_len >= 3)
                        vp9_frame_size |= s_get_byte(fp) << 16;
                    if (frame_size_len >= 4)
                        vp9_frame_size |= s_get_byte(fp) << 24;
                    dec_data->vp9_superframe_size[vp9_idx] = vp9_frame_size;
                    total_superframe_size += vp9_frame_size;
                }
                temp_data = s_get_byte(fp); // marker

                if (temp_data) {
                    //
                }

                dec_data->vp9_superframe_en = 1;
                dec_data->vp9_superframe_idx = 1;
                dec_data->vp9_superframe_num = num_frame;
                size = dec_data->vp9_superframe_size[0];
                fseek(fp, frame_start_pos, 0);

                // clac superframe header bytes
                if (obu_size >= total_superframe_size)
                {
                    dec_data->super_frame_header_size = obu_size - total_superframe_size;
                }
                else
                {
                    VLOG(ERR,"TOTAL SIZE OF SUBFRAMES IS BIGGER THAN CHUNK SIZE");
                }
            }
            //if (init_flag == 0)
            break;
        }
        else //AVC HEVC
        {
            byte_data_3 = ((byte_data_3 & 0xFFFF) << 8) | byte_data;
            if (byte_data_3 == 0x000001) {
                byte_data = s_get_byte(fp);
                if (byte_data == EOF) {
                    dec_data->end_of_stream = 1;
                    return 0;
                }
                // AVC
                if (codec_std == STD_AVC_DEC) {
                    //nal_ref_idc   = (byte_data >> 5) & 3;
                    nal_unit_type = byte_data & 0x1F;
                    if (nal_unit_type == 1 || nal_unit_type == 5) {         // 1(non-IDR), 5(IDR)
                        if (frame_header_found == 1) {// && nal_unit_type == prev_nal_unit_type) {
                            // check first slice : mb_addr = 0 ===> next picture
                            byte_data = s_get_byte(fp);
                            if (byte_data & 0x80) {                         // ue : first_mb_in_slice
                                fseek(fp, -(5+lead_zero_byte_cnt), SEEK_CUR);
                                break;
                            }
                        } else {
                            //prev_nal_unit_type = nal_unit_type;
                            frame_header_found = 1;
                        }
                    } else if (nal_unit_type >= 6 && nal_unit_type <= 9) {
                        // NAL type : SEI(6), SPS(7), PPS(8), AUD(9) ==> next picture
                        if (frame_header_found == 1) {
                            fseek(fp, -(4+lead_zero_byte_cnt), SEEK_CUR);
                            break;
                        }
                    }
                }
                // HEVC
                if (codec_std == STD_HEVC_DEC) 
                {
                    nal_unit_type = (byte_data >> 1) & 0x3F;

                    if (nal_unit_type <= 21)
                    {
                        if (frame_header_found == 1)
                        {
                            // check first slice
                            byte_data = s_get_byte(fp);
                            byte_data = s_get_byte(fp);
                            // FUNNY : VCL(1st) - SPS - VCL(last) - VCL(1st)
                            if (byte_data & 0x80) { // ue : first_mb_in_slice
                                if (prev_pic_end_pos == 0) {            //no SPS,PPS, ... in frame end
                                    fseek(fp, -(6+lead_zero_byte_cnt), SEEK_CUR);
                                } else {
                                    fseek(fp, prev_pic_end_pos, SEEK_SET);
                                }
                                break;
                            } else {
                                prev_pic_end_pos = 0;                   // for FUNNY case (180, 190, 200 frame)
                            }
                        }
                        else
                        {
                            //prev_nal_unit_type = nal_unit_type;
                            frame_header_found = 1;
                            prev_pic_end_pos = 0;
                        }
                    } else if ((nal_unit_type >= 32 && nal_unit_type <= 35)) {// || (nal_unit_type == 39)) {
                        // NAL type : VPS(32), SPS(33), PPS(34), AUD(35), PREFIX_SEI(39) ==> next picture
                        nal_layer_id = (byte_data & 1) << 5;
                        byte_data = s_get_byte(fp);
                        nal_layer_id |= (byte_data >> 3) & 0x1F;
                        // Allegro_HEVC_Main_HT50_SEI_00_192x200@60Hz_2.3.bin
                        //      330 frame, PREFIX_SEI exist inside picture (stream error ?)
                        // Allegro_HEVC_Main_HT50_FUNNY_00_192x200@60Hz_2.9.bin
                        //      180 frame, SPS exist inside picture (stream error ?)
                        //      190 frame, PPS exist inside picture (stream error ?)
                        if (frame_header_found == 1 && nal_layer_id == 0) {
                            if (prev_pic_end_pos == 0)
                                prev_pic_end_pos = ftell(fp) - (5+lead_zero_byte_cnt);
                        }
                    }
                }
            } else {
                if (byte_data_3 == 0x000000)
                    lead_zero_byte_cnt++;
                else
                    lead_zero_byte_cnt = 0;
            }
        }
    }

    if (frame_header_found) {

    }

    if (codec_std != STD_VP9_DEC)
    {
        size = ftell(fp) - sta_pos;
    }
    //if (0 && codec_std == STD_VP9_DEC)// && init_flag == 1) // lint error --> Warning 506: Constant value Boolean
    //    size = size + 8;
    fseek(fp, sta_pos, SEEK_SET);
    if (size > CPB_SIZE)
        VLOG(ERR,"FRAME byte size is too large [%d] MB\n", size/1024/1024);
    fread(frame_cpb_buf, size, 1, fp);

    return size;
}

/*
static void s_consume_byte(int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        s_get_byte();
    }
}
*/

static int s_get_byte(FILE* fp)
{
    int  ch;
    ch = fgetc(fp);
    return ch;
}

static int s_get_bits(host_dec_internal_data* dec_data, FILE* fp, int bits)
{
    int rtn = 0;
    int re_bits;

    if (dec_data->remain_bits == 0)
    {
        dec_data->data = 0;
        dec_data->data = fgetc(fp);
        dec_data->data = (dec_data->data << 8) | fgetc(fp);
        dec_data->data = (dec_data->data << 8) | fgetc(fp);
        dec_data->data = (dec_data->data << 8) | fgetc(fp);        
        dec_data->remain_bits = 32;
    }
    
    if (dec_data->remain_bits < bits)
    {
        rtn = dec_data->data & bitMap[dec_data->remain_bits - 1];
        re_bits = bits - dec_data->remain_bits;
        
        dec_data->data = 0;
        dec_data->data = fgetc(fp);
        dec_data->data = (dec_data->data << 8) | fgetc(fp);
        dec_data->data = (dec_data->data << 8) | fgetc(fp);
        dec_data->data = (dec_data->data << 8) | fgetc(fp);
        dec_data->remain_bits = 32;

        rtn = (rtn << re_bits) | ((dec_data->data >> (dec_data->remain_bits - re_bits)) & bitMap[re_bits - 1]);
        dec_data->remain_bits -= re_bits;
    }
    else
    {
        rtn = ((dec_data->data >> (dec_data->remain_bits - bits)) & bitMap[bits - 1]);
        dec_data->remain_bits -= bits;
    }

    return rtn;
}

static int  s_show_byte(FILE* fp)
{
    int  ch;
    ch = fgetc(fp);
    fseek(fp, -1, SEEK_CUR);
    return ch;
}


static int  s_get_av1_obu_size(host_dec_internal_data* dec_data, FILE* fp, int *p_used_byte)
{
    int leb128byte, i;
    int size_byte = 0;
    int obu_size = 0;
    for (i = 0; i < 8; i++)
    {
        dec_data->remain_size -= 1;       
        leb128byte = s_get_byte(fp);
        size_byte++;
        obu_size |= ((leb128byte & 0x7f) << (i * 7));
        if (!(leb128byte & 0x80))
            break;
    }
    *p_used_byte = size_byte;
    return obu_size;

}

static void  s_get_av1_sps(host_dec_internal_data* dec_data, FILE* fp)
{       
    int pos;
    
    int leadingZeros, done, i;

    int temp = 0;
    int timing_info_present_flag = 0;
    int buffer_delay_length_minus_1 = 0;
    int decoder_model_info_present_flag = 0;
    int initial_display_delay_present_flag = 0;
    int operating_points_cnt_minus_1 = 0;
    int operating_point_idc[32] = { 0, };
    int seq_level_idx[32] = { 0, };
    int operatingPoint = 0;

    osal_memset(operating_point_idc, 0x00, sizeof(int)*32);

    pos = ftell(fp); // for rewind 
    dec_data->remain_bits = 0;
    //remain_bits = 0;

    s_get_bits(dec_data, fp, 3); // seq_profile    
    s_get_bits(dec_data, fp, 1); // still_picture    
    dec_data->reduced_still_pic_header = s_get_bits(dec_data, fp, 1); // reduced_still_pic_header
    
    if (dec_data->reduced_still_pic_header)
    {
        s_get_bits(dec_data, fp, 5); // seq_level_idx    
    }
    else 
    {
        timing_info_present_flag = s_get_bits(dec_data, fp, 1);

        if (timing_info_present_flag == 1)
        {            
            s_get_bits(dec_data, fp, 32); // num_units_in_display_tick
            s_get_bits(dec_data, fp, 32); // time_scale
            temp = s_get_bits(dec_data, fp, 1); // equal_picture_interval
            if (temp == 1)
            {
                // uvlc() num_ticks_per_picture_minus_1
                leadingZeros = 0;
                while (1) {
                    done = s_get_bits(dec_data, fp, 1);
                    if (done)
                        break;
                    leadingZeros++;
                }

                if (leadingZeros < 32) 
                    s_get_bits(dec_data, fp, leadingZeros);
            }

            decoder_model_info_present_flag = s_get_bits(dec_data, fp, 1); // decoder_model_info_present_flag

            if (decoder_model_info_present_flag)
            {
                buffer_delay_length_minus_1 = s_get_bits(dec_data, fp, 5); // buffer_delay_length_minus_1
                s_get_bits(dec_data, fp, 32); // num_units_in_decoding_tick
                s_get_bits(dec_data, fp, 5); // buffer_removal_time_length_minus_1
                s_get_bits(dec_data, fp, 5); // frame_presentation_time_length_minus_1
            }
        }

        initial_display_delay_present_flag = s_get_bits(dec_data, fp, 1); // initial_display_delay_present_flag
        operating_points_cnt_minus_1 = s_get_bits(dec_data, fp, 5); // operating_points_cnt_minus_1

        for (i = 0; i <= operating_points_cnt_minus_1; i++)
        {
            operating_point_idc[i] = s_get_bits(dec_data, fp, 12); // operating_point_idc
            seq_level_idx[i] = s_get_bits(dec_data, fp, 5);  // seq_level_idx

            if (seq_level_idx[i] > 7)
            {
                s_get_bits(dec_data, fp, 1);  // seq_level_idx
            }

            if (decoder_model_info_present_flag)
            {
                temp = s_get_bits(dec_data, fp, 1); // decoder_model_present_for_this_op
                if (temp == 1)
                {
                    s_get_bits(dec_data, fp, buffer_delay_length_minus_1 + 1); // decoder_buffer_delay
                    s_get_bits(dec_data, fp, buffer_delay_length_minus_1 + 1); // encoder_buffer_delay
                    s_get_bits(dec_data, fp, 1); // low_delay_mode_flag
                }
            }

            if (initial_display_delay_present_flag == 1)
            {
                temp = s_get_bits(dec_data, fp, 1); // initial_display_delay_present_for_this_op
                if (temp)
                    s_get_bits(dec_data, fp, 4); // initial_display_delay_minus_1
            }
        }
    }

    //operatingPoint;
    dec_data->operatingPointIdc = operating_point_idc[operatingPoint];


    fseek(fp, pos, SEEK_SET);
}

