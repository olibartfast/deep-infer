#include "DeepStreamPipeline.hpp"
#include "utils.hpp"

#include <filesystem>
#include <string>
#include <stdexcept>

namespace {
constexpr const char* kNvmmMemoryFeature = "memory:NVMM";
}

DeepStreamPipeline::DeepStreamPipeline(const Config& config)
    : pipeline_(nullptr)
    , source_(nullptr)
    , decoder_(nullptr)
    , streammux_(nullptr)
    , pgie_(nullptr)
    , nvvidconv_(nullptr)
    , nvosd_(nullptr)
    , egltransform_(nullptr)
    , sink_(nullptr)
    , tracker_(nullptr)
    , analytics_(nullptr)
    , loop_(nullptr)
    , bus_(nullptr)
    , bus_watch_id_(0)
    , config_(config)
    , logger_(Logger::GetInstance()) {
    gst_init(nullptr, nullptr);
}

DeepStreamPipeline::~DeepStreamPipeline() {
    Stop();
}

bool DeepStreamPipeline::Initialize() {
    logger_.Info("Initializing DeepStream pipeline...");

    if (!CreateElements()) {
        logger_.Error("Failed to create pipeline elements");
        return false;
    }

    if (!ConfigureElements()) {
        logger_.Error("Failed to configure pipeline elements");
        return false;
    }

    if (!LinkElements()) {
        logger_.Error("Failed to link pipeline elements");
        return false;
    }

    GstPad* osd_sink_pad = gst_element_get_static_pad(nvosd_, "sink");
    if (!osd_sink_pad) {
        logger_.Error("Failed to get OSD sink pad");
        return false;
    }

    gst_pad_add_probe(
        osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER, OsdSinkPadBufferProbe, this, nullptr);
    gst_object_unref(osd_sink_pad);

    loop_ = g_main_loop_new(nullptr, FALSE);
    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    bus_watch_id_ = gst_bus_add_watch(bus_, BusCall, this);
    gst_object_unref(bus_);
    bus_ = nullptr;

    logger_.Info("DeepStream pipeline initialized successfully");
    return true;
}

bool DeepStreamPipeline::CreateElements() {
    pipeline_ = gst_pipeline_new("deepstream-pipeline");
    if (!pipeline_) {
        logger_.Error("Failed to create pipeline");
        return false;
    }

    source_ = gst_element_factory_make("uridecodebin", "uri-source");
    streammux_ = gst_element_factory_make("nvstreammux", "stream-muxer");
    pgie_ = gst_element_factory_make("nvinfer", "primary-inference");
    nvvidconv_ = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");
    nvosd_ = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");

    if (config_.show_frame) {
        egltransform_ = gst_element_factory_make("nvegltransform", "nvvideo-transform");
        sink_ = gst_element_factory_make("nveglglessink", "nvvideo-renderer");
    } else {
        sink_ = gst_element_factory_make("fakesink", "fake-renderer");
    }

    if (!source_ || !streammux_ || !pgie_ || !nvvidconv_ || !nvosd_ || !sink_) {
        logger_.Error("Failed to create pipeline elements");
        return false;
    }

    const std::string source_uri = BuildSourceUri();
    g_object_set(G_OBJECT(source_), "uri", source_uri.c_str(), nullptr);
    g_signal_connect(source_, "pad-added", G_CALLBACK(DecodebinPadAdded), this);

    if (config_.use_tracker && !CreateTracker()) {
        return false;
    }

    if (config_.use_analytics && !CreateAnalytics()) {
        return false;
    }

    return true;
}

bool DeepStreamPipeline::CreatePrimaryGIE() {
    if (!config_.config_file.empty()) {
        g_object_set(
            G_OBJECT(pgie_), "config-file-path", config_.config_file.c_str(), nullptr);
        return true;
    }

    logger_.Error("Config file required for primary GIE");
    return false;
}

bool DeepStreamPipeline::CreateTracker() {
    tracker_ = gst_element_factory_make("nvtracker", "tracker");
    if (!tracker_) {
        logger_.Error("Failed to create tracker element");
        return false;
    }

    if (!config_.tracker_config.empty()) {
        g_object_set(
            G_OBJECT(tracker_), "ll-config-file", config_.tracker_config.c_str(), nullptr);
    }

    return true;
}

