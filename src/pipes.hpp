#ifndef __PIPES_HPP_INCLUDED__
#define __PIPES_HPP_INCLUDED__

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include <sys/poll.h>
#include <unistd.h> // STDIN_FILENO

namespace pipes {
    class pipe {
        private:
            FILE *fp {nullptr};
        public:
            pipe(const int fd) {
                if ((fp = fdopen(fd, "r")) == nullptr)
                    throw std::runtime_error("Failed to open pipe for reading");
            }

            ~pipe() {
                fclose(fp);
                fp = nullptr;
            }

            std::string read() {
                std::stringstream ss;
                char buffer[32];
                while (!feof(fp)) {
                    if (fgets(buffer, 32, fp) == nullptr)
                        continue;
                    ss << buffer;
                }
                return ss.str();
            }
    };

    std::string read_pipe(const int fd) {
        FILE *fp = fdopen(fd, "r");
        if (fp == nullptr)
            throw std::runtime_error("Failed to open pipe for reading");

        std::stringstream ss;
        char buffer[32];
        while (!feof(fp)) {
            if (fgets(buffer, 32, fp) == nullptr)
                continue;
            ss << buffer;
        }

        fclose(fp);
        fp = nullptr;
        return ss.str();
    }

    bool stdin_has_data(int timeout = 0) {
        if (timeout < 0)
            return true; // Force read (blocking)

        struct pollfd poller;
        poller.fd = STDIN_FILENO;
        poller.events = POLLIN;
        poller.revents = 0;
        return poll(&poller, 1, timeout) > 0;
    }

    std::vector<std::string> read_stdin(char delimiter, int timeout = 0) {
        std::vector<std::string> result;
        if (!stdin_has_data(timeout))
            return result;

        const int buffer_size = 256;
        char buffer[buffer_size];
        while(!std::cin.eof()){
            if (std::cin.fail())
                throw std::runtime_error("stdin read failure.");
            std::cin.getline(buffer, buffer_size, delimiter);
            result.push_back(std::string(buffer));
        }
        return result;
    }

    std::string read_stdin(int timeout = 0) {
        if (!stdin_has_data(timeout))
            return "";

        std::string result {""};
        char c;
        while(!(std::cin.get(c)).eof())
            result += c;
        return result;
    }
}

#endif //__PIPES_HPP_INCLUDED__
