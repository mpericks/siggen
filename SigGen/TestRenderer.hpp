//
//  TestRenderer.hpp
//  SigGen
//
//  Created by Mike Erickson on 10/10/22.
//

#pragma once
#include <memory>
#include <numbers>
#include "RenderGraph.h"
#include "base_waveforms.hpp"
#include "envelope.hpp"

class TestRenderer
{
public:
    TestRenderer(const neato::audio_stream_description_t& stream_desc);
    std::shared_ptr<neato::IRenderReturn> Render(neato::render_params_t params);
private:
    float frequency;
    neato::MutableSine carrier;
    neato::MutableSaw modulator;
    neato::audio_stream_description_t _stream_desc;
    float modulator_gain;
    float current_frequency;
    std::shared_ptr<neato::IEnvelope> bell_envelope;
};

