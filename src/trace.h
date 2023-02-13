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

#ifndef SRC_TRACE_H_
#define SRC_TRACE_H_

#include <cstddef>
#include <type_traits>
#include <vector>

#include "gpsim_classes.h"

namespace trace {

typedef uint8_t EntryType;

namespace internal {

class EntryBase
{
protected:
  EntryBase() = default;
};

}  // namespace internal

template<EntryType Type>
class Entry : public internal::EntryBase
{
public:
  static constexpr EntryType type() { return Type; }
};

class EmptyEntry : public Entry<0>
{
};

class EntryConstRef
{
public:
  EntryType type() const { return type_; }

  template<typename T>
  const T& as() const
  {
    const T* e = asOr<T>(nullptr);
    if (e) return *e;
    std::abort();
  }

  template<typename T>
  const T* asOr(const T *def) const
  {
    static_assert(std::is_base_of_v<internal::EntryBase, T>, "Can only cast to subclasses of trace::Entry");
    if (T::type() == EmptyEntry::type() || type() == T::type())
      return static_cast<const T*>(entry_);
    return def;
  }

private:
  friend class TraceBuffer;

  EntryConstRef(const internal::EntryBase *entry, EntryType type)
    : entry_(entry), type_(type) {}

private:
  const internal::EntryBase *entry_;
  EntryType type_;
};

class TraceBuffer
{
  typedef uint8_t EntrySizeType;

  struct EntryMeta {
    EntrySizeType size;
    EntryType type;
  };

  typedef std::vector<std::max_align_t> DataVector;
  typedef std::vector<EntryMeta> MetaVector;

public:
  typedef EmptyEntry value_type;
  typedef DataVector::size_type size_type;

  struct const_iterator {
    friend class TraceBuffer;

    typedef EmptyEntry value_type;
    typedef DataVector::size_type size_type;
    typedef DataVector::difference_type difference_type;
    typedef EntryConstRef const_reference;

    bool operator ==(const const_iterator& that) const
    {
      return data_ == that.data_;
    }

    bool operator !=(const const_iterator& that) const
    {
      return data_ != that.data_;
    }

    const_iterator& operator ++()
    {
      auto n = meta_->size;
      data_ += n;
      meta_ += n;
      if (data_ == buffer_->data_.cend()) {
        data_ = buffer_->data_.cbegin();
        meta_ = buffer_->metas_.cbegin();
      }
      return *this;
    }

    const_reference operator *() const
    {
      return EntryConstRef(reinterpret_cast<const internal::EntryBase*>(&*data_), meta_->type);
    }

    const_reference operator ->() const
    {
      return EntryConstRef(reinterpret_cast<const internal::EntryBase*>(&*data_), meta_->type);
    }

  private:
    const_iterator(DataVector::const_iterator data, MetaVector::const_iterator meta, const TraceBuffer *buffer)
      : data_(data), meta_(meta), buffer_(buffer)
    {}

    DataVector::const_iterator data_;
    MetaVector::const_iterator meta_;
    const TraceBuffer *buffer_;
  };

  friend struct const_iterator;

  explicit TraceBuffer(size_type size)
    : data_(size),
      metas_(size)
  {}

  bool empty() const
  {
    return back_ == front_;
  }

  size_type size() const
  {
    return clamp(back_ - front_ + data_.size());
  }

  size_type discarded() const { return discarded_; }

  EntryConstRef front() const { return EntryConstRef(reinterpret_cast<const internal::EntryBase*>(&data_[front_]), metas_[front_].type); }

  const_iterator cbegin() const { return const_iterator(data_.cbegin() + front_, metas_.cbegin() + front_, this); }
  const_iterator cend() const { return const_iterator(data_.cbegin() + back_, metas_.cbegin() + back_, this); }

  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }

  template<typename T, typename... Args>
  void emplace(Args&&... args)
  {
    static_assert(std::is_base_of_v<internal::EntryBase, T>, "Can only emplace subclasses of trace::Entry");
    static_assert(std::is_trivially_copyable_v<T>, "Entry classes must be trivial");
    static_assert(std::alignment_of_v<T> <= sizeof(DataVector::value_type), "Entry classes must not have excessive alignment requirements");

    constexpr size_type entry_size = (sizeof(T) + sizeof(DataVector::value_type) - 1) / sizeof(DataVector::value_type);

    static_assert(entry_size <= std::numeric_limits<EntrySizeType>::max(), "Entry object too large");

    new (push(entry_size, T::type())) T(std::forward<Args>(args)...);
  }

  void pop();

private:
  void* push(size_type n, EntryType type);

  size_type clamp(size_type n) const {
    if (n >= data_.size()) {
      return n % data_.size();
    }
    return n;
  }

private:
  DataVector data_;
  MetaVector metas_;
  size_type front_ = 0;
  size_type back_ = 0;
  size_type discarded_ = 0;
};

