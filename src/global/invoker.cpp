#define INVOKER_MACROS

#include "invoker.h"

#undef INVOKER_MACROS

#include "core/runtime_config.h"

namespace {
    constexpr uint32_t MAX_SUBSTITUTION_NESTING = 5;
}

namespace invoker {
    exec_rules executionRules;
    comp_rules compilationRules;
    substitutions_map substitutions;

    // initializer implementation

    initializer::initializer() {
        if (initialized) {
            throw std::runtime_error(
                    "invoker initializer have been already set");
        }
        init(executionRules, compilationRules, substitutions);
        initialized = true;
    }

    void initializer::addSubstitution(std::string const &pattern, substitution s) {
        substitutions[pattern] = std::move(s);
    }

    void initializer::addExecutionRule(exec_rule_ctor const &rule) {
        addRules(executionRules, rule);
    }

    void initializer::addCompilationRule(comp_rule_ctor const &rule) {
        addRules(compilationRules, rule);
    }

    /*
     * Substitutions:
     * ${COMPILED_PATH} -> ${CACHE_DIR}/${FILENAME}${EXEC_EXT}
     *
     * ${PATH} -> path to unit
     * ${FILENAME} -> filename of unit
     * ${DIR} -> dir where unit is
     * ${DIRNAME} -> name of dir where unit is
     *
     * ${CACHE_DIR} -> ${STRESS_DIR}/cache/
     * ${LOGS_DIR} -> ${STRESS_DIR}/logs/
     * ${STRESS_DIR} -> ${CWD}/stress/
     * ${CWD} -> current working directory
     *
     * ${EXEC_EXT} -> OS-dependent EXEC_EXT value
     * ${SHELL_EXT} -> OS-dependent SHELL_EXT value
     */

    void initializer::init(exec_rules &executor, comp_rules &compiler, substitutions_map &m) {
        addRules(m,
                 S_RULE("${COMPILED_PATH}", "${CACHE_DIR}/${FILENAME}${EXEC_EXT}"),
                 S_RULE_U("${PATH}", u.file.string()),
                 S_RULE_U("${FILENAME}", u.filename()),
                 S_RULE_U("${DIR}", u.file.parent_path().string()),
                 S_RULE_U("${DIRNAME}", u.file.parent_path().filename().string()),
                 S_RULE("${CACHE_DIR}", "${STRESS_DIR}/cache/"),
                 S_RULE("${LOGS_DIR}", "${STRESS_DIR}/logs/"),
                 S_RULE("${STRESS_DIR}", "stress/"),
                 S_RULE("${EXEC_EXT}", invoker::EXEC_EXT),
                 S_RULE("${SHELL_EXT}", invoker::SHELL_EXT));

        addRules(executor,
                 E_RULE(EXEC_EXT, "${PATH}"),
                 E_RULE(SHELL_EXT, "${PATH}"),
                 E_RULE(".class", "java", "-cp", "${DIR}", "${FILENAME}"),
                 E_RULE(".jar", "-jar", "${PATH}"));

        addRules(compiler,
                 C_RULE(".c", "${COMPILED_PATH}", "gcc", "-std=c11",
                        "-static", "-w", "-o", "${COMPILED_PATH}", "${PATH}"),
                 C_RULE(".cpp", "${COMPILED_PATH}", "c++", "-std=c++17",
                        "-static", "-w", "-o", "${COMPILED_PATH}", "${PATH}"),
                 C_RULE(".rs", "${COMPILED_PATH}", "rustc", "-o",
                        "${COMPILED_PATH}", "--crate-name", "test", "${PATH}"),
                 C_RULE(".java", "${CACHE_DIR}/${FILENAME}.class", "javac",
                        "-d", "${CACHE_DIR}", "${PATH}"),
                 C_RULE(".go", "${COMPILED_PATH}", "go", "build", "-o",
                        "${COMPILED_PATH}", "${PATH}"));

        customInit(executor, compiler, m);
    }

    // compiled expressions

    exec_commands executionCommands;
    comp_commands compilationCommands;

    template <typename T>
    auto compileExpr(runtime_config const &cfg, units::unit const &u, T t) {
        auto visitor = [&cfg, &u](auto &arg) {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::string>) {
                return arg;
            } else {
                return arg(cfg, u);
            }
        };
        auto instantiate = [&visitor](std::string &s) {
            uint32_t nesting = 0;
            bool flag = true;

            while (flag) {
                flag = false;
                ++nesting;

                if (nesting > MAX_SUBSTITUTION_NESTING) {
                    throw std::runtime_error("[!] Maximal substitution nesting reached");
                }

                for (auto &j: substitutions) {
                    size_t pos = s.find(j.first);
                    for (; pos != std::string::npos; pos = s.find(j.first, pos)) {
                        s = s.replace(pos, j.first.size(),
                                      std::visit(visitor, j.second));
                        flag = true;
                    }
                }
            }
        };
        if constexpr (std::is_same_v<T, std::string>) {
            instantiate(t);
        } else {
            for (std::string &i: t) {
                instantiate(i);
            }
        }
        return t;
    }

    command const &getExecutionCommand(
            runtime_config const &cfg, units::unit const &u) {
        if (!executionCommands.count(u.cat)) {
            const std::string &ext =
                    u.file.extension().string();

            executionCommands[u.cat] =
                    executionRules.count(ext) ?
                    compileExpr(cfg, u, executionRules.at(ext)) : command();
        }
        return executionCommands[u.cat];
    }

    comp_variant getCompilationCommand(
            runtime_config const &cfg, units::unit const &u) {
        if (compilationCommands.count(u.cat)) {
            return compilationCommands.at(u.cat);
        } else {
            const std::string &ext =
                    u.file.extension().string();

            if (!compilationRules.count(ext)) {
                return compilationCommands[u.cat] = {};
            }

            return std::visit([&cfg, &u](auto &arg) -> comp_variant {
                if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, comp_rule>) {
                    return compilationCommands[u.cat] = comp_rule(
                            compileExpr(cfg, u, arg.first),
                            compileExpr(cfg, u, arg.second));
                } else {
                    return arg;
                }
            }, compilationRules.at(ext));
        }
    }

    // utils

    bool isExecutable(path const &file) {
        return executionRules.count(file.extension().string()) != 0;
    }

    bool isCompilable(path const &file) {
        return compilationRules.count(file.extension().string()) != 0;
    }

    bool compile(runtime_config const & cfg, units::unit & unit) {
        auto variant = getCompilationCommand(cfg, unit);

        return std::visit([&cfg, &unit](auto &arg) -> bool {
            if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, comp_handler>) {
                return arg(unit);
            } else {
                return utils::compile(cfg, unit, path(arg.first), arg.second);
            }
        }, variant);
    }
}

