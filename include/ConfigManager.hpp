#ifndef DEEP_INFER_INCLUDE_CONFIG_MANAGER_HPP_
#define DEEP_INFER_INCLUDE_CONFIG_MANAGER_HPP_

#include <iosfwd>

#include "Config.hpp"

class Logger;

struct ParseResult {
  Config config;
  bool help_requested = false;
};

class ConfigManager {
 public:
  // Parse command-line arguments into a Config. Both "--opt value" and
  // "--opt=value" forms are accepted. "--help"/"-h" prints usage and exits.
  static Config ParseCommandLine(int argc, const char* argv[]);
  static ParseResult ParseCommandLine(int argc, const char* argv[], std::ostream& help_out,
                                      Logger& logger);

 private:
  static void PrintHelp(std::ostream& out);
};

#endif  // DEEP_INFER_INCLUDE_CONFIG_MANAGER_HPP_
