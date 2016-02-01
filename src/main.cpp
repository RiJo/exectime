#include "exectime.hpp"
#include "statistics.hpp"
#include "console.hpp"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <future>
#include <algorithm>
#include <chrono>

void print_usage() {
    std::cout << "usage: " << PROGRAM_NAME << " [--color] [i <x>] <command>" << std::endl;
    std::cout << std::endl;
    std::cout << "Execute a given command and measure the time consumed." << std::endl;
    std::cout << std::endl;
    std::cout << "  --cmp-stdout          Enable stdout comparison per iteration. If stdout differ then fail execution." << std::endl;
    std::cout << "  --color               Colorized output for easier interpretation." << std::endl;
    std::cout << "  --help                Print this help and exit." << std::endl;
    std::cout << "  -i <x>                Number of iterations to execute the command. Default is 1." << std::endl;
    std::cout << "  --ref-stdout=<file>   Enable stdout reference comparison to file contents. If stdout differ then fail execution." << std::endl;
    std::cout << "  --version             Print out version information." << std::endl;
    std::cout << std::endl;
    std::cout << "                  Copyright (C) " PROGRAM_YEAR ". Licensed under " PROGRAM_LICENSE "." << std::endl;
}

void print_version() {
    std::cout << PROGRAM_NAME << " v" PROGRAM_VERSION ", built " __DATE__ " " __TIME__ "." << std::endl;
}

inline std::string tostring(const std::string &item) {
    return item;
}

template<typename T>
inline std::string tostring(const T &item) {
    return std::to_string(item);
}

template<typename T>
std::string join(const T &list, const std::string delimiter) {
    if (list.size() == 0)
        return "";

    std::string result = "";
    for (const auto &item: list) {
        if (result.length() > 0)
            result += delimiter;
        result += tostring(item);
    }
    return result;
}

std::string get_file_contents(const std::string &filename) {
    std::stringstream content;
    std::ifstream stream(filename);
    if (!stream.is_open())
        throw std::runtime_error("Failed to open file for reading: " + filename);

    while (stream.peek() != EOF)
        content << (char) stream.get();
    stream.close();
    return content.str();
}

