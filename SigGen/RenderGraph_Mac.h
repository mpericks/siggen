//
//  RenderGraph_Mac.hpp
//  SigGen
//
//  Created by Mike Erickson on 10/7/22.
//

#pragma once

#include <stdio.h>
#include <AudioToolbox/AudioToolbox.h>

#define PLATFORM_FORMAT_MEMBERS_REQUIRED 0

namespace neato
{
    typedef OSStatus OS_RETURN;

    struct render_params_t
    {
        AudioUnitRenderActionFlags *ioActionFlags;
        const AudioTimeStamp *inTimeStamp;
        UInt32 inBusNumber;
        UInt32 inNumberFrames;
        AudioBufferList *ioData;
    };
};

