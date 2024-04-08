//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// 
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
// 
// The entire notice above must be reproduced on all authorized copies.
// 
// Description  : 
//-----------------------------------------------------------------------------

#include "main_helper.h"
#ifdef SUPPORT_FFMPEG_DEMUX
#include <libavformat/avformat.h>
#include <aom/aom_codec.h>

#define RCV_V2
#define IS_VP9_SUPERFRAME(__header)     ((__header & 0xe0) == 0xc0)
#define MAX_VP9_SUPERFRAME_INDEX_SIZE   34
#define VP9_MAX_SUBFRAMES               8

#define AV1_MAX_SUBFRAMES               8

typedef struct VP9Superframe {
    Uint32  nframes;
    Uint32  frameSize[VP9_MAX_SUBFRAMES];
    void*   frames[VP9_MAX_SUBFRAMES];   /* A superframe has multiple frames up to 8 frames. */
    Uint32  currentIndex;
} VP9Superframe;

typedef struct AV1SubFrame {
    Uint32  nframes;
    Uint32  frameSize[AV1_MAX_SUBFRAMES];
    void*   frames[AV1_MAX_SUBFRAMES];   /* up to 8 frames. */
    Uint32  currentIndex;
} AV1SubFrame;

static BOOL initFFmpeg;

typedef struct FeederFrameContext {
    AVFormatContext*    avContext;
    AVBSFContext*       avbsFilterContext;
    BOOL                isFirstPacket;
    Uint32              videoIndex;
    Uint32              standard;
    Uint32              mp4ClassId;
    Uint8*              tempBuffer;
    Uint32              tempRdPtr;
    Uint32              tempWrPtr;
    VP9Superframe       superframe;
    AV1SubFrame         av1Subframes;
} FeederFrameContext;

static Uint32 u_bytes(
    Uint8*  data,
    Uint32  len
    )
{
    Uint32  i;
    Uint32  val=0;

    for (i=0; i<len; i++) {
        val |= data[i] << (i*8);
    }

    return val;
}

#define AV1_LEB128_BYTE_CAL_MASK        0x7f
#define AV1_LEB128_BYTE_END_MASK        0x80

typedef struct ObuHeaderSyntax {
    Uint8   forbidden_bit;
    Uint8   obu_type;
    Uint8   obu_extension_flag;
    Uint8   obu_has_size_field;
} ObuHeaderSyntax;

static Uint32 av1_obu_header_parsing(Uint8 header, ObuHeaderSyntax* obuHeaderSyntax) 
{
    Uint8 header_type = 0;
    if (NULL != obuHeaderSyntax) {
        obuHeaderSyntax->obu_type = ((header >> 3) & 0x07);
        obuHeaderSyntax->obu_extension_flag = ((header >> 2) & 0x01);
        obuHeaderSyntax->obu_has_size_field = ((header >> 1) & 0x01);
        header_type = obuHeaderSyntax->obu_type;
    }
    return header_type;
}

static Uint32 av1_uleb_obu_size(Uint8* data, Uint32 *length) 
{
    Uint32 idx =0;
    Uint8 decoded_byte = 0;
    Uint64 value = 0;

    if (data && length) {
        *length = 0;
        for (idx=0; idx < 8; idx++) {
            decoded_byte = *(data+idx) ;
            value |= ((decoded_byte & AV1_LEB128_BYTE_CAL_MASK) << (idx * 7));
            if (!(*(data+idx) & AV1_LEB128_BYTE_END_MASK)) {
                *length = idx+1;
                break;
            }
        }
    }
    return value;
}
static Uint32 AV1Parser(AVPacket* avpacket, AV1SubFrame* av1SubFrame)
{
    Uint8* pData = NULL;
    Uint8* curData = NULL;
    Uint32 length = 0;
    Uint64 obu_frame_size = 0;
    Uint32 next_byte = 0;
    Uint32 fNumInTD = 0;
    BOOL lastFrame = FALSE;
    ObuHeaderSyntax obuHeaderSyntax;

    if (NULL != avpacket && NULL != av1SubFrame) {
        osal_memset(av1SubFrame, 0x00, sizeof(AV1SubFrame));
        pData = avpacket->buf->data;
        curData = pData;

        while(TRUE) {
            memset(&obuHeaderSyntax, 0x00, sizeof(ObuHeaderSyntax));
            switch(av1_obu_header_parsing(*(pData+next_byte), &obuHeaderSyntax)) {
            case OBU_SEQUENCE_HEADER :
            case OBU_TEMPORAL_DELIMITER :
            case OBU_FRAME_HEADER :
            case OBU_TILE_GROUP :
            case OBU_METADATA :
            case OBU_FRAME :
            case OBU_REDUNDANT_FRAME_HEADER :
            case OBU_TILE_LIST :
            case OBU_PADDING :
                if (!(obuHeaderSyntax.obu_extension_flag)) {
                    next_byte++;
                } else {
                    next_byte+=2;
                }
                break;
            default:
                break;
            }

            if (obuHeaderSyntax.obu_has_size_field) {
                obu_frame_size = av1_uleb_obu_size(pData+next_byte, &length);
                next_byte += obu_frame_size + length;
            }

            if (OBU_FRAME == obuHeaderSyntax.obu_type) {

                if (next_byte >= avpacket->size) {
                    lastFrame = TRUE;
                }

                if (0 == fNumInTD) {
                    av1SubFrame->frameSize[fNumInTD] = next_byte;
                    if (lastFrame) break;
                } else {
                    av1SubFrame->frameSize[fNumInTD] = 1+obu_frame_size+length;
                }
                av1SubFrame->frames[fNumInTD] = osal_malloc(av1SubFrame->frameSize[fNumInTD]);
                osal_memcpy(av1SubFrame->frames[fNumInTD], curData, av1SubFrame->frameSize[fNumInTD]);
                curData+=av1SubFrame->frameSize[fNumInTD];
                av1SubFrame->nframes = ++fNumInTD;
                if (lastFrame) {
                    break;
                }
            }
            if (next_byte >= avpacket->size) {
                return fNumInTD;
            }
        } //while
    }
    return fNumInTD;
}

