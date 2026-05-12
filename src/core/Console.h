#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>

#define LOG(msg)  do { std::ostringstream _oss; _oss << msg; Console::get().log(_oss.str()); } while(0)
#define LOGW(msg) do { std::ostringstream _oss; _oss << "[WARN] " << msg; Console::get().log(_oss.str()); } while(0)
#define LOGE(msg) do { std::ostringstream _oss; _oss << "[ERR]  " << msg; Console::get().log(_oss.str()); } while(0)

using CmdHandler = std::function<void(const std::vector<std::string>& args)>;

class Console {
public:
    static Console& get() {
        static Console inst;
        return inst;
    }

    void log(const std::string& text);
    void execute(const std::string& line);

    // Register a command with a handler and optional help text
    void addCommand(const std::string& name, CmdHandler handler, const std::string& help = "");

    // Lookup command history
    const std::vector<std::string>& history() const { return m_history; }

    // Textual dump of the entire history to stdout (for debugging without ImGui)
    void dumpToStdout() const;

    // Auto-completion based on a prefix
    std::vector<std::string> complete(const std::string& prefix) const;

private:
    Console();
    std::vector<std::string>                          m_history;
    std::unordered_map<std::string, CmdHandler>       m_commands;
    std::unordered_map<std::string, std::string>      m_help;
    static constexpr size_t MAX_HISTORY = 512;

    static std::vector<std::string> tokenize(const std::string& line);
};
