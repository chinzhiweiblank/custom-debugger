#include "debugger.hpp"
#include "linenoise.h"
#include "utils.hpp"
#include <iostream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace dbg;

void Debugger::handle_command(const std::string &line) {
  auto args = split_string(line, ' ');
  auto command = args[0];
  if (is_prefix(command, "continue")) {
    continue_execution();
  } else {
    std::cerr << "Unknown command " << command;
  }
}

void Debugger::run() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  char *line = nullptr;
  while ((line = linenoise("dbg>")) != nullptr) {
    handle_command(line);
    linenoiseHistoryAdd(line);
    linenoiseFree(line);
  }
}

void execute_debugee(const std::string &prog_name) {
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    std::cerr << "Error in ptrace\n";
    return;
  }
  execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Program name not specified";
    return -1;
  }
  auto prog = argv[1];
  auto pid = fork();
  if (pid == 0) {

  } else if (pid >= 1) {
    // parent
    std::cout << "Started debugging process " << pid << "\n";
    Debugger dbg{prog, pid};
    dbg.run();
  }
}