static BOOL VP9ParseSuperframe(
    void*           chunk,
    Uint32          size,
    VP9Superframe*  superframe
    )
{
    Uint32  startPos;
    Uint32  frameSizeLength = 0;
    Uint32  numFrames, totalSubframeSize = 0;
    Uint32  i;
    Uint8*  pData = NULL, superframeMarker;

    startPos = size - 1;

    pData = (Uint8*)chunk;
    pData = &pData[startPos];
    superframeMarker = *pData;

    frameSizeLength = ((*pData>>3) & 0x03) + 1;
    numFrames       = (*pData&0x07) + 1;
    pData           -= frameSizeLength * numFrames + 1;
    /* Check first SUPERFRAME_MARKER */
    if (*pData != superframeMarker) {
        VLOG(ERR, "INVALID POST SUPERFRAME_MARKER\n");
        return FALSE;
    }
    pData++;

    for (i=0; i<numFrames; i++) {
        superframe->frameSize[i] = u_bytes(pData, frameSizeLength);
        pData += frameSizeLength;
    }

    /* Check size */
    for (i=0; i<numFrames; i++) {
        totalSubframeSize += superframe->frameSize[i];
    }
    if (totalSubframeSize >= size) {
        VLOG(ERR, "TOTAL SIZE OF SUBFRAMES IS BIGGER THAN CHUNK SIZE\n");
        return FALSE;
    }

    pData = (Uint8*)chunk;
    for (i=0; i<numFrames; i++) {
        superframe->frames[i] = (void*)osal_malloc(superframe->frameSize[i]);
        osal_memcpy(superframe->frames[i], (void*)pData, superframe->frameSize[i]);
        pData += superframe->frameSize[i];
    }
    superframe->currentIndex = 0;
    superframe->nframes      = numFrames;

    return TRUE;
}

