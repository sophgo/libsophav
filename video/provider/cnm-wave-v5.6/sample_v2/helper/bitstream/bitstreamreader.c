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
#include "misc/debug.h"
#ifdef PLATFORM_NON_OS
#else
#include <sys/stat.h>
#endif

BitstreamReader BitstreamReader_Create(
    Uint32      type,
    char*       path,
    EndianMode  endian,
    EncHandle*  handle
    )
{
    AbstractBitstreamReader* reader;
    osal_file_t *fp=NULL;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX) || defined(_MSC_VER)
    char         *dirname;
#endif

    if ( path[0] != 0) {
#ifdef PLATFORM_NON_OS
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
        dirname = GetDirname(path); 
        mkdir_recursive(dirname, S_IRWXU | S_IRWXG | S_IRWXO);
        osal_free(dirname);//malloc in GetDirname
#elif defined(_MSC_VER)
        dirname = GetDirname(path); 
        MkDir(dirname);
        osal_free(dirname);//malloc in GetDirname
#endif
        if ((fp=osal_fopen(path, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open bin file: %s\n", __FUNCTION__, __LINE__, path);
            return FALSE;
        }
        VLOG(INFO, "output bin file: %s\n", path);
    }
    else
        VLOG(TRACE, "%s:%d Bitstream File path is NULL : no save\n", __FUNCTION__, __LINE__);

    reader = (AbstractBitstreamReader*)osal_malloc(sizeof(AbstractBitstreamReader));

    osal_memset(reader, 0x00, sizeof(AbstractBitstreamReader));

    reader->fp      = fp;
    reader->handle  = handle;
    reader->type    = type;
    reader->endian  = endian;
    reader->streamBufFull = FALSE;

    return reader;
}

#define STREAM_READ_CNT 2
BOOL BitstreamReader_Act(
    BitstreamReader reader,
    PhysicalAddress bitstreamBuffer,        /* ringbuffer only */
    Uint32          bitstreamBufferSize,    /* ringbuffer only */
    Uint32          streamReadSize,
    Comparator      comparator,
    Uint32          useParam                /* use bitstreamBuffer, streamReadSize */
    )
{
    AbstractBitstreamReader* absReader = (AbstractBitstreamReader*)reader;
    osal_file_t     *fp;
    EncHandle       *handle;
    RetCode         ret = RETCODE_SUCCESS;
    PhysicalAddress paRdPtr;
    PhysicalAddress paWrPtr;
    int             size = 0;
    Int32           loadSize = 0;
    PhysicalAddress paBsBufStart = bitstreamBuffer;                         // ringbuffer only
    PhysicalAddress paBsBufEnd   = bitstreamBuffer+bitstreamBufferSize;     // ringbuffer only
    Uint8*          buf          = NULL;
    Uint32          coreIdx;
    BOOL            success      = TRUE;

    if (reader == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    if (streamReadSize == 0) {
        return TRUE;
    }
    fp          = absReader->fp;
    handle      = absReader->handle;
    coreIdx     = VPU_HANDLE_CORE_INDEX(*handle);
    
    if (useParam) {
        paRdPtr = bitstreamBuffer;
        size    = streamReadSize;
    } else {
        ret = VPU_EncGetBitstreamBuffer(*handle, &paRdPtr, &paWrPtr, &size);
    }
    if (size > 0) {
        if (streamReadSize > 0) {
            if ((Uint32)size < streamReadSize) {
                loadSize = size;
            }
            else {
                loadSize = streamReadSize;
            }
        }
        else {
            loadSize = size;
        }

        buf = (Uint8*)osal_malloc(loadSize);
        if (buf == NULL) {
            return FALSE;
        }

        if (absReader->type == BUFFER_MODE_TYPE_RINGBUFFER) {
            if ((paRdPtr+loadSize) > paBsBufEnd) {
                Uint32   room = paBsBufEnd - paRdPtr;
                vdi_read_memory(coreIdx, paRdPtr, buf, room,  absReader->endian);
                vdi_read_memory(coreIdx, paBsBufStart, buf+room, (loadSize-room), absReader->endian);
            } else {
                vdi_read_memory(coreIdx, paRdPtr, buf, loadSize, absReader->endian); 
            }
        } else {
            /* Linebuffer */
            vdi_read_memory(coreIdx, paRdPtr, buf, loadSize, absReader->endian); 
        }

        if (fp != NULL) {
            osal_fwrite((void *)buf, sizeof(Uint8), loadSize, fp);
            osal_fflush(fp);
        }

        if (comparator != NULL) {
            if (Comparator_Act(comparator, buf, loadSize, 0) == FALSE) {
                success = FALSE;
            }
        }
        osal_free(buf);

        //not to call VPU_EncUpdateBitstreamBuffer function if line buffer
        if ( absReader->type == BUFFER_MODE_TYPE_RINGBUFFER || ( absReader->type != BUFFER_MODE_TYPE_RINGBUFFER && absReader->streamBufFull)) {
            ret = VPU_EncUpdateBitstreamBuffer(*handle, loadSize);
            if( ret != RETCODE_SUCCESS ) {
                VLOG(ERR, "VPU_EncUpdateBitstreamBuffer failed Error code is 0x%x \n", ret );
                success = FALSE;
            }
        }
    }

    return success;
}

BOOL BitstreamReader_Destroy(
    BitstreamReader reader
    )
{
    AbstractBitstreamReader* absReader = (AbstractBitstreamReader*)reader;

    if (absReader == NULL) {
        VLOG(ERR, "%s:%d Invalid handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    osal_fclose(absReader->fp);
    osal_free(absReader);

    return TRUE;
}

