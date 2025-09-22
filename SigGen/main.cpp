//
//  main.cpp
//  SigGen
//
//  Created by Mike Erickson on 10/7/22.
//
#include "RenderGraph.h"
#include <iostream>
#include "TestRenderer.hpp"

int main(int argc, const char * argv[])
{

    int ret_val = 0;
    neato::audio_stream_description_t create_params;
    std::shared_ptr<neato::PlatformRenderConstantsDictionary> render_constants = neato::CreateRenderConstantsDictionary();
    create_params.format_id = render_constants->Format(neato::format_id_pcm);
    create_params.flags = 0
                          | render_constants->Flag(neato::format_flag_signed_int)
                          | render_constants->Flag(neato::format_flag_packed)
                          | render_constants->Flag(neato::format_flag_non_interleaved);
    create_params.bytes_per_packet = 2;
    create_params.bytes_per_frame = 2;
    create_params.frames_per_packet = 1;
    create_params.channels_per_frame = 2;
    create_params.bits_per_channel = 16;
    create_params.sample_rate = 48000;

    std::shared_ptr<neato::IRenderCallback> callback = std::make_shared<TestRenderer>(create_params);
    
    std::shared_ptr<neato::IRenderGraph> renderer;
    try
    {
        renderer = neato::CreateRenderGraph(create_params, callback);
    }
    catch(std::runtime_error e)
    {
        std::cout << e.exception::what();
        return -1;
    }
    
    std::shared_ptr<neato::IRenderReturn> ret = renderer->Start();
    std::cout << "Press enter to stop annoying sound" << std::endl;
    getchar();
    renderer->Stop();

    return ret_val;
}