static Int32 BuildSeqHeader(
    Uint8*          pbHeader,
    const CodStd    codStd,
    const AVStream* st,
    Int32*          sizelength
    )
{
    /*lint -save -e438 */
    AVCodecParameters* avc = st->codecpar;

    Uint8*        pbMetaData = avc->extradata;
    Int32         nMetaData = avc->extradata_size;
    Uint8* p =    pbMetaData;
    Uint8 *a =    p + 4 - ((long) p & 3);
    Uint8* t =    pbHeader;
    Int32         size;
    Int32         fourcc;
    Int32         sps, pps, i, nal;
    Int32         frameRate = 0;

    fourcc = avc->codec_tag;
    if (!fourcc)
        fourcc = ConvCodecIdToFourcc(avc->codec_id);

    if (st->avg_frame_rate.den && st->avg_frame_rate.num)
        frameRate = (Int32)((double)st->avg_frame_rate.num/(double)st->avg_frame_rate.den);

    size = 0;
    *sizelength = 4; // default size length(in bytes) = 4
    if (codStd == STD_AVC || codStd == STD_AVS) {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01) {
            // check mov/mo4 file format stream
            p += 4;
            *sizelength = (*p++ & 0x3) + 1;
            sps = (*p & 0x1f); // Number of sps
            p++;
            for (i = 0; i < sps; i++) {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }

            pps = *(p++); // number of pps
            for (i = 0; i < pps; i++)
            {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }
        }
        else if(nMetaData > 3) {
            size = -1;// return to meaning of invalid stream data;
            for (; p < a; p++) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1)  {
                    // find startcode
                    size = avc->extradata_size;
                    if (codStd == STD_AVC) {
                        if (pbMetaData && 0x00 == pbMetaData[size-1]) {
                            size-=1;
                        }
                    }
                    if (!pbHeader || !pbMetaData)
                        return 0;
                    PUT_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    }
    else if (codStd == STD_HEVC) {
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01) {
            static const Uint8 nalu_header[4] = { 0, 0, 0, 1 };
            Int32 numOfArrays = 0;
            Uint16 numNalus = 0;
            Uint16 nalUnitLength = 0;
            Uint32 offset = 0;

            p += 21;
            *sizelength = (*p++ & 0x3) + 1;
            numOfArrays = *p++;

            while(numOfArrays--) {
                p++;   // NAL type
                numNalus = (*p << 8) + *(p + 1);
                p+=2;
                for(i = 0;i < numNalus;i++)
                {
                    nalUnitLength = (*p << 8) + *(p + 1);
                    p+=2;
                    //if(i == 0)
                    {
                        osal_memcpy(pbHeader + offset, nalu_header, 4);
                        offset += 4;
                        osal_memcpy(pbHeader + offset, p, nalUnitLength);
                        offset += nalUnitLength;
                    }
                    p += nalUnitLength;
                }
            }

            size = offset;
        }
        else if(nMetaData > 3)
        {
            size = -1;// return to meaning of invalid stream data;

            for (; p < a; p++)
            {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) // find startcode
                {
                    size = avc->extradata_size;
                    if (!pbHeader || !pbMetaData)
                        return 0;
                    PUT_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    }
    else if (codStd == STD_VC1)
    {
        if (!fourcc)
            return -1;
        if (fourcc == MKTAG('W', 'V', 'C', '1') || fourcc == MKTAG('W', 'M', 'V', 'A'))	//VC AP
        {
            size = nMetaData;
            PUT_BUFFER(pbHeader, pbMetaData, size);
            //if there is no seq startcode in pbMetatData. VPU will be failed at seq_init stage.
        }
        else
        {
#ifdef RCV_V2
            PUT_LE32(pbHeader, ((0xC5 << 24)|0));
            size += 4; //version
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, avc->height);
            size += 4;
            PUT_LE32(pbHeader, avc->width);
            size += 4;
            PUT_LE32(pbHeader, 12);
            size += 4;
            PUT_LE32(pbHeader, 2 << 29 | 1 << 28 | 0x80 << 24 | 1 << 0);
            size += 4; // STRUCT_B_FRIST (LEVEL:3|CBR:1:RESERVE:4:HRD_BUFFER|24)
            PUT_LE32(pbHeader, avc->bit_rate);
            size += 4; // hrd_rate
            PUT_LE32(pbHeader, frameRate);
            size += 4; // frameRate
#else	//RCV_V1
            PUT_LE32(pbHeader, (0x85 << 24) | 0x00);
            size += 4; //frames count will be here
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, avc->height);
            size += 4;
            PUT_LE32(pbHeader, avc->width);
            size += 4;
#endif
        }
    }
    else if (codStd == STD_RV)
    {
        Int32 st_size =0;

        if (!fourcc)
            return -1;
        if (fourcc != MKTAG('R','V','3','0') && fourcc != MKTAG('R','V','4','0'))
            return -1;

        size = 26 + nMetaData;
        PUT_BE32(pbHeader, size); //Length
        PUT_LE32(pbHeader, MKTAG('V', 'I', 'D', 'O')); //MOFTag
        PUT_LE32(pbHeader, fourcc); //SubMOFTagl
        PUT_BE16(pbHeader, avc->width);
        PUT_BE16(pbHeader, avc->height);
        PUT_BE16(pbHeader, 0x0c); //BitCount;
        PUT_BE16(pbHeader, 0x00); //PadWidth;
        PUT_BE16(pbHeader, 0x00); //PadHeight;

        PUT_LE32(pbHeader, frameRate);
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData); //OpaqueDatata
        size += st_size; //add for startcode pattern.
    }
    else if (codStd == STD_DIV3) {
        // not implemented yet
        if (!nMetaData) {
            PUT_LE32(pbHeader, MKTAG('C', 'N', 'M', 'V')); //signature 'CNMV'
            PUT_LE16(pbHeader, 0x00);                      //version
            PUT_LE16(pbHeader, 0x20);                      //length of header in bytes
            PUT_LE32(pbHeader, MKTAG('D', 'I', 'V', '3')); //codec FourCC
            PUT_LE16(pbHeader, avc->width);                //width
            PUT_LE16(pbHeader, avc->height);               //height
            PUT_LE32(pbHeader, st->avg_frame_rate.num);      //frame rate
            PUT_LE32(pbHeader, st->avg_frame_rate.den);      //time scale(?)
            PUT_LE32(pbHeader, st->nb_index_entries);      //number of frames in file
            PUT_LE32(pbHeader, 0); //unused
            size += 32;
            return size;
        }

        PUT_BE32(pbHeader, nMetaData);
        size += 4;

        PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
        size += nMetaData;
    }
    else if (codStd == STD_VP8) {
        PUT_LE32(pbHeader, MKTAG('D', 'K', 'I', 'F')); //signature 'DKIF'
        PUT_LE16(pbHeader, 0x00);                      //version
        PUT_LE16(pbHeader, 0x20);                      //length of header in bytes
        PUT_LE32(pbHeader, MKTAG('V', 'P', '8', '0')); //codec FourCC
        PUT_LE16(pbHeader, avc->width);                //width
        PUT_LE16(pbHeader, avc->height);               //height
        PUT_LE32(pbHeader, st->avg_frame_rate.num);      //frame rate
        PUT_LE32(pbHeader, st->avg_frame_rate.den);      //time scale(?)
        PUT_LE32(pbHeader, st->nb_index_entries);      //number of frames in file
        PUT_LE32(pbHeader, 0); //unused
        size += 32;
    }
    else if (codStd == STD_VP9) {
        size = 0; // no header is needed because VP9 doesn't support interrupt mode.
    }
    else {
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
        size = nMetaData;
    }

    return size;
    /*lint -restore */
}

