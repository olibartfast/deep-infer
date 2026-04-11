#include "DeepStreamPipeline.hpp"

#include <utility>

DeepStreamPipeline::DeepStreamPipeline(const Config& config)
    : pipeline_(nullptr)
    , source_(nullptr)
    , decoder_(nullptr)
    , streammux_(nullptr)
    , pgie_(nullptr)
    , nvvidconv_(nullptr)
    , nvosd_(nullptr)
    , sink_(nullptr)
    , tracker_(nullptr)
    , analytics_(nullptr)
    , loop_(nullptr)
    , bus_(nullptr)
    , bus_watch_id_(0)
    , config_(config)
    , logger_(Logger::GetInstance()) {}

DeepStreamPipeline::~DeepStreamPipeline() {
    Stop();
}

bool DeepStreamPipeline::Initialize() {
    logger_.Error(
        "This build was configured without the NVIDIA DeepStream SDK. "
        "Install DeepStream and rerun CMake with -DDEEPSTREAM_DIR=<path> "
        "to enable the runtime pipeline.");
    return false;
}

bool DeepStreamPipeline::Run() {
    logger_.Error("DeepStream runtime support is unavailable in this build.");
    return false;
}

void DeepStreamPipeline::Stop() {}

bool DeepStreamPipeline::CreateElements() {
    return false;
}

bool DeepStreamPipeline::LinkElements() {
    return false;
}

bool DeepStreamPipeline::ConfigureElements() {
    return false;
}

GstPadProbeReturn DeepStreamPipeline::OsdSinkPadBufferProbe(
    GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer /*user_data*/) {
    return GST_PAD_PROBE_OK;
}

gboolean DeepStreamPipeline::BusCall(
    GstBus* /*bus*/, GstMessage* /*msg*/, gpointer /*data*/) {
    return TRUE;
}

bool DeepStreamPipeline::CreatePrimaryGIE() {
    return false;
}

bool DeepStreamPipeline::CreateTracker() {
    return false;
}

bool DeepStreamPipeline::CreateAnalytics() {
    return false;
}

std::string DeepStreamPipeline::GetSourceType() const {
    if (config_.source.find("rtsp://") == 0 ||
        config_.source.find("rtmp://") == 0) {
        return "rtsp";
    }
    return "uri";
}

void DeepStreamPipeline::SetFrameCallback(FrameCallback callback) {
    frame_callback_ = std::move(callback);
}

void DeepStreamPipeline::SetConfig(const Config& config) {
    config_ = config;
}
