//
//  TestRenderer.cpp
//  SigGen
//
//  Created by Mike Erickson on 10/10/22.
//

#include "TestRenderer.hpp"
#include <cmath>

std::shared_ptr<neato::IRenderReturn> TestRenderer::Render(neato::render_params_t params, const neato::audio_stream_description_t& stream_desc)
{
    std::shared_ptr<neato::IRenderReturn> error = neato::CreateRenderReturn();

    static float frequency = 440;

    SInt16 *left = (SInt16 *)params.ioData->mBuffers[0].mData;
    for (UInt32 frame = 0; frame < params.inNumberFrames; ++frame)
    {
        left[frame] = (SInt16)(std::sinf(theta) * 32767.0f);
        theta += two_pi * frequency / stream_desc.sample_rate;
        if (theta > two_pi)
        {
            theta -= two_pi;
        }
        frequency += 0.05;
        if (frequency > 2000.0f)
        {
            frequency = 440;
        }
    }

    // Copy left channel to right channel
    memcpy(params.ioData->mBuffers[1].mData, left, params.ioData->mBuffers[1].mDataByteSize);
    
    return error;
}
