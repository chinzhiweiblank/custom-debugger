#ifndef DBG_HPP
#define DBG_HPP

#include <breakpoint.hpp>
#include <linux/types.h>
#include <string>
#include <unordered_map>
#include <utility>

class Debugger {
public:
  Debugger(std::string prog_name, pid_t pid)
      : m_prog_name{std::move(prog_name)}, m_pid{pid} {}
  void run();
  void set_breakpoint_at_addr(std::intptr_t addr);

private:
  uint64_t read_memory(uint64_t addr);
  void write_memory(uint64_t addr, uint64_t value);
  void dump_registers();
  void handle_command(const std::string &line);
  void continue_execution();
  void step_over_breakpoint();
  uint64_t get_pc();
  void set_pc(uint64_t val);
  void wait_for_signal();
  std::string m_prog_name;
  pid_t m_pid;
  std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
};

#endif