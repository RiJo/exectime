#ifndef __PROCESS_HPP_INCLUDED__
#define __PROCESS_HPP_INCLUDED__

#include <stdexcept>

// fork()
#include <iostream>
#include <sstream>
#include <string>
// Required by for routine
#include <sys/types.h>
#include <unistd.h> // dup2()
//#include <stdlib.h> // exit()

//

#include <sys/wait.h>

//
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

    // TODO: use pipes::read(int): move to pipes.hpp!
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

        //~ int ch;
        //~ while ((ch = getc(fp)) != EOF)
            //~ putc(ch, stdout);

        //~ fflush(stdout);
        fclose(fp);
        fp = nullptr;
        return ss.str();
    }

    exec_result_t run(const std::string &command) {
        int fd_stdout[2]; // [0]=read, [1]=write
        if (pipe(fd_stdout) != 0)
            throw std::runtime_error("pipe() stdout:" + std::to_string(errno));

        int fd_stderr[2]; // [0]=read, [1]=write
        if (pipe(fd_stderr) != 0)
            throw std::runtime_error("pipe() stderr:" + std::to_string(errno));

        pid_t pid = fork(); // TODO: use vfork() instead?
        if (pid < 0)
            throw std::runtime_error("fork(): " + command);

        bool is_parent = (pid > 0);
        if (is_parent) {
            // Parent process
            close(fd_stdout[1]);
            close(fd_stderr[1]);

            int status;
            pid_t ws = waitpid(pid, &status, 0);
            if (ws != pid)
                throw std::runtime_error("Failed to wait for pid " + std::to_string(pid));

            std::string output_stdout = read_pipe(fd_stdout[0]);
            close(fd_stdout[0]);

            std::string output_stderr = read_pipe(fd_stderr[0]);
            close(fd_stderr[0]);

            int exit_code = -1;
            std::cout << "pid:" << pid << ", ws:" << ws << ", status:" << status << ", WEXITSTATUS(status):" << WEXITSTATUS(status) << ", errno:" << errno << std::endl;
            if (WIFEXITED(status)) {
                //printf("exited, status=%d\n", WEXITSTATUS(status));
                exit_code = WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n", WTERMSIG(status));
            }
            else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d\n", WSTOPSIG(status));
            }
            else if (WIFCONTINUED(status)) {
                printf("continued\n");
            }
            else {
                printf("unhandled\n");
            }

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

            execl(command.c_str(), command.c_str(), "/tmp");
            throw std::runtime_error("execv(): " + std::to_string(errno));
        }
    }
}

#endif //__PROCESS_HPP_INCLUDED__
