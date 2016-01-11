#include "exectime.hpp"
#include "console.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <algorithm>
#include <cmath>
#include <chrono>

void print_usage() {
    std::cout << "usage: " << PROGRAM_NAME << " [--color] [i <x>] <command>" << std::endl;
    std::cout << std::endl;
    std::cout << "Execute a given command and measure the time consumed." << std::endl;
    std::cout << std::endl;
    std::cout << "  --color     Colorized output for easier interpretation." << std::endl;
    std::cout << "  --help      Print this help and exit." << std::endl;
    std::cout << "  -i <x>      Number of iterations to execute the command. Default is 1." << std::endl;
    std::cout << "  --version   Print out version information." << std::endl;
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

int main(int argc, const char *argv[]) {
    // Parse arguments
    std::vector<std::string> command;
    bool colorize {false};
    unsigned int iterations {1};
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

    std::vector<std::chrono::microseconds> execution_times;

    for (unsigned int iteration = 0; iteration < iterations; iteration++) {
#ifdef DEBUG
        std::cout << PROGRAM_NAME <<  ": Iteration " << (iteration + 1) << "/" << iterations << std::endl;
#endif
        std::chrono::microseconds elapsed;
        std::future<console::exec_result_t> future = std::async(std::launch::async, [&] {
            auto begin = std::chrono::high_resolution_clock::now();
            console::exec_result_t result = console::exec(cmd);
            auto end = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end-begin);
            execution_times.push_back(elapsed);
            return  result;

        });

        future.wait();
        console::exec_result_t result { future.get() };

#ifdef DEBUG
        std::cout << PROGRAM_NAME << ": Execution completed with code " << result.exit_code << ", took " << (elapsed.count() / 1000.0) << "ms" << std::endl;
#endif

#ifdef DEBUG
        std::cout << PROGRAM_NAME << ": Output" << std::endl;
        std::cout << result.stdout << std::endl;
#endif
    }

    size_t size = execution_times.size();
    if (size == 0) {
        std::cerr << console::color::red << PROGRAM_NAME << ": No time measurements generated" << console::color::reset << std::endl;
        return 2;
    }

    // Preprocess data set
    std::sort(execution_times.begin(), execution_times.end());

    // Calculate statistics
    unsigned long max {std::numeric_limits<unsigned long>::min()};
    unsigned long min {std::numeric_limits<unsigned long>::max()};
    unsigned long avg {0}; // mean
    unsigned long var {0}; // variance
    unsigned long sd {0}; // standard deviation
    unsigned long sd1 {0}; // standard deviation item count
    unsigned long sd2 {0}; // standard deviation item count
    unsigned long sd3 {0}; // standard deviation item count

    unsigned long med {0}; // median
    if (size  % 2 == 0)
        med = (execution_times[size / 2 - 1].count() + execution_times[size / 2].count()) / 2;
    else
        med = execution_times[size / 2].count();

    for (const auto &execution_time: execution_times) {
        unsigned long elapsed = execution_time.count();
        if (elapsed < min)
            min = elapsed;
        if (elapsed > max)
            max = elapsed;
        avg += elapsed;
    }
    avg /= size;

    for (const auto &execution_time: execution_times) {
        unsigned long elapsed = execution_time.count();

        var += std::pow(elapsed - avg, 2);
    }
    var /= size;

    sd = std::sqrt(var);

    for (const auto &execution_time: execution_times) {
        unsigned long elapsed = execution_time.count();

        if (elapsed >= std::min(std::numeric_limits<unsigned long>::min(), avg - sd) && elapsed <= (avg + sd))
            sd1++;
        if (elapsed >= std::min(std::numeric_limits<unsigned long>::min(), avg - 2 * sd) && elapsed <= (avg + 2 * sd))
            sd2++;
        if (elapsed >= std::min(std::numeric_limits<unsigned long>::min(), avg - 3 * sd) && elapsed <= (avg + 3 * sd))
            sd3++;
    }

    // Dump result
#ifdef DEBUG
    std::cout << PROGRAM_NAME << ": cmd \"" << cmd << "\"" << std::endl;
#endif
    std::cout << PROGRAM_NAME << ": minimum..........................." << (min / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": maximum..........................." << (max / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": average/mean......................" << (avg / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": median............................" << (med / 1000.0) << "ms" << std::endl;
    //std::cout << PROGRAM_NAME << ": variance.........................." << (var / 1000.0) << std::endl;
    std::cout << PROGRAM_NAME << ": std. deviation...................." << (sd / 1000.0) << "ms" << " (" << ((avg - sd) / 1000.0) << "-" << ((avg + sd) / 1000.0) << "ms)" << std::endl;
    std::cout << PROGRAM_NAME << ": norm.distr. mean±1σ (68.27%)......" << ((static_cast<double>(sd1) / size) * 100.0) << "% (" << sd1 << "/" << size << ")" << std::endl;
    std::cout << PROGRAM_NAME << ":             mean±2σ (95.45%)......" << ((static_cast<double>(sd2) / size) * 100.0) << "% (" << sd2 << "/" << size << ")" << std::endl;
    std::cout << PROGRAM_NAME << ":             mean±3σ (99.73%)......" << ((static_cast<double>(sd3) / size) * 100.0) << "% (" << sd3 << "/" << size << ")" << std::endl;


    // TODO: render graph(s)
    return 0;
}
