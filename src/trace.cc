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

#include "trace.h"

#include <cassert>


namespace trace {

  void* TraceBuffer::push(size_type n, EntryType type)
  {
    if (n == 0) n = 1;

    assert(n <= std::numeric_limits<EntrySizeType>::max());

    if (back_ + n > data_.size()) {
      // We need a contiguous slice of memory, so fill the head with
      // blanks if needed to wrap around.
      metas_[back_] = {
        .size = static_cast<EntrySizeType>(data_.size() - back_),
        .type = EmptyEntry::type(),
      };
      back_ = 0;
    }

    while (data_.size() - size() < n) {
      pop();
      ++discarded_;
    }

    void *p = &data_[back_];

    metas_[back_] = {
      .size = static_cast<EntrySizeType>(n),
      .type = type,
    };
    back_ = clamp(back_ + n);

    return p;
  }

  void TraceBuffer::pop()
  {
    front_ = clamp(front_ + metas_[front_].size);

    if (empty()) discarded_ = 0;
  }

  namespace {

    TraceBuffer& global_buffer()
    {
      static TraceBuffer global_buffer(1 << 16);
      return global_buffer;
    }

  }

  TraceWriter global_writer()
  {
    return TraceWriter(&global_buffer());
  }

  TraceReader global_reader()
  {
    return TraceReader(&global_buffer());
  }

}  // namespace trace
