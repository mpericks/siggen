//
//  RenderGraph_Mac.cpp
//  SigGen
//
//  Created by Mike Erickson on 10/7/22.
//
#include <sstream>
#include <cmath>
#include <numbers>
#include "RenderGraph.h"
//#include "TestRenderer.hpp"

OSStatus InternalRenderingCallback(void *inRefCon,
                           AudioUnitRenderActionFlags *ioActionFlags,
                           const AudioTimeStamp *inTimeStamp,
                           UInt32 inBusNumber,
                           UInt32 inNumberFrames,
                           AudioBufferList *ioData)
{
    neato::IRenderGraph* renderer = (neato::IRenderGraph*)(inRefCon);
    neato::render_params_t params = {ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData};
    std::shared_ptr<neato::IRenderReturn> ret_ptr = renderer->Render(params);
    return ret_ptr->GetErrorCode();
}


class MacRenderConstants : public  neato::PlatformRenderConstantsDictionary
{
public:
    virtual uint32_t Format(uint32_t format) const
    {
        // TODO: finish
        return kAudioFormatLinearPCM;
    }
    virtual uint32_t Flag(uint32_t flag) const
    {
        uint32_t platform_flag = 0;
        switch (flag)
        {
            case neato::format_flag_signed_int:
                platform_flag = kAudioFormatFlagIsSignedInteger;
                break;
            case neato::format_flag_packed:
                platform_flag = kAudioFormatFlagIsPacked;
                break;
            case neato::format_flag_non_interleaved:
                platform_flag = kAudioFormatFlagIsNonInterleaved;
                break;
            default:
                platform_flag = 0;
        }
        
        return platform_flag;
    }
};

class MacRenderReturn : public neato::IRenderReturn
{
public:
    MacRenderReturn()
    {
        SetCodeAndDescription(noErr);
    }
    explicit MacRenderReturn(OSStatus code)
    {
        SetCodeAndDescription(code);
    }
    explicit MacRenderReturn(OSStatus code, utf8_string desc)
    {
        SetCodeAndDescription(code, desc);
    }
    virtual neato::OS_RETURN GetErrorCode() const
    {
        return _code;
    }
    virtual utf8_string GetErrorString() const
    {
        return _description;
    }
    virtual bool DidSucceed() const
    {
        return (_code == noErr);
    }
    void SetDescription(utf8_string description)
    {
        _description = description;
    }
    void SetCode(OSStatus error)
    {
        _code = error;
    }
    void SetCodeAndDescription(OSStatus error)
    {
        _code = error;
        _description = _code_to_decription(error);
    }
    void SetCodeAndDescription(OSStatus error, utf8_string desc)
    {
        _code = error;
        _description = desc;
    }
private:
    utf8_string _code_to_decription(OSStatus code)
    {
        utf8_string desc("");
        return desc;
    }
private:
    utf8_string _description;
    OSStatus _code;
};

std::shared_ptr<neato::IRenderReturn> neato::CreateRenderReturn()
{
    return std::make_shared<MacRenderReturn>();
}
std::shared_ptr<neato::IRenderReturn> neato::CreateRenderReturn(OS_RETURN status, const utf8_string& desc)
{
    return std::make_shared<MacRenderReturn>(status, desc);
}

class MacRenderGraph : public neato::IRenderGraph
{
public:
    MacRenderGraph(const neato::audio_stream_description_t& params, std::shared_ptr<neato::IRenderCallback> callback)
    : _generic_stream_desc(params)
    , _renderImpl(callback)
    {
        OSErr err;
        _component_description.componentType = kAudioUnitType_Output;
        _component_description.componentSubType = kAudioUnitSubType_DefaultOutput;
        _component_description.componentManufacturer = kAudioUnitManufacturer_Apple;

        _output = AudioComponentFindNext(NULL, &_component_description);
        if (!_output)
        {
            std::runtime_error error("Can't find default audio output in MacRenderGraph");
            throw error;
        }

        err = AudioComponentInstanceNew(_output, &_unit);
        if (err)
        {
            std::stringstream error_string;
            error_string << "Error creating audio unit in MacRenderGraph: " << err;
            std::runtime_error error(error_string.str());
            throw error;
        }

        _input.inputProc = InternalRenderingCallback;
        _input.inputProcRefCon = this;
        err = AudioUnitSetProperty(_unit,
                                   kAudioUnitProperty_SetRenderCallback,
                                   kAudioUnitScope_Input,
                                   0,
                                   &_input,
                                   sizeof(_input)
                                   );
        if (err)
        {
            std::stringstream error_string;
            error_string << "Error setting callback: " << err ;
            std::runtime_error error("Can't find default audio output in MacRenderGraph");
            throw error;
        }
        
        _stream_description.mFormatID = params.format_id;
        _stream_description.mFormatFlags = params.flags;
        _stream_description.mSampleRate = params.sample_rate;
        _stream_description.mBitsPerChannel = params.bits_per_channel;
        _stream_description.mChannelsPerFrame = params.channels_per_frame;
        _stream_description.mFramesPerPacket = params.frames_per_packet;
        _stream_description.mBytesPerFrame = params.bytes_per_frame;
        _stream_description.mBytesPerPacket = params.bytes_per_packet;
        
        err = AudioUnitSetProperty(_unit,
                                   kAudioUnitProperty_StreamFormat,
                                   kAudioUnitScope_Input,
                                   0,
                                   &_stream_description,
                                   sizeof(_stream_description));
        
        if (err)
        {
            std::stringstream error_string;
            error_string << "Error setting stream format: " << err ;
            std::runtime_error error(error_string.str());
            throw error;
        }
        
        err = AudioUnitInitialize(_unit);
    }
    
    virtual ~MacRenderGraph()
    {
        AudioUnitUninitialize(_unit);
        AudioComponentInstanceDispose(_unit);
    }
    
    virtual std::shared_ptr<neato::IRenderReturn> Start()
    {
        std::shared_ptr<MacRenderReturn> error = std::make_shared<MacRenderReturn>();
        OSErr err = AudioOutputUnitStart(_unit);
        if (err)
        {
            error->SetCodeAndDescription(err, "Could not start audio unit");
        }
        return error;
    }
    
    virtual std::shared_ptr<neato::IRenderReturn> Stop()
    {
        std::shared_ptr<MacRenderReturn> error = std::make_shared<MacRenderReturn>();
        OSStatus status = AudioOutputUnitStop(_unit);
        if (status)
        {
            error->SetCodeAndDescription(status, "Could not stop audio unit");
        }
        return error;
    }
    
    std::shared_ptr<neato::IRenderReturn> Render(const neato::render_params_t& params)
    {
        return _renderImpl->Render(params);
    }
private:
    AudioComponentDescription _component_description;
    AudioComponent _output;
    AudioUnit _unit;
    AURenderCallbackStruct _input;
    AudioStreamBasicDescription _stream_description;
    neato::audio_stream_description_t _generic_stream_desc;
    std::shared_ptr<neato::IRenderCallback> _renderImpl;

};

std::shared_ptr<neato::PlatformRenderConstantsDictionary> neato::CreateRenderConstantsDictionary()
{
    return std::make_shared<MacRenderConstants>();
}

std::shared_ptr<neato::IRenderGraph> neato::CreateRenderGraph(const neato::audio_stream_description_t& creation_params, std::shared_ptr<neato::IRenderCallback> callback)
{
    std::shared_ptr<neato::IRenderGraph> graph = std::make_shared<MacRenderGraph>(creation_params, std::shared_ptr<neato::IRenderCallback> callback);
    return graph;
}