static Int32 BuildPicHeader(
    Uint8*        pbHeader,
    const CodStd    codStd,
    const AVStream* st,
    const AVPacket* pkt,
    Int32         sizelength
    )
{
    AVCodecParameters* avc = st->codecpar;
    Int64         frame_number = st->nb_frames;
    Uint8*        pbChunk = pkt->data;
    Int32         size = 0;
    Int32         fourcc;
    Int32         cSlice, nSlice;
    Int32         i, val, offset;
    BOOL          hasStartCode = 0;

    size = 0;
    offset = 0;
    fourcc = avc->codec_tag;
    if (!fourcc)
        fourcc = ConvCodecIdToFourcc(avc->codec_id);

    if (codStd == STD_VC1) {
        if (!fourcc)
            return -1;

        if (fourcc == MKTAG('W', 'V', 'C', '1') || fourcc == MKTAG('W', 'M', 'V', 'A')) {
            if (pbChunk[0] != 0 || pbChunk[1] != 0 || pbChunk[2] != 1) {
                // check start code as prefix (0x00, 0x00, 0x01)
                pbHeader[0] = 0x00;
                pbHeader[1] = 0x00;
                pbHeader[2] = 0x01;
                pbHeader[3] = 0x0D;	// replace to the correct picture header to indicate as frame

                size += 4;
            }
        }
        else {
            PUT_LE32(pbHeader, pkt->size | ((pkt->flags & AV_PKT_FLAG_KEY) ? 0x80000000 : 0));
            size += 4;
#ifdef RCV_V2
            if (AV_NOPTS_VALUE == pkt->pts) {
                PUT_LE32(pbHeader, 0);
            }
            else {
                PUT_LE32(pbHeader, (int)((double)(pkt->pts/st->time_base.den))); // milli_sec
            }
            size += 4;
#endif
        }
    }
    else if (codStd == STD_HEVC) {
        if(pkt->size < 5)
            return 0;

        if (!(avc->extradata_size > 1 && avc->extradata && avc->extradata[0] == 0x01))
        {
            const Uint8 *pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);

            for (; pbChunk < pbEnd ; pbChunk++) {
                if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1) {
                    hasStartCode = TRUE;
                    break;
                }
            }
        }

        if ((!hasStartCode && avc->extradata[0] == 0x01) ||
            (avc->extradata_size > 1 && avc->extradata && avc->extradata[0] == 0x01)) {
            // check sequence metadata if the stream is mov/mo4 file format.
            pbChunk = pkt->data;
            while (offset < pkt->size) {
                if(sizelength == 3) {
                    nSlice = pbChunk[offset] << 16 | pbChunk[offset+1] << 8 | pbChunk[offset+2];

                    pbChunk[offset] = 0x00;
                    pbChunk[offset+1] = 0x00;
                    pbChunk[offset+2] = 0x01;

                    offset += 3;
                }
                else {// sizeLength = 4
                    nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];

                    pbChunk[offset] = 0x00;
                    pbChunk[offset+1] = 0x00;
                    pbChunk[offset+2] = 0x00;
                    pbChunk[offset+3] = 0x01;		//replace size to startcode

                    offset += 4;
                }

                switch ((pbChunk[offset]&0x7E)>>1) { /* NAL unit */
                case 39: /* PREFIX SEI */
                case 40: /* SUFFIX SEI */
                case 32: /* VPS */
                case 33: /* SPS */
                case 34: /* PPS */
                    /* check next */
                    break;
                }

                offset += nSlice;
            }
        }
    }
    else if (codStd == STD_RV) {
        int st_size = 0;

        if (!fourcc)
            return -1;
        if (fourcc != MKTAG('R','V','3','0') && fourcc != MKTAG('R','V','4','0')) // RV version 8, 9 , 10
            return -1;

        cSlice = pbChunk[0] + 1;
        nSlice =  pkt->size - 1 - (cSlice * 8);
        size = 20 + (cSlice*8);

        PUT_BE32(pbHeader, nSlice);
        if (AV_NOPTS_VALUE == pkt->pts) {
            PUT_LE32(pbHeader, 0);
        }
        else {
            PUT_LE32(pbHeader, (int)((double)(pkt->pts/st->time_base.den))); // milli_sec
        }

        PUT_BE16(pbHeader, frame_number);
        PUT_BE16(pbHeader, 0x02); //Flags
        PUT_BE32(pbHeader, 0x00); //LastPacket
        PUT_BE32(pbHeader, cSlice); //NumSegments
        offset = 1;
        for (i = 0; i < (int) cSlice; i++)
        {
            val = (pbChunk[offset+3] << 24) | (pbChunk[offset+2] << 16) | (pbChunk[offset+1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //isValid
            offset += 4;
            val = (pbChunk[offset+3] << 24) | (pbChunk[offset+2] << 16) | (pbChunk[offset+1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //Offset
            offset += 4;
        }

        size += st_size;
    }
    else if (codStd == STD_AVC) {
        if(pkt->size < 5)
            return 0;

        if (!(avc->extradata_size > 1 && avc->extradata && avc->extradata[0] == 0x01)) {
            const Uint8 *pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);

            for (; pbChunk < pbEnd ; pbChunk++) {
                if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1) {
                    hasStartCode = 1;
                    break;
                }
            }
        }

        if ((!hasStartCode && avc->extradata[0] == 0x01) ||
            (avc->extradata_size > 1 && avc->extradata && avc->extradata[0] == 0x01)) {
            // check sequence metadata if the stream is mov/mo4 file format.
            pbChunk = pkt->data;

            while (offset < pkt->size) {
                if(sizelength == 3) {
                    nSlice = pbChunk[offset] << 16 | pbChunk[offset+1] << 8 | pbChunk[offset+2];
                    pbChunk[offset] = 0x00;
                    pbChunk[offset+1] = 0x00;
                    pbChunk[offset+2] = 0x01;
                    offset += 3;
                }
                else {  // size length = 4
                    nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];
                    pbChunk[offset] = 0x00;
                    pbChunk[offset+1] = 0x00;
                    pbChunk[offset+2] = 0x00;
                    pbChunk[offset+3] = 0x01;		//replace size to startcode
                    offset += 4;
                }

                switch (pbChunk[offset]&0x1f) { /* NAL unit */
                case 6: /* SEI */
                case 7: /* SPS */
                case 8: /* PPS */
                case 9: /* AU */
                    /* check next */
                    break;
                }

                offset += nSlice;
            }
        }
    }
    else if(codStd == STD_AVS) {
        const Uint8* pbEnd;

        if(pkt->size < 5)
            return 0;

        pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);

        for (; pbChunk < pbEnd ; pbChunk++) {
            if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1) {
                hasStartCode = 1;
                break;
            }
        }

        if(hasStartCode == 0) {
            pbChunk = pkt->data;

            while (offset < pkt->size) {
                nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];

                pbChunk[offset]   = 0x00;
                pbChunk[offset+1] = 0x00;
                pbChunk[offset+2] = 0x00;
                pbChunk[offset+3] = 0x00;		//replace size to startcode
                pbChunk[offset+4] = 0x01;

                offset += 4;

                switch (pbChunk[offset]&0x1f) /* NAL unit */
                {
                case 6: /* SEI */
                case 7: /* SPS */
                case 8: /* PPS */
                case 9: /* AU */
                    /* check next */
                    break;
                }

                offset += nSlice;
            }
        }
    }
    else if (codStd == STD_DIV3 || codStd == STD_VP8) {
        PUT_LE32(pbHeader,pkt->size);
        PUT_LE32(pbHeader,0);
        PUT_LE32(pbHeader,0);
        size += 12;
    }
    return size;
}



