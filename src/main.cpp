#include "exectime.hpp"
#include "console.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <chrono>

void print_usage(const std::string &application) {
    std::cout << "usage: " << PROGRAM_NAME << " [--color] [i <x>] <command>" << std::endl;
    std::cout << std::endl;
    std::cout << "Execute a given command and measure the time consumed." << std::endl;
    std::cout << std::endl;
    std::cout << "  --color     Colorized output for easier interpretation." << std::endl;
    std::cout << "  --help      Print this help and exit." << std::endl;
    std::cout << "  -i          Number of iterations to execute the command. Default is 1." << std::endl;
    std::cout << "  --version   Print out version information." << std::endl;
    std::cout << std::endl;
    std::cout << "                  Copyright (C) " PROGRAM_YEAR ". Licensed under " PROGRAM_LICENSE "." << std::endl;
}

void print_version(const std::string &application) {
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
            print_usage(std::string(argv[0]));
            return 0;
        }
        else if(arg.key == "--version" || arg.key == "-v") {
            print_version(std::string(argv[0]));
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

    std::string foo = join(command, " ");
    for (unsigned int iteration = 0; iteration < iterations; iteration++) {
    #ifdef DEBUG
        std::cout << PROGRAM_NAME <<  ": Iteration " << (iteration + 1) << "/" << iterations << std::endl;
        std::cout << PROGRAM_NAME <<  ": Executing \"" << foo << "\"..." << std::endl;
    #endif
        unsigned long elapsed_ns {0};
        std::future<exec_result_t> future = std::async(std::launch::async, [&] {
            auto begin = std::chrono::high_resolution_clock::now();
            exec_result_t result = exec(foo);
            auto end = std::chrono::high_resolution_clock::now();
            elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
            return result;

        });

        future.wait();
        exec_result_t result { future.get() };

    #ifdef DEBUG
        std::cout << PROGRAM_NAME << ": Execution completed with code " << result.exit_code << std::endl;
    #endif

    #ifdef DEBUG
        std::cout << PROGRAM_NAME << ": Output" << std::endl;
        std::cout << result.stdout << std::endl;
    #endif
        std::cout << "took " << (elapsed_ns / 1000.0 / 1000.0) << "ms" << std::endl;
    }
    return 0;
}
