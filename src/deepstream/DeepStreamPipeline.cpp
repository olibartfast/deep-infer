#include "DeepStreamPipeline.hpp"
#include "utils.hpp"
#include <stdexcept>

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
    , logger_(Logger::GetInstance())
{
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
    
    // Add probe to get buffers
    GstPad* osd_sink_pad = gst_element_get_static_pad(nvosd_, "sink");
    if (!osd_sink_pad) {
        logger_.Error("Failed to get OSD sink pad");
        return false;
    }
    
    gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                     OsdSinkPadBufferProbe, this, nullptr);
    gst_object_unref(osd_sink_pad);
    
    // Create main loop
    loop_ = g_main_loop_new(nullptr, FALSE);
    
    // Add bus watch
    bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    bus_watch_id_ = gst_bus_add_watch(bus_, BusCall, this);
    gst_object_unref(bus_);
    
    logger_.Info("DeepStream pipeline initialized successfully");
    return true;
}

bool DeepStreamPipeline::CreateElements() {
    // Create pipeline
    pipeline_ = gst_pipeline_new("deepstream-pipeline");
    if (!pipeline_) {
        logger_.Error("Failed to create pipeline");
        return false;
    }
    
    // Determine source type and create appropriate element
    std::string source_type = GetSourceType();
    
    if (source_type == "uri") {
        source_ = gst_element_factory_make("uridecodebin", "uri-source");
        g_object_set(G_OBJECT(source_), "uri", 
                    (std::string("file://") + config_.source).c_str(), nullptr);
    } else if (source_type == "rtsp") {
        source_ = gst_element_factory_make("rtspsrc", "rtsp-source");
        g_object_set(G_OBJECT(source_), "location", config_.source.c_str(), nullptr);
    } else {
        logger_.Error("Unsupported source type");
        return false;
    }
    
    if (!source_) {
        logger_.Error("Failed to create source element");
        return false;
    }
    
    // Create other elements
    streammux_ = gst_element_factory_make("nvstreammux", "stream-muxer");
    pgie_ = gst_element_factory_make("nvinfer", "primary-inference");
    nvvidconv_ = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");
    nvosd_ = gst_element_factory_make("nvdsosd", "nv-onscreendisplay");
    
    // Create sink based on output configuration
    if (config_.show_frame || config_.write_frame) {
        sink_ = gst_element_factory_make("nveglglessink", "nvvideo-renderer");
    } else {
        sink_ = gst_element_factory_make("fakesink", "fake-renderer");
    }
    
    if (!streammux_ || !pgie_ || !nvvidconv_ || !nvosd_ || !sink_) {
        logger_.Error("Failed to create pipeline elements");
        return false;
    }
    
    // Create tracker if enabled
    if (config_.use_tracker) {
        if (!CreateTracker()) {
            return false;
        }
    }
    
    // Create analytics if enabled
    if (config_.use_analytics) {
        if (!CreateAnalytics()) {
            return false;
        }
    }
    
    return true;
}

bool DeepStreamPipeline::CreatePrimaryGIE() {
    if (!config_.config_file.empty()) {
        g_object_set(G_OBJECT(pgie_), "config-file-path", 
                    config_.config_file.c_str(), nullptr);
    } else {
        logger_.Error("Config file required for primary GIE");
        return false;
    }
    
    return true;
}

