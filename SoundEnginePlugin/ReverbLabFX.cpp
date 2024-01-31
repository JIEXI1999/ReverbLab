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
    , m_pContext(nullptr),
    reverb(50.0, 2.0, 0.5, 0.5)
{
}

ReverbLabFX::~ReverbLabFX()
{
}

AKRESULT ReverbLabFX::Init(AK::IAkPluginMemAlloc* in_pAllocator, AK::IAkEffectPluginContext* in_pContext, AK::IAkPluginParam* in_pParams, AkAudioFormat& in_rFormat)
{
    m_pParams = (ReverbLabFXParams*)in_pParams;
    m_pAllocator = in_pAllocator;
    m_pContext = in_pContext;

    // Spec Config
    spec.maximumBlockSize = 512;
    spec.sampleRate = in_rFormat.uSampleRate;
    spec.numChannels = 1;
    filter.prepare(spec);

    // Init Filter
    filter.reset();
    filter.setType(StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(300.f);
    reverb.configure(spec.sampleRate);

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
    AkUInt32 totalTailFrames = spec.sampleRate * myParams.DELAYTIME;
    m_FXTailHandler.HandleTail(io_pBuffer, totalTailFrames);
    const AkUInt32 uNumChannels = io_pBuffer->NumChannels();

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
            //AkReal32 filteredSample = filter.processSample(0, pBuf[uFramesProcessed]);
            //pBuf[uFramesProcessed] = filteredSample;
        }

        multiChannelMixer.stereoToMulti(stereoInput, multiChannelInput);
        multiChannelOutput = reverb.process(multiChannelInput);
        multiChannelMixer.multiToStereo(multiChannelOutput, stereoOutput);

        for (AkUInt32 i = 0; i < uNumChannels; ++i)
        {
            AkReal32* AK_RESTRICT pBuf = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(i);
            pBuf[uFramesProcessed] = static_cast<AkReal32>(stereoOutput[i]);
        }

        ++uFramesProcessed;
    }
   
}

AKRESULT ReverbLabFX::TimeSkip(AkUInt32 in_uFrames)
{
    return AK_DataReady;
}
