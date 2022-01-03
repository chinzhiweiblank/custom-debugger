#ifndef DBG_HPP
#define DBG_HPP

#include "breakpoint.hpp"
#include "symbols.hpp"
#include <linux/types.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <fcntl.h>
#include <vector>
#include <dwarf/dwarf++.hh>
#include <elf/elf++.hh>

class Debugger {
public:
  Debugger(std::string prog_name, pid_t pid)
      : m_prog_name{std::move(prog_name)}, m_pid{pid} {
        auto fd = open(m_prog_name.c_str(), O_RDONLY);
        m_elf = elf::elf{elf::create_mmap_loader(fd)};
        m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};
      }
  void run();

private:
  void dump_registers();
  void handle_command(const std::string &line);
  void continue_execution();
  uint64_t get_pc();
  void set_pc(uint64_t val);
  void wait_for_signal();
  void init_load_addr();
  // Set breakpoints
  void set_breakpoint_at_addr(std::intptr_t addr);
  void remove_breakpoint(std::intptr_t addr);
  void set_breakpoint_at_function(const std::string& name);
  void set_breakpoint_at_source_line(const std::string& filename, unsigned line);
  // Stepping functions
  void step_out();
  void step_in();
  void step_over();
  void single_step_over();
  void step_over_breakpoint();
  void single_step_over_with_breakpoint_check();
  // Signal functions
  void handle_sigtrap(siginfo_t siginfo);
  siginfo_t get_signal_info();
  // Memory functions
  uint64_t read_memory(uint64_t addr);
  void write_memory(uint64_t addr, uint64_t value);
  // DWARF functions
  dwarf::die get_function_from_pc(uint64_t pc);
  dwarf::line_table::iterator get_line_entry_from_pc(uint64_t pc);
  void print_source(const std::string& file_name, unsigned line, unsigned n_lines_context);
  uint64_t offset_load_addr(uint64_t addr);
  uint64_t offset_dwarf_addr(uint64_t addr);
  uint64_t get_offset_pc();
  std::vector<symbol> lookup_symbol(const std::string& name);
  std::string m_prog_name;
  dwarf::dwarf m_dwarf;
  elf::elf m_elf;
  pid_t m_pid;
  uint64_t m_load_addr;
  std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
};

#endif