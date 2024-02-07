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

#include "ReverbLabFX.h"
#include "../ReverbLabConfig.h"

#include <AK/AkWwiseSDKVersion.h>

AK::IAkPlugin* CreateReverbLabFX(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, ReverbLabFX());
}

AK::IAkPluginParam* CreateReverbLabFXParams(AK::IAkPluginMemAlloc* in_pAllocator)
{
    return AK_PLUGIN_NEW(in_pAllocator, ReverbLabFXParams());
}

AK_IMPLEMENT_PLUGIN_FACTORY(ReverbLabFX, AkPluginTypeEffect, ReverbLabConfig::CompanyID, ReverbLabConfig::PluginID)

ReverbLabFX::ReverbLabFX()
    : m_pParams(nullptr)
    , m_pAllocator(nullptr)
    , m_pContext(nullptr)
{
        reverb = std::make_unique<BasicReverb<CHANNELS, DIFFUSER_STEPS>>(80, 2.0, 0.5, 0.5);
}

ReverbLabFX::~ReverbLabFX()
{
}

AKRESULT ReverbLabFX::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkEffectPluginContext* in_pContext, AK::IAkPluginParam* in_pParams, AkAudioFormat& in_rFormat)
{
    m_pParams = (ReverbLabFXParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pContext;

    spec.maximumBlockSize = 512;
    spec.sampleRate = in_rFormat.uSampleRate;
    spec.numChannels = 1;

    reverb->configure(spec);

    return AK_Success;
}

AKRESULT ReverbLabFX::Term(AK::IAkPluginMemAlloc* in_pAllocator)
{
    AK_PLUGIN_DELETE(in_pAllocator, this);
    return AK_Success;
}

AKRESULT ReverbLabFX::Reset()
{
    return AK_Success;
}

AKRESULT ReverbLabFX::GetPluginInfo(AkPluginInfo& out_rPluginInfo)
{
    out_rPluginInfo.eType = AkPluginTypeEffect;
    out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bCanProcessObjects = false;
    out_rPluginInfo.uBuildVersion = AK_WWISESDK_VERSION_COMBINED;
    return AK_Success;
}

void ReverbLabFX::Execute(AkAudioBuffer* io_pBuffer)
{
    AkUInt32 totalTailFrames = spec.sampleRate * m_pParams->RTPC.fRT;
    m_FXTailHandler.HandleTail(io_pBuffer, totalTailFrames);
    const AkUInt32 uNumChannels = io_pBuffer->NumChannels();

    //if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_ROOMSIZE_ID))
    //{
    //    io_pBuffer->ClearData();
    //    reverb = std::make_unique<BasicReverb<CHANNELS, DIFFUSER_STEPS>>(m_pParams->RTPC.fRoomSize, 4.0, 0.5, 0.5);
    //    reverb->configure(spec);
    //}

    AkUInt16 uFramesProcessed;
    uFramesProcessed = 0;
    while (uFramesProcessed < io_pBuffer->uValidFrames)
    {
        std::array<AkReal32, 2> stereoInput;
        std::array<AkReal32, 2> stereoOutput;
        std::array<AkReal32, 8> multiChannelInput;
        std::array<AkReal32, 8> multiChannelOutput;

        for (AkUInt32 i = 0; i < uNumChannels; ++i)
        {
            AkReal32* AK_RESTRICT pBuf = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(i);
            stereoInput[i] = pBuf[uFramesProcessed];
        }
        multiChannelMixer.stereoToMulti(stereoInput, multiChannelInput);

        if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_RT_ID))
        {
            reverb->setRt60(m_pParams->RTPC.fRT);
        }

        if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_DAMPING_ID))
        {
            reverb->setDamping(m_pParams->RTPC.fDamping);
        }
        if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_DRYWETMIX_ID))
        {
            reverb->wet = m_pParams->RTPC.fDryWetMix;
            reverb->dry = 1.f - reverb->wet;
        }

        multiChannelOutput = reverb->process(multiChannelInput);

        multiChannelMixer.multiToStereo(multiChannelOutput, stereoOutput);

        //stereo widening
        AkReal32* AK_RESTRICT pBufL = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(0);
        AkReal32* AK_RESTRICT pBufR = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(1);
        AkReal32 width = 0.5;
        AkReal32 lS = static_cast<AkReal32>(stereoOutput[0]) * GAIN_CALIBR;
        AkReal32 rS = static_cast<AkReal32>(stereoOutput[1]) * GAIN_CALIBR;
        AkReal32 mS = (lS + rS)*0.5;
        AkReal32 sS = (lS - rS)*0.5*width;
        
        pBufL[uFramesProcessed] = mS - sS;
        pBufR[uFramesProcessed] = mS + sS;

        ++uFramesProcessed;

        if (uFramesProcessed % 128 == 0)
        {
            reverb->filterSnapToZero();
        }

    }
   
}

AKRESULT ReverbLabFX::TimeSkip(AkUInt32 in_uFrames)
{
    return AK_DataReady;
}
