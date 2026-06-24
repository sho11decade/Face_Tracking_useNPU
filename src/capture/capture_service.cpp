#include "npu_vtube/capture/capture_service.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#endif

namespace npuvt::capture {

#if defined(_WIN32)

using Microsoft::WRL::ComPtr;

namespace {

constexpr DWORD kFirstVideoStream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);

std::string hr_to_string(const HRESULT hr) {
    char buffer[32] {};
    std::snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(hr));
    return buffer;
}

std::string wide_to_utf8(const std::wstring_view value) {
    if (value.empty()) {
        return {};
    }

    const auto required_size = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (required_size <= 0) {
        return {};
    }

    std::string converted(static_cast<std::size_t>(required_size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        converted.data(),
        required_size,
        nullptr,
        nullptr);
    return converted;
}

std::string get_allocated_string(IMFActivate* activate, const GUID& key) {
    WCHAR* raw_value = nullptr;
    std::uint32_t length = 0;
    const HRESULT hr = activate->GetAllocatedString(key, &raw_value, &length);
    if (FAILED(hr) || raw_value == nullptr) {
        return {};
    }

    const std::wstring_view wide_value(raw_value, length);
    const auto result = wide_to_utf8(wide_value);
    CoTaskMemFree(raw_value);
    return result;
}

HRESULT enumerate_video_devices(std::vector<ComPtr<IMFActivate>>& devices) {
    devices.clear();

    ComPtr<IMFAttributes> attributes;
    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        return hr;
    }

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr)) {
        return hr;
    }

    IMFActivate** raw_devices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(attributes.Get(), &raw_devices, &count);
    if (FAILED(hr)) {
        return hr;
    }

    for (UINT32 i = 0; i < count; ++i) {
        ComPtr<IMFActivate> device;
        device.Attach(raw_devices[i]);
        devices.push_back(std::move(device));
    }

    CoTaskMemFree(raw_devices);
    return S_OK;
}

HRESULT select_native_media_type(
    IMFSourceReader* reader,
    const CaptureOptions& options,
    UINT32* selected_width,
    UINT32* selected_height) {
    ComPtr<IMFMediaType> best_type;
    UINT32 best_width = 0;
    UINT32 best_height = 0;
    std::uint64_t best_score = std::numeric_limits<std::uint64_t>::max();

    for (DWORD index = 0;; ++index) {
        ComPtr<IMFMediaType> media_type;
        const HRESULT hr = reader->GetNativeMediaType(kFirstVideoStream, index, &media_type);
        if (hr == MF_E_NO_MORE_TYPES) {
            break;
        }
        if (FAILED(hr)) {
            return hr;
        }

        GUID major_type = GUID_NULL;
        if (FAILED(media_type->GetGUID(MF_MT_MAJOR_TYPE, &major_type)) || major_type != MFMediaType_Video) {
            continue;
        }

        UINT32 width = 0;
        UINT32 height = 0;
        if (FAILED(MFGetAttributeSize(media_type.Get(), MF_MT_FRAME_SIZE, &width, &height))) {
            continue;
        }

        const auto score = static_cast<std::uint64_t>(std::abs(static_cast<int>(width) - static_cast<int>(options.width))) +
                           static_cast<std::uint64_t>(std::abs(static_cast<int>(height) - static_cast<int>(options.height)));
        if (score < best_score) {
            best_score = score;
            best_width = width;
            best_height = height;
            best_type = media_type;
        }
    }

    if (!best_type) {
        return MF_E_INVALIDMEDIATYPE;
    }

    HRESULT hr = reader->SetCurrentMediaType(kFirstVideoStream, nullptr, best_type.Get());
    if (FAILED(hr)) {
        return hr;
    }

    *selected_width = best_width;
    *selected_height = best_height;
    return S_OK;
}

