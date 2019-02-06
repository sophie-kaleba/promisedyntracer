#include "recorder.h"
#include "lookup.h"

void write_environment_variables(const std::string &filepath) {
    std::ofstream fout(filepath, std::ios::trunc);

    auto serialize_row = [&fout](const std::string &key,
                                 const std::string &value) {
        fout << key << "=" << value << std::endl;
    };

    serialize_row("R_COMPILE_PKGS", to_string(getenv("R_COMPILE_PKGS")));
    serialize_row("R_DISABLE_BYTECODE",
                  to_string(getenv("R_DISABLE_BYTECODE")));
    serialize_row("R_ENABLE_JIT", to_string(getenv("R_ENABLE_JIT")));
    serialize_row("R_KEEP_PKG_SOURCE", to_string(getenv("R_KEEP_PKG_SOURCE")));
}

void write_configuration(const Context &context, const std::string &filepath) {
    std::ofstream fout(filepath, std::ios::trunc);
    const AnalysisSwitch &analysis_switch{context.get_analysis_switch()};
    auto serialize_row = [&fout](const std::string &key,
                                 const std::string &value) {
        fout << key << "=" << value << std::endl;
    };

    serialize_row("truncate", std::to_string(context.get_truncate()));
    serialize_row("verbose", std::to_string(context.is_verbose()));
    serialize_row("binary", std::to_string(context.is_binary()));
    serialize_row("compression_level",
                  std::to_string(context.get_compression_level()));
    serialize_row("GIT_COMMIT_INFO", GIT_COMMIT_INFO);
}
