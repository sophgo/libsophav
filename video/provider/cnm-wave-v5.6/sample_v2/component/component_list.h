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
extern ComponentImpl feederComponentImpl;
extern ComponentImpl rendererComponentImpl;

#ifdef CNM_WAVE_SERIES
extern ComponentImpl waveDecoderComponentImpl; 
#else
extern ComponentImpl coda9DecoderComponentImpl;
#endif //CNM_WAVE_SERIES

static ComponentImpl* componentList[] = {
    &feederComponentImpl,
    &rendererComponentImpl,
#ifdef CNM_WAVE_SERIES
    &waveDecoderComponentImpl,
#else
    &coda9DecoderComponentImpl,
#endif //CNM_WAVE_SERIES
    NULL
};
#endif