typedef struct {
    Uint8    start_code;
    Uint32   offset;    // File size must be less than 4GB
    Uint32   size;
} start_code_info_t;

typedef struct {
    Uint32      four_cc; // Magic number: 'A' 'V' 'S' '2'
    osal_file_t *f_in;
    Uint32      f_rd_ptr;
} avs2_reader_t;

static void avs2_read_frame(avs2_reader_t *reader, BSChunk *packet)
{
    const Uint32   AVS2_START_CODE_I_PIC     = 0xB3;
    const Uint32   AVS2_START_CODE_PB_PIC    = 0xB6;
    const Uint32   AVS2_START_CODE_SEQ_BEGIN = 0xB0;
    const Uint32   AVS2_START_CODE_SEQ_END   = 0xB1;

    Uint32   word = 0xFFFFFFFF;
    Uint8   c;

    int pic_ctr = 0;
    Uint32   start_ptr = reader->f_rd_ptr;

    start_code_info_t sci = { 0, (Uint32)-1,  0};

    while (osal_fread(&c, 1, 1, reader->f_in) == 1)
    {
        reader->f_rd_ptr++;

        word = (word << 8) | (c & 0xFF);

        if ((word & 0xFFFFFF00) == 0x00000100)
        {
            Uint8   start_code = (word & 0xFF);

            Uint32   f_offset = reader->f_rd_ptr - 4;

            if (start_code == AVS2_START_CODE_I_PIC || start_code == AVS2_START_CODE_PB_PIC)
                pic_ctr++;

            if (sci.offset == (Uint32)-1) {
                sci.start_code = word & 0xFF;
                sci.offset = f_offset;
            } else {
                sci.size = f_offset - sci.offset;

                if ((pic_ctr == 2) ||
                    (pic_ctr == 1 && (start_code == AVS2_START_CODE_SEQ_END || start_code == AVS2_START_CODE_SEQ_BEGIN)))
                {
                    Uint32   tmp;

                    packet->size = f_offset - start_ptr;
                    if (start_code == AVS2_START_CODE_SEQ_END) {
                        packet->size += 4;
                    }

                    osal_fseek(reader->f_in, start_ptr, SEEK_SET);
                    osal_fread(packet->data, 1, packet->size, reader->f_in);
                    reader->f_rd_ptr = osal_ftell(reader->f_in);

                    if (4 == osal_fread(&tmp, 1, 4, reader->f_in)) {
                        osal_fseek(reader->f_in, reader->f_rd_ptr, SEEK_SET);
                        packet->eos = FALSE;
                    } else {
                        packet->eos = TRUE;
                    }
                    //printf("AVS2 Feeder: offset = %08x end_offset = %08x size = %d\n", start_ptr, reader->f_rd_ptr, packet->size);
                    return;
                }

                sci.start_code = word & 0xFF;
                sci.offset = f_offset;
            }
        }
    }

    // End of file
    packet->eos = TRUE;
    packet->size = reader->f_rd_ptr - sci.offset;
    sci.size = reader->f_rd_ptr - sci.offset;
    osal_fseek(reader->f_in, start_ptr, SEEK_SET);
    osal_fread(packet->data, 1, packet->size, reader->f_in);
    reader->f_rd_ptr = osal_ftell(reader->f_in);
}

static void* AVS2Video_BSFreederFrameSize_Create(
    const char* path,
    CodStd      codecId)
{
    avs2_reader_t* avs2Reader = NULL;

    if (codecId != STD_AVS2) {
        VLOG(ERR, "%s:%d Invalid codecId:%d\n", __FUNCTION__, __LINE__, codecId);
        return NULL;
    }

    if ((avs2Reader=(avs2_reader_t*)osal_malloc(sizeof(avs2_reader_t))) == NULL)
        return NULL;

    avs2Reader->four_cc  = MKTAG('A', 'V', 'S', '2');
    avs2Reader->f_in     = osal_fopen(path, "rb");
    avs2Reader->f_rd_ptr = 0;

    return (void*)avs2Reader;
}