bool DeepStreamPipeline::CreateTracker() {
    tracker_ = gst_element_factory_make("nvtracker", "tracker");
    if (!tracker_) {
        logger_.Error("Failed to create tracker element");
        return false;
    }
    
    if (!config_.tracker_config.empty()) {
        g_object_set(G_OBJECT(tracker_), "ll-config-file",
                    config_.tracker_config.c_str(), nullptr);
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
    // Configure streammux
    g_object_set(G_OBJECT(streammux_),
                "width", config_.input_width,
                "height", config_.input_height,
                "batch-size", config_.batch_size,
                "batched-push-timeout", 4000000,
                "gpu-id", config_.gpu_id,
                nullptr);
    
    // Configure primary GIE
    if (!CreatePrimaryGIE()) {
        return false;
    }
    
    // Configure OSD
    g_object_set(G_OBJECT(nvosd_),
                "process-mode", 1,  // GPU mode
                "display-text", 1,
                "gpu-id", config_.gpu_id,
                nullptr);
    
    // Configure sink
    g_object_set(G_OBJECT(sink_),
                "sync", 0,
                "async", 0,
                nullptr);
    
    return true;
}

bool DeepStreamPipeline::LinkElements() {
    gst_bin_add_many(GST_BIN(pipeline_),
                    source_, streammux_, pgie_, nvvidconv_, nvosd_, sink_,
                    nullptr);
    
    // Link elements based on configuration
    if (config_.use_tracker && tracker_) {
        gst_bin_add(GST_BIN(pipeline_), tracker_);
    }
    
    if (config_.use_analytics && analytics_) {
        gst_bin_add(GST_BIN(pipeline_), analytics_);
    }
    
    // Link the pipeline
    // Source -> StreamMux -> PGIE -> [Tracker] -> [Analytics] -> NvVidConv -> OSD -> Sink
    
    if (!gst_element_link_many(streammux_, pgie_, nullptr)) {
        logger_.Error("Failed to link streammux and pgie");
        return false;
    }
    
    GstElement* last_element = pgie_;
    
    if (config_.use_tracker && tracker_) {
        if (!gst_element_link(last_element, tracker_)) {
            logger_.Error("Failed to link tracker");
            return false;
        }
        last_element = tracker_;
    }
    
    if (config_.use_analytics && analytics_) {
        if (!gst_element_link(last_element, analytics_)) {
            logger_.Error("Failed to link analytics");
            return false;
        }
        last_element = analytics_;
    }
    
    if (!gst_element_link_many(last_element, nvvidconv_, nvosd_, sink_, nullptr)) {
        logger_.Error("Failed to link remaining elements");
        return false;
    }
    
    return true;
}

bool DeepStreamPipeline::Run() {
    logger_.Info("Starting DeepStream pipeline...");
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        logger_.Error("Failed to set pipeline to PLAYING state");
        return false;
    }
    
    logger_.Info("Pipeline running...");
    g_main_loop_run(loop_);
    
    return true;
}

void DeepStreamPipeline::Stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline_));
        pipeline_ = nullptr;
    }
    
    if (loop_) {
        g_main_loop_unref(loop_);
        loop_ = nullptr;
    }
    
    if (bus_watch_id_) {
        g_source_remove(bus_watch_id_);
        bus_watch_id_ = 0;
    }
}

std::string DeepStreamPipeline::GetSourceType() const {
    if (config_.source.find("rtsp://") == 0 || 
        config_.source.find("rtmp://") == 0) {
        return "rtsp";
    }
    return "uri";
}

GstPadProbeReturn DeepStreamPipeline::OsdSinkPadBufferProbe(
    GstPad* pad, GstPadProbeInfo* info, gpointer user_data) {
    
    DeepStreamPipeline* pipeline = static_cast<DeepStreamPipeline*>(user_data);
    
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    NvDsBatchMeta* batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    
    if (!batch_meta) {
        return GST_PAD_PROBE_OK;
    }
    
    // Process each frame in the batch
    for (NvDsMetaList* l_frame = batch_meta->frame_meta_list; 
         l_frame != nullptr; l_frame = l_frame->next) {
        
        NvDsFrameMeta* frame_meta = (NvDsFrameMeta*)(l_frame->data);
        
        // Get surface from buffer
        NvBufSurface* surface = nullptr;
        GstMapInfo map_info;
        
        if (gst_buffer_map(buf, &map_info, GST_MAP_READ)) {
            surface = (NvBufSurface*)map_info.data;
            
            // Convert to OpenCV Mat if callback is set
            if (pipeline->frame_callback_ && surface) {
                // Extract frame data and create Result
                Result result;
                // Process metadata and create result
                // This would be implemented based on specific task
                
                pipeline->frame_callback_(result);
            }
            
            gst_buffer_unmap(buf, &map_info);
        }
    }
    
    return GST_PAD_PROBE_OK;
}

gboolean DeepStreamPipeline::BusCall(GstBus* bus, GstMessage* msg, gpointer data) {
    DeepStreamPipeline* pipeline = static_cast<DeepStreamPipeline*>(data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            pipeline->logger_.Info("End of stream");
            g_main_loop_quit(pipeline->loop_);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar* debug;
            GError* error;
            gst_message_parse_error(msg, &error, &debug);
            pipeline->logger_.Errorf("Error: %s", error->message);
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(pipeline->loop_);
            break;
        }
        
        default:
            break;
    }
    
    return TRUE;
}

void DeepStreamPipeline::SetFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

void DeepStreamPipeline::SetConfig(const Config& config) {
    config_ = config;
}