int main(int argc, const char *argv[]) {
    // Parse arguments
    std::vector<std::string> command;
    bool colorize {false};
    unsigned int iterations {1};
    bool stdout_compare {false};
    bool stdout_ref_set {false};
    std::string stdout_reference {""};
    bool skip_next_arg {false};
    bool command_detected {false};

    for(const auto &arg: console::parse_args(argc, argv)) {
        if (command_detected) {
            if (arg.value.length() > 0)
                command.push_back(arg.key + "=" + arg.value);
            else
                command.push_back(arg.key);
            continue;
        }
        if(skip_next_arg) {
            skip_next_arg = false;
            continue;
        }

        if(arg.key == "--help") {
            print_usage();
            return 0;
        }
        else if(arg.key == "--version" || arg.key == "-v") {
            print_version();
            return 0;
        }
        else if(arg.key == "--color") {
            colorize = true;
        }
        else if (arg.key == "--cmp-stdout") {
            stdout_compare = true;
        }
        else if (arg.key == "--ref-stdout") {
            try {
                stdout_reference = get_file_contents(arg.value);
                stdout_ref_set = true;
            }
            catch (const std::exception &e) {
                std::cerr << console::color::red << PROGRAM_NAME << ": --ref-stdout exception: " << e.what() << console::color::reset << std::endl;
            }
        }
        else if (arg.key == "-i" && arg.next) {
            int temp = std::stoi(arg.next->key); // TODO: sanity check
            if (temp >= 1)
                iterations = temp;
            else
                std::cerr << console::color::red << PROGRAM_NAME << ": Invalid iteration argument, ignoring: " << temp << console::color::reset << std::endl;
            skip_next_arg = true;
        }
        else if (arg.key[0] == '-' && !command_detected) {
            if (arg.value.length() == 0)
                std::cerr << console::color::red << PROGRAM_NAME << ": Unhandled argument flag: \"" << arg.key << "\"" << console::color::reset << std::endl;
            else
                std::cerr << console::color::red << PROGRAM_NAME << ": Unhandled argument key: \"" << arg.key << "\", value: \"" << arg.value << "\"" << console::color::reset << std::endl;
        }
        else {
            if (arg.value.length() > 0)
                command.push_back(arg.key + "=" + arg.value);
            else
                command.push_back(arg.key);
            command_detected = true;
        }
    }

    // Set console properties
    console::color::enable = colorize;

    // Determine tty width
    //~ int columns;
    //~ {
        //~ console::tty temp;
        //~ columns = temp.cols;
    //~ }

    if (command.size() == 0) {
        std::cerr << console::color::red << PROGRAM_NAME << ": No command given" << console::color::reset << std::endl;
        return 1;
    }
    std::string cmd = join(command, " ");

    using time_resolution_t = std::chrono::microseconds;
    std::vector<time_resolution_t> execution_times;

    for (unsigned int iteration = 0; iteration < iterations; iteration++) {
#ifdef DEBUG
        std::cout << PROGRAM_NAME <<  ": Iteration " << (iteration + 1) << "/" << iterations << std::endl;
#endif
        time_resolution_t elapsed;
        std::future<console::exec_result_t> future = std::async(std::launch::async, [&] {
            auto begin = std::chrono::high_resolution_clock::now();
            console::exec_result_t result = console::exec(cmd);
            auto end = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration_cast<time_resolution_t>(end-begin);
            execution_times.push_back(elapsed);
            return  result;

        });

        future.wait();
        console::exec_result_t result { future.get() };

        if (stdout_ref_set || stdout_compare) {
            if (!stdout_ref_set) {
                // Use first iteration's stdout as reference
                stdout_reference = result.stdout;
                stdout_ref_set = true;
            }
            else if (result.stdout != stdout_reference) {
                std::cerr << console::color::red << PROGRAM_NAME << ": stdout comparison failed." << console::color::reset << std::endl;
                std::cerr << console::color::red << PROGRAM_NAME << ":     expected:" << console::color::reset << std::endl;
                std::cerr << stdout_reference << std::endl;
                std::cerr << console::color::red << PROGRAM_NAME << ":     actual:" << console::color::reset << std::endl;
                std::cerr << result.stdout << std::endl;
                return 2;
            }
        }

#ifdef DEBUG
        std::cout << PROGRAM_NAME << ": Execution completed with code " << result.exit_code << ", took " << (elapsed.count() / 1000.0) << "ms" << std::endl;
#endif

#ifdef DEBUG
        std::cout << PROGRAM_NAME << ": Output" << std::endl;
        std::cout << result.stdout << std::endl;
#endif
    }

    if (execution_times.size() == 0) {
        std::cerr << console::color::red << PROGRAM_NAME << ": No time measurements generated" << console::color::reset << std::endl;
        return 3;
    }

    std::function<unsigned long (const time_resolution_t &)> selector = [] (const auto &value) { return value.count(); };
    statistics::statistics_t<unsigned long> s = statistics::calculate(execution_times, selector);

    double standard_deviation1 {0.0};
    double standard_deviation2 {0.0};
    double standard_deviation3 {0.0};
    for (const auto &item: execution_times) {
        auto value = selector(item);
        if (value >= std::min(std::numeric_limits<double>::min(), s.average - s.standard_deviation) && value <= (s.average + s.standard_deviation))
            standard_deviation1++;
        if (value >= std::min(std::numeric_limits<double>::min(), s.average - 2 * s.standard_deviation) && value <= (s.average + 2 * s.standard_deviation))
            standard_deviation2++;
        if (value >= std::min(std::numeric_limits<double>::min(), s.average - 3 * s.standard_deviation) && value <= (s.average + 3 * s.standard_deviation))
            standard_deviation3++;
    }

    // Dump result
#ifdef DEBUG
    std::cout << PROGRAM_NAME << ": cmd \"" << cmd << "\"" << std::endl;
#endif
    //std::cout << PROGRAM_NAME << ": minimum..........................." << (s.minimum / 1000.0) << "ms" << std::endl;
    //std::cout << PROGRAM_NAME << ": maximum..........................." << (s.maximum / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": range............................." << ((s.maximum - s.minimum) / 1000.0) << "ms (" << (s.minimum / 1000.0) << "-" << (s.maximum / 1000.0) << "ms)" << std::endl;
    std::cout << PROGRAM_NAME << ": average/mean......................" << (s.average / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": median............................" << (s.median / 1000.0) << "ms" << std::endl;
    //std::cout << PROGRAM_NAME << ": variance.........................." << (s.variance / 1000.0) << std::endl;
    std::cout << PROGRAM_NAME << ": std. deviation...................." << (s.standard_deviation / 1000.0) << "ms (" << ((s.average - s.standard_deviation) / 1000.0) << "-" << ((s.average + s.standard_deviation) / 1000.0) << "ms)" << std::endl;
    std::cout << PROGRAM_NAME << ": norm. distr. mean±1σ (68.27%)....." << ((static_cast<double>(standard_deviation1) / s.sample_size) * 100.0) << "% (" << standard_deviation1 << "/" << s.sample_size << ")" << std::endl;
    std::cout << PROGRAM_NAME << ":              mean±2σ (95.45%)....." << ((static_cast<double>(standard_deviation2) / s.sample_size) * 100.0) << "% (" << standard_deviation2 << "/" << s.sample_size << ")" << std::endl;
    std::cout << PROGRAM_NAME << ":              mean±3σ (99.73%)....." << ((static_cast<double>(standard_deviation3) / s.sample_size) * 100.0) << "% (" << standard_deviation3 << "/" << s.sample_size << ")" << std::endl;
    std::cout << PROGRAM_NAME << ": std. error........................" << s.standard_error << " (relative: " << ((s.standard_error / s.average) * 100) << "%)" << std::endl;

    // TODO: render graph(s)
    return 0;
}
