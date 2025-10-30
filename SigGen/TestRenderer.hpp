//
//  TestRenderer.hpp
//  SigGen
//
//  Created by Mike Erickson on 10/10/22.
//

#pragma once

#include "RenderGraph.h"
#include "base_waveforms.hpp"

class TestRenderer : public neato::IRenderCallback
{
public:
    TestRenderer();
    virtual std::shared_ptr<neato::IRenderReturn> Render(const neato::render_params_t& params) override;
    virtual void RendererCreated(const neato::audio_stream_description_t& creation_params) override;
private:
    neato::audio_stream_description_t _stream_desc;
    std::shared_ptr<neato::ISampleSource> signal;
};