static Int32 AVS2Video_BSFeederFrameSize_Act (
    void*       feeder,
    BSChunk*    packet
    )
{
    avs2_reader_t* avs2Reader = (avs2_reader_t*)feeder;

    if (avs2Reader->f_in == NULL || osal_feof(avs2Reader->f_in)) {
        packet->eos = TRUE;
        return 0;
    }

    avs2_read_frame(avs2Reader, packet);

    return packet->size;
}

void* BSFeederFrameSize_Create(
    const char* path,
    CodStd      codecId
    )
{
    /*lint -esym(438, avContext) */
    FeederFrameContext* ffmpegReader = NULL;
    AVFormatContext*    avContext    = NULL;
    AVCodecParameters*  codec        = NULL;
    AVInputFormat*    fmt          = NULL;
    Int32             error;
    Int32             videoIndex;
    Uint32            mp4ClassId;
    Int32             standard;

    if (codecId == STD_AVS2) {
        return AVS2Video_BSFreederFrameSize_Create(path, codecId);
    }

    vdi_lock(0);
    if (initFFmpeg == FALSE) {
        initFFmpeg = TRUE;
    }
    vdi_unlock(0);

    if ((avContext=avformat_alloc_context()) == NULL) {
        return NULL;
    }

    avContext->flags |= AV_CODEC_FLAG_TRUNCATED;
    if ((error=avformat_open_input(&avContext, path, fmt, NULL))) {
        VLOG(ERR, "%s:%d failed to av_open_input_file error(%d), %s\n",
             __FILE__, __LINE__, error, path);
        goto __failed_to_end;
    }

    if ((error=avformat_find_stream_info(avContext, NULL)) < 0) {
        VLOG(ERR, "%s:%d failed to avformat_find_stream_info. error(%d)\n",
            __FUNCTION__, __LINE__, error);
        goto __failed_to_end;
    }

    videoIndex = av_find_best_stream(avContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoIndex < 0) {
        VLOG(ERR, "%s:%d failed to av_find_best_stream.\n", __FUNCTION__, __LINE__);
        goto __failed_to_end;
    }

    codec = avContext->streams[videoIndex]->codecpar;
    if (codecId == STD_AVS2)
        standard = STD_AVS2;
    else if (codecId == STD_AV1)
        standard = STD_AV1;
    else
        standard = ConvFOURCCToCodStd(codec->codec_tag);

    if (standard == -1)
        standard = ConvCodecIdToCodStd(codec->codec_id);

    mp4ClassId = ConvFOURCCToMp4Class(codec->codec_tag);
    if (mp4ClassId == (Uint32)-1)
        mp4ClassId = ConvCodecIdToMp4Class(codec->codec_id);

    if (standard != STD_MPEG4) {
        mp4ClassId = 0;
    }

    if ((ffmpegReader=(FeederFrameContext*)osal_malloc(sizeof(FeederFrameContext))) == NULL)
        goto __failed_to_end;

    ffmpegReader->standard      = standard;
    ffmpegReader->mp4ClassId    = mp4ClassId;
    ffmpegReader->avContext     = avContext;
    ffmpegReader->videoIndex    = videoIndex;
    ffmpegReader->isFirstPacket = TRUE;
    ffmpegReader->tempBuffer    = NULL;
    ffmpegReader->tempRdPtr     = 0;
    ffmpegReader->tempWrPtr     = 0;
    osal_memset((void*)&ffmpegReader->superframe, 0x00, sizeof(VP9Superframe));
    osal_memset((void*)&ffmpegReader->av1Subframes, 0x00, sizeof(AV1SubFrame));


    return (void*)ffmpegReader;

__failed_to_end:
    if (avContext) {
        avformat_free_context(avContext);
        avContext = NULL;
    }

    if (ffmpegReader) {
        osal_free(ffmpegReader);
    }

    return NULL;
    /*lint +esym(438, avContext) */
}

