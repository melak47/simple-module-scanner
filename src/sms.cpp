#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <lyra/lyra.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace sms {
    #ifdef _WIN32
    auto wide_to_utf8(std::wstring_view wide) -> std::string {
        auto input_length = static_cast<int>(wide.length());
        auto requested = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), input_length, nullptr, 0, nullptr, nullptr);
        if (!requested) {
            throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte failed");
        }

        std::string buffer(requested, '\0');
        if (!::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), input_length, buffer.data(), requested, nullptr, nullptr)) {
            throw std::system_error(::GetLastError(), std::system_category(), "WideCharToMultiByte failed");
        }
        return buffer;
    }
    #endif

    auto entry(int argc, char* argv[]) -> int {

        std::string source_path;
        std::string key_target;
        std::string dyndep_path;
        std::string module_dir;
        bool show_help = false;

        auto cli = lyra::cli_parser()
            | lyra::help(show_help)
                .description("Simple module scanner for C++20.")
            | lyra::arg(source_path, "source")
                .required()
                .help("input source file")
            | lyra::opt(key_target, "key")
                .required()
                .name("--key")
                .help("rule output key file (usually an object file)")
            | lyra::opt(module_dir, "module_dir")
                .required()
                .name("--module_dir")
                .help("module directory")
            | lyra::opt(dyndep_path, "dyndep")
                .required()
                .name("--dyndep")
                .help("dyndep output file")
        ;

        auto result = cli.parse({argc, argv});
        if (show_help) {
            std::cerr << "simple module scanner for C++20\n" << cli << '\n';
            return 0;
        }

        if (!result) {
            std::cerr << "error: " << result.errorMessage() << '\n';
            std::cerr << cli << '\n';
            return 1;
        }

        std::ofstream dyndep_file;
        std::ostream& dyndep = [&]() -> std::ostream& {
            if (dyndep_path.empty()) {
                return std::cout;
            }
            else {
                dyndep_file.open(dyndep_path);
                if (!dyndep_file) {
                    std::cerr << "error: failed to open output file: " << dyndep_path << "\n";
                    std::exit(1);
                }
                return dyndep_file;
            }
        }();

        dyndep << "ninja_dyndep_version = 1\n\n";

        std::ifstream source{source_path};
        if (!source) {
            std::cerr << "error: no such file: " << source_path << "\n";
            return 1;
        }

        std::regex module{R"(\s*module\s+([\w:\.]+)\s*;\s*)"};
        std::regex import{R"~~(\s*(?:export\s+)?import\s+(?:([\w:\.]+)|"([\w\.]+)"|<([\w\.]+)>)\s*;\s*)~~"};

        dyndep << "build " << key_target << ": dyndep";

        auto emit_dep = [&, once = true](auto&& module_name) mutable {
            if (once) {
                dyndep << " |";
                once = false;
            }

            dyndep << ' ' << module_dir << '/' << module_name << ".ifc";
        };

        for (std::string line; std::getline(source, line); ) {
            std::cmatch match;
            if (std::regex_match(line.c_str(), match, module)) {
                emit_dep(match[1].str());
            }
            else if (std::regex_match(line.c_str(), match, import)) {
                if (match[1].length()) {
                    emit_dep(match[1].str());
                }
                else if (match[2].length()) {
                    emit_dep(match[2].str());
                }
                else if (match[3].length()) {
                    emit_dep(match[3].str());
                }
            }
        }
        dyndep << '\n';

        return 0;
    }
}

#ifdef _WIN32

int wmain(int argc, wchar_t* argv[]) {
    std::vector<std::string> narrow_args(argc);
    std::vector<char*> args(argc + 1);
    for (int i = 0; i < argc; ++i) {
        auto& arg = narrow_args[i] = sms::wide_to_utf8(argv[i]);
        args[i] = arg.data();
    }

    return sms::entry(argc, args.data());
}

#else

int main(int argc, char* argv[]) {
    return sms::entry(argc, argv);
}

#endif
