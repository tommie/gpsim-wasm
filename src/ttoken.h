/*
   Copyright (C) 2004 T. Scott Dattalo

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

#ifndef SRC_TTOKEN_H_
#define SRC_TTOKEN_H_

namespace gpsim {

  /// Token - a class used to synchronize two threads.
  /// Token will spawn a new thread and keep it synchronized
  /// with the thread that called it. In gpsim, the simulation
  /// engine is a single thread application. However, it's 
  /// sometimes quite difficult to implement modules within the
  /// context of this thread. So modules that are instantiated
  /// from within the context of the simulator can use the Token
  /// class to spawn a new thread.
  /// 
  /// Note - the underlying implementation depends on pthreads.
  /// However, that implementation is completely hidden with an
  /// opaque pointer to the PThreadWrapper object. This way,
  /// if it turns out that pthreads are not installed or there's
  /// another implementation preferred for the synchronization
  /// then it's easy to change with impacting the whole system.

  struct ThreadWrapper;

  class Token
  {
  public:
    Token();
    Token(const Token &) = delete;
    Token& operator =(const Token &) = delete;
    ~Token();

    /// Initialize - a wrapper for pthread create.

    void Initialize(void *(*child) (void *), void *data);

    /// 
    void waitForChild();
    void passToChild();
    void passToParent();
    void grab();

  private:

    ThreadWrapper *thread;
  };

} // end of namespace gpsim

#endif // SRC_TTOKEN_H_
