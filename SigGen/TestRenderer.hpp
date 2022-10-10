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

class TestRenderer
{
public:
    std::shared_ptr<neato::IRenderReturn> Render(neato::render_params_t params, const neato::audio_stream_description_t& stream_desc);
private:
    float theta;
    constexpr static const float two_pi = std::numbers::pi * 2.0f;
};

