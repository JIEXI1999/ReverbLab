--[[----------------------------------------------------------------------------
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
------------------------------------------------------------------------------]]

if not _AK_PREMAKE then
    error('You must use the custom Premake5 scripts by adding the following parameter: --scripts="Scripts\\Premake"', 1)
end

local Plugin = {}
Plugin.name = "ReverbLab"
Plugin.factoryheader = "../SoundEnginePlugin/ReverbLabFXFactory.h"
Plugin.appleteamid = ""
Plugin.sdk = {}
Plugin.sdk.static = {}
Plugin.sdk.shared = {}
Plugin.authoring = {}

-- SDK STATIC PLUGIN SECTION
Plugin.sdk.static.includedirs = -- https://github.com/premake/premake-core/wiki/includedirs
{
}
Plugin.sdk.static.files = -- https://github.com/premake/premake-core/wiki/files
{
    "../JuceModules/juce_core/juce_core.h",
    "../JuceModules/juce_core/juce_core.cpp",
    "../JuceModules/juce_audio_formats/juce_audio_formats.h",
    "../JuceModules/juce_audio_formats/juce_audio_formats.cpp",
    "../JuceModules/juce_audio_basics/juce_audio_basics.h",
    "../JuceModules/juce_audio_basics/juce_audio_basics.cpp",
    "../JuceModules/juce_dsp/juce_dsp.h",
    "../JuceModules/juce_dsp/juce_dsp.cpp",

    "**.cpp",
    "**.h",
    "**.hpp",
    "**.c",
}
Plugin.sdk.static.excludes = -- https://github.com/premake/premake-core/wiki/removefiles
{
    "ReverbLabFXShared.cpp"
}
Plugin.sdk.static.links = -- https://github.com/premake/premake-core/wiki/links
{
}
Plugin.sdk.static.libsuffix = "FX"
Plugin.sdk.static.libdirs = -- https://github.com/premake/premake-core/wiki/libdirs
{
}
Plugin.sdk.static.defines = -- https://github.com/premake/premake-core/wiki/defines
{
    "JUCE_MODULE_AVAILABLE_juce_audio_basics=1",
    "JUCE_MODULE_AVAILABLE_juce_audio_formats=1",
    "JUCE_MODULE_AVAILABLE_juce_core=1",
    "JUCE_MODULE_AVAILABLE_juce_dsp=1",
    "JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1",
}

Plugin.sdk.static. custom = function ()
    cppdialect "C++17"
    rtti("on")
    exceptionhandling ("on")
end

-- SDK SHARED PLUGIN SECTION
Plugin.sdk.shared.includedirs =
{
}
Plugin.sdk.shared.files =
{
    "ReverbLabFXShared.cpp",
    "ReverbLabFXFactory.h",
}
Plugin.sdk.shared.excludes =
{
}
Plugin.sdk.shared.links =
{
}
Plugin.sdk.shared.libdirs =
{
}
Plugin.sdk.shared.defines =
{
}

-- AUTHORING PLUGIN SECTION
Plugin.authoring.includedirs =
{
}
Plugin.authoring.files =
{
    "**.cpp",
    "**.h",
    "**.hpp",
    "**.c",
    "ReverbLab.def",
    "ReverbLab.xml",
    "**.rc",
}
Plugin.authoring.excludes =
{
}
Plugin.authoring.links =
{
}
Plugin.authoring.libdirs =
{
}
Plugin.authoring.defines =
{
}

return Plugin
