#ifndef DEEP_INFER_INCLUDE_DEEPSTREAM_PIPELINE_HPP_
#define DEEP_INFER_INCLUDE_DEEPSTREAM_PIPELINE_HPP_

#include "common.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include <functional>
#include <memory>

class DeepStreamPipeline {
 public:
  explicit DeepStreamPipeline(const Config& config);
  ~DeepStreamPipeline();
  
  // Pipeline control
  bool Initialize();
  bool Run();
  void Stop();
  
  // Configuration
  void SetConfig(const Config& config);
  Config GetConfig() const { return config_; }
  
  // Callbacks
  using FrameCallback = std::function<void(const AppResult&)>;
  void SetFrameCallback(FrameCallback callback);
    
 private:
  // GStreamer pipeline elements
  GstElement* pipeline_;
  GstElement* source_;
  GstElement* decoder_;
  GstElement* streammux_;
  GstElement* pgie_;  // Primary GIE (inference engine)
  GstElement* nvvidconv_;
  GstElement* nvosd_;  // On-screen display
  GstElement* egltransform_;
  GstElement* sink_;
  GstElement* tracker_;  // Optional tracker
  
  // Optional analytics
  GstElement* analytics_;
  
  // Pipeline state
  GMainLoop* loop_;
  GstBus* bus_;
  guint bus_watch_id_;
  
  Config config_;
  Logger& logger_;
  FrameCallback frame_callback_;
  
  // Pipeline building
  bool CreateElements();
  bool LinkElements();
  bool ConfigureElements();
  
  // Callbacks
  static GstPadProbeReturn OsdSinkPadBufferProbe(
      GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
  static gboolean BusCall(GstBus* bus, GstMessage* msg, gpointer data);
  static void DecodebinPadAdded(GstElement* decodebin, GstPad* pad, gpointer user_data);
  
  // Helper methods
  bool CreatePrimaryGIE();
  bool CreateTracker();
  bool CreateAnalytics();
  std::string BuildSourceUri() const;
  bool IsLiveSource() const;
  std::string GetSourceType() const;
};

#endif  // DEEP_INFER_INCLUDE_DEEPSTREAM_PIPELINE_HPP_
