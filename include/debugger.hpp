#ifndef DBG_HPP
#define DBG_HPP

#include "breakpoint.hpp"
#include <linux/types.h>
#include <string>
#include <unordered_map>
#include <utility>

namespace dbg {
class Debugger {
public:
  Debugger(std::string prog_name, pid_t pid)
      : m_prog_name{std::move(prog_name)}, m_pid{pid} {}
  void run();
  void set_breakpoint_at_addr(std::intptr_t addr);

private:
  void handle_command(const std::string &line);
  void continue_execution();
  std::string m_prog_name;
  pid_t m_pid;
  std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
};
} // namespace dbg

#endif