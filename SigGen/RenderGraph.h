//
//  RenderGraph.h
//  SigGen
//
//  Created by Mike Erickson on 10/7/22.
//

#pragma once
#include <string>
#include <memory>
#include <stdint.h>

typedef std::string utf8_string;

#if defined (__APPLE__)
#include "RenderGraph_Mac.h"
#endif //__APPLE__

namespace neato
{
    const uint32_t format_id_pcm = 1;

    const uint32_t format_flag_signed_int = 1;
    const uint32_t format_flag_packed = 2;
    const uint32_t format_flag_non_interleaved = 4;

    struct PlatformRenderConstantsDictionary
    {
        virtual uint32_t Format(uint32_t format) const = 0;
        virtual uint32_t Flag(uint32_t flags) const = 0;
    };

    struct audio_stream_description_t
    {
        uint32_t format_id;
        uint32_t flags;
        double sample_rate;
        uint32_t bits_per_channel;
        uint32_t channels_per_frame;
        uint32_t frames_per_packet;
        uint32_t bytes_per_frame;
        uint32_t bytes_per_packet;
#if PLATFORM_FORMAT_MEMBERS_REQUIRED == 1
        PlatformAudioFormatMembers platform_specific_info;
#endif
    };

    struct IRenderReturn
    {
        virtual OS_RETURN GetErrorCode() const = 0;
        virtual utf8_string GetErrorString() const = 0;
        virtual bool DidSucceed() const = 0;
        virtual void SetCode(OS_RETURN code) = 0;
        virtual void SetDescription(utf8_string desc) = 0;
        virtual void SetCodeAndDescription(OS_RETURN code, utf8_string dec) = 0;
    };

    struct IRenderGraph
    {
        virtual std::shared_ptr<IRenderReturn> Render(const render_params_t& args) = 0;
        virtual std::shared_ptr<IRenderReturn> Start() = 0;
        virtual std::shared_ptr<IRenderReturn> Stop() = 0;
    };

    std::shared_ptr<PlatformRenderConstantsDictionary> CreateRenderConstantsDictionary();
    std::shared_ptr<IRenderReturn> CreateRenderReturn();
    std::shared_ptr<IRenderReturn> CreateRenderReturn(OS_RETURN, const utf8_string&);
    std::shared_ptr<IRenderGraph> CreateRenderGraph(const audio_stream_description_t& creation_params);
};
