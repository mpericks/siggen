#include <crtdbg.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>
#include <format>
#include <audiopolicy.h>
#include <memory>
#include "RenderGraph.h"

using Microsoft::WRL::ComPtr;

class WinRenderConstants : public  neato::PlatformRenderConstantsDictionary
{
public:
    virtual uint32_t Format(uint32_t format) const
    {
        // TODO: finish
        return WAVE_FORMAT_PCM;// kAudioFormatLinearPCM;
    }
    virtual uint32_t Flag(uint32_t flag) const
    {
        uint32_t platform_flag = 0;
        switch (flag)
        {
            case neato::format_flag_signed_int:
                //platform_flag = kAudioFormatFlagIsSignedInteger;
                break;
            case neato::format_flag_packed:
                //platform_flag = kAudioFormatFlagIsPacked;
                break;
            case neato::format_flag_non_interleaved:
                //platform_flag = kAudioFormatFlagIsNonInterleaved;
                break;
            default:
                platform_flag = 0;
        }

        return platform_flag;
    }
};

class WinRenderReturn : public neato::IRenderReturn
{
public:
    WinRenderReturn()
    {
        SetCodeAndDescription(NOERROR);
    }
    explicit WinRenderReturn(DWORD code)
    {
        SetCodeAndDescription(code);
    }
    explicit WinRenderReturn(DWORD code, utf8_string desc)
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
        return (_code == NOERROR);
    }
    void SetDescription(utf8_string description)
    {
        _description = description;
    }
    void SetCode(DWORD error)
    {
        _code = error;
    }
    void SetCodeAndDescription(DWORD error)
    {
        _code = error;
        _description = _code_to_decription(error);
    }
    void SetCodeAndDescription(DWORD error, utf8_string desc)
    {
        _code = error;
        _description = desc;
    }
private:
    utf8_string _code_to_decription(DWORD code)
    {
        utf8_string desc("");
        return desc;
    }
private:
    utf8_string _description;
    DWORD _code;
};

std::shared_ptr<neato::IRenderReturn> neato::CreateRenderReturn()
{
    return std::make_shared<WinRenderReturn>();
}
std::shared_ptr<neato::IRenderReturn> neato::CreateRenderReturn(OS_RETURN status, const utf8_string& desc)
{
    return std::make_shared<WinRenderReturn>(status, desc);
}

class WinRenderGraph : public neato::IRenderGraph
{
public:
    WinRenderGraph(const neato::audio_stream_description_t& params, std::shared_ptr<neato::IRenderCallback> callback)
        : _generic_stream_desc(params)
        , _stream_switch_in_progress(false)
        , _renderImpl(callback)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_device_enumerator));
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to instantiate device enumerator: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_endpoint);
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to get default device: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _shutdown_event = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (_shutdown_event == NULL)
        {
            std::string msg = std::format("Unable to create shutdown event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _audio_samples_needed_event = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (_audio_samples_needed_event == NULL)
        {
            std::string msg = std::format("Unable to create samples needed event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _stream_switch_complete_event = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        if (_stream_switch_complete_event == NULL)
        {
            std::string msg = std::format("Unable to create stream switch event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _endpoint->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, &_audio_client);
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to activate audio client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _audio_client->GetMixFormat(&_mix_format);
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to get mix format on audio client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _frame_size = _mix_format->nBlockAlign;

        _audio_stream_description = CreateNeutralStreamDescription(*_mix_format);

        _renderImpl->RendererCreated(_audio_stream_description);

        hr = _audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
            500 * 10000,
            0,
            _mix_format,
            NULL);

        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to initialize audio client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _audio_client->GetBufferSize(&_buffer_size);
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to get audio client buffer: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _audio_client->SetEventHandle(_audio_samples_needed_event);
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to set ready event: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _audio_client->GetService(IID_PPV_ARGS(&_render_client));
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to get new render client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }
    }

    virtual ~WinRenderGraph()
    {

    }

    virtual std::shared_ptr<neato::IRenderReturn> Start()
    {
        std::shared_ptr<WinRenderReturn> error = std::make_shared<WinRenderReturn>();

        return error;
    }

    virtual std::shared_ptr<neato::IRenderReturn> Stop()
    {
        std::shared_ptr<WinRenderReturn> error = std::make_shared<WinRenderReturn>();

        return error;
    }

    std::shared_ptr<neato::IRenderReturn> Render(const neato::render_params_t& params)
    {
        return _renderImpl->Render(params);
    }

private:
    neato::audio_stream_description_t CreateNeutralStreamDescription(const WAVEFORMATEX& wave_format)
    {
        neato::audio_stream_description_t ret_val;
        //uint32_t format_id;
        //uint32_t flags;
        ret_val.sample_rate = wave_format.nSamplesPerSec;
        ret_val.bits_per_channel = wave_format.wBitsPerSample;
        ret_val.channels_per_frame = wave_format.nChannels;
        //uint32_t frames_per_packet;
        //uint32_t bytes_per_frame;
        //uint32_t bytes_per_packet;

        if (wave_format.wFormatTag == WAVE_FORMAT_PCM ||
            wave_format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&wave_format)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
        {
            if (wave_format.wBitsPerSample != 16)
            {
                std::string msg = "Unknown PCM integer sample type";
                _RPTF0(_CRT_ERROR, msg.c_str());
                throw std::exception(msg.c_str());
            }
            ret_val.format_id = neato::format_id_pcm;
            ret_val.bytes_per_frame = (wave_format.wBitsPerSample / 8) * wave_format.nChannels;
        }
        else if (wave_format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (wave_format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&wave_format)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
        {
            ret_val.format_id = neato::format_id_pcm;
            ret_val.bytes_per_frame = (wave_format.wBitsPerSample / 8) * wave_format.nChannels;
        }
        else
        {
            // not going to deal with floating point or other formats right now
            std::string msg = "unrecognized device format.";
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        return ret_val;
    }

private:
    ComPtr<IMMDeviceEnumerator> _device_enumerator;
    ComPtr<IMMDevice> _endpoint;
    ComPtr<IAudioClient> _audio_client;
    ComPtr<IAudioRenderClient> _render_client;
    ComPtr<IAudioSessionControl> _session_control;
    WAVEFORMATEX* _mix_format;
    neato::audio_stream_description_t _audio_stream_description;
    uint32_t _frame_size;
    uint32_t _buffer_size;
    bool _stream_switch_in_progress;
    // a whole bunch of windows asynch events for handling stream switching, shutdown, and rendering
    HANDLE _shutdown_event;
    HANDLE _audio_samples_needed_event;
    HANDLE _stream_switch_event;           // Set when the current session is disconnected or the default device changes.
    HANDLE _stream_switch_complete_event;  // Set when the default device has been changed internally.

    neato::audio_stream_description_t _generic_stream_desc;
    std::shared_ptr<neato::IRenderCallback> _renderImpl;
};

std::shared_ptr<neato::PlatformRenderConstantsDictionary> neato::CreateRenderConstantsDictionary()
{
    return std::make_shared<WinRenderConstants>();
}

std::shared_ptr<neato::IRenderGraph> neato::CreateRenderGraph(const neato::audio_stream_description_t& creation_params, std::shared_ptr<neato::IRenderCallback> callback)
{
    std::shared_ptr<neato::IRenderGraph> graph = std::make_shared<WinRenderGraph>(creation_params, callback);
    return graph;
}