HRESULT set_output_media_type(IMFSourceReader* reader, const CaptureOptions& options, UINT32 width, UINT32 height) {
    ComPtr<IMFMediaType> output_type;
    HRESULT hr = MFCreateMediaType(&output_type);
    if (FAILED(hr)) {
        return hr;
    }

    hr = output_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        return hr;
    }

    const GUID subtype = (options.preferred_format == npuvt::core::PixelFormat::rgb8) ? MFVideoFormat_RGB24 : MFVideoFormat_RGB32;
    hr = output_type->SetGUID(MF_MT_SUBTYPE, subtype);
    if (FAILED(hr)) {
        return hr;
    }

    hr = MFSetAttributeSize(output_type.Get(), MF_MT_FRAME_SIZE, width, height);
    if (FAILED(hr)) {
        return hr;
    }

    return reader->SetCurrentMediaType(kFirstVideoStream, nullptr, output_type.Get());
}

class TemporaryMediaFoundationSession {
public:
    HRESULT startup() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr)) {
            com_initialized_ = true;
        } else if (hr != RPC_E_CHANGED_MODE) {
            return hr;
        }

        hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) {
            if (com_initialized_) {
                CoUninitialize();
                com_initialized_ = false;
            }
            return hr;
        }

        mf_started_ = true;
        return S_OK;
    }

    ~TemporaryMediaFoundationSession() {
        if (mf_started_) {
            MFShutdown();
        }
        if (com_initialized_) {
            CoUninitialize();
        }
    }

private:
    bool com_initialized_ {false};
    bool mf_started_ {false};
};

}  // namespace

struct CaptureService::Impl {
    bool com_initialized {false};
    bool mf_started {false};
    UINT32 width {0};
    UINT32 height {0};
    npuvt::core::PixelFormat pixel_format {npuvt::core::PixelFormat::unknown};
    std::uint64_t next_frame_id {0};
    ComPtr<IMFSourceReader> reader;
};

#else

struct CaptureService::Impl {
};

#endif

CaptureService::CaptureService() : impl_(std::make_unique<Impl>()) {
}

CaptureService::~CaptureService() {
    shutdown();
}

CaptureService::CaptureService(CaptureService&& other) noexcept = default;

CaptureService& CaptureService::operator=(CaptureService&& other) noexcept = default;

std::string_view CaptureService::describe() const noexcept {
#if defined(_WIN32)
    return "Media Foundation capture service";
#else
    return "Capture service unavailable on this platform";
#endif
}

std::vector<CaptureDeviceInfo> CaptureService::list_devices() const {
    std::vector<CaptureDeviceInfo> device_infos;

#if defined(_WIN32)
    TemporaryMediaFoundationSession session;
    if (FAILED(session.startup())) {
        return device_infos;
    }

    std::vector<ComPtr<IMFActivate>> devices;
    if (FAILED(enumerate_video_devices(devices))) {
        return device_infos;
    }

    device_infos.reserve(devices.size());
    for (const auto& device : devices) {
        device_infos.push_back({
            .name = get_allocated_string(device.Get(), MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME),
            .symbolic_link = get_allocated_string(device.Get(), MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK),
        });
    }
#endif

    return device_infos;
}