bool DeepStreamPipeline::CreateAnalytics() {
    analytics_ = gst_element_factory_make("nvdsanalytics", "analytics");
    if (!analytics_) {
        logger_.Error("Failed to create analytics element");
        return false;
    }

    return true;
}

bool DeepStreamPipeline::ConfigureElements() {
    g_object_set(
        G_OBJECT(streammux_),
        "width", config_.input_width,
        "height", config_.input_height,
        "batch-size", config_.batch_size,
        "batched-push-timeout", 4000000,
        "gpu-id", config_.gpu_id,
        "live-source", IsLiveSource(),
        nullptr);

    if (!CreatePrimaryGIE()) {
        return false;
    }

    g_object_set(
        G_OBJECT(nvosd_),
        "process-mode", 1,
        "display-text", 1,
        "gpu-id", config_.gpu_id,
        nullptr);

    g_object_set(G_OBJECT(sink_), "sync", 0, "async", 0, nullptr);
    return true;
}

bool DeepStreamPipeline::LinkElements() {
    gst_bin_add_many(
        GST_BIN(pipeline_), source_, streammux_, pgie_, nvvidconv_, nvosd_, nullptr);

    if (tracker_) {
        gst_bin_add(GST_BIN(pipeline_), tracker_);
    }

    if (analytics_) {
        gst_bin_add(GST_BIN(pipeline_), analytics_);
    }

    if (egltransform_) {
        gst_bin_add(GST_BIN(pipeline_), egltransform_);
    }

    gst_bin_add(GST_BIN(pipeline_), sink_);

    if (!gst_element_link(streammux_, pgie_)) {
        logger_.Error("Failed to link streammux and pgie");
        return false;
    }

    GstElement* last_element = pgie_;

    if (tracker_) {
        if (!gst_element_link(last_element, tracker_)) {
            logger_.Error("Failed to link tracker");
            return false;
        }
        last_element = tracker_;
    }

    if (analytics_) {
        if (!gst_element_link(last_element, analytics_)) {
            logger_.Error("Failed to link analytics");
            return false;
        }
        last_element = analytics_;
    }

    if (!gst_element_link_many(last_element, nvvidconv_, nvosd_, nullptr)) {
        logger_.Error("Failed to link nvvidconv and nvosd");
        return false;
    }

    if (egltransform_) {
        if (!gst_element_link_many(nvosd_, egltransform_, sink_, nullptr)) {
            logger_.Error("Failed to link display sink chain");
            return false;
        }
    } else if (!gst_element_link(nvosd_, sink_)) {
        logger_.Error("Failed to link sink");
        return false;
    }

    return true;
}

bool DeepStreamPipeline::Run() {
    logger_.Info("Starting DeepStream pipeline...");

    const GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        logger_.Error("Failed to set pipeline to PLAYING state");
        return false;
    }

    logger_.Info("Pipeline running...");
    g_main_loop_run(loop_);
    return true;
}

void DeepStreamPipeline::Stop() {
    if (loop_ != nullptr && g_main_loop_is_running(loop_)) {
        g_main_loop_quit(loop_);
    }

    if (bus_watch_id_ != 0) {
        g_source_remove(bus_watch_id_);
        bus_watch_id_ = 0;
    }

    if (pipeline_ != nullptr) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline_));
        pipeline_ = nullptr;
    }

    if (loop_ != nullptr) {
        g_main_loop_unref(loop_);
        loop_ = nullptr;
    }
}

std::string DeepStreamPipeline::BuildSourceUri() const {
    if (IsLiveSource()) {
        return config_.source;
    }

    const std::filesystem::path input_path(config_.source);
    const std::filesystem::path resolved_path =
        input_path.is_absolute() ? input_path : std::filesystem::absolute(input_path);
    return "file://" + resolved_path.string();
}

bool DeepStreamPipeline::IsLiveSource() const {
    return GetSourceType() == "rtsp";
}

std::string DeepStreamPipeline::GetSourceType() const {
    if (config_.source.find("rtsp://") == 0 || config_.source.find("rtmp://") == 0) {
        return "rtsp";
    }
    return "uri";
}

