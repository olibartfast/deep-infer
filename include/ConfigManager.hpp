#ifndef DEEP_INFER_INCLUDE_CONFIG_MANAGER_HPP_
#define DEEP_INFER_INCLUDE_CONFIG_MANAGER_HPP_

#include "Config.hpp"

class ConfigManager {
 public:
  // Parse command-line arguments into a Config. Both "--opt value" and
  // "--opt=value" forms are accepted. "--help"/"-h" prints usage and exits.
  static Config ParseCommandLine(int argc, const char* argv[]);

 private:
  static void PrintHelp();
};

#endif  // DEEP_INFER_INCLUDE_CONFIG_MANAGER_HPP_