BOOL BSFeederFrameSize_Destroy(
    void*   feeder
    )
{
    FeederFrameContext*  ctx = (FeederFrameContext*)feeder;
    Uint32          i;

    if (MKTAG('A', 'V', 'S', '2') == *(Uint32*)feeder)
    {
        avs2_reader_t *avs2_reader = (avs2_reader_t*)feeder;
        if (avs2_reader->f_in)
            osal_fclose(avs2_reader->f_in);
        osal_free(avs2_reader);
        return TRUE;
    }

    if (ctx == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (ctx->avContext)
        avformat_close_input(&ctx->avContext);


    for (i=0; i<ctx->superframe.nframes; i++) {
        if (ctx->superframe.frames[i] != NULL) {
            osal_free(ctx->superframe.frames[i]);
        }
    }

    osal_free(ctx);

    return TRUE;
}

Int32 BSFeederFrameSize_Act(
    void*       feeder,
    BSChunk*    packet
    )
{
    FeederFrameContext* ffmpegReader = (FeederFrameContext*)feeder;
    AVFormatContext*    avFormatContext = ffmpegReader->avContext;
    AVPacket            avpacket;
    Int32               error;
    Uint8*              seqHeader = NULL;
    Uint8*              picHeader = NULL;
    Uint8*              ptr;
    Int32               seqHeaderSize;
    Int32               picHeaderSize;
    Uint32              vindex, size;
    Int32               retSize = -1;
    Int32               packetSize = -1;
    Uint8*              pData;
    Uint32              lastIndex;

    if (MKTAG('A', 'V', 'S', '2') == *(Uint32*)feeder)
        return AVS2Video_BSFeederFrameSize_Act(feeder, packet);

    if (ffmpegReader->tempBuffer) {
        goto __consume_tempBuffer;
    }

    packet->isHeader = FALSE;

    if (ffmpegReader->standard == STD_VP9) {
        VP9Superframe* superframe = &ffmpegReader->superframe;
        if (superframe->nframes > 0) {
            Uint32 idx = superframe->currentIndex;
            if (idx < superframe->nframes) {
                osal_memcpy(packet->data, superframe->frames[idx], superframe->frameSize[idx]);
                packet->size = superframe->frameSize[idx];
                superframe->currentIndex++;
                return packet->size;
            }
            else {
                Uint32 i;
                for (i=0; i<VP9_MAX_SUBFRAMES; i++) {
                    if (superframe->frames[i] != NULL) {
                        osal_free(superframe->frames[i]);
                    }
                }
                osal_memset((void*)superframe, 0x00, sizeof(VP9Superframe));
            }
        }
    } else if (ffmpegReader->standard == STD_AV1) {
        AV1SubFrame* av1SubFrame = &ffmpegReader->av1Subframes;
        if (av1SubFrame->nframes > 0) {
            Uint32 idx = av1SubFrame->currentIndex;
            if (idx < av1SubFrame->nframes) {
                osal_memcpy(packet->data, av1SubFrame->frames[idx], av1SubFrame->frameSize[idx]);
                packet->size = av1SubFrame->frameSize[idx];
                av1SubFrame->currentIndex++;
                return packet->size;
            } else  {
                Uint32 jdx;
                for (jdx =0; jdx <AV1_MAX_SUBFRAMES; jdx++) {
                    if (av1SubFrame->frames[jdx]) {
                        osal_free(av1SubFrame->frames[jdx]);
                    }
                }
            }
        }
    }

    seqHeaderSize = 0;
    picHeaderSize = 0;

    //av_init_packet(&avpacket); //deprecated function from ffmpeg v4.4.1

    while (TRUE) {
        error = av_read_frame(avFormatContext, &avpacket);
        if (error < 0) {
            if (error == AVERROR_EOF || avFormatContext->pb->eof_reached == TRUE) {
                packet->eos = TRUE;
                return 0;
            }
            else {
                VLOG(ERR, "%s:%d failed to av_read_frame error(0x%08x)\n",
                    __FUNCTION__, __LINE__, error);
                goto __end_read;
            }
        }

        if (avpacket.stream_index != ffmpegReader->videoIndex)
            continue;

        break;
    }

    if (avpacket.size == 0)
        return 0;

    if (avpacket.size >= (signed)packet->size )
    {
        VLOG(ERR, "one packet size(%d) is bigger than STREAM_BUF_SIZE(%d)\n", avpacket.size, packet->size);
        return -1;
    }

    osal_memset(packet->data, 0x00, packet->size);

    vindex = ffmpegReader->videoIndex;

    if (ffmpegReader->isFirstPacket) {
        AVCodecParameters* codec;

        codec = ffmpegReader->avContext->streams[vindex]->codecpar;
        seqHeader = (Uint8*)osal_malloc(codec->extradata_size + 1024);
        if (seqHeader == NULL) {
            goto __end_read;
        }
        osal_memset((void*)seqHeader, 0x00, codec->extradata_size + 1024);

        seqHeaderSize = BuildSeqHeader(seqHeader, (CodStd)ffmpegReader->standard,
                                       ffmpegReader->avContext->streams[vindex], &retSize);
        if (seqHeaderSize < 0) {
            VLOG(ERR, "%s:%d Can't make sequence header!\n", __FUNCTION__, __LINE__);
            packetSize = -1;
            goto __end_read;
        }

        packet->seqHeaderSize = seqHeaderSize;
        ffmpegReader->isFirstPacket = FALSE;
    }

    picHeader     = (Uint8*)osal_malloc(1024);
    picHeaderSize = BuildPicHeader(picHeader, (CodStd)ffmpegReader->standard,
                                   ffmpegReader->avContext->streams[vindex], &avpacket, 0);
    if (picHeaderSize < 0) {
        VLOG(ERR, "%s:%d failed to build picture header\n", __FUNCTION__, __LINE__);
        goto __end_read;
    }

    ptr  = avpacket.data;
    size = avpacket.size;
    switch (ffmpegReader->standard) {
    case STD_RV:
        if (seqHeaderSize) {
            if ( !seqHeader )
                goto __end_read;
            osal_memcpy((char*)packet->data, seqHeader, seqHeaderSize);
        }

        if (picHeaderSize)
            osal_memcpy((char*)packet->data+seqHeaderSize, picHeader, picHeaderSize);

        if (ffmpegReader->standard == STD_RV) {
            int cSlice = ptr[0] + 1;
            int nSlice = avpacket.size - 1 - (cSlice*8);
            ptr += (1+(cSlice*8));
            size = nSlice;
        }

        osal_memcpy((char*)packet->data+seqHeaderSize+picHeaderSize, ptr, size);
        packetSize = seqHeaderSize + picHeaderSize + size;
        break;
    case STD_VP9:
    case STD_AV1:
        packet->size    = size;
        osal_memcpy((char *)packet->data, ptr, size);
        packetSize      = size;
        break;
    default:
        if (picHeaderSize)
            osal_memcpy((char*)packet->data, picHeader, picHeaderSize);

        osal_memcpy((char*)packet->data+picHeaderSize, ptr, size);
        packetSize = picHeaderSize + size;
        break;
    }

    if (avFormatContext->pb->eof_reached && avpacket.size <= 0) {
        packet->eos = TRUE;
        VLOG(ERR, "packet->eor is true \n");
    }

    switch (ffmpegReader->standard) {
        case STD_RV :
        case STD_VP8 :
            break;
        case STD_AV1 :
            if (0 < AV1Parser(&avpacket, &(ffmpegReader->av1Subframes))) {
                AV1SubFrame* av1SubFrame = &ffmpegReader->av1Subframes;
                osal_memcpy(packet->data, av1SubFrame->frames[0], av1SubFrame->frameSize[0]);
                packet->size = av1SubFrame->frameSize[0];
                packetSize   = packet->size;
                av1SubFrame->currentIndex++;
            }
            break;
        case STD_VP9 :
            pData = (Uint8*)packet->data;
            lastIndex = packet->size - 1;
            if (IS_VP9_SUPERFRAME(pData[lastIndex]) == TRUE) {
                VP9Superframe*  superframe = &ffmpegReader->superframe;
                if (VP9ParseSuperframe(pData, packet->size, superframe) == TRUE) {
                    osal_memcpy(packet->data, superframe->frames[0], superframe->frameSize[0]);
                    packet->size = superframe->frameSize[0];
                    packetSize   = packet->size;
                    superframe->currentIndex++;
                }
            }
            break;
        default :
            if (seqHeaderSize > 0) {
                ffmpegReader->tempBuffer = (Uint8*)osal_malloc(packetSize);
                if (!ffmpegReader->tempBuffer)
                    goto __end_read;
                ffmpegReader->tempWrPtr  = packetSize;
                osal_memcpy(ffmpegReader->tempBuffer, (Uint8*)(packet->data), packetSize);
                if ( !seqHeader)
                    goto __end_read;
                osal_memcpy(packet->data, seqHeader, seqHeaderSize);
                if (!seqHeader)
                    goto __end_read;
                packetSize = seqHeaderSize;
                goto __end_read;
            }
            break;
    }

__end_read:
    av_packet_unref(&avpacket);

    if (picHeader) {
        osal_free(picHeader);
        packet->isHeader = TRUE;
    }
        
    if (seqHeader) {
        osal_free(seqHeader);
        packet->isHeader = TRUE;
    }
        

    return packetSize;

__consume_tempBuffer:
    if (ffmpegReader->tempBuffer != NULL) {
        osal_memcpy(packet->data, ffmpegReader->tempBuffer, ffmpegReader->tempWrPtr);
        packetSize = ffmpegReader->tempWrPtr;
        osal_free(ffmpegReader->tempBuffer);
        ffmpegReader->tempBuffer = NULL;
        ffmpegReader->tempWrPtr  = 0;
        ffmpegReader->tempRdPtr  = 0;
    }

    return packetSize;
}

BOOL BSFeederFrameSize_Rewind(
    void* feeder
    )
{
    FeederFrameContext*      ffmpegReader = (FeederFrameContext*)feeder;
    AVFormatContext*    avFormatContext = ffmpegReader->avContext;
    Int32               ret;

    if ((ret=av_seek_frame(avFormatContext, ffmpegReader->videoIndex, 0, 0)) < 0) {
        VLOG(ERR, "%s:%d Failed to av_seek_frame:(ret:%d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    return TRUE;
}

Int32 BSFeederFrameSize_GetStandard(
    void* feeder
    )
{
    FeederFrameContext*      ffmpegReader = (FeederFrameContext*)feeder;
    if (NULL != ffmpegReader) {
        return ffmpegReader->standard;
    }
    return -1;
}
#else
void* BSFeederFrameSize_Create(
    const char* path,
    CodStd  codecId
    )
{
    UNREFERENCED_PARAMETER(path);
    UNREFERENCED_PARAMETER(codecId);

    VLOG(ERR, "PLEASE PORT THIS %s ON YOUR ANDROID SYSTEM\n", __FUNCTION__);

    return NULL;
}

BOOL BSFeederFrameSize_Destroy(
    void*   feeder
    )
{
    UNREFERENCED_PARAMETER(feeder);

    VLOG(ERR, "PLEASE PORT THIS %s ON YOUR ANDROID SYSTEM\n", __FUNCTION__);

    return FALSE;
}

Int32 BSFeederFrameSize_Act(
    void*       feeder,
    BSChunk*    packet
    )
{
    UNREFERENCED_PARAMETER(feeder);
    UNREFERENCED_PARAMETER(packet);

    VLOG(ERR, "PLEASE PORT THIS %s ON YOUR ANDROID SYSTEM\n", __FUNCTION__);

    return -1;
}

BOOL BSFeederFrameSize_Rewind(
    void* feeder
    )
{
    VLOG(ERR, "PLEASE PORT THIS %s ON YOUR ANDROID SYSTEM\n", __FUNCTION__);

    return FALSE;

}

Int32 BSFeederFrameSize_GetStandard(
    void* feeder
    )
{
    UNREFERENCED_PARAMETER(feeder);
    VLOG(ERR, "PLEASE PORT THIS %s ON YOUR ANDROID SYSTEM\n", __FUNCTION__);

    return -1;
}
#endif /* SUPPORT_FFMPEG_DEMUX */

