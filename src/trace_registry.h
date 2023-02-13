/*
   Copyright (C) 2023 Tommie Gannert

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

#ifndef SRC_TRACE_REGISTRY_H_
#define SRC_TRACE_REGISTRY_H_

#include <variant>

#include "trace.h"


namespace trace {

// An entry that is not listed in the registry, and therefore cannot
// be parsed further.
class UnknownEntry {
public:
  UnknownEntry(EntryType type) : type_(type) {}

  EntryType type() const { return type_; }

private:
  EntryType type_;
};

namespace internal {

template<EntryType MaxType, typename... Entries>
struct EntryRegistry {
  typedef std::variant<Entries..., UnknownEntry> variant;

  // Parses the given ref into an std::variant. Returns an
  // UnknownEntry if the type is unknown.
  static variant parse(const EntryConstRef &ref) {
    correct_order<0, Entries...>();

    if (ref.type() >= parsers.size()) return UnknownEntry(ref.type());
    return parsers[ref.type()](ref);
  }

private:
  template<typename Entry>
  static variant parse_entry(const EntryConstRef &ref) {
    static_assert(std::is_base_of_v<EntryBase, Entry>, "Can only parse subclasses of Entry");
    return variant(std::in_place_type<Entry>, ref.as<const Entry>());
  }

  // An overload to allow specifying UnknownEntry in the registry, to
  // fill gaps in the type enum.
  template<>
  static variant parse_entry<UnknownEntry>(const EntryConstRef &ref) {
    return variant(std::in_place_type<UnknownEntry>, ref.type());
  }

  typedef variant(*parser_type)(const EntryConstRef&);

  static const std::array<parser_type, sizeof...(Entries)> parsers;

  // A constexpr that checks the order of Entries matches
  // Entry::type(), so the parsers array is correct.
  template<EntryType ET, typename Entry, typename... Tail>
  static constexpr void correct_order() {
    static_assert(ET == Entry::type(), "Bad entry type ordering in registry");
    if constexpr (sizeof...(Tail) > 0) correct_order<ET + 1, Tail...>();
  }

  // A constexpr that returns the EntryType of the last Entry.
  template<typename Head, typename... Tail>
  static constexpr EntryType last_entry_type() {
    if constexpr (sizeof...(Tail) > 0) return last_entry_type<Tail...>();
    return Head::type();
  }

  static_assert(last_entry_type<Entries...>() == MaxType, "Not all EntryTypes are in the registry");
};

template<EntryType MaxType, typename... Entries>
const std::array<typename EntryRegistry<MaxType, Entries...>::parser_type, sizeof...(Entries)> EntryRegistry<MaxType, Entries...>::parsers = {{
    &EntryRegistry::parse_entry<Entries>...,
  }};

}  // namespace internal

// All trace entry types, so they can easily be parsed.
using EntryRegistry = internal::EntryRegistry<
  NUM_ENTRY_TYPES - 1,
  EmptyEntry,
  CycleCounterEntry,
  ReadRegisterEntry,
  WriteRegisterEntry,
  SetPCEntry,
  IncrementPCEntry,
  SkipPCEntry,
  BranchPCEntry,
  InterruptEntry,
  ResetEntry>;

}  // namespace trace

#endif
