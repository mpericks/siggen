#include <vector>
#include <crtdbg.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <wrl/client.h>
#include <format>
#include <audiopolicy.h>
#include <memory>
#include "RenderGraph.h"

using Microsoft::WRL::ComPtr;

using com_memory_ptr_t = std::unique_ptr<void, decltype(&CoTaskMemFree)>;
using auto_handle_t = std::unique_ptr<void, decltype(&CloseHandle)>;

class WinRenderConstants : public  Neato::PlatformRenderConstantsDictionary
{
public:
    virtual uint32_t Format(const uint32_t format) const
    {
        uint32_t format_out = WAVE_FORMAT_PCM;
        switch (format)
        {
        case Neato::format_id_pcm:
            format_out = WAVE_FORMAT_PCM;
            break;
        case Neato::format_id_float_32:
			format_out = WAVE_FORMAT_IEEE_FLOAT;
			break;
        default:
            format_out = WAVE_FORMAT_IEEE_FLOAT;
            break;
        }
        return format_out;
    }
    virtual uint32_t Flag(uint32_t flag) const
    {
        uint32_t platform_flag = 0;
        switch (flag)
        {
            case Neato::format_flag_signed_int:
                //platform_flag = kAudioFormatFlagIsSignedInteger;
                break;
            case Neato::format_flag_packed:
                //platform_flag = kAudioFormatFlagIsPacked;
                break;
            case Neato::format_flag_non_interleaved:
                //platform_flag = kAudioFormatFlagIsNonInterleaved;
                break;
            default:
                platform_flag = 0;
        }

        return platform_flag;
    }
};

class WinRenderReturn : public Neato::IRenderReturn
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
    virtual Neato::OS_RETURN GetErrorCode() const
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

std::shared_ptr<Neato::IRenderReturn> Neato::CreateRenderReturn()
{
    return std::make_shared<WinRenderReturn>();
}
std::shared_ptr<Neato::IRenderReturn> Neato::CreateRenderReturn(OS_RETURN status, const utf8_string& desc)
{
    return std::make_shared<WinRenderReturn>(status, desc);
}

DWORD WINAPI RenderThread(void* param);

class WinRenderGraph : public Neato::IRenderGraph
{
public:
    WinRenderGraph(const Neato::audio_stream_description_t& params, std::shared_ptr<Neato::IRenderCallback> callback)
        : _generic_stream_desc(params)
        , _stream_switch_in_progress(false)
        , _thread(nullptr, &::CloseHandle)
        , _shutdown_event(nullptr, &::CloseHandle)
        , _audio_samples_needed_event(nullptr, &::CloseHandle)
        , _stream_switch_event(nullptr, &::CloseHandle)
        , _stream_switch_complete_event(nullptr, &::CloseHandle)
        , _renderImpl(callback)
    {
        Initialize();
    }

    virtual ~WinRenderGraph()
    {
        Stop();
    }

