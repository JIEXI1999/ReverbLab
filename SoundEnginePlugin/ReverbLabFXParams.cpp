/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the
"Apache License"); you may not use this file except in compliance with the
Apache License. You may obtain a copy of the Apache License at
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Copyright (c) 2023 Audiokinetic Inc.
*******************************************************************************/

#include "ReverbLabFXParams.h"

#include <AK/Tools/Common/AkBankReadHelpers.h>

ReverbLabFXParams::ReverbLabFXParams()
{
}

ReverbLabFXParams::~ReverbLabFXParams()
{
}

ReverbLabFXParams::ReverbLabFXParams(const ReverbLabFXParams& in_rParams)
{
    RTPC = in_rParams.RTPC;
    NonRTPC = in_rParams.NonRTPC;
    m_paramChangeHandler.SetAllParamChanges();
}

AK::IAkPluginParam* ReverbLabFXParams::Clone(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, ReverbLabFXParams(*this));
}

AKRESULT ReverbLabFXParams::Init(AK::IAkPluginMemAlloc* in_pAllocator, const void* in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
    if (in_ulBlockSize == 0)
    {
        // Initialize default parameters here
        RTPC.fRT = 0.5f; 
        RTPC.fHFCutoff = 15000.f;
        RTPC.fHFAttenuation = 0.f;
        RTPC.fStereoWidth = 1.f;
        RTPC.fDryWetMix = 50.f;
        RTPC.fOutputGain = 0.f;
        m_paramChangeHandler.SetAllParamChanges();
        return AK_Success;
    }

    return SetParamsBlock(in_pParamsBlock, in_ulBlockSize);
}

AKRESULT ReverbLabFXParams::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    AK_PLUGIN_DELETE(in_pAllocator, this);
    return AK_Success;
}

AKRESULT ReverbLabFXParams::SetParamsBlock(const void* in_pParamsBlock, AkUInt32 in_ulBlockSize)
{
    AKRESULT eResult = AK_Success;
    AkUInt8* pParamsBlock = (AkUInt8*)in_pParamsBlock;

    // Read bank data here
    RTPC.fRT = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fHFCutoff = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fHFAttenuation = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fStereoWidth = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fDryWetMix = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fOutputGain = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    CHECKBANKDATASIZE(in_ulBlockSize, eResult);
    m_paramChangeHandler.SetAllParamChanges();

    return eResult;
}

AKRESULT ReverbLabFXParams::SetParam(AkPluginParamID in_paramID, const void* in_pValue, AkUInt32 in_ulParamSize)
{
    AKRESULT eResult = AK_Success;

    // Handle parameter change here
    switch (in_paramID)
    {
    case PARAM_RT_ID:
        RTPC.fRT = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_RT_ID);
        break;
    case PARAM_HFCUTOFF_ID:
        RTPC.fHFCutoff = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_HFCUTOFF_ID);
        break;
    case PARAM_HFATTENUATION_ID:
        RTPC.fHFAttenuation = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_HFATTENUATION_ID);
        break;
    case PARAM_STEREOWIDTH_ID:
        RTPC.fStereoWidth = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_STEREOWIDTH_ID);
        break;
    case PARAM_DRYWETMIX_ID:
        RTPC.fDryWetMix = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_DRYWETMIX_ID);
        break;
    case PARAM_OUTPUTGAIN:
        RTPC.fOutputGain = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_OUTPUTGAIN);
        break;
    default:
        eResult = AK_InvalidParameter;
        break;
    }

    return eResult;
}
