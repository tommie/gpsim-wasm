/*
   Copyright (C) 1998-2003 T. Scott Dattalo

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

#include <pthread.h>

#include "ttoken.h"
#include "pthread-wrap.h"

namespace gpsim {

  Token::Token()
  {
    thread = new ThreadWrapper();
  }

  Token::~Token()
  {
    // TODO: should delete the thread.
  }

  void Token::Initialize(void *(*child) (void *), void *data)
  {

    pthread_mutex_init(&thread->mutex, nullptr);
    pthread_cond_init (&thread->cvWaitOnParent, nullptr);
    pthread_cond_init (&thread->cvWaitOnChild, nullptr);

    grab();

    pthread_attr_init(&thread->thAttribute);
    pthread_attr_setdetachstate(&thread->thAttribute, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thread->thHostInterface, &thread->thAttribute, child, data);
  }

  void Token::waitForChild()
  {
    pthread_cond_wait(&thread->cvWaitOnChild, &thread->mutex);
  }

  void Token::grab()
  {
    pthread_mutex_lock(&thread->mutex);
  }

  void Token::passToChild()
  {
    pthread_cond_signal(&thread->cvWaitOnParent);
    pthread_cond_wait(&thread->cvWaitOnChild, &thread->mutex);
  }

  void Token::passToParent()
  {
    pthread_cond_signal(&thread->cvWaitOnChild);
    pthread_cond_wait(&thread->cvWaitOnParent, &thread->mutex);
  }

} // end of namespace gpsim
