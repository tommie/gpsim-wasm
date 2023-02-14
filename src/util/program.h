/*
   Copyright (C) 2022 Tommie Gannert

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#ifndef SRC_UTIL_PROGRAM_
#define SRC_UTIL_PROGRAM_

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>


class Processor;

namespace util {

typedef std::basic_string<uint8_t> u8string;

enum class SourceSymbolType {
  UNKNOWN,
  DATA,
  PROGRAM,
  CONSTANT,
};

}  // namespace util

template<>
struct std::hash<std::pair<util::SourceSymbolType, int>>
{
  std::size_t operator()(const std::pair<util::SourceSymbolType, int>& v) const noexcept
  {
    std::size_t h1 = std::hash<util::SourceSymbolType>{}(v.first);
    std::size_t h2 = std::hash<int>{}(v.second);
    return h1 ^ (h2 << 1);
  }
};

namespace util {

struct CodeRange {
  uint64_t addr;
  u8string code;
};

struct SourceSymbol {
  SourceSymbolType type;
  std::string name;
  int value;
};

struct SourceLineRef {
  uint64_t addr;
  std::string_view file;  // Note this is just a string_view.
  int line;
};

struct SourceDirective {
  uint64_t addr;
  std::string type;
  std::string text;
};

class Program {
public:
  Program() = default;
  Program(std::vector<CodeRange> &&code,
          std::vector<SourceSymbol> &&symbols,
          std::vector<SourceLineRef> &&line_refs,
          std::vector<SourceDirective> &&directives,
          std::string_view target_processor_type);

  // Builds indices used by some of the find functions.
  int build_indices();

  const std::vector<CodeRange>& code() const { return m_code; }
  const std::vector<SourceSymbol>& symbols() const { return m_symbols; }
  const std::vector<SourceLineRef>& line_refs() const { return m_line_refs; }
  const std::vector<SourceDirective>& directives() const { return m_directives; }
  std::string_view target_processor_type() const { return m_target_proc_type; }

  const CodeRange* find_code_range(uint64_t addr) const;
  u8string code_at(uint64_t addr, std::size_t size, uint8_t fill = 0xFF) const;

  std::vector<const SourceSymbol*> find_symbols(std::string_view name) const;
  std::vector<const SourceSymbol*> find_symbols(SourceSymbolType type, int value) const;

  std::vector<const SourceLineRef*> find_lines(std::string_view file, int line) const;
  std::vector<const SourceLineRef*> find_lines(uint64_t addr) const;

  std::vector<const SourceDirective*> find_directives_by_type(std::string_view type) const;
  std::vector<const SourceDirective*> find_directives(uint64_t addr) const;

private:
  void intern_strings();
  void build_symbols_indices();
  int  build_lines_index();
  void build_directives_index();

private:
  std::vector<CodeRange> m_code;              // Ordered by addr.
  std::vector<SourceSymbol> m_symbols;
  std::vector<SourceLineRef> m_line_refs;     // Ordered by addr.
  std::vector<SourceDirective> m_directives;  // Ordered by (addr, type).
  std::string m_target_proc_type;
  std::unordered_set<std::string> m_strings;

  // Indices.

  std::unordered_map<std::string_view, std::vector<SourceSymbol*>> m_symbols_by_name;
  std::unordered_multimap<std::pair<SourceSymbolType, int>, SourceSymbol*> m_symbols_by_value;
  std::multimap<std::pair<std::string_view, int>, SourceLineRef*> m_lines_by_loc;
  std::unordered_map<std::string_view, std::vector<SourceDirective*>> m_directives_by_type;
};

int upload(::Processor *proc, const Program &prog);

}  // namespace util

#endif  // SRC_UTIL_PROGRAM_
