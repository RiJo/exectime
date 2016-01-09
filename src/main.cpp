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

    for(const auto &arg: console::parse_args(argc, argv)) {
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
            iterations = std::stoi(arg.next->key); // TODO: sanity check
            skip_next_arg = true;
        }
        else {
            command.push_back(arg.key); // TODO: may parse exec args wrong
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

    std::vector<std::chrono::microseconds> execution_times;
    std::string foo = join(command, " ");
    for (unsigned int iteration = 0; iteration < iterations; iteration++) {
    #ifdef DEBUG
        std::cout << PROGRAM_NAME <<  ": Iteration " << (iteration + 1) << "/" << iterations << std::endl;
    #endif
        std::chrono::microseconds elapsed;
        std::future<exec_result_t> future = std::async(std::launch::async, [&] {
            auto begin = std::chrono::high_resolution_clock::now();
            exec_result_t result = exec(foo);
            auto end = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end-begin);
            execution_times.push_back(elapsed);
            return  result;

        });

        future.wait();
        exec_result_t result { future.get() };

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
        std::cout << "Nothing..." << std::endl;
        return 0;
    }

    // Preprocess data set
    std::sort(execution_times.begin(), execution_times.end());

    // Calculate statistics
    unsigned long max {std::numeric_limits<unsigned long>::min()};
    unsigned long min {std::numeric_limits<unsigned long>::max()};
    unsigned long avg {0}; // mean
    unsigned long var {0}; // variance
    unsigned long sd {0}; // standard deviation

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
    var = std::sqrt(var);

    sd = std::sqrt(var);

    // Dump result
    std::cout << std::endl;
#ifdef DEBUG
    std::cout << PROGRAM_NAME << ": cmd \"" << foo << "\"" << std::endl;
#endif
    std::cout << PROGRAM_NAME << ": min " << (min / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": max " << (max / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": avg " << (avg / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": med " << (med / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": var " << (var / 1000.0) << "ms" << std::endl;
    std::cout << PROGRAM_NAME << ": sd  " << (sd / 1000.0) << "ms" << " (" << ((avg - sd) / 1000.0) << "-" << ((avg + sd) / 1000.0) << "ms)" << std::endl;

    // TODO: render graph(s)
    return 0;
}