    void Initialize()
    {
        _device_enumerator.Reset();
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_device_enumerator));
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to instantiate device enumerator: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, _endpoint.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to get default device: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _shutdown_event.reset(CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!_shutdown_event)
        {
            std::string msg = std::format("Unable to create shutdown event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _audio_samples_needed_event.reset(CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!_audio_samples_needed_event)
        {
            std::string msg = std::format("Unable to create samples needed event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _stream_switch_complete_event.reset(CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!_stream_switch_complete_event)
        {
            std::string msg = std::format("Unable to create stream switch complete event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _stream_switch_event.reset(CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!_stream_switch_event)
        {
            std::string msg = std::format("Unable to create stream switch event: GetLastError() = 0x{:x}", GetLastError());
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _endpoint->Activate(__uuidof(IAudioClient3), CLSCTX_INPROC_SERVER, NULL, (void**)_audio_client.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to activate audio client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        WAVEFORMATEXTENSIBLE wave_format_in = CreateWaveFormat(_generic_stream_desc);
        WAVEFORMATEX* p_wave_format_out = (WAVEFORMATEX*)::CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
        com_memory_ptr_t wave_format_cleanup(p_wave_format_out, &::CoTaskMemFree);

        hr = _audio_client->GetCurrentSharedModeEnginePeriod(&p_wave_format_out, &_buffer_frame_count);

        if (FAILED(hr))
        {
            std::string msg = std::format("Error asking for a compatible format on audio client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        _generic_stream_desc = CreateNeutralStreamDescription((WAVEFORMATEXTENSIBLE)*p_wave_format_out, _buffer_frame_count);
        std::memcpy(&_mix_format, p_wave_format_out, sizeof(WAVEFORMATEXTENSIBLE));

        _frame_size = _mix_format.Format.nBlockAlign;

        _renderImpl->RendererCreated(_generic_stream_desc);

        hr = _audio_client->InitializeSharedAudioStream(AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                        _buffer_frame_count,
                                                        (WAVEFORMATEX*)&_mix_format,
                                                        NULL);

        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to initialize audio client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _audio_client->SetEventHandle(_audio_samples_needed_event.get());
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to set ready event: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        hr = _audio_client->GetService(IID_PPV_ARGS(_render_client.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
        {
            std::string msg = std::format("Unable to get new render client: hr = 0x{:x}", hr);
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }
    }

    virtual std::shared_ptr<Neato::IRenderReturn> Start()
    {
        std::shared_ptr<WinRenderReturn> error = std::make_shared<WinRenderReturn>();

        _thread.reset(::CreateThread(nullptr, 0, RenderThread, this, 0, nullptr));

        HRESULT hr = _audio_client->Start();

        return error;
    }

    virtual std::shared_ptr<Neato::IRenderReturn> Stop()
    {
        std::shared_ptr<WinRenderReturn> error = std::make_shared<WinRenderReturn>();

        _audio_client->Stop();

        ::SetEvent(_shutdown_event.get());
        WaitForSingleObject(_thread.get(), INFINITE);

        return error;
    }

    void Render()
    {
        Neato::render_params_t params;
        bool stillPlaying = true;
        std::vector<HANDLE> wait_handles = { _shutdown_event.get(), _stream_switch_event.get(), _audio_samples_needed_event.get()};
        HRESULT hr;
        constexpr uint32_t shutdown_triggered = WAIT_OBJECT_0 + 0;
        constexpr uint32_t stream_switch_triggered = WAIT_OBJECT_0 + 1;
        constexpr uint32_t audio_samples_requested = WAIT_OBJECT_0 + 2;

        while (stillPlaying)
        {
            DWORD waitResult = WaitForMultipleObjects((DWORD)wait_handles.size(), wait_handles.data(), FALSE, INFINITE);
            if (WAIT_FAILED == waitResult)
            {
                std::string msg = std::format("Unable to wait: GetlastError = 0x{:x}", GetLastError());
                _RPTF0(_CRT_ERROR, msg.c_str());
            }
            switch (waitResult)
            {
			case shutdown_triggered:
				stillPlaying = false;       // exit the loop.
				break;
			case stream_switch_triggered:
				//  We've received a stream switch request.
				//
				//  We need to stop the renderer, tear down the _audio_client and _render_cClient objects and re-create them on the new
				//  endpoint if possible.  If this fails, abort the thread.
				//
				//if (!HandleStreamSwitchEvent())
				//{
				stillPlaying = false;
				//}
				break;
			case audio_samples_requested:

				uint32_t padding;

				// "padding" is specified in number of frames
				hr = _audio_client->GetCurrentPadding(&padding);
				if (SUCCEEDED(hr))
				{
					//  Calculate the number of frames available.
                    const uint32_t frames_available_count = _buffer_frame_count - padding;
					hr = _render_client->GetBuffer(frames_available_count, &params.frame_buffer);
					if (SUCCEEDED(hr))
					{
						params.frame_count = frames_available_count;
						_renderImpl->Render(params);
						hr = _render_client->ReleaseBuffer(frames_available_count, 0);
					}
				}
				break;
            }
        }

        return;
    }

private:
    Neato::audio_stream_description_t CreateNeutralStreamDescription(const WAVEFORMATEXTENSIBLE& wave_format, uint32_t buffer_frame_count)
    {
        Neato::audio_stream_description_t ret_val;
        ret_val.sample_rate = wave_format.Format.nSamplesPerSec;
        ret_val.bits_per_channel = wave_format.Format.wBitsPerSample;
        ret_val.channels_per_frame = wave_format.Format.nChannels;
        // The size of an audio frame is specified by the nBlockAlign member of the WAVEFORMATEX structure 
        // that the client obtains by calling the IAudioClient::GetMixFormat method.
        // https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudiorenderclient-getbuffer
        ret_val.bytes_per_frame = wave_format.Format.nBlockAlign;
        ret_val.hardware_buffer_frame_count = buffer_frame_count;

        if (wave_format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            if (::IsEqualGUID(wave_format.SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
            {
                if (32 == wave_format.Format.wBitsPerSample)
                {
                    ret_val.format_id = Neato::format_id_float_32;
                }
                
                else if (64 == wave_format.Format.wBitsPerSample)
                {
                    ret_val.format_id = Neato::format_id_float_64;
                }
                else
                {
                    // i'm not dealing with anything fancier than double
                    std::string msg = "unrecognized floating point format.";
                    _RPTF0(_CRT_ERROR, msg.c_str());
                    throw std::exception(msg.c_str());
                }
            }
            else if (::IsEqualGUID(wave_format.SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
            {
                if (wave_format.Format.wBitsPerSample != 16)
                {
                    std::string msg = "Unknown PCM integer sample type";
                    _RPTF0(_CRT_ERROR, msg.c_str());
                    throw std::exception(msg.c_str());
                }
                ret_val.format_id = Neato::format_id_pcm;
            }
            else if (::IsEqualGUID(wave_format.SubFormat, GUID_NULL))
            {
                // some chucklehead is making us guess
                if (wave_format.Format.wBitsPerSample == 32)
                {
                    ret_val.format_id = Neato::format_id_float_32;
                }
            }
        }
        else if (wave_format.Format.wFormatTag == WAVE_FORMAT_PCM)
        {
            if (wave_format.Format.wBitsPerSample != 16)
            {
                std::string msg = "Unknown PCM integer sample type";
                _RPTF0(_CRT_ERROR, msg.c_str());
                throw std::exception(msg.c_str());
            }
            ret_val.format_id = Neato::format_id_pcm;
        }
        else if (wave_format.Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        {
            //vary between 1.0 and -1.0
            if (32 == wave_format.Format.wBitsPerSample)
            {
                ret_val.format_id = Neato::format_id_float_32;
            }
            else if (64 == wave_format.Format.wBitsPerSample)
            {
                ret_val.format_id = Neato::format_id_float_64;
            }
            else
            {
                // don't know what this is
                std::string msg = "unrecognized floating point format.";
                _RPTF0(_CRT_ERROR, msg.c_str());
                throw std::exception(msg.c_str());
            }
        }
        else
        {
            // not going to deal with other formats right now
            std::string msg = "unrecognized device format.";
            _RPTF0(_CRT_ERROR, msg.c_str());
            throw std::exception(msg.c_str());
        }

        return ret_val;
    }

    WAVEFORMATEXTENSIBLE CreateWaveFormat(const Neato::audio_stream_description_t& stream_desc)
    {
        WAVEFORMATEXTENSIBLE wave_format = { 0 };
        wave_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wave_format.Format.nSamplesPerSec = (DWORD)stream_desc.sample_rate;
        wave_format.Format.wBitsPerSample = stream_desc.bits_per_channel;
        wave_format.Format.nChannels = stream_desc.channels_per_frame;
        wave_format.Format.nBlockAlign = stream_desc.bytes_per_frame;
        wave_format.Format.nAvgBytesPerSec = wave_format.Format.nBlockAlign * wave_format.Format.nSamplesPerSec;
        wave_format.Format.cbSize = 22;
        // not really the right way to set the PCM format tag but see:
        // https://learn.microsoft.com/en-us/windows/win32/coreaudio/device-formats
        // >>> Some device drivers will report that they support a 1-channel or 2-channel PCM format 
        // >>> if the format is specified by a stand-alone WAVEFORMATEX structure, 
        // >>> but will reject the same format if it is specified by a WAVEFORMATEXTENSIBLE structure.
        if (stream_desc.format_id == Neato::format_id_pcm)
        {
            if (stream_desc.channels_per_frame < 3)
            {
                wave_format.Format.wFormatTag = WAVE_FORMAT_PCM;
                wave_format.Format.cbSize = 0;
            }
            wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        }
        else if (stream_desc.format_id == Neato::format_id_float_32)
        {
            wave_format.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        }
        return wave_format;
    }

private:
    ComPtr<IMMDeviceEnumerator> _device_enumerator;
    ComPtr<IMMDevice> _endpoint;
    ComPtr<IAudioClient3> _audio_client;
    ComPtr<IAudioRenderClient> _render_client;
    ComPtr<IAudioSessionControl> _session_control;
    Neato::audio_stream_description_t _generic_stream_desc;
    WAVEFORMATEXTENSIBLE _mix_format;
    uint32_t _frame_size;
    uint32_t _buffer_frame_count;
    bool _stream_switch_in_progress;
    auto_handle_t _thread;
    auto_handle_t _shutdown_event;
    auto_handle_t _audio_samples_needed_event;
    auto_handle_t _stream_switch_event;           // Set when the current session is disconnected or the default device changes.
    auto_handle_t _stream_switch_complete_event;  // Set when the default device has been changed internally.

    std::shared_ptr<Neato::IRenderCallback> _renderImpl;
};

DWORD WINAPI RenderThread(void* param)
{
    WinRenderGraph* render_graph = (WinRenderGraph*)param;
    render_graph->Render();
    return 0;
}

std::shared_ptr<Neato::PlatformRenderConstantsDictionary> Neato::CreateRenderConstantsDictionary()
{
    return std::make_shared<WinRenderConstants>();
}

std::shared_ptr<Neato::IRenderGraph> Neato::CreateRenderGraph(const Neato::audio_stream_description_t& creation_params, std::shared_ptr<Neato::IRenderCallback> callback)
{
    std::shared_ptr<Neato::IRenderGraph> graph = std::make_shared<WinRenderGraph>(creation_params, callback);
    return graph;
}