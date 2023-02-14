#include "program.h"

#include <algorithm>

#include "../processor.h"


#ifndef NDEBUG
#include <iostream>
# define DRETURN(err, msg) do {                                 \
    const auto err_ = (err);                                    \
    std::cerr << "Err " << err_ << ": " << msg << std::endl;    \
    return err_;                                                \
  } while (0)
#else
# define DRETURN(err, msg) return (err)
#endif


namespace util {

namespace {

bool matches_processor(const ::Processor &proc, const Program &prog)
{
  const auto *cons = proc.m_pConstructorObject;
  auto type = prog.target_processor_type();

  if (cons == nullptr || type.empty()) {
    return true;
  }

  for (int i = 0; i < nProcessorNames; ++i) {
    if (type == cons->names[i])
      return true;
  }

  return false;
}

}

Program::Program(std::vector<CodeRange> &&code,
                 std::vector<SourceSymbol> &&symbols,
                 std::vector<SourceLineRef> &&line_refs,
                 std::vector<SourceDirective> &&directives,
                 std::string_view target_processor_type)
  : m_code(std::move(code)),
    m_symbols(std::move(symbols)),
    m_line_refs(std::move(line_refs)),
    m_directives(std::move(directives)),
    m_target_proc_type(target_processor_type)
{
  intern_strings();

  std::sort(m_code.begin(), m_code.end(),
            [](const CodeRange &a, const CodeRange &b) { return a.addr < b.addr; });
  std::sort(m_line_refs.begin(), m_line_refs.end(),
            [](const SourceLineRef &a, const SourceLineRef &b) { return a.addr < b.addr; });
  std::sort(m_directives.begin(), m_directives.end(),
            [](const SourceDirective &a, const SourceDirective &b) {
              if (a.addr != b.addr)
                return a.addr < b.addr;
              return a.type < b.type;
            });
}

void Program::intern_strings()
{
  // Intern the SourceLineRef::file string_view.
  for (auto &ref : m_line_refs) {
    auto s = std::string(ref.file);
    auto it = m_strings.find(s);
    if (it == m_strings.end()) {
      m_strings.insert(s);
      it = m_strings.find(s);
    }

    ref.file = *it;
  }
}

int Program::build_indices()
{
  build_symbols_indices();

  if (int err=  build_lines_index(); err)
    return err;

  build_directives_index();

  return 0;
}

const CodeRange* Program::find_code_range(uint64_t addr) const
{
  auto it = std::lower_bound(m_code.begin(), m_code.end(), addr,
                             [](const auto &r, uint64_t addr) { return r.addr < addr; });

  if (it == m_code.end())
    return nullptr;

  if (addr >= it->addr + it->code.size())
    return nullptr;

  return &*it;
}

u8string Program::code_at(uint64_t addr, std::size_t size, uint8_t fill) const
{
  u8string out;

  while (size) {
    const CodeRange *r = find_code_range(addr);

    if (!r) break;

    if (r->addr > addr) {
      // There is a gap to the next range.
      std::size_t n = r->addr - addr;
      if (n > size) n = size;

      out.append(n, fill);
      addr += n;
      size -= n;
    }

    std::size_t n = addr - r->addr + r->code.size();
    if (n > size) n = size;

    out.append(r->code, addr - r->addr, n);

    addr += n;
    size -= n;
  }

  return out;
}

std::vector<const SourceSymbol*> Program::find_symbols(std::string_view name) const
{
  auto it = m_symbols_by_name.find(name);

  if (it == m_symbols_by_name.end())
    return {};

  return std::vector<const SourceSymbol*>(it->second.begin(), it->second.end());
}

std::vector<const SourceSymbol*> Program::find_symbols(SourceSymbolType type, int value) const
{
  auto [start, end] = m_symbols_by_value.equal_range(std::make_pair(type, value));
  std::vector<const SourceSymbol*> out;
  std::transform(start, end, std::back_inserter(out), [](const auto &entry) { return entry.second; });
  return out;
}

void Program::build_symbols_indices()
{
  for (auto &sym : m_symbols) {
    m_symbols_by_name[sym.name].push_back(&sym);
    m_symbols_by_value.insert({{sym.type, sym.value}, &sym});
  }
}

std::vector<const SourceLineRef*> Program::find_lines(std::string_view file, int line) const
{
  auto [start, end] = m_lines_by_loc.equal_range(std::make_pair(file, line));
  std::vector<const SourceLineRef*> out;
  std::transform(start, end, std::back_inserter(out), [](const auto &entry) { return entry.second; });
  return out;
}

std::vector<const SourceLineRef*> Program::find_lines(uint64_t addr) const
{
  auto start = std::lower_bound(m_line_refs.rbegin(), m_line_refs.rend(), addr,
                                [](const auto &ref, uint64_t addr) { return ref.addr > addr; });

  if (start == m_line_refs.rend())
    return {};

  std::vector<const SourceLineRef*> out;
  for (auto it = start; it != m_line_refs.rend() && it->addr == start->addr; ++it) {
    out.push_back(&*it);
  }
  return out;
}

int Program::build_lines_index()
{
  for (auto &ref : m_line_refs) {
    m_lines_by_loc.insert({std::make_pair(ref.file, ref.line), &ref});
  }

  return 0;
}

std::vector<const SourceDirective*> Program::find_directives_by_type(std::string_view type) const
{
  auto it = m_directives_by_type.find(type);

  if (it == m_directives_by_type.end())
    return {};

  return std::vector<const SourceDirective*>(it->second.begin(), it->second.end());
}

std::vector<const SourceDirective*> Program::find_directives(uint64_t addr) const
{
  auto start = std::lower_bound(m_directives.rbegin(), m_directives.rend(), addr,
                                [](const auto &dir, uint64_t addr) { return dir.addr > addr; });

  if (start == m_directives.rend())
    return {};

  std::vector<const SourceDirective*> out;
  for (auto it = start; it != m_directives.rend() && it->addr == start->addr; ++it) {
    out.push_back(&*it);
  }
  return out;
}

void Program::build_directives_index()
{
  for (auto &dir : m_directives) {
    m_directives_by_type[dir.type].push_back(&dir);
  }
}

int upload(::Processor *proc, const Program &prog)
{
  if (!matches_processor(*proc, prog))
    DRETURN(EINVAL, "mismatching target processor type: " << prog.target_processor_type());

  for (const auto &r : prog.code()) {
    if (!proc->IsAddressInRange(proc->map_pm_index2address(r.addr + r.code.size())))
      DRETURN(EINVAL, "address out of range: " << std::hex << r.addr + r.code.size());

    proc->init_program_memory_at_index(r.addr, r.code.data(), r.code.size());
  }

  return 0;
}

}  // namespace util