enum EntryTypes : EntryType
{
  BASE = EmptyEntry::type(),
  CYCLE_COUNTER,
  READ_REGISTER,
  WRITE_REGISTER,
  SET_PC,
  INCREMENT_PC,
  SKIP_PC,
  BRANCH_PC,
  INTERRUPT,
  RESET,

  NUM_ENTRY_TYPES,
};

class CycleCounterEntry : public Entry<CYCLE_COUNTER>
{
public:
  explicit CycleCounterEntry(uint64_t cycle) : cycle_(cycle) {}

  uint64_t cycle() const { return cycle_; }

private:
  uint64_t cycle_;
};

class RegisterEntryBase
{
protected:
  RegisterEntryBase(uint16_t addr, uint8_t value, uint8_t mask)
    : addr_(addr), value_(value), mask_(mask) {}

public:
  uint16_t address() const { return addr_; }
  uint8_t value() const { return value_; }
  uint8_t mask() const { return mask_; }

private:
  uint16_t addr_;
  uint8_t value_;
  uint8_t mask_;
};

class ReadRegisterEntry : public Entry<READ_REGISTER>, public RegisterEntryBase
{
public:
  ReadRegisterEntry(uint16_t addr, uint8_t value, uint8_t mask = 0xFF)
    : RegisterEntryBase(addr, value, mask) {}
};

class WriteRegisterEntry : public Entry<WRITE_REGISTER>, public RegisterEntryBase
{
public:
  WriteRegisterEntry(uint16_t addr, uint8_t value, uint8_t mask = 0xFF)
    : RegisterEntryBase(addr, value, mask) {}
};

class PCEntryBase
{
protected:
  explicit PCEntryBase(uint16_t addr) : addr_(addr) {}

public:
  uint16_t address() const { return addr_; }

private:
  uint16_t addr_;
};

class SetPCEntry : public Entry<SET_PC>, public PCEntryBase
{
public:
  SetPCEntry(uint16_t addr, uint16_t target)
    : PCEntryBase(addr), target_(target) {}

  uint16_t target() const { return target_; }

private:
  uint16_t target_;
};

class IncrementPCEntry : public Entry<INCREMENT_PC>, public PCEntryBase {
public:
  IncrementPCEntry(uint16_t addr) : PCEntryBase(addr) {}
};

class SkipPCEntry : public Entry<SKIP_PC>, public PCEntryBase {
public:
  SkipPCEntry(uint16_t addr) : PCEntryBase(addr) {}
};

class BranchPCEntry : public Entry<BRANCH_PC>, public PCEntryBase {
public:
  BranchPCEntry(uint16_t addr) : PCEntryBase(addr) {}
};

class InterruptEntry : public Entry<INTERRUPT> {};

class ResetEntry : public Entry<RESET> {
public:
  explicit ResetEntry(RESET_TYPE cause)
    : cause_(cause) {}

  RESET_TYPE cause() const { return cause_; }

private:
  RESET_TYPE cause_;
};

/**
 * A proxy for TraceBuffer that only allows writing entries.
 */
class TraceWriter
{
public:
  explicit TraceWriter(TraceBuffer *buffer)
    : buffer_(buffer) {}

  // Pushes a new entry, constructing it in-place.
  //
  // If the buffer is full, entries are pop()ed until there is enough
  // room. discarded() is incremented when this happens.
  template<typename T, typename... Args>
  void emplace(Args&&... args)
  {
    buffer_->emplace<T>(std::forward<Args>(args)...);
  }

private:
  TraceBuffer *buffer_;
};

/**
 * A proxy for TraceBuffer that only allows reading entries.
 */
class TraceReader
{
public:
  typedef TraceBuffer::value_type value_type;
  typedef TraceBuffer::size_type size_type;
  typedef TraceBuffer::const_iterator const_iterator;

  explicit TraceReader(TraceBuffer *buffer)
    : buffer_(buffer) {}

  // Returns true if the buffer is empty.
  bool empty() const { return buffer_->empty(); }

  // Returns the number of entries in the buffer.
  size_type size() const { return buffer_->size(); }

  // Returns the number of entries that were discarded by emplace() to
  // make room for new entries. It is reset to zero when pop() makes
  // the buffer empty.
  size_type discarded() const { return buffer_->discarded(); }

  // Returns a reference to the current front entry. This reference is
  // invalidated by emplace() and pop().
  EntryConstRef front() const { return buffer_->front(); }

  const_iterator cbegin() const { return buffer_->cbegin(); }
  const_iterator cend() const { return buffer_->cend(); }

  const_iterator begin() const { return buffer_->begin(); }
  const_iterator end() const { return buffer_->end(); }

  // Removes the front entry. Calling this when empty() is false leads
  // to undefined behavior.
  void pop() { buffer_->pop(); }

private:
  TraceBuffer *buffer_;
};

// Returns a write handle to the global trace buffer.
TraceWriter global_writer();

// Returns a read handle to the global trace buffer.
TraceReader global_reader();

}  // namespace trace

#endif
