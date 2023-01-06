/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
   T. Scott Dattalo and Ralf Forsberg

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef GUI_GUI_STACK_H_
#define GUI_GUI_STACK_H_

#include "gui_object.h"
#include <gtk/gtk.h>

class GUI_Processor;

//
// The stack window
//
class Stack_Window : public GUI_Object
{
 public:
  explicit Stack_Window(GUI_Processor *gp);

  void Build() override;
  void Update() override;

private:
  int last_stacklen;
  GtkListStore *stack_list;
  GtkTreeModel *sort_stack_list;
  GtkWidget *tree;
};


#endif // GUI_GUI_STACK_H_

