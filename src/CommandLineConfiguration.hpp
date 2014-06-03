#ifndef COMMAND_LINE_CONFIGURATION_HPP
#define COMMAND_LINE_CONFIGURATION_HPP

#include <string>
#include <utility>
#include <vector>

namespace Json { class Value; }
namespace TCLAP { class Arg; }

namespace warped {
// An internal class that creates a set of command line parameters based on
// JSON data.
class CommandLineConfiguration {
public:
    CommandLineConfiguration(Json::Value& root)
        : root_(&root) {}

    // Parse command line arguments into the JSON root object and return two
    // values: a flag indicating that the user requested a dump of the
    // configuration and the name of the config file. An empty string is
    // returned as the second value if no config file is specified by the
    // user.
    std::pair<bool, std::string> parse(const std::string& description, int argc,
                                       const char* const* argv,
                                       const std::vector<TCLAP::Arg*>& user_args);

private:
    Json::Value* root_;
};

} // namespace warped

#endif