/*
   Copyright (C) 1999 T. Scott Dattalo

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

#ifndef CLI_CMD_STIMULUS_H_
#define CLI_CMD_STIMULUS_H_

#include "../src/expr.h"
#include "command.h"

class cmd_options_str;

class cmd_stimulus : public command
{
public:
    int valid_options;
    int options_entered;
    int have_data;

    cmd_stimulus();
    void stimulus();

    void stimulus(int bit_flag);
    void stimulus(cmd_options_expr *coe);
    void stimulus(cmd_options_str *cos);
    void stimulus(ExprList_t *);
    void end();

    bool can_span_lines() override { return 1; }
};

extern cmd_stimulus c_stimulus;

#endif

