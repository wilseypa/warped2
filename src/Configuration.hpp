#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <memory>
#include <string>
#include <vector>

namespace TCLAP { class Arg; }
namespace Json { class Value; }

namespace warped {

class EventDispatcher;

class Configuration {
public:
    Configuration(const std::string& model_description, int argc, const char* const* argv);
    Configuration(const std::string& model_description, int argc, const char* const* argv,
                  const std::vector<TCLAP::Arg*>& cmd_line_args);
    Configuration(const std::string& config_file_name, unsigned int max_time);
    ~Configuration();
    std::unique_ptr<EventDispatcher> makeDispatcher();

private:
    void init(const std::string& model_description, int argc, const char* const* argv,
              const std::vector<TCLAP::Arg*>& cmd_line_args);
    void readUserConfig();

    std::string config_file_name_;
    unsigned int max_sim_time_;
    std::unique_ptr<Json::Value> root_;
};

} // namespace warped

#endif

