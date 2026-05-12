#include "core/Console.h"
#include <iostream>
#include <sstream>
#include <algorithm>

Console::Console() {
    //commands
    addCommand("help", [this](const std::vector<std::string>& args) {
        log("=== Available commands ===");
        for (auto& [name, helpStr] : m_help)
            log("  " + name + "  —  " + helpStr);
    }, "Show this help");

    addCommand("clear", [this](const std::vector<std::string>&) {
        m_history.clear();
    }, "Clear console history");

    addCommand("echo", [this](const std::vector<std::string>& args) {
        std::string out;
        for (size_t i = 1; i < args.size(); ++i) out += args[i] + " ";
        log(out);
    }, "echo <text>");
}

void Console::log(const std::string& text) {
    std::cout << "[CON] " << text << "\n";
    m_history.push_back(text);
    if (m_history.size() > MAX_HISTORY)
        m_history.erase(m_history.begin());
}

void Console::addCommand(const std::string& name, CmdHandler handler, const std::string& help) {
    m_commands[name] = std::move(handler);
    m_help[name]     = help;
}

void Console::execute(const std::string& line) {
    if (line.empty()) return;
    log("> " + line);

    auto tokens = tokenize(line);
    if (tokens.empty()) return;

    auto it = m_commands.find(tokens[0]);
    if (it == m_commands.end()) {
        log("Unknown command: '" + tokens[0] + "'. Type 'help'.");
        return;
    }
    try {
        it->second(tokens);
    } catch (const std::exception& e) {
        log(std::string("[ERR] Command threw: ") + e.what());
    }
}

void Console::dumpToStdout() const {
    for (auto& line : m_history)
        std::cout << line << "\n";
}

std::vector<std::string> Console::complete(const std::string& prefix) const {
    std::vector<std::string> result;
    for (auto& [name, _] : m_commands)
        if (name.rfind(prefix, 0) == 0)
            result.push_back(name);
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> Console::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}
