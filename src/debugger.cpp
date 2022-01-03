#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "linenoise.h"
#include "debugger.hpp"
#include "regs.hpp"
#include "utils.hpp"
#include "breakpoint.hpp"
#include "symbols.hpp"

void Debugger::handle_command(const std::string &line) {
  auto args = split_string(line, ' ');
  auto command = args[0];
  if (is_prefix(command, "continue")) {
    continue_execution();
  } else if (is_prefix(command, "break")) {
    if (args[1][0] == '0' && args[1][1] == 'x') {
      // address
      std::string addr{args[1], 2};
      set_breakpoint_at_addr(std::stol(addr, 0, 16));
    } else if (args[1].find(":") != std::string::npos) {
      // file and line
      auto file_and_line = split_string(args[1], ':');
      set_breakpoint_at_source_line(file_and_line[0], std::stoi(file_and_line[1]));
    } else {
      // function
      set_breakpoint_at_function(args[1]);
    }
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
    } else if (is_prefix(command, "stepi")) {
      single_step_over_with_breakpoint_check();
      auto line_entry = get_line_entry_from_pc(get_pc());
      print_source(line_entry->file->path, line_entry->line, 2U);
    } else if (is_prefix(command, "step")) {
      step_in();
    } else if (is_prefix(command, "next")) {
      step_over();
    } else if (is_prefix(command, "finish")) {
      step_out();
    } else if (is_prefix(command, "symbol")) {
      auto symbols = lookup_symbol(args[1]);
      for (auto&& s : symbols) {
        std::cout << s.name << ' ' << to_string(s.type) << " 0x" << std::hex << s.addr << std::endl;
      }
    } else {
    std::cerr << "Unknown command " << command << std::endl;
  }
}

void Debugger::step_out() {
    auto frame_pointer = get_register_value(m_pid, reg::rbp);
    auto return_address = read_memory(frame_pointer+8); // Function return address

    bool should_remove_breakpoint = false;
    if (!m_breakpoints.count(return_address)) {
        set_breakpoint_at_addr(return_address);
        should_remove_breakpoint = true;
    }

    continue_execution();

    if (should_remove_breakpoint) {
        remove_breakpoint(return_address);
    }
}

void Debugger::step_in() {
   auto line = get_line_entry_from_pc(get_offset_pc())->line;

   while (get_line_entry_from_pc(get_offset_pc())->line == line) {
      single_step_over_with_breakpoint_check();
   }

   auto line_entry = get_line_entry_from_pc(get_offset_pc());
   print_source(line_entry->file->path, line_entry->line, 2U);
}

dwarf::die Debugger::get_function_from_pc(uint64_t pc) {
  for (auto &cu : m_dwarf.compilation_units()) {
    if (die_pc_range(cu.root()).contains(pc)) {
      for (const auto &die : cu.root()) {
        if (die.tag == dwarf::DW_TAG::subprogram) {
          if (die_pc_range(die).contains(pc)) {
            return die;
          }
        }
      }
    }
  }
  throw std::out_of_range("Cannot find function for"+pc);
}
void Debugger::step_over() {
  const auto func = get_function_from_pc(get_offset_pc());
  const auto func_entry = at_low_pc(func);
  const auto func_end = at_high_pc(func);

  auto line = get_line_entry_from_pc(func_entry);
  const auto start_line = get_line_entry_from_pc(get_offset_pc());

  std::vector<std::intptr_t> to_delete{};
  while (line->address < func_end) {
    auto load_addr = offset_dwarf_addr(line->address);
    if (line->address != start_line->address && !m_breakpoints.count(load_addr)) {
      set_breakpoint_at_addr(load_addr);
      to_delete.push_back(load_addr);
    }
    ++line;
  }
  const auto frame_ptr = get_register_value(m_pid, reg::rbp);
  const auto return_addr = read_memory(frame_ptr+8); // 64 bit architecture
  if (!m_breakpoints.count(return_addr)) {
    set_breakpoint_at_addr(return_addr);
    to_delete.push_back(return_addr);
  }
  for (auto addr : to_delete) {
    remove_breakpoint(addr);
  }
}

uint64_t Debugger::offset_dwarf_addr(uint64_t addr) {
  return addr + m_load_addr;
}
uint64_t Debugger::get_offset_pc() {
   return offset_load_addr(get_pc());
}

void Debugger::remove_breakpoint(std::intptr_t addr) {
    if (m_breakpoints.at(addr).is_enabled()) {
        m_breakpoints.at(addr).disable();
    }
    m_breakpoints.erase(addr);
}

