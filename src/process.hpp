#ifndef __PROCESS_HPP_INCLUDED__
#define __PROCESS_HPP_INCLUDED__

#include "pipes.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>

#include <unistd.h> // close(), fork(), execv(), dup2(), STDOUT_FILENO, STDERR_FILENO
#include <sys/wait.h> // waitpid()
#include <string.h> // strdup()

namespace process {
    struct exec_result_t {
        int exit_code;
        std::string stdout;
        std::string stderr;
    };

#ifdef FALSE
    exec_result_t run(const std::string &command) {
        FILE* fp = popen(command.c_str(), "r");
        if (fp == nullptr)
            throw std::runtime_error("Failed to open pipe: \"" + command + "\"");

        std::stringstream ss;
        char buffer[32];
        while (!feof(fp)) {
            if (fgets(buffer, 32, fp) == nullptr)
                continue;
            ss << buffer;
        }

        int pclose_status = pclose(fp);
        fp = nullptr;

        return exec_result_t {WEXITSTATUS(pclose_status), ss.str()};
    }
#endif

    exec_result_t run(const std::vector<std::string> &command) {
        if (command.size() == 0)
            throw std::runtime_error("No command given");

        int fd_stdout[2]; // [0]=read, [1]=write
        if (pipe(fd_stdout) != 0)
            throw std::runtime_error("pipe() stdout:" + std::to_string(errno));

        int fd_stderr[2]; // [0]=read, [1]=write
        if (pipe(fd_stderr) != 0)
            throw std::runtime_error("pipe() stderr:" + std::to_string(errno));

        pid_t pid = fork(); // TODO: use vfork() instead?
        if (pid < 0)
            throw std::runtime_error("fork(): " + std::to_string(errno));

        bool is_parent = (pid > 0);
        if (is_parent) {
            // Parent process
            close(fd_stdout[1]);
            close(fd_stderr[1]);

            int status;
            pid_t ws = waitpid(pid, &status, 0);
            if (ws != pid)
                throw std::runtime_error("Failed to wait for pid " + std::to_string(pid));

            std::string output_stdout = pipes::read_pipe(fd_stdout[0]);
            close(fd_stdout[0]);

            std::string output_stderr = pipes::read_pipe(fd_stderr[0]);
            close(fd_stderr[0]);

            int exit_code = -1;
            if (WIFEXITED(status)) {
                exit_code = WEXITSTATUS(status);
            }
#ifdef DEBUG
            else if (WIFSIGNALED(status)) {
                if (output_stderr.length() > 0)
                    output_stderr += '\n';
                output_stderr += "killed by signal %d\n", WTERMSIG(status);
            }
            else if (WIFSTOPPED(status)) {
                if (output_stderr.length() > 0)
                    output_stderr += '\n';
                output_stderr += "stopped by signal %d\n", WSTOPSIG(status);
            }
            else if (WIFCONTINUED(status)) {
                if (output_stderr.length() > 0)
                    output_stderr += '\n';
                output_stderr += "continued\n";
            }
            else {
                if (output_stderr.length() > 0)
                    output_stderr += '\n';
                output_stderr += "unhandled exit status\n";
            }
#endif

            return exec_result_t {exit_code, output_stdout, output_stderr};
        }
        else {
            // Child process
            if (dup2(fd_stdout[1], STDOUT_FILENO) < 0)
                throw std::runtime_error("dup2() stdout: " + std::to_string(errno));
            if (dup2(fd_stderr[1], STDERR_FILENO) < 0)
                throw std::runtime_error("dup2() stderr: " + std::to_string(errno));

            close(fd_stdout[0]);
            close(fd_stdout[1]);
            close(fd_stderr[0]);
            close(fd_stderr[1]);

            unsigned int i = 0;
            char * exec_args[1024];
            for (i = 0; i < command.size(); i++)
               exec_args[i] = strdup(command[i].c_str());
            exec_args[i] = '\0';
            execv(command[0].c_str(), exec_args);
            throw std::runtime_error("execv(): " + std::to_string(errno));
        }
    }
}

#endif //__PROCESS_HPP_INCLUDED__