void DeepStreamPipeline::DecodebinPadAdded(
    GstElement* /*decodebin*/, GstPad* pad, gpointer user_data) {
    auto* pipeline = static_cast<DeepStreamPipeline*>(user_data);

    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    if (!caps) {
        pipeline->logger_.Error("Failed to query source pad caps");
        return;
    }

    const GstStructure* structure = gst_caps_get_structure(caps, 0);
    const gchar* media_type = gst_structure_get_name(structure);
    if (!media_type || !g_str_has_prefix(media_type, "video/")) {
        gst_caps_unref(caps);
        return;
    }

    GstCapsFeatures* features = gst_caps_get_features(caps, 0);
    if (!features || !gst_caps_features_contains(features, kNvmmMemoryFeature)) {
        pipeline->logger_.Error("Decoded source pad does not use NVMM memory");
        gst_caps_unref(caps);
        return;
    }

    GstPad* sink_pad = gst_element_get_request_pad(pipeline->streammux_, "sink_0");
    if (!sink_pad) {
        pipeline->logger_.Error("Failed to get nvstreammux sink pad");
        gst_caps_unref(caps);
        return;
    }

    if (gst_pad_is_linked(sink_pad)) {
        gst_object_unref(sink_pad);
        gst_caps_unref(caps);
        return;
    }

    const GstPadLinkReturn link_result = gst_pad_link(pad, sink_pad);
    if (link_result != GST_PAD_LINK_OK) {
        pipeline->logger_.Errorf(
            "Failed to link decodebin to nvstreammux: %d", static_cast<int>(link_result));
    }

    gst_object_unref(sink_pad);
    gst_caps_unref(caps);
}

GstPadProbeReturn DeepStreamPipeline::OsdSinkPadBufferProbe(
    GstPad* /*pad*/, GstPadProbeInfo* info, gpointer user_data) {
    auto* pipeline = static_cast<DeepStreamPipeline*>(user_data);

    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) {
        return GST_PAD_PROBE_OK;
    }

    NvDsBatchMeta* batch_meta = gst_buffer_get_nvds_batch_meta(buffer);
    if (!batch_meta) {
        return GST_PAD_PROBE_OK;
    }

    for (NvDsMetaList* frame_node = batch_meta->frame_meta_list;
         frame_node != nullptr;
         frame_node = frame_node->next) {
        auto* frame_meta = static_cast<NvDsFrameMeta*>(frame_node->data);
        if (!pipeline->frame_callback_) {
            continue;
        }

        AppResult result;
        for (NvDsMetaList* object_node = frame_meta->obj_meta_list;
             object_node != nullptr;
             object_node = object_node->next) {
            auto* object_meta = static_cast<NvDsObjectMeta*>(object_node->data);

            vision_core::Detection detection;
            detection.bbox = cv::Rect2f(
                object_meta->rect_params.left,
                object_meta->rect_params.top,
                object_meta->rect_params.width,
                object_meta->rect_params.height);
            detection.class_confidence = object_meta->confidence;
            detection.class_id = object_meta->class_id;
            result.results.push_back(detection);
        }

        pipeline->frame_callback_(result);
    }

    return GST_PAD_PROBE_OK;
}

gboolean DeepStreamPipeline::BusCall(
    GstBus* /*bus*/, GstMessage* msg, gpointer data) {
    auto* pipeline = static_cast<DeepStreamPipeline*>(data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            pipeline->logger_.Info("End of stream");
            g_main_loop_quit(pipeline->loop_);
            break;

        case GST_MESSAGE_ERROR: {
            gchar* debug = nullptr;
            GError* error = nullptr;
            gst_message_parse_error(msg, &error, &debug);
            pipeline->logger_.Errorf("Error: %s", error != nullptr ? error->message : "unknown");
            if (debug != nullptr) {
                g_free(debug);
            }
            if (error != nullptr) {
                g_error_free(error);
            }
            g_main_loop_quit(pipeline->loop_);
            break;
        }

        default:
            break;
    }

    return TRUE;
}

void DeepStreamPipeline::SetFrameCallback(FrameCallback callback) {
    frame_callback_ = std::move(callback);
}

void DeepStreamPipeline::SetConfig(const Config& config) {
    config_ = config;
}
