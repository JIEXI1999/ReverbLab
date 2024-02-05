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
        RTPC.fDamping = 15000.f;
        RTPC.fRoomSize = 50.f;
        RTPC.fDryWetMix = 0.5f;
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
    RTPC.fDamping = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fRoomSize = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
    RTPC.fDryWetMix = READBANKDATA(AkReal32, pParamsBlock, in_ulBlockSize);
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
    case PARAM_DAMPING_ID:
        RTPC.fDamping = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_DAMPING_ID);
        break;
    case PARAM_ROOMSIZE_ID:
        RTPC.fRoomSize = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_ROOMSIZE_ID);
        break;
    case PARAM_DRYWETMIX_ID:
        RTPC.fDryWetMix = *((AkReal32*)in_pValue);
        m_paramChangeHandler.SetParamChange(PARAM_DRYWETMIX_ID);
        break;
    default:
        eResult = AK_InvalidParameter;
        break;
    }

    return eResult;
}
