#include "debugger.hpp"
#include "breakpoint.hpp"
#include "utils.hpp"
#include "linenoise.h"
#include "regs.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void Debugger::handle_command(const std::string &line) {
  auto args = split_string(line, ' ');
  auto command = args[0];
  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if (is_prefix(command, "break")) {
    std::string addr{args[1], 2};
    set_breakpoint_at_addr(std::stol(addr, 0, 16));
  } else if (is_prefix(command, "register")) {
      if (is_prefix(args[1], "dump")) {
        dump_registers();
      } else if (is_prefix(args[1], "read")) {
        std::cout << get_register_value(m_pid, get_register_from_name(args[2])) << std::endl;
      } else if (is_prefix(args[1], "write")) {
        std::string val {args[3], 2};
        set_register_value(m_pid, get_register_from_name(args[2]), std::stol(val, 0, 16));
      }
    } else if (is_prefix(command, "memory")) {
        auto addr = std::stol(args[2], 0, 16);
        if (is_prefix(args[1], "read")) {
          std::cout << read_memory(addr) << std::endl;
        } else if (is_prefix(args[1], "write")) {
          std::string val {args[3], 2};
          write_memory(addr, std::stol(val, 0 , 16));
        }
    } else {
    std::cerr << "Unknown command " << command << std::endl;
  }
}

void Debugger::continue_execution() {
  step_over_breakpoint();
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
  wait_for_signal();
}

void Debugger::run() {
  wait_for_signal();

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

uint64_t Debugger::read_memory(uint64_t addr) {
  return ptrace(PTRACE_PEEKDATA, m_pid, addr, nullptr);
}

void Debugger::write_memory(uint64_t addr, uint64_t value) {
  ptrace(PTRACE_POKEDATA, m_pid, addr, value);
}

void Debugger::set_breakpoint_at_addr(std::intptr_t addr) {
  Breakpoint bp{m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
}

uint64_t Debugger::get_pc() {
  return get_register_value(m_pid, reg::rip);
}

void Debugger::set_pc(uint64_t val) {
  set_register_value(m_pid, reg::rip, val);
}

void Debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);
}

void Debugger::step_over_breakpoint() {
  auto possible_bp_loc = get_pc()-1;
  if (m_breakpoints.count(possible_bp_loc)) {
    auto& bp = m_breakpoints[possible_bp_loc];
    if (bp.is_enabled()) {
      set_pc(possible_bp_loc);
      bp.disable();
      ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
      wait_for_signal();
      bp.enable();
      std::cout << "Breakpoint disabled";
    }
  }
}
void Debugger::dump_registers() {
  for (const auto &rd : g_register_descriptors) {
    std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16)
              << std::hex << get_register_value(m_pid, rd.r) << std::endl;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Program name not specified" << std::endl;
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
