#ifndef BREAKPOINT_HPP
#define BREAKPOINT_HPP
namespace dbg {
class Breakpoint {
public:
  Breakpoint(pid_t pid, std::intptr_t addr)
      : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{} {}
  void enable();
  void disable();

  auto is_enabled() const -> bool { return m_enabled; }
  auto get_address() const -> std::intptr_t { return m_addr; }

private:
  pid_t m_pid;
  std::intptr_t m_addr;
  bool m_enabled;
  uint8 m_saved_data;
}
} // namespace dbg
#endif