#pragma once

#include "units/unit.h"
#include "core/run.h"
#include <variant>
#include <functional>

#ifdef INVOKER_MACROS
#define E_RULE(key, ...) \
    exec_rule_ctor(key, std::vector<std::string>({__VA_ARGS__}))

#define C_RULE(key, ext, ...) \
    comp_rule_ctor(key, E_RULE(ext, __VA_ARGS__))

#define C_HANDLER(key, func) \
    comp_handler(key, func)

#define S_HANDLER_U(expr) \
    [](runtime_config const&, units::unit const& u) \
    -> std::string {return expr;}

#define S_HANDLER_CFG(expr) \
    [](runtime_config const& cfg, units::unit const&) \
    -> std::string {return expr;}

#define S_RULE(key, str) \
    substituion_ctor(key, str)

#define S_RULE_U(key, expr) \
    substituion_ctor(key, S_HANDLER_U(expr))

#define S_RULE_CFG(key, expr) \
    substituion_ctor(key, S_HANDLER_CFG(expr))
#endif

namespace invoker {
    using path = std::filesystem::path;

    // substitutions aliases
    using substituion_handler = std::function<
            std::string(runtime_config const &, units::unit const &)>;
    using substitution = std::variant<std::string, substituion_handler>;
    using substituion_ctor = std::pair<std::string, substitution>;
    using substitutions_map = std::unordered_map<std::string, substitution>;

    // execution rules aliases
    using command = std::vector<std::string>;
    using exec_rule = command;
    using exec_rule_ctor = std::pair<std::string, command>;
    using exec_rules = std::unordered_map<std::string, exec_rule>;

    // compilation rules aliases
    using comp_rule = std::pair<std::string, command>;
    using comp_handler = std::function<bool(units::unit &)>;
    using comp_variant = std::variant<comp_handler, comp_rule>;
    using comp_rule_ctor = std::pair<std::string, comp_variant>;
    using comp_rules = std::unordered_map<std::string, comp_variant>;

    // compiled expressions aliases
    using exec_commands = std::unordered_map<units::unit_category, command>;
    using comp_commands = std::unordered_map<units::unit_category, comp_rule>;

    // for example:
    // ".exe" and ".bat" on Windows
    // "" and ".sh on Linux
    extern const char EXEC_EXT[];
    extern const char SHELL_EXT[];

    extern exec_rules executionRules;
    extern comp_rules compilationRules;
    extern substitutions_map substitutions;

    class initializer {
    public:
        initializer();

        void addSubstitution(std::string const &, substitution);

        void addExecutionRule(exec_rule_ctor const &);

        void addCompilationRule(comp_rule_ctor const &);

    private:
        template<typename T, typename R>
        static void addRules(T &rules, R const &rule);

        template<typename T, typename R, typename... Args>
        static void addRules(
                T &rules, R const &rule, Args const &... args);

        // OS-independent
        static void init(exec_rules &, comp_rules &, substitutions_map &);

        // OS-dependent
        static void customInit(exec_rules &, comp_rules &, substitutions_map &);

        inline static bool initialized = false;
    };

    extern exec_commands executionCommands;
    extern comp_commands compilationCommands;

    command const &getExecutionCommand(
            runtime_config const &, units::unit const &);

    comp_variant getCompilationCommand(
            runtime_config const &, units::unit const &);

    // check if unit can be executed
    bool isExecutable(path const &);

    // check if unit needs to be compiled
    bool isCompilable(path const &);

    // os-specific execution
    bool execute(runtime_config const &,
                 units::unit const &,
                 std::string const &,
                 std::string &,
                 std::string &,
                 execution_result &);

    // compile unit
    bool compile(runtime_config const &, units::unit &);

    namespace utils {
        // os-specific compilation
        bool compile(runtime_config const &, units::unit &, path&&, command const&);
    }
}
