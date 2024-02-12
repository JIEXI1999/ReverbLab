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
    // Reverb constructor
    reverb = std::make_unique<BasicReverb<CHANNELS, DIFFUSER_STEPS>>(ROOM_SIZE, 2.0);
}

ReverbLabFX::~ReverbLabFX()
{
}

AKRESULT ReverbLabFX::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkEffectPluginContext* in_pContext, AK::IAkPluginParam* in_pParams, AkAudioFormat& in_rFormat)
{
    m_pParams = (ReverbLabFXParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pContext;

    // Configure ProcessSpec. Default block size for Authoring is 512
    spec.maximumBlockSize = 512;
    spec.sampleRate = in_rFormat.uSampleRate;
    spec.numChannels = 1;

    // Init using ProcessSpec
    outputGain.prepare(spec);
    outputGain.setGainDecibels(0.f);
    outputGain.setRampDurationSeconds(0.2f);
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
    // Configure tail handler based on reverb length after input cutoff
    AkUInt32 totalTailFrames = spec.sampleRate * m_pParams->RTPC.fRT;
    m_FXTailHandler.HandleTail(io_pBuffer, totalTailFrames);

    const AkUInt32 uNumChannels = io_pBuffer->NumChannels();    // input channels (default: 2)
    AkUInt16 uFramesProcessed = 0;                              // current sample index
    while (uFramesProcessed < io_pBuffer->uValidFrames)
    {
        // Save stereo and multichannel (for reverb calculation) input and output
        std::array<AkReal32, 2> stereoInput;
        std::array<AkReal32, 2> stereoOutput;
        std::array<AkReal32, CHANNELS> multiChannelInput;
        std::array<AkReal32, CHANNELS> multiChannelOutput;

        // Stereo input of current sample; Expand up to 8 channels based on sinusoidal coefficient;
        for (AkUInt32 i = 0; i < uNumChannels; ++i)
        {
            AkReal32* AK_RESTRICT pBuf = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(i);
            stereoInput[i] = pBuf[uFramesProcessed];
        }
        multiChannelMixer.stereoToMulti(stereoInput, multiChannelInput);

        // If Decay Time has changed，reinvoke related setup function
        if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_RT_ID))
        {
            reverb->setRt60(m_pParams->RTPC.fRT);
        }
        // If Damping parameters changed, recalculate coefficients and update HS filter
        if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_HFCUTOFF_ID)|| 
            m_pParams->m_paramChangeHandler.HasChanged(PARAM_HFATTENUATION_ID))
        {
            reverb->setDamping(m_pParams->RTPC.fHFCutoff, m_pParams->RTPC.fHFAttenuation);
        }
        // Same for output gain
        if (m_pParams->m_paramChangeHandler.HasChanged(PARAM_OUTPUTGAIN))
        {
            outputGain.setGainDecibels(m_pParams->RTPC.fOutputGain);
        }

        // Call reverb algorithm (see revalg.h). Downmix back to stereo after processing.
        multiChannelOutput = reverb->process(multiChannelInput);
        multiChannelMixer.multiToStereo(multiChannelOutput, stereoOutput);

        // Get obtained wet signals  
        AkReal32 revL = static_cast<AkReal32>(stereoOutput[0]) * GAIN_CALIBR;
        AkReal32 revR = static_cast<AkReal32>(stereoOutput[1]) * GAIN_CALIBR;

        // Transfer L-R signal to M-S encoding for stereo expanding or narrowing
        AkReal32 revM = (revL + revR)*0.5;
        AkReal32 revS = (revL - revR)*0.5* m_pParams->RTPC.fStereoWidth;
        
        // Get data store in left-right channel buffer
        AkReal32* AK_RESTRICT pBufL = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(0);
        AkReal32* AK_RESTRICT pBufR = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(1);

        // Calculate dry wet mix
        AkReal32 dryMix = 1.0f - m_pParams->RTPC.fDryWetMix / 100.f;
        AkReal32 wetMix = m_pParams->RTPC.fDryWetMix / 100.f;

        // Transfer M-S back to L-R and apply output gain
        pBufL[uFramesProcessed] = outputGain.processSample(pBufL[uFramesProcessed] * dryMix + (revM - revS)* wetMix);
        pBufR[uFramesProcessed] = outputGain.processSample(pBufR[uFramesProcessed] * dryMix + (revM + revS) * wetMix);

        // End the loop, proceed to next sample.
        ++uFramesProcessed;

        // Periodically call snapToZero function of IIR Filter, optimize unnecessary resource allocation; 
        if (uFramesProcessed % 256 == 0)
        {
            reverb->filterSnapToZero();
        }

    }
   
}

AKRESULT ReverbLabFX::TimeSkip(AkUInt32 in_uFrames)
{
    return AK_DataReady;
}
