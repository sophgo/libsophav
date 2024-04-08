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

#ifndef __COMPONENT_LIST_H__
#define __COMPONENT_LIST_H__

//Common components
extern ComponentImpl yuvFeederComponentImpl;
extern ComponentImpl readerComponentImpl;

#ifdef CNM_WAVE_SERIES
extern ComponentImpl waveEncoderComponentImpl;
#else
extern ComponentImpl coda9EncoderComponentImpl;
#endif //CNM_WAVE_SERIES

static ComponentImpl* componentList[] = {
    &yuvFeederComponentImpl,
    &readerComponentImpl,
#ifdef CNM_WAVE_SERIES
    &waveEncoderComponentImpl,
#else
    &coda9EncoderComponentImpl,
#endif //CNM_WAVE_SERIES
    NULL
};
#endif