dwarf::line_table::iterator Debugger::get_line_entry_from_pc(uint64_t pc) {
    for (auto &cu : m_dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            auto &lt = cu.get_line_table();
            auto it = lt.find_address(pc);
            if (it == lt.end()) {
                throw std::out_of_range{"Cannot find line entry"};
            }
            else {
                return it;
            }
        }
    }

    throw std::out_of_range{"Cannot find line entry"};
}
void Debugger::continue_execution() {
  step_over_breakpoint();
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
  wait_for_signal();
}

void Debugger::init_load_addr() {
  if (m_elf.get_hdr().type == elf::et::dyn) {
    std::ifstream map("/proc" + std::to_string(m_pid) + "/maps");

    std::string addr;
    std::getline(map, addr, '-');

    m_load_addr = std::stoi(addr, 0, 16);
  }
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

void Debugger::set_breakpoint_at_function(const std::string& name) {
  for (const auto& cu : m_dwarf.compilation_units()) {
    for (const auto& die : cu.root()) {
      if (die.has(dwarf::DW_AT::name) && at_name(die) == name) {
        auto low_pc = at_low_pc(die);
        auto entry = get_line_entry_from_pc(get_offset_pc());
        ++entry;
        set_breakpoint_at_addr(offset_dwarf_addr(entry->address));
      }
    }
  }
}

void Debugger::set_breakpoint_at_source_line(const std::string& filename, unsigned line) {
  for (const auto& cu: m_dwarf.compilation_units()) {
    for (const auto& die : cu.root()) {
      if (is_suffix(filename, at_name(cu.root()))) {
        const auto& lt = cu.get_line_table();
        for (const auto& line_entry : lt) {
          if (line_entry.is_stmt && line_entry.line == line) {
            set_breakpoint_at_addr(offset_dwarf_addr(line_entry.address));
            return;
          }
        }
      }
    }
  }
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
  auto siginfo = get_signal_info();
  
  switch (siginfo.si_signo) {
  case SIGTRAP:
    handle_sigtrap(siginfo);
    break;
  case SIGSEGV:
    std::cout << "segfault: " << siginfo.si_code << std::endl;
    break;
  default:
    std::cout << "Unknown signal: " << siginfo.si_code << std::endl;
  }
}

void Debugger::handle_sigtrap(siginfo_t siginfo) {
  switch (siginfo.si_code) {
  case SI_KERNEL:
  case TRAP_BRKPT:
  {
    set_pc(get_pc()-1);
    auto line_entry = get_line_entry_from_pc(get_offset_pc());
    print_source(line_entry->file->path, line_entry->line, 2U);
    return;
  }
  case TRAP_TRACE:
    return;
  default:
    std::cout << "Received signal code: " << siginfo.si_code << std::endl;
    return;
  }
}

uint64_t Debugger::offset_load_addr(uint64_t addr) {
  return addr - m_load_addr;
}


void Debugger::print_source(const std::string& file_name, unsigned line, unsigned n_lines_context) {
  std::ifstream file {file_name};
  auto start_line = line <= n_lines_context ? 1 : line - n_lines_context;
  auto end_line = line + n_lines_context + (line < n_lines_context ? n_lines_context - line : 0) + 1;

  char c{};
  auto cur_line = 1u;
  while (cur_line != start_line && file.get(c)) {
    if (c == '\n') {
      ++cur_line;
    }
  }
  
  std::cout << (cur_line==line? "> " : " ");
  while (cur_line <= end_line && file.get(c)) {
    std::cout << c;
    if (c == '\n') {
      ++cur_line;
      std::cout << (cur_line==line?"> ": " ");
    }
  }
  std::cout << std::endl;
}

siginfo_t Debugger::get_signal_info() {
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &info);
    return info;
}

void Debugger::single_step_over() {
  ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
  wait_for_signal();
}

void Debugger::step_over_breakpoint() {
  if (m_breakpoints.count(get_pc())) {
    auto& bp = m_breakpoints[get_pc()];
    if (bp.is_enabled()) {
      bp.disable();
      ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
      wait_for_signal();
      bp.enable();
    }
  }
}

void Debugger::single_step_over_with_breakpoint_check() {
  if (m_breakpoints.count(get_pc())) {
    step_over_breakpoint();
  } else {
    single_step_over();
  }
}
void Debugger::dump_registers() {
  for (const auto &rd : g_register_descriptors) {
    std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16)
              << std::hex << get_register_value(m_pid, rd.r) << std::endl;
  }
}

std::vector<symbol> Debugger::lookup_symbol(const std::string& name) {
    std::vector<symbol> syms;

    for (auto &sec : m_elf.sections()) {
        if (sec.get_hdr().type != elf::sht::symtab && sec.get_hdr().type != elf::sht::dynsym)
            continue;

        for (auto sym : sec.as_symtab()) {
            if (sym.get_name() == name) {
                auto &d = sym.get_data();
                syms.push_back(symbol{to_symbol_type(d.type()), sym.get_name(), d.value});
            }
        }
    }

    return syms;
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
