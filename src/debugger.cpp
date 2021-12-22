#include "debugger.hpp"
#include "breakpoint.hpp"
#include "linenoise.h"
#include <iostream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/personality.h>
#include <sstream>
#include <vector>

bool is_prefix(const std::string &s, const std::string &prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }
  return std::equal(s.begin(), s.end(), prefix.begin());
}

std::vector<std::string> split_string(const std::string &s,
                                      const char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss{s};
  std::string item;

  while (std::getline(ss, item, delimiter)) {
    result.push_back(item);
  }
  return result;
}

void Debugger::handle_command(const std::string &line) {
  auto args = split_string(line, ' ');
  auto command = args[0];
  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if (is_prefix(command, "break")) {
    std::string addr{args[1], 2};
    set_breakpoint_at_addr(std::stol(addr, 0, 16));
  } else {
    std::cerr << "Unknown command " << command;
  }
}
void Debugger::continue_execution() {
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);
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

void execute_debugee(const std::string& prog_name) {
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    std::cerr << "Error in ptrace\n";
    return;
  }
  execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

void Debugger::set_breakpoint_at_addr(std::intptr_t addr) {
  Breakpoint bp{m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Program name not specified";
    return -1;
  }
  auto prog = argv[1];
  auto pid = fork();
  if (pid == 0) {
    personality(ADDR_NO_RANDOMIZE);
    execute_debugee(prog);
  } else if (pid >= 1) {
    // parent
    std::cout << "Started debugging process " << pid << "\n";
    Debugger dbg{prog, pid};
    dbg.run();
  }
}

