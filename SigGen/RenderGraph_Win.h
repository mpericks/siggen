//
//  RenderGraph_Win.hpp
//  SigGen
//
//  Created by Mike Erickson on 09/15/25.
//

#pragma once
#include <Windows.h>

constexpr bool PLATFORM_FORMAT_MEMBERS_REQUIRED = 0;

namespace Neato
{
    using OS_RETURN = DWORD;

    struct render_params_t
    {
        render_params_t() : frame_count(0), frame_buffer(nullptr) {}
        uint32_t frame_count;
        uint8_t* frame_buffer;
    };
};