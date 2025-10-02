//
//  RenderGraph_Mac.hpp
//  SigGen
//
//  Created by Mike Erickson on 09/15/25.
//

#pragma once
#include <Windows.h>

#define PLATFORM_FORMAT_MEMBERS_REQUIRED 0

namespace neato
{
    using OS_RETURN = DWORD;

    struct render_params_t
    {
        uint32_t frame_count;
        uint8_t* frame_buffer;
    };
};