bool CaptureService::initialize(const CaptureOptions& options, std::string* error_message) {
    shutdown();

#if !defined(_WIN32)
    if (error_message != nullptr) {
        *error_message = "Media Foundation capture is only supported on Windows.";
    }
    return false;
#else
    auto set_error = [error_message](const std::string& message) {
        if (error_message != nullptr) {
            *error_message = message;
        }
    };

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        impl_->com_initialized = true;
    } else if (hr != RPC_E_CHANGED_MODE) {
        set_error("CoInitializeEx failed: " + hr_to_string(hr));
        return false;
    }

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        set_error("MFStartup failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }
    impl_->mf_started = true;

    std::vector<ComPtr<IMFActivate>> devices;
    hr = enumerate_video_devices(devices);
    if (FAILED(hr)) {
        set_error("MFEnumDeviceSources failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    if (options.device_index >= devices.size()) {
        set_error("Requested capture device index is out of range.");
        shutdown();
        return false;
    }

    ComPtr<IMFMediaSource> media_source;
    hr = devices[options.device_index]->ActivateObject(IID_PPV_ARGS(&media_source));
    if (FAILED(hr)) {
        set_error("ActivateObject failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 2);
    if (FAILED(hr)) {
        set_error("MFCreateAttributes failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    hr = attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    if (FAILED(hr)) {
        set_error("SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS) failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    if (FAILED(hr)) {
        set_error("SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING) failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    hr = MFCreateSourceReaderFromMediaSource(media_source.Get(), attributes.Get(), &impl_->reader);
    if (FAILED(hr)) {
        set_error("MFCreateSourceReaderFromMediaSource failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    UINT32 selected_width = 0;
    UINT32 selected_height = 0;
    hr = select_native_media_type(impl_->reader.Get(), options, &selected_width, &selected_height);
    if (FAILED(hr)) {
        set_error("Selecting native media type failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    hr = set_output_media_type(impl_->reader.Get(), options, selected_width, selected_height);
    if (FAILED(hr)) {
        set_error("Setting output media type failed: " + hr_to_string(hr));
        shutdown();
        return false;
    }

    impl_->width = selected_width;
    impl_->height = selected_height;
    impl_->pixel_format = (options.preferred_format == npuvt::core::PixelFormat::rgb8)
                              ? npuvt::core::PixelFormat::rgb8
                              : npuvt::core::PixelFormat::bgra8;
    impl_->next_frame_id = 0;
    return true;
#endif
}

void CaptureService::shutdown() noexcept {
#if defined(_WIN32)
    if (impl_ == nullptr) {
        return;
    }

    impl_->reader.Reset();

    if (impl_->mf_started) {
        MFShutdown();
        impl_->mf_started = false;
    }

    if (impl_->com_initialized) {
        CoUninitialize();
        impl_->com_initialized = false;
    }

    impl_->width = 0;
    impl_->height = 0;
    impl_->pixel_format = npuvt::core::PixelFormat::unknown;
    impl_->next_frame_id = 0;
#endif
}

bool CaptureService::is_initialized() const noexcept {
#if defined(_WIN32)
    return impl_ != nullptr && impl_->reader != nullptr;
#else
    return false;
#endif
}

npuvt::core::FramePacket CaptureService::grab_frame(std::string* error_message) {
#if !defined(_WIN32)
    if (error_message != nullptr) {
        *error_message = "Media Foundation capture is only supported on Windows.";
    }
    return make_placeholder_frame();
#else
    auto set_error = [error_message](const std::string& message) {
        if (error_message != nullptr) {
            *error_message = message;
        }
    };

    if (!is_initialized()) {
        set_error("CaptureService is not initialized.");
        return make_placeholder_frame();
    }

    for (;;) {
        DWORD stream_flags = 0;
        ComPtr<IMFSample> sample;
        const HRESULT hr = impl_->reader->ReadSample(
            kFirstVideoStream,
            0,
            nullptr,
            &stream_flags,
            nullptr,
            &sample);
        if (FAILED(hr)) {
            set_error("ReadSample failed: " + hr_to_string(hr));
            return make_placeholder_frame();
        }

        if ((stream_flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0U) {
            set_error("Capture stream reached end of stream.");
            return make_placeholder_frame();
        }

        if (sample == nullptr) {
            continue;
        }

        ComPtr<IMFMediaBuffer> media_buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&media_buffer))) {
            set_error("ConvertToContiguousBuffer failed.");
            return make_placeholder_frame();
        }

        BYTE* raw_bytes = nullptr;
        DWORD max_length = 0;
        DWORD current_length = 0;
        if (FAILED(media_buffer->Lock(&raw_bytes, &max_length, &current_length))) {
            set_error("IMFMediaBuffer::Lock failed.");
            return make_placeholder_frame();
        }

        npuvt::core::FramePacket frame {
            .frame_id = impl_->next_frame_id++,
            .timestamp = std::chrono::steady_clock::now(),
            .width = static_cast<int>(impl_->width),
            .height = static_cast<int>(impl_->height),
            .format = impl_->pixel_format,
            .image = {},
        };
        frame.image.bytes.resize(current_length);
        std::memcpy(frame.image.bytes.data(), raw_bytes, current_length);

        media_buffer->Unlock();
        return frame;
    }
#endif
}

npuvt::core::FramePacket CaptureService::make_placeholder_frame() const {
    return {
        .frame_id = 0,
        .timestamp = std::chrono::steady_clock::now(),
        .width = 1280,
        .height = 720,
        .format = npuvt::core::PixelFormat::bgra8,
        .image = {},
    };
}

}  // namespace npuvt::capture
