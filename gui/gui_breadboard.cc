/*
   Copyright (C) 2000,2001,2002
   Ralf Forsberg

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

#include "../config.h"
#ifdef HAVE_GUI

#include <cairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>

#include "../src/errors.h"
#include "../src/gpsim_object.h"
#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/processor.h"
#include "../src/stimuli.h"
#include "../src/symbol.h"
#include "../src/ui.h"
#include "../src/value.h"
#include "../src/xref.h"

#include "gui.h"
#include "gui_breadboard.h"
#include "gui_processor.h"

#define PINLINEWIDTH 3
#define CASELINEWIDTH 4

#define CASEOFFSET (CASELINEWIDTH/2)

#define FOORADIUS (CASELINEWIDTH) // radius of center top milling

#define LABELPAD 4 // increase this so wide lines doesn't clutter labels

static GdkColor high_output_color;
static GdkColor low_output_color;

#define PINLENGTH (4*PINLINEWIDTH)

static int pinspacing = PINLENGTH;

#define LAYOUTSIZE_X 800
#define LAYOUTSIZE_Y 800

#define STRING_SIZE 128

#define ROUTE_RES (2*PINLINEWIDTH) // grid spacing

static void treeselect_module(GuiModule *p);

#define XSIZE LAYOUTSIZE_X/ROUTE_RES // grid size
#define YSIZE LAYOUTSIZE_Y/ROUTE_RES


/* If HMASK is set in board_matrix, then this position
 is unavailable for horizontal track */
#define HMASK 1
#define VMASK 2
/*
 board matrix contains information about how a track can be routed.
 */
static unsigned char *board_matrix;
// mask_matrix is used by trace_two_poins to know where is has been, and
// how quickly it came here. (depth is stored here if lower)
static unsigned short *mask_matrix;
static unsigned int xsize;
static unsigned int ysize;


//========================================================================

class BreadBoardXREF : public CrossReferenceToGUI {
public:
  void Update(int /* new_value */) override
  {
    auto bbw  = static_cast<Breadboard_Window *>(parent_window);
    bbw->Update();
  }
  void Remove() override {}
};


//========================================================================


BB_ModuleLabel::BB_ModuleLabel(const std::string &text, PangoFontDescription *font)
{
  m_label = gtk_label_new(text.c_str());
  gtk_widget_modify_font(m_label, font);
  gtk_widget_show(m_label);
}


BB_ModuleLabel::~BB_ModuleLabel()
{
  gtk_widget_destroy(m_label);
}


static inline unsigned char *board_matrix_pt(unsigned int x, unsigned int y)
{
  if (x < xsize && y < ysize) {
    return board_matrix + y * xsize + x;
  }
  return nullptr;
}


static inline unsigned short *mask_matrix_pt(unsigned int x, unsigned int y)
{
  if (x < xsize && y < ysize) {
    return mask_matrix + (y * xsize + x);
  }
  return nullptr;
}


/* Check the flags in board_matrix to see if we are allowed to
   route horizontally here */
static inline bool allow_horiz(const point &p)
{
  unsigned char *pt = board_matrix_pt(p.x, p.y);

  if (pt && !(*pt & HMASK)) {
    return true;
  }

  return false;
}


/* Check the flags in board_matrix to see if we are allowed to
   route vertically here */
static inline bool allow_vert(const point &p)
{
  unsigned char *pt = board_matrix_pt(p.x, p.y);

  if (pt && !(*pt & VMASK)) {
    return true;
  }

  return false;
}


// Find the direction to go to get from s to e if there are no obstacles.
static inline route_direction calculate_route_direction(point s, point e)
{
  if (abs(s.x - e.x) > abs(s.y - e.y)) {
    // Left or right
    if (s.x < e.x) {
      return R_RIGHT;
    }

    return R_LEFT;
  }

  if (s.y < e.y) {
    return R_UP;
  }

  return R_DOWN;
}


// Put point p as first point in pat
static void prepend_point_to_path(std::list<path> &pat, point p)
{
  route_direction dir = R_NONE;
  path new_point;
  new_point.p = p;

  if (!pat.empty()) {
    dir = calculate_route_direction(p, pat.front().p);

    if (pat.front().dir == R_NONE) {
      pat.front().dir = dir;
    }
  }

  new_point.dir = dir;
  pat.push_front(new_point);
}

// maxdepth is shortest path from start to end
static unsigned short maxdepth;


// Penalty for making a turn in a trace

static int turnq(route_direction a, route_direction b)
{
  return (a != b) * 10;
}


/*
This is an recursive routine that tries to find a path between p and end.
*/
static bool trace_two_points(std::list<path> &pat,   // Pointer to resulting path
                             point p,  // Where we are now
                             point end,    // Where we want to go
                             int depth,
                             route_direction lastdir)    // How deep in we are
{
  point up, left, right, down;

  if (depth == 0) {
    // Initialize mask_matrix and maxdepth on first call
    //maxdepth=500;
    for (int x = 0; x < (int)xsize; x++) {
      for (int y = 0; y < (int)ysize; y++) {
        *mask_matrix_pt(x, y) = maxdepth;
      }
    }

    pat.clear();
  }

  ////////////////////////////////////////
  // Recusion termination
  ////////////////////////////////////////
  if (depth > maxdepth) {
    return false;
  }

  if (depth > *mask_matrix_pt(p.x, p.y)) {
    return false;
  }

  if (abs(p.x - end.x) + abs(p.y - end.y) + depth > maxdepth) {
    return false;
  }

  if (p.x == end.x && p.y == end.y) {
    // We are at end point
    if (depth < maxdepth) {
      // We found a new shortest path.
      //printf("Found path with length %d\n",depth);
      maxdepth = depth;
      pat.clear();
      prepend_point_to_path(pat, p);
      return true;
    }

    return false;
  }

  // Store new (closer) depth in mask_matrix.
  *mask_matrix_pt(p.x, p.y) = depth;
  // Find the general direction we want to go
  route_direction dir = calculate_route_direction(p, end);
  // Recursion return value
  bool retval = false;
  // Convenience
  up = p;
  up.y++;
  down = p;
  down.y--;
  left = p;
  left.x--;
  right = p;
  right.x++;

  /* Depending on where we wish to go, do recursion so that we likely
     will quickly find at least some kind of path to end.

     If we don't do that maxdepth will stay large, and the traceing will
     likely take a long time.

     We use allow_vert and allow_horiz to determine if the movement is
     allowed, of if there is something in our way.

  */

  switch (dir) {
  case R_UP:
    if (allow_vert(up)) {
      retval |= trace_two_points(pat, up, end, depth + 1 + turnq(lastdir, R_UP), R_UP);
    }

    if (p.x < end.x) {
      if (allow_horiz(right)) {
        retval |= trace_two_points(pat, right, end, depth + 1 + turnq(lastdir, R_RIGHT), R_RIGHT);
      }

      if (allow_horiz(left)) {
        retval |= trace_two_points(pat, left, end, depth + 1 + turnq(lastdir, R_LEFT), R_LEFT);
      }

    } else {
      if (allow_horiz(left)) {
        retval |= trace_two_points(pat, left, end, depth + 1 + turnq(lastdir, R_LEFT), R_LEFT);
      }

      if (allow_horiz(right)) {
        retval |= trace_two_points(pat, right, end, depth + 1 + turnq(lastdir, R_RIGHT), R_RIGHT);
      }
    }

    if (allow_vert(down)) {
      retval |= trace_two_points(pat, down, end, depth + 1 + turnq(lastdir, R_DOWN), R_DOWN);
    }

    break;

  case R_DOWN:
    if (allow_vert(down)) {
      retval |= trace_two_points(pat, down, end, depth + 1 + turnq(lastdir, R_DOWN), R_DOWN);
    }

    if (p.x < end.x) {
      if (allow_horiz(right)) {
        retval |= trace_two_points(pat, right, end, depth + 1 + turnq(lastdir, R_RIGHT), R_RIGHT);
      }

      if (allow_horiz(left)) {
        retval |= trace_two_points(pat, left, end, depth + 1 + turnq(lastdir, R_LEFT), R_LEFT);
      }

    } else {
      if (allow_horiz(left)) {
        retval |= trace_two_points(pat, left, end, depth + 1 + turnq(lastdir, R_LEFT), R_LEFT);
      }

      if (allow_horiz(right)) {
        retval |= trace_two_points(pat, right, end, depth + 1 + turnq(lastdir, R_RIGHT), R_RIGHT);
      }
    }

    if (allow_vert(up)) {
      retval |= trace_two_points(pat, up, end, depth + 1 + turnq(lastdir, R_UP), R_UP);
    }

    break;

  case R_LEFT:
    if (allow_horiz(left)) {
      retval |= trace_two_points(pat, left, end, depth + 1 + turnq(lastdir, R_LEFT), R_LEFT);
    }

    if (p.y < end.y) {
      if (allow_vert(up)) {
        retval |= trace_two_points(pat, up, end, depth + 1 + turnq(lastdir, R_UP), R_UP);
      }

      if (allow_vert(down)) {
        retval |= trace_two_points(pat, down, end, depth + 1 + turnq(lastdir, R_DOWN), R_DOWN);
      }

    } else {
      if (allow_vert(down)) {
        retval |= trace_two_points(pat, down, end, depth + 1 + turnq(lastdir, R_DOWN), R_DOWN);
      }

      if (allow_vert(up)) {
        retval |= trace_two_points(pat, up, end, depth + 1 + turnq(lastdir, R_UP), R_UP);
      }
    }

    if (allow_horiz(right)) {
      retval |= trace_two_points(pat, right, end, depth + 1 + turnq(lastdir, R_RIGHT), R_RIGHT);
    }

    break;

  case R_RIGHT:
    if (allow_horiz(right)) {
      retval |= trace_two_points(pat, right, end, depth + 1 + turnq(lastdir, R_RIGHT), R_RIGHT);
    }

    if (p.y < end.y) {
      if (allow_vert(up)) {
        retval |= trace_two_points(pat, up, end, depth + 1 + turnq(lastdir, R_UP), R_UP);
      }

      if (allow_vert(down)) {
        retval |= trace_two_points(pat, down, end, depth + 1 + turnq(lastdir, R_DOWN), R_DOWN);
      }

    } else {
      if (allow_vert(down)) {
        retval |= trace_two_points(pat, down, end, depth + 1 + turnq(lastdir, R_DOWN), R_DOWN);
      }

      if (allow_vert(up)) {
        retval |= trace_two_points(pat, up, end, depth + 1 + turnq(lastdir, R_UP), R_UP);
      }
    }

    if (allow_horiz(left)) {
      retval |= trace_two_points(pat, left, end, depth + 1 + turnq(lastdir, R_LEFT), R_LEFT);
    }

    break;

  case R_NONE:
    break;
  }

  // Check if some of the recursive traces went well.
  if (retval) {
    // We found a path to end. Add point p to path.
    // This is first or second point.
    prepend_point_to_path(pat, p);
    return true;
  }

  return false;
}


static std::vector<std::list<path>> nodepath_list;


// Clear nodes in nodepath_list
void Breadboard_Window::clear_nodes()
{
  nodepath_list.clear();
}


// Draw node in nodepath_list
void Breadboard_Window::draw_nodes()
{
  gtk_widget_queue_draw(layout);
}


gboolean Breadboard_Window::layout_expose(GtkWidget *,
    GdkEventExpose *, Breadboard_Window *bbw)
{
  cairo_t *cr = gdk_cairo_create(gtk_layout_get_bin_window(GTK_LAYOUT(bbw->layout)));
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  auto iter = nodepath_list.begin();

  for (; iter != nodepath_list.end(); ++iter) {
    std::list<path>::const_iterator current_path = (*iter).begin();
    int last_x = current_path->p.x * ROUTE_RES;
    int last_y = current_path->p.y * ROUTE_RES;
    cairo_move_to(cr, last_x, last_y);

    for (++current_path; current_path != (*iter).end(); ++current_path) {
      int x = current_path->p.x * ROUTE_RES;
      int y = current_path->p.y * ROUTE_RES;
      cairo_line_to(cr, x, y);
    }
  }

  cairo_stroke(cr);
  cairo_destroy(cr);
  return FALSE;
}


// Here we fill board_matrix with module packages, so that trace_two_points
// know not to trace over them.
void Breadboard_Window::update_board_matrix()
{
  int x, y, width, height;
  int i;
  gtk_window_get_size(GTK_WINDOW(window), &width, &height);

  if (width / ROUTE_RES > (int)xsize || height / ROUTE_RES > (int)ysize) {
    xsize = width / ROUTE_RES;
    ysize = height / ROUTE_RES;
    delete [] board_matrix;
    delete [] mask_matrix;
    board_matrix = new unsigned char[xsize * ysize];
    mask_matrix = new unsigned short[xsize * ysize];
  }

  // Clear first.
  std::fill_n(board_matrix, xsize * ysize, '\0');

  // Mark board outline, so we limit traces here
  for (x = 0; x < (int)xsize; x++) {
    *board_matrix_pt(x, 0) = (HMASK | VMASK);
    *board_matrix_pt(x, ysize - 1) = (HMASK | VMASK);
  }

  for (y = 0; y < (int)ysize; y++) {
    *board_matrix_pt(0, y) = (HMASK | VMASK);
    *board_matrix_pt(xsize - 1, y) = (HMASK | VMASK);
  }

  // Loop all modules, and put its package and pins to board_matrix
  auto mi = modules.begin();

  for (; mi != modules.end(); ++mi) {
    GuiModule *p = *mi;

    if (p && p->IsBuilt()) {
      width = p->width();
      height = p->height();

      for (y = p->y() - ROUTE_RES;
           y < p->y() + height + ROUTE_RES && y / ROUTE_RES < (int)ysize;
           y += ROUTE_RES) {
        for (x = p->x();
             x < p->x() + width && x / ROUTE_RES < (int)xsize;
             x += ROUTE_RES) {
          unsigned char *pt = board_matrix_pt(x / ROUTE_RES, y / ROUTE_RES);

          if (pt) {
            *pt = (HMASK | VMASK);
          }
        }
      }

      // Draw barriers around pins so the tracker can only get in
      // straigt to the pin and not from the side.
      for (i = 1; i <= p->pin_count(); i++) {
        std::vector<GuiPin *> *e = p->pins();
        GuiPin *gp = (*e)[i - 1];

        switch (gp->orientation) {
        case LEFT:
          y = gp->y() - gp->height() / 2;

          for (x = gp->x() -  PINLENGTH;
               x < gp->x() + gp->width();
               x += ROUTE_RES) {
            unsigned char *pt = board_matrix_pt(x / ROUTE_RES, y / ROUTE_RES);

            if (pt) {
              *pt = (HMASK | VMASK);
            }
          }

          y = gp->y() + gp->height() / 2;

          for (x = gp->x() -  PINLENGTH;
               x < gp->x() + gp->width();
               x += ROUTE_RES) {
            unsigned char *pt = board_matrix_pt(x / ROUTE_RES, y / ROUTE_RES);

            if (pt) {
              *pt = (HMASK | VMASK);
            }
          }

          break;

        case RIGHT:
          y = gp->y() - gp->height() / 2;

          for (x = gp->x() - PINLENGTH;
               x < gp->x() + gp->width();
               x += ROUTE_RES) {
            unsigned char *pt = board_matrix_pt(x / ROUTE_RES, y / ROUTE_RES);

            if (pt) {
              *pt = (HMASK | VMASK);
            }
          }

          y = gp->y() + gp->height() / 2;

          for (x = gp->x() - PINLENGTH;
               x < gp->x() + gp->width();
               x += ROUTE_RES) {
            unsigned char *pt = board_matrix_pt(x / ROUTE_RES, y / ROUTE_RES);

            if (pt) {
              *pt = (HMASK | VMASK);
            }
          }

          break;

        default:
          assert(0);
        }
      }
    }
  }

  clear_nodes();
  draw_nodes();
}


// Add path to board_matrix. This will make trace_two_point to not trace
// at its place. It can trace over it when in straight angle.
static void add_path_to_matrix(const std::list<path> &pat)
{
  int x = -1;
  int y = -1;
  auto iter = pat.begin();

  if (iter != pat.end()) {
    x = iter->p.x;
    y = iter->p.y;
    ++iter;
  }

  for (; iter != pat.end(); ++iter) {
    unsigned char *pt = board_matrix_pt(x, y);

    if (pt && (iter->dir == R_LEFT || iter->dir == R_RIGHT)) {
      *pt |= HMASK;
    }

    if (pt && (iter->dir == R_DOWN || iter->dir == R_UP)) {
      *pt |= VMASK;
    }

    while (x != iter->p.x || y != iter->p.y) {
      if (x < iter->p.x) {
        x++;
      }

      if (x > iter->p.x) {
        x--;
      }

      if (y < iter->p.y) {
        y++;
      }

      if (y > iter->p.y) {
        y--;
      }

      pt = board_matrix_pt(x, y);

      if (pt && (iter->dir == R_LEFT || iter->dir == R_RIGHT)) {
        *pt |= HMASK;
      }

      if (pt && (iter->dir == R_DOWN || iter->dir == R_UP)) {
        *pt |= VMASK;
      }
    }
  }
}


static GuiPin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin);

static int pathlen[100][100];

static void reverse_path_if_endpoint(point startpoint, std::list<path> &pat)
{
  point pat_start = pat.front().p;
  point pat_end = pat.back().p;
  int dist_start = abs(pat_start.x - startpoint.x) + abs(pat_start.y - startpoint.y);
  int dist_end = abs(pat_end.x - startpoint.x) + abs(pat_end.y - startpoint.y);

  if (dist_start > dist_end && dist_end < 5) {
    // Reverse the list pat
    pat.reverse();
  }
}


static void reverse_path_if_startpoint(point startpoint, std::list<path> &pat)
{
  point pat_start = pat.front().p;
  point pat_end = pat.back().p;
  int dist_start = abs(pat_start.x - startpoint.x) + abs(pat_start.y - startpoint.y);
  int dist_end = abs(pat_end.x - startpoint.x) + abs(pat_end.y - startpoint.y);

  if (dist_start < dist_end && dist_start < 5) {
    // Reverse the list pat
    pat.reverse();
  }
}


static void path_copy_and_cat(std::list<path> &pat, std::list<path> &source)
{
  if (!pat.empty()) {
    reverse_path_if_startpoint(source.front().p, pat);
    reverse_path_if_endpoint(pat.back().p, source);
  }

  pat.insert(pat.end(), source.begin(), source.end());
}


//
// Trace a node, and add result to nodepath_list
// return true if OK, false if it could not find a route
//

static bool trace_node(gui_node *gn)
{
  Breadboard_Window *bbw = gn->bbw;;
  point start = {-1, -1}, end;
  stimulus *stimulus = gn->node->stimuli;
  std::vector<GuiPin *> pinlist;

  // Make a glist of all gui_pins in the node
  while (stimulus) {
    GuiPin *p = find_gui_pin(bbw, stimulus);

    if (p) {
      pinlist.push_back(p);
    }

    stimulus = stimulus->next;
  }

  // Allocate an array of shortest_paths, indexed with 2x glist position.
  std::map<std::pair<int, int>, std::list<path>> shortest_path;

  // Trace between all stimulus, and store the distances in the array.
  for (int i = 0; i < (int)pinlist.size(); i++) {
    GuiPin *pi = pinlist[i];

    for (int j = i + 1; j < (int)pinlist.size(); j++) {
      GuiPin *pj = pinlist[j];
      start.x = pi->x() / ROUTE_RES;
      start.y = pi->y() / ROUTE_RES;
      end.x = pj->x() / ROUTE_RES;
      end.y = pj->y() / ROUTE_RES;
      //    printf("Tracing from %d,%d to %d,%d\n",start.x,start.y,end.x,end.y);
      maxdepth = abs(start.x - end.x) + abs(start.y - end.y);
      maxdepth = maxdepth * 2 + 100; // Twice the distance, and 5 turns
      //    printf("Trying maxdepth %d\n",maxdepth);
      const std::pair<int, int> coord = std::make_pair(i, j);
      const std::pair<int, int> coord2 = std::make_pair(j, i);
      trace_two_points(shortest_path[coord], start, end, 0, R_UP);

      if (shortest_path[coord].empty()) {
        //printf("\n### Couldn't trace from pin %s to pin %s!\n",
        //  pi->getIOpin()->name().c_str(),
        //  pj->getIOpin()->name().c_str());
        return false;
      }

      pathlen[i][j] = maxdepth;
      pathlen[j][i] = maxdepth;
      shortest_path[coord2] = shortest_path[coord];
    }
  }

  int *permutations = new int[pinlist.size()];
  int *shortest_permutation = new int[pinlist.size()];

  for (int i = 0; i < (int)pinlist.size(); i++) {
    permutations[i] = i;
  }

  // Find the combination that produces the shortest node.
  int minlen = 100000;

  do {
    int sum = 0;

    for (int i = 0; i < (int)pinlist.size() - 1; i++) {
      sum += pathlen[permutations[i]][permutations[i + 1]];
    }

    if (sum < minlen) {
      minlen = sum;
      std::copy(permutations, permutations + pinlist.size(), shortest_permutation);
    }

    // Fixme, I'd rather use next_combination().
  } while (std::next_permutation(permutations, permutations + pinlist.size()));

  std::list<path> nodepath;

  for (int i = 0; i < (int)pinlist.size() - 1; i++) {
    const std::pair<int, int> coord = std::make_pair(
                                        shortest_permutation[i],
                                        shortest_permutation[i + 1]);
    auto iter = shortest_path.find(coord);

    if (iter != shortest_path.end()) {
      path_copy_and_cat(nodepath, iter->second);
    }
  }

  delete[] permutations;
  delete[] shortest_permutation;

  if (!nodepath.empty()) {
    add_path_to_matrix(nodepath);
    nodepath_list.push_back(nodepath);
  }

  return true;
}


GuiPin *find_gui_pin(Breadboard_Window *bbw, stimulus *pin)
{
  for (auto &m : bbw->modules) {
    for (int i = 1; i <= m->module()->get_pin_count(); ++i) {
      stimulus *p = m->module()->get_pin(i);

      if (p == pin) {
        return (*m->pins())[i - 1];
      }
    }
  }

  return nullptr;
}


static void treeselect_stimulus(GuiPin *pin)
{
  char text[STRING_SIZE];
  char string[STRING_SIZE];
  const char *pText = "Not connected";
  const char *pString = "Stimulus";

  if (!pin) {
    return;
  }

  gtk_widget_show(pin->bbw()->stimulus_frame);
  gtk_widget_hide(pin->bbw()->node_frame);
  gtk_widget_hide(pin->bbw()->module_frame);

  if (pin->getIOpin()) {
    g_snprintf(string, sizeof(string), "Stimulus %s", pin->getIOpin()->name().c_str());
    pString = string;

    if (pin->getSnode() != nullptr) {
      g_snprintf(text, sizeof(text), "Connected to node %s", pin->getSnode()->name().c_str());

    } else {
      g_snprintf(text, sizeof(text), "Not connected");
    }

    pText = text;
  }

  gtk_frame_set_label(GTK_FRAME(pin->bbw()->stimulus_frame), pString);
  gtk_label_set_text(GTK_LABEL(pin->bbw()->stimulus_settings_label), pText);
  pin->bbw()->selected_pin = pin;
}


static stimulus  *stim;
static std::string module_name;
static std::string full_name;


static void dumpStimulus(const SymbolEntry_t &sym)
{
  auto ps = dynamic_cast<stimulus *>(sym.second);

  if (ps == stim) {
    full_name = module_name + std::string(".") + ps->name();
  }
}


static void scanModules(const SymbolTableEntry_t &st)
{
  module_name = st.first;
  (st.second)->ForEachSymbolTable(dumpStimulus);
}


static const char * stim_full_name(stimulus *stimulus)
{
  stim = stimulus;
  globalSymbolTable().SymbolTable::ForEachModule(scanModules);
  return full_name.c_str();
}


static void treeselect_node(gui_node *gui_node)
{
  stimulus *stimulus;
  GtkListStore *list_store;

  //    printf("treeselect_node %p\n",gui_node);

  if (gui_node->node != nullptr) {
    char str[STRING_SIZE];
    g_snprintf(str, sizeof(str), "Node %s", gui_node->node->name().c_str());
    gtk_frame_set_label(GTK_FRAME(gui_node->bbw->node_frame), str);
    gtk_widget_show(gui_node->bbw->node_frame);

  } else {
    gtk_widget_hide(gui_node->bbw->node_frame);
  }

  gtk_widget_hide(gui_node->bbw->stimulus_frame);
  gtk_widget_hide(gui_node->bbw->module_frame);
  // Clear node_clist
  g_object_get(gui_node->bbw->node_clist, "model", &list_store, nullptr);
  gtk_list_store_clear(list_store);

  if (gui_node->node != nullptr) {
    // Add to node_clist
    stimulus = gui_node->node->stimuli;

    while (stimulus != nullptr) {
      GtkTreeIter iter;
      gtk_list_store_append(list_store, &iter);
      gtk_list_store_set(list_store, &iter, 0, stim_full_name(stimulus), 1, stimulus, -1);
      stimulus = stimulus->next;
    }
  }

  gui_node->bbw->selected_node = gui_node;
}


static void treeselect_cb(GtkTreeSelection *selection, gpointer)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gtk_tree_selection_get_selected(selection, &model, &iter);

  if (!iter.stamp) {
    return;
  }

  GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
  gchar *spath = gtk_tree_path_to_string(path);

  if (spath[0] == '0') {
    gui_node *gn;
    gtk_tree_model_get(model, &iter, 1, &gn, -1);

    if (strlen(spath) > 1) {
      treeselect_node(gn);

    } else {
      gtk_widget_hide(gn->bbw->node_frame);
    }

  } else {
    // For a module spath is just a number
    // For pin spath is module:pin
    if (strpbrk(spath, ":")) { //Pin
      GuiPin *gp = NULL;
      gtk_tree_model_get(model, &iter, 1, &gp, -1);
      treeselect_stimulus(gp);

    } else {			//Module
      GuiModule *module;
      gtk_tree_model_get(model, &iter, 1, &module, -1);
      treeselect_module(module);
    }
  }

  g_free(spath);
  gtk_tree_path_free(path);
}


static const char *mod_name;

static void settings_clist_cb(GtkTreeSelection *selection,
                              Breadboard_Window *bbw)
{
  // Save the Attribute*
  Value *attr;
  char str[256];
  char val[256];
  GtkTreeIter iter;
  GtkTreeModel *model;
  gtk_tree_selection_get_selected(selection, &model, &iter);

  if (!iter.stamp) { // check if iter is valid
    return;
  }

  gtk_tree_model_get(model, &iter, 1, &attr, -1);
  attr->get(val, sizeof(val));

  if (mod_name) {
    g_snprintf(str, sizeof(str), "%s.%s = %s", mod_name, attr->name().c_str(), val);

  } else {
    g_snprintf(str, sizeof(str), "%s = %s", attr->name().c_str(), val);
  }

  gtk_entry_set_text(GTK_ENTRY(bbw->attribute_entry), str);
}


static void settings_set_cb(GtkWidget *, Breadboard_Window *bbw)
{
  char attribute_name[256];
  char attribute_newval[256];
  // We get here from both the button and entry->enter
  // Check the entry.
  const char *entry_string = gtk_entry_get_text(GTK_ENTRY(bbw->attribute_entry));
  sscanf(entry_string, "%255s = %255s", attribute_name, attribute_newval);
  printf("change attribute \"%s\" to \"%s\"\n", attribute_name, attribute_newval);
  // Change the Attribute
  //attr = bbw->selected_module->module->get_attribute(attribute_name);
  Value *attr = globalSymbolTable().findValue(attribute_name);

  if (attr) {
    try {
      // Set attribute
      attr->set(attribute_newval);
      // Update clist
      treeselect_module(bbw->selected_module);

    } catch (const Error &err) {
      std::cout << __func__ << ": " << err.what() << '\n';
    }

  } else {
    printf("Could not find attribute \"%s\"\n", attribute_name);
  }
}


//static Breadboard_Window *lpBW;
static GtkWidget *attribute_clist;

static void clistOneAttribute(const SymbolEntry_t &sym)
{
  auto pVal = dynamic_cast<Value *>(sym.second);

  if (attribute_clist && pVal) {
    // read attributes and add to clist
    char attribute_value[STRING_SIZE];
    char attribute_string[STRING_SIZE];
    GtkListStore *list_store;
    GtkTreeIter iter;

    // Filter out non-attributes
    if (!strstr(typeid(*pVal).name(), "Attribute")) {
      return;
    }

    pVal->get(attribute_value, sizeof(attribute_value));
    g_snprintf(attribute_string, sizeof(attribute_string), "%s = %s",
               pVal->name().c_str(), attribute_value);
    g_object_get(attribute_clist, "model", &list_store, nullptr);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter,
                       0, attribute_string,
                       1, (gpointer) pVal, -1);
  }
}


static void buildCLISTAttribute(const SymbolTableEntry_t &st)
{
  if (st.first == mod_name) {
    if (verbose) {
      std::cout << " gui Module Attribute Window: " << st.first << '\n';
    }

    (st.second)->ForEachSymbolTable(clistOneAttribute);
  }
}


static void UpdateModuleFrame(GuiModule *p, Breadboard_Window *)
{
  char buffer[STRING_SIZE];
  g_snprintf(buffer, sizeof(buffer), "%s settings", p->module()->name().c_str());
  gtk_frame_set_label(GTK_FRAME(p->bbw()->module_frame), buffer);

  if (!gtk_widget_get_visible(p->bbw()->attribute_clist)) {
    return;
  }

  // clear clist
  gtk_list_store_clear((GtkListStore*)
                       gtk_tree_view_get_model((GtkTreeView*)p->bbw()->attribute_clist));
  attribute_clist = p->bbw()->attribute_clist;
  mod_name = p->module()->name().c_str();
  globalSymbolTable().ForEachModule(buildCLISTAttribute);
  attribute_clist = nullptr;
  gtk_entry_set_text(GTK_ENTRY(p->bbw()->attribute_entry), "");
}


static void treeselect_module(GuiModule *p)
{
  if (p) {
    gtk_widget_hide(p->bbw()->stimulus_frame);
    gtk_widget_hide(p->bbw()->node_frame);
    gtk_widget_show(p->bbw()->module_frame);
    UpdateModuleFrame(p, p->bbw());
    p->bbw()->selected_module = p;
  }
}


void GuiModule::SetPosition(int nx, int ny)
{
  nx = nx - nx % pinspacing;
  ny = ny - ny % pinspacing;

  if (nx != m_x || ny != m_y) {
    m_x = nx;
    m_y = ny;
    Value *xpos = dynamic_cast<Value *>(m_module->findSymbol("xpos"));
    Value *ypos = dynamic_cast<Value *>(m_module->findSymbol("ypos"));

    if (xpos) {
      xpos->set(m_x);
    }

    if (ypos) {
      ypos->set(m_y);
    }

    // Position module_widget
    if (m_pinLabel_widget) {
      gtk_layout_move(GTK_LAYOUT(m_bbw->layout), m_pinLabel_widget,
                      m_x,
                      m_y);
    }

    if (m_module_widget) {
      gtk_layout_move(GTK_LAYOUT(m_bbw->layout), m_module_widget,
                      m_x + m_module_x,
                      m_y + m_module_y);
    }

    // Position module_name
    gtk_layout_move(GTK_LAYOUT(m_bbw->layout), m_name_widget->gobj(), m_x, m_y - 20);
    // Position pins

    for (auto &pin : m_pins) {
      if (pin->orientation == RIGHT) {
        pin->SetPosition(m_x + pin->module_x() + PINLENGTH, m_y + pin->module_y() + pin->height() / 2);

      } else {
        pin->SetPosition(m_x + pin->module_x(), m_y + pin->module_y() + pin->height() / 2);
      }

      gtk_layout_move(GTK_LAYOUT(m_bbw->layout),
                      pin->m_pinDrawingArea, m_x + pin->module_x(), m_y + pin->module_y());
    }
  }
}


void GuiModule::GetPosition(int &x, int &y)
{
  Value *xpos = dynamic_cast<Value *>(m_module->findSymbol("xpos"));
  Value *ypos = dynamic_cast<Value *>(m_module->findSymbol("ypos"));
  x = xpos ? (gint64) * xpos : m_x;
  y = ypos ? (gint64) * ypos : m_y;
}


/* FIXME: calculate distance to the edges instead of the corners. */
double GuiModule::Distance(int px, int py)
{
  double distance;
  double min_distance = 100000000.0;
  // Upper left
  distance = sqrt((double)abs(m_x - px) * abs(m_x - px) +
                  abs(m_y - py) * abs(m_y - py));

  if (distance < min_distance) {
    min_distance = distance;
  }

  // Upper right
  distance = sqrt((double)abs(m_x + m_width - px) * abs(m_x + m_width - px) +
                  abs(m_y - py) * abs(m_y - py));

  if (distance < min_distance) {
    min_distance = distance;
  }

  // Lower left
  distance = sqrt((double)abs(m_x - px) * abs(m_x - px) +
                  abs(m_y + m_height - py) * abs(m_y + m_height - py));

  if (distance < min_distance) {
    min_distance = distance;
  }

  // Lower right
  distance = sqrt((double)abs(m_x + m_width - px) * abs(m_x + m_width - px) +
                  abs(m_y + m_height - py) * abs(m_y + m_height - py));

  if (distance < min_distance) {
    min_distance = distance;
  }

  return min_distance;
}


static GuiModule *find_closest_module(Breadboard_Window *bbw, int x, int y)
{
  GuiModule *closest = nullptr;
  double min_distance = 1000000.0;

  for (auto &p : bbw->modules) {
    double distance = p->Distance(x, y);

    if (distance < min_distance) {
      closest = p;
      min_distance = distance;
    }
  }

  return closest;
}


// FIXME
static GuiModule *dragged_module = nullptr;
static int dragging = 0;
static int all_trace = 0;
static int grab_next_module = 0;


void grab_module(GuiModule *p)
{
  dragged_module = p;
  gdk_pointer_grab(gtk_widget_get_window(p->bbw()->layout),
                   TRUE,
                   (GdkEventMask)(GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK),
                   gtk_widget_get_window(p->bbw()->layout),
                   nullptr,
                   GDK_CURRENT_TIME);
  treeselect_module(dragged_module);
  dragging = 1;
  p->bbw()->clear_nodes();
  p->bbw()->draw_nodes();
  gtk_widget_set_app_paintable(p->bbw()->layout, FALSE);
}


static void trace_all(GtkWidget *button, Breadboard_Window *bbw);


void Breadboard_Window::pointer_cb(GtkWidget *w,
                                   GdkEventButton *event,
                                   Breadboard_Window *bbw)
{
  int x = (int)(event->x);
  int y = (int)(event->y);

  switch (event->type) {
  case GDK_MOTION_NOTIFY:
    if (dragging && dragged_module) {
      dragged_module->SetPosition(x + pinspacing, y + pinspacing);
    }

    break;

  case GDK_BUTTON_PRESS:
    if (grab_next_module) {
      if (dragging) {
        gdk_pointer_ungrab(GDK_CURRENT_TIME);
        dragging = 0;
        gtk_widget_set_app_paintable(bbw->layout, TRUE);
        grab_next_module = 0;
        bbw->update_board_matrix();
      }

    } else {
      dragged_module = find_closest_module(bbw, x, y);

      if (dragged_module) {
        gdk_pointer_grab(gtk_widget_get_window(w),
                         TRUE,
                         (GdkEventMask)(GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK),
                         gtk_widget_get_window(w),
                         nullptr,
                         GDK_CURRENT_TIME);
        treeselect_module(dragged_module);
        dragging = 1;
        bbw->clear_nodes();
        bbw->draw_nodes();
        gtk_widget_set_app_paintable(bbw->layout, FALSE);
      }
    }

    break;

  case GDK_2BUTTON_PRESS:
    break;

  case GDK_BUTTON_RELEASE:
    if (dragging) {
      gdk_pointer_ungrab(GDK_CURRENT_TIME);
      bbw->update_board_matrix();
      dragging = 0;
      gtk_widget_set_app_paintable(bbw->layout, TRUE);

      if (all_trace) {
        trace_all(w, bbw);
      }

      UpdateModuleFrame(dragged_module, bbw);
    }

    break;

  default:
    printf("Whoops? event type %d\n", event->type);
    break;
  }
}


// When clicked on a pin
static gboolean button(GtkWidget *,
                       GdkEventButton *event,
                       GuiPin *p)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    if (p->getSnode()) {
      auto gn = static_cast<gui_node *>(
                       g_object_get_data((GObject*)p->bbw()->tree, p->getSnode()->name().c_str()));

      if (gn) {
        treeselect_node(gn);
        return TRUE;
      }
    }

    treeselect_stimulus(p);
    //puts("Stimulus should now be selected");
    return TRUE;
  }

  if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
    p->toggleState();
    return TRUE;
  }

  if (event->type == GDK_BUTTON_PRESS && event->button == 2) {
    if (p->getSnode()) {
      auto gn = static_cast<gui_node *>(
                       g_object_get_data((GObject*)p->bbw()->tree,
                                         p->getSnode()->name().c_str()));

      if (gn) {
        trace_node(gn);
        gn->bbw->draw_nodes();
      }
    }

    return TRUE;
  }

  return FALSE;
}


// get_string
static void a_cb(GtkWidget *, GtkDialog *dialog)
{
  gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}


// used for reading a value from user when break on value is requested
static std::string gui_get_string(const char *prompt, const char *initial_text)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("enter value",
                      nullptr,
                      GTK_DIALOG_MODAL,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                      nullptr);
  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
  GtkWidget *label = gtk_label_new("Enter string:");
  GtkWidget *label2 = gtk_label_new(prompt);
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), initial_text);
  gtk_widget_grab_focus(entry);
  g_signal_connect(entry, "activate", G_CALLBACK(a_cb), (gpointer)dialog);
  gtk_box_pack_start(GTK_BOX(content_area), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(content_area), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show_all(dialog);
  gint retval = gtk_dialog_run(GTK_DIALOG(dialog));
  std::string string;

  if (retval == GTK_RESPONSE_ACCEPT) {
    string = gtk_entry_get_text(GTK_ENTRY(entry));
  }

  gtk_widget_destroy(dialog);
  return string;
}


static void add_new_snode(GtkWidget *, Breadboard_Window *)
{
  std::string node_name = gui_get_string("Node name", "");

  if (!node_name.empty()) {
    new Stimulus_Node(node_name.c_str());
  }
}


////////////////////////////////////////////////////////////////////
static void select_node_ok_cb(GtkTreeView *tree_view,
                              GtkTreePath *path,
                              GtkTreeViewColumn *,
                              GtkDialog *dialog)
{
  Stimulus_Node* snode;
  GtkTreeModel *model;
  GtkTreeIter iter;
  model = gtk_tree_view_get_model(tree_view);
  gtk_tree_model_get_iter(model, &iter, path);
  gtk_tree_model_get(model, &iter, 1, &snode, -1);
  g_object_set_data((GObject*)dialog, "snode", snode);
  gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}


static void select_module_ok_cb(GtkTreeView *,
                                GtkTreePath *,
                                GtkTreeViewColumn *,
                                GtkDialog *dialog)
{
  gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}


static void copy_tree_to_clist(GtkTreeModel *model, GtkListStore *list_store)
{
  gui_node *gn;
  GtkTreeIter node_iter, iter, new_iter;

  if (gtk_tree_model_get_iter_first(model, &node_iter)) {
    if (gtk_tree_model_iter_n_children(model, &node_iter) > 0) {
      gtk_tree_model_iter_children(model, &iter, &node_iter);

      do {
        gtk_tree_model_get(model, &iter, 1, &gn, -1);
        gtk_list_store_append(list_store, &new_iter);
        gtk_list_store_set(list_store, &new_iter,
                           0, gn->node->name().c_str(), 1, (gpointer) gn->node, -1);

        if (!iter.stamp) {
          break;
        }
      } while (gtk_tree_model_iter_next(model, &iter));
    }
  }
}


static Stimulus_Node *select_node_dialog(Breadboard_Window *bbw)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Select node to connect to",
                      GTK_WINDOW(bbw->window),
                      GTK_DIALOG_MODAL,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      nullptr);
  GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *scrolledwindow = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkListStore *list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  GtkWidget *node_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(node_list), FALSE);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(node_list),
      0, "Nodes", renderer, "text", 0, nullptr);
  gtk_container_add(GTK_CONTAINER(scrolledwindow), node_list);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 220, 400);
  copy_tree_to_clist(
    gtk_tree_view_get_model((GtkTreeView*) bbw->tree),
    list_store);
  g_signal_connect(node_list, "row-activated",
                   G_CALLBACK(select_node_ok_cb), dialog);
  gtk_widget_show_all(dialog);
  int resp = gtk_dialog_run((GtkDialog*)dialog);
  auto snode = static_cast<Stimulus_Node*>(g_object_get_data((GObject*) dialog, "snode"));
  gtk_widget_destroy(dialog);

  if (resp == GTK_RESPONSE_ACCEPT) {
    return snode;
  }

  return nullptr;
}


static std::string select_module_dialog(Breadboard_Window *bbw)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Select module to load",
                      GTK_WINDOW(bbw->window), GTK_DIALOG_MODAL,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      nullptr);
  GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *scrolledwindow = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  const gchar *module_clist_titles[] = {"Name1", "Name2", "Library"};
#ifdef OLD_MODULE_LIBRARY
  int n_columns = 2;
  GtkListStore *list_store = gtk_list_store_new(n_columns + 1,
                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  int width = 220;
#else
  int n_columns = 3;
  GtkListStore *list_store = gtk_list_store_new(n_columns + 1,
                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  int width = 320;
#endif
  GtkWidget *module_clist = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);

  for (int col = 0; col < n_columns; ++col) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
                                  module_clist_titles[col], renderer,
                                  "text", col, nullptr);
    g_object_set(column,
                 "resizable", TRUE,
                 "sort-indicator", TRUE,
                 "sort-column-id", col,
                 nullptr);
    gtk_tree_view_append_column((GtkTreeView*) module_clist, column);
  }

  gtk_container_add(GTK_CONTAINER(scrolledwindow), module_clist);
  g_signal_connect(module_clist, "row-activated",
                   G_CALLBACK(select_module_ok_cb), dialog);
  gtk_window_set_default_size(GTK_WINDOW(dialog), width, 400);
#ifdef OLD_MODULE_LIBRARY
  ModuleLibrary::FileList::iterator  mi;
  ModuleLibrary::FileList::iterator  itFileListEnd(ModuleLibrary::GetFileList().end());

  // Add all modules
  for (mi = ModuleLibrary::GetFileList().begin();
       mi != itFileListEnd;
       ++mi) {
    ModuleLibrary::File *t = *mi;
    cout << t->name() << '\n';
    Module_Types * pFileTypes;

    if ((pFileTypes = t->get_mod_list())) {
      // Loop through the list and display all of the modules.
      int i = 0;

      while (pFileTypes[i].names[0]) {
        char name[STRING_SIZE];
        char library[STRING_SIZE];
        char *text[2] = {name, library};
        GtkTreeIter iter;
        g_strlcpy(name, pFileTypes[i].names[0], STRING_SIZE);
        g_strlcpy(library, t->name(), STRING_SIZE);
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           0, name,
                           1, library,
                           2, pFileTypes[i].names[1],
                           -1);
        i++;
      }
    }
  }

#else //OLD_MODULE_LIBRARY
  extern ModuleLibraries_t ModuleLibraries;

  for (const auto &mti : ModuleLibraries) {
    Module_Types * (*get_mod_list)() = mti.second->mod_list();
    Module_Types *pLibModList = get_mod_list();
    std::string text_2 = mti.second->user_name();

    if (pLibModList) {
      for (Module_Types *pModTypes = pLibModList;  pModTypes->names[0]; pModTypes++) {
        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           0, pModTypes->names[0],
                           1, pModTypes->names[1],
                           2, text_2.c_str(),
                           3, pModTypes->names[0],
                           -1);
      }
    }
  }

#endif
  gtk_widget_show_all(dialog);
  int resp = gtk_dialog_run((GtkDialog*)dialog);

  if (resp == GTK_RESPONSE_ACCEPT) {
    GtkTreeIter iter;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(module_clist));

    if (!gtk_tree_selection_get_selected(sel, nullptr, &iter)) {
      gtk_widget_destroy(dialog);
      return "";
    }

    gchar *s;
    gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 0, &s, -1);
    std::string str = s;
    g_free(s);
    gtk_widget_destroy(dialog);
    return str;
  }

  gtk_widget_destroy(dialog);
  return "";
}


static void stimulus_add_node(GtkWidget *, Breadboard_Window *bbw)
{
  Stimulus_Node *node = select_node_dialog(bbw);

  if (node && bbw->selected_pin) {
    node->attach_stimulus(bbw->selected_pin->getIOpin());
    // Update stimulus frame
    treeselect_stimulus(bbw->selected_pin);
  }
}


static void add_library(GtkWidget *, Breadboard_Window *)
{
  std::string library_name
    = gui_get_string("Module library name (e.g. libgpsim_modules)", "");
#ifdef OLD_MODULE_LIBRARY

  if (!library_name.empty()) {
    ModuleLibrary::LoadFile(library_name.c_str());
  }

#else

  if (!library_name.empty()) {
    ModuleLibrary::LoadFile(library_name);
  }

#endif
}


static void add_module(GtkWidget *, Breadboard_Window *bbw)
{
  std::string module_type = select_module_dialog(bbw);

  if (!module_type.empty()) {
    std::string name = gui_get_string("Module name", module_type.c_str());
    grab_next_module = 1;

    if (!name.empty())
#ifdef OLD_MODULE_LIBRARY
      ModuleLibrary::NewObject(module_type.c_str(), name.c_str());

#else
    {
      if (!ModuleLibrary::InstantiateObject(module_type, name))
        fprintf(stderr, "Module load of %s %s failed\n",
                module_type.c_str(), name.c_str());
    }
#endif
  }
}


void Breadboard_Window::remove_module(GtkWidget *, Breadboard_Window *bbw)
{
  delete bbw->selected_module->module();
  // FIXME the rest should be as callback from src
  // Remove pins
  std::vector<GuiPin *> *e = bbw->selected_module->pins();
  auto pin_iter = e->begin();

  for (; pin_iter != e->end(); ++pin_iter) {
    GuiPin *pin = *pin_iter;
    gtk_widget_destroy(GTK_WIDGET(pin->m_pinDrawingArea));
  }

  // Remove widget
  if (bbw->selected_module->module_widget())
    gtk_container_remove(GTK_CONTAINER(bbw->layout),
                         bbw->selected_module->module_widget());

  if (bbw->selected_module->pinLabel_widget())
    gtk_container_remove(GTK_CONTAINER(bbw->layout),
                         bbw->selected_module->pinLabel_widget());

  gtk_container_remove(GTK_CONTAINER(bbw->layout),
                       bbw->selected_module->name_widget());
  // Remove module from tree
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  selection = gtk_tree_view_get_selection((GtkTreeView*) bbw->tree);
  gtk_tree_selection_get_selected(selection, &model, &iter);
  gtk_tree_store_set((GtkTreeStore*) model, &iter, 1, nullptr, -1);
  gtk_tree_store_remove((GtkTreeStore*) model, &iter);
  // Remove from local list of modules
  auto mi = std::find(bbw->modules.begin(), bbw->modules.end(), bbw->selected_module);

  if (mi != bbw->modules.end()) {
    bbw->modules.erase(mi);
  }

  gtk_widget_hide(bbw->module_frame);
  delete bbw->selected_module;
  bbw->selected_module = nullptr;
}


static void remove_node(GtkWidget *, Breadboard_Window *bbw)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gui_node *gn;
  selection = gtk_tree_view_get_selection((GtkTreeView*) bbw->tree);
  gtk_tree_selection_get_selected(selection, &model, &iter);
  gtk_tree_model_get(model, &iter, 1, &gn, -1);
  gtk_tree_store_remove((GtkTreeStore*) model, &iter);
  g_object_set_data(G_OBJECT(bbw->tree), gn->node->name().c_str(), nullptr);
  delete gn;
  gtk_widget_hide(bbw->node_frame);
  gtk_widget_hide(bbw->stimulus_frame);
  gtk_widget_hide(bbw->module_frame);
}


static void remove_node_stimulus(GtkWidget *, Breadboard_Window *bbw)
{
  stimulus *s;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  selection = gtk_tree_view_get_selection((GtkTreeView*) bbw->node_clist);
  gtk_tree_selection_get_selected(selection, &model, &iter);
  gtk_tree_model_get(model, &iter, 1, &s, -1);
  bbw->selected_node->node->detach_stimulus(s);
  gtk_list_store_remove((GtkListStore*) model, &iter);
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

// Returns a file name which must be freed with g_free() or NULL
static char *gui_get_filename(const std::string &filename)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Log settings",
                      nullptr,
                      GTK_FILE_CHOOSER_ACTION_SAVE,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                      nullptr);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

  if (!filename.empty()) {
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), filename.c_str());

  } else {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), ".");
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "netlist.stc");
  }

  char *file = nullptr;

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
  }

  gtk_widget_destroy(dialog);
  return file;
}


static FILE *fo;


static void OneAttribute(const SymbolEntry_t &sym)
{
  auto pVal = dynamic_cast<Value *>(sym.second);

  if (pVal && fo) {
    // read attributes and add to clist
    if (strstr(typeid(*pVal).name(), "Attribute")) {
      char attribute_value[STRING_SIZE];
      pVal->get(attribute_value, sizeof(attribute_value));
      fprintf(fo, "%s.%s = %s\n", mod_name, pVal->name().c_str(),
              attribute_value);
    }
  }
}


static std::string stc_file;


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
static void save_stc(GtkWidget *, Breadboard_Window *bbw)
{
  Module *m;
  char *filename = gui_get_filename(stc_file);

  if (!filename) {
    return;
  }

  if ((fo = fopen(filename, "w")) == nullptr) {
    perror(filename);
    g_free(filename);
    return;
  }

  stc_file = filename;
  fprintf(fo, "\n# This file was written by gpsim.\n");
  fprintf(fo, "\n# You can use this file for example like this:");
  fprintf(fo, "\n#     gpsim -s mycode.cod -c netlist.stc\n");
  fprintf(fo, "\n# If you want to add commands, you can create another .stc file");
  fprintf(fo, "\n# and load this file from it. Something like this:");
  fprintf(fo, "\n# ----------- myproject.stc ---------------");
  fprintf(fo, "\n# load s mycode.cod");
  fprintf(fo, "\n# frequency 12000000");
  fprintf(fo, "\n# load c netlist.stc");
  fprintf(fo, "\n# -----------------------------------------");
  fprintf(fo, "\n# You can then just load this new file:");
  fprintf(fo, "\n#     gpsim -c myproject.stc");
  fprintf(fo, "\n# and use netlist.stc whenever you save from the breadboard.");
  fprintf(fo, "\n#");
  fprintf(fo, "\n");
  fprintf(fo, "\n\n# Processor position:\n");
#ifdef OLD_MODULE_LIBRARY
  // Save module libraries
  fprintf(fo, "\n\n# Module libraries:\n");
  ModuleLibrary::FileList::iterator  mi;

  // Add all modules
  for (mi = ModuleLibrary::GetFileList().begin();
       mi != ModuleLibrary::GetFileList().end();
       ++mi) {
    ModuleLibrary::File *t = *mi;
    fprintf(fo, "module library %s\n",
            t->name());
  }

#else
  extern ModuleLibraries_t ModuleLibraries;

  for (const auto &mti : ModuleLibraries) {
    fprintf(fo, "module library %s\n", mti.second->user_name().c_str());
  }

#endif
  // Save modules
  fprintf(fo, "\n\n# Modules:\n");

  for (const auto &p : bbw->modules) {
    m = p->module();
    SymbolTable_t *st = &m->getSymbolTable();
    auto cpu = dynamic_cast<Processor*>(m);

    if (cpu == nullptr) {
      // Module, not a processor, so add the load command
      fprintf(fo, "module load %s %s\n",
              m->type(),
              m->name().c_str());
    }

    mod_name = m->name().c_str();
    st->ForEachSymbolTable(OneAttribute);
    fprintf(fo, "\n");
  }

  // Save nodes and connections
  fprintf(fo, "\n\n# Connections:\n");

  for (const auto &node : bbw->nodes) {
    fprintf(fo, "node %s\n", node->name().c_str());

    if (node->stimuli) {
      fprintf(fo, "attach %s", node->name().c_str());

      for (stimulus *stimulus = node->stimuli; stimulus; stimulus = stimulus->next) {
        fprintf(fo, " %s", stim_full_name(stimulus));
      }
    }

    fprintf(fo, "\n\n");
  }

  fprintf(fo, "\n\n# End.\n");
  fclose(fo);
  fo = nullptr;
}


static void clear_traces(GtkWidget *, Breadboard_Window *bbw)
{
  all_trace = 0;
  bbw->update_board_matrix();
}


static void trace_all(GtkWidget *, Breadboard_Window *bbw)
{
  gui_node *gn;
  GtkTreeModel *model;
  GtkTreeIter p_iter, c_iter;
  bool did_work = true;
  bbw->update_board_matrix();

  if ((model = gtk_tree_view_get_model((GtkTreeView*) bbw->tree)) == nullptr) {
    return;
  }

  if (!gtk_tree_model_get_iter_first(model, &p_iter)) {
    return;
  }

  if (!gtk_tree_model_iter_children(model, &c_iter, &p_iter)) {
    return;
  }

  do {
    gtk_tree_model_get(model, &c_iter, 1, &gn, -1);

    if (!trace_node(gn)) {
      did_work = false;
    }
  } while (gtk_tree_model_iter_next(model, &c_iter));

  bbw->draw_nodes();

  gtk_label_set_text(GTK_LABEL(bbw->status_line), did_work ? "" : "Cannot trace all nodes");

  all_trace = 1;

  if (verbose) {
    puts("Trace all is done.");
  }
}


//========================================================================
GuiBreadBoardObject::GuiBreadBoardObject(Breadboard_Window *bbw, int x, int y)
  : m_bbw(bbw), m_x(x), m_y(y), m_width(0), m_height(0), m_bIsBuilt(false)
{
}


void GuiBreadBoardObject::SetPosition(int x, int y)
{
  m_x = x;
  m_y = y;
}


void GuiPin::SetModulePosition(int x, int y)
{
  m_module_x = x;
  m_module_y = y;
}


//========================================================================
GuiPin::GuiPin(Breadboard_Window *_bbw,
               GuiModule *pModule,
               Package *_package,
               unsigned int pin_number)
  : GuiBreadBoardObject(_bbw, 0, 0),
    package(_package),
    m_pModule(pModule), m_module_x(0), m_module_y(0),
    m_label_x(0), m_label_y(0),
    m_pkgPinNumber(pin_number)
{
  IOPIN *iopin = getIOpin();
  m_width = pinspacing;
  m_height = pinspacing;
  orientation = LEFT;

  if (iopin) {
    value = iopin->getState();
    direction = iopin->get_direction() == 0 ? PIN_INPUT : PIN_OUTPUT;
    //orientation=_orientation;
    type = PIN_DIGITAL;

  } else {
    value = false;
    direction = PIN_INPUT;
    //orientation=_orientation;
    type = PIN_OTHER;
  }

  // Create widget
  m_pinDrawingArea = gtk_drawing_area_new();
  gtk_widget_set_events(m_pinDrawingArea,
                        gtk_widget_get_events(m_pinDrawingArea) |
                        GDK_BUTTON_PRESS_MASK);
  g_signal_connect(m_pinDrawingArea,
                   "button_press_event",
                   G_CALLBACK(button),
                   this);
  gtk_widget_set_size_request(m_pinDrawingArea, m_width, m_height);
  g_signal_connect(m_pinDrawingArea,
                   "expose_event",
                   G_CALLBACK(expose_pin),
                   this);
  gtk_widget_show(m_pinDrawingArea);
}


const char *GuiPin::pinName()
{
  IOPIN *iopin = getIOpin();
  return iopin ? iopin->name().c_str() : nullptr;
}


//------------------------------------------------------------------------
// GuiPin::update() - check the state of the iopin and make the gui match
//
void GuiPin::Update()
{
  IOPIN *iopin = getIOpin();

  if (iopin) {
    bool value = iopin->getState();
    eDirection dir = iopin->get_direction() == 0 ? PIN_INPUT : PIN_OUTPUT;

    if (value != getState() || dir != direction) {
      putState(value);
      direction = dir;
      Draw();
    }
  }
}


//------------------------------------------------------------------------
void GuiPin::toggleState()
{
  IOPIN *iopin = getIOpin();

  if (iopin) {
    char cPinState = iopin->getForcedDrivenState();

    switch (cPinState) {
    case '0':
    case 'Z':
    case 'X':
      iopin->forceDrivenState('1');
      break;

    case '1':
      iopin->forceDrivenState('0');
      break;

    case 'W':
      iopin->forceDrivenState('w');
      break;

    case 'w':
      iopin->forceDrivenState('W');
      break;
    }

    m_bbw->Update();
  }
}


//------------------------------------------------------------------------
// GuiPin::draw() - draw a single pin
//
//
void GuiPin::Draw()
{
  GdkWindow *gdk_win = gtk_widget_get_window(m_pinDrawingArea);

  if (!gdk_win) {
    return;
  }

  gdk_window_invalidate_rect(gdk_win, nullptr, FALSE);
}


gboolean GuiPin::expose_pin(GtkWidget *widget, GdkEventExpose *, GuiPin *p)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  int pointx;
  int wingheight, wingx;
  int casex, endx;

  switch (p->orientation) {
  case LEFT:
    casex = p->m_width;
    endx = 0;
    break;

  default:
    casex = 0;
    endx = p->m_width;
    break;
  }

  int y = p->m_height / 2;

  if (p->type != PIN_OTHER) {
    gdk_cairo_set_source_color(cr, p->getState() ? &high_output_color : &low_output_color);
  }

  // Draw actual pin
  cairo_set_line_width(cr, PINLINEWIDTH);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_move_to(cr, casex, y);
  cairo_line_to(cr, endx, y);
  cairo_stroke(cr);

  if (p->type == PIN_OTHER) {
    cairo_destroy(cr);
    return FALSE;
  }

  // Draw direction arrow
  wingheight = p->m_height / 3;

  if (casex > endx) {
    if (p->direction == PIN_OUTPUT) {
      pointx = endx + PINLENGTH / 3;
      wingx = endx + (PINLENGTH * 2) / 3;

    } else {
      pointx = endx + (PINLENGTH * 2) / 3;
      wingx = endx + PINLENGTH / 3;
    }

  } else {
    if (p->direction == PIN_OUTPUT) {
      pointx = casex + (PINLENGTH * 2) / 3;
      wingx = casex + PINLENGTH / 3;

    } else {
      pointx = casex + PINLENGTH / 3;
      wingx = casex + (PINLENGTH * 2) / 3;
    }
  }

  // Draw an arrow poining at (endx,endy)
  cairo_move_to(cr, wingx, wingheight + y);
  cairo_line_to(cr, pointx, y);
  cairo_line_to(cr, wingx, wingheight - y);
  cairo_stroke(cr);
  cairo_destroy(cr);
  return FALSE;
}


void GuiPin::SetLabelPosition(int x, int y)
{
  m_label_x = x;
  m_label_y = y;
}


//------------------------------------------------------------------------
// GuiPin::DrawLabel() - draw the label for a single pin
//
//
void GuiPin::DrawLabel(cairo_t *cr)
{
  IOPIN *iopin = getIOpin();

  if (!iopin || !m_bbw) {
    return;
  }

  const std::string &name = iopin->GUIname().empty()
                     ? iopin->name() : iopin->GUIname();

  if (!name.empty()) {
    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_font_description(layout, m_bbw->pinnamefont);
    pango_layout_set_text(layout, name.c_str(), -1);
    pango_cairo_update_layout(cr, layout);
    cairo_move_to(cr, m_label_x,
                  m_label_y - pango_layout_get_baseline(layout) / PANGO_SCALE);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
  }
}


//------------------------------------------------------------------------
// GuiPin::DrawGUIlabel() - Gte it pin has changed since last access
//
//
bool GuiPin::DrawGUIlabel()
{
  IOPIN *iopin = getIOpin();

  if (iopin && iopin->is_newGUIname()) {
    iopin->clr_is_newGUIname();
    return true;
  }

  return false;
}


//------------------------------------------------------------------------
void GuiPin::addXref(CrossReferenceToGUI *newXref)
{
  xref = newXref;
}


//------------------------------------------------------------------------
void GuiPin::Destroy()
{
  if (xref) {
    getIOpin()->remove_xref(xref);
  }

  gtk_widget_destroy(m_pinDrawingArea);
}


//------------------------------------------------------------------------

gboolean GuiModule::module_expose(GtkWidget *widget,
                                  GdkEventExpose *, GuiModule *p)
{
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
  p->DrawCaseOutline(cr);

  for (const auto &pin : p->m_pins) {
    pin->DrawLabel(cr);
  }

  cairo_destroy(cr);
  return FALSE;
}


void GuiModule::Draw()
{
}


void GuiModule::Destroy()
{
}


#define PACKAGESPACING 15

void GuiModule::Update()
{
  g_object_ref(m_pinLabel_widget);
  gtk_container_remove(GTK_CONTAINER(m_bbw->layout), m_pinLabel_widget);

  // Delete the static module pixmap if there is no widget
  // in the module.
  if (m_module->get_widget() == nullptr) {
    gtk_widget_destroy(m_pinLabel_widget);
  }

  // Delete the pins
  for (const auto &pin : m_pins) {
    pin->Destroy();
  }

  // Destroy name widget
  delete m_name_widget;
  // Remove module from list
  auto iter = std::find(m_bbw->modules.begin(), m_bbw->modules.end(), this);

  if (iter != m_bbw->modules.end()) {
    m_bbw->modules.erase(iter);
  }

  // rebuild module
  Build();
  g_object_unref(m_pinLabel_widget);
}


void GuiModule::UpdatePins()
{
  bool change = false;

  for (const auto &pin : m_pins) {
    change |= pin->DrawGUIlabel();
    pin->Update();
  }

  if (change) {
    gtk_widget_queue_draw(m_pinLabel_widget);
  }
}


//========================================================================
//========================================================================
class PositionAttribute : public Float {
protected:
  Breadboard_Window *bbw;
public:
  PositionAttribute(Breadboard_Window *_bbw, const char *n, double v);
  void set(Value *v) override;
};


PositionAttribute::PositionAttribute(Breadboard_Window *_bbw, const char *n, double v)
  : Float(v), bbw(_bbw)
{
  new_name(n);
}


void PositionAttribute::set(Value *v)
{
  Float::set(v);

  if (bbw) {
    bbw->Update();
  }
}


//------------------------------------------------------------------------
void GuiModule::DrawCaseOutline(cairo_t *cr)
{
  cairo_rectangle(cr, CASEOFFSET, CASEOFFSET, m_width - 2 * CASEOFFSET, m_height - 2 * CASEOFFSET);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill_preserve(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_stroke(cr);
}


//------------------------------------------------------------------------

//
static float hackPackageHeight = 0.0;

void GuiModule::AddPin(unsigned int pin_number)
{
  IOPIN *iopin = m_module->get_pin(pin_number);
  BreadBoardXREF *cross_reference = nullptr;

  if (iopin) {
    // Create xref
    cross_reference = new BreadBoardXREF();
    cross_reference->parent_window = (gpointer) m_bbw;
    cross_reference->data = nullptr;
    iopin->add_xref(cross_reference);
  }

  auto pin = new GuiPin(m_bbw, this, m_module->package, pin_number);
  pin->addXref(cross_reference);
  m_pins.push_back(pin);
}


void GuiModule::AddPinGeometry(GuiPin *pin)
{
  eOrientation orientation;
  unsigned int pin_number = pin->number();
  // Get the X and Y coordinates for this pin
  // (the coordinates are referenced to the module's origin)
  int pin_x, pin_y;
  int label_x, label_y;
  const PinGeometry *pPinGeometry = m_module->package->getPinGeometry(pin_number);

  if (pPinGeometry->bNew) {
    switch (pPinGeometry->m_orientation) {
    case UP:
      pin_x   = (int)pPinGeometry->m_x;
      pin_y   = (int)pPinGeometry->m_y;
      label_x = pin_x + LABELPAD + CASELINEWIDTH;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;
      orientation = UP;
      break;

    case LEFT:
      pin_x   = (int)pPinGeometry->m_x - pinspacing;
      pin_y   = (int)pPinGeometry->m_y;
      label_x = LABELPAD + CASELINEWIDTH;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;
      orientation = LEFT;
      break;

    case RIGHT:
      pin_x = m_width + (int)pPinGeometry->m_x;
      pin_y = (int)pPinGeometry->m_y;
      label_x = pin_x + LABELPAD + CASELINEWIDTH + m_width / 2;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;
      orientation = RIGHT;
      break;

    case DOWN:
      pin_x = (int)pPinGeometry->m_x;
      pin_y = m_height + (int)pPinGeometry->m_y;
      label_x = pin_x + LABELPAD + CASELINEWIDTH;
      label_y = pin_y + LABELPAD + CASELINEWIDTH;
      orientation = DOWN;
      break;

    default:
      printf("################### Error:\nUndefined orientation.\n");
      assert(0);
    }

  } else {
    // old style -- to be deprecated.
    float pin_position = m_module->package->get_pin_position(pin_number);

    // Put pin in layout
    if (pin_position >= 0.0 && pin_position < 1.0) {
      pin_x = -pinspacing;
      pin_y = (int)(m_height / 2 + ((pin_position - 0.5) * hackPackageHeight)) - pinspacing / 2;
      orientation = LEFT;
      label_x = LABELPAD + CASELINEWIDTH;
      label_y = (int)(pin_position * hackPackageHeight);
      label_y += LABELPAD + CASELINEWIDTH + pinspacing / 2 - m_bbw->pinnameheight / 3;
      label_y += PINLENGTH / 2;

    } else if (pin_position >= 2.0 && pin_position < 3.0) {
      pin_x = m_width;
      pin_y = (int)(m_height / 2 + ((3.0 - pin_position - 0.5) * hackPackageHeight)) - pinspacing / 2;
      orientation = RIGHT;
      label_x = m_width / 2  + CASELINEWIDTH;
      label_y = (int)((3.0 - pin_position) * hackPackageHeight);
      label_y += LABELPAD + CASELINEWIDTH + pinspacing / 2 - m_bbw->pinnameheight / 3;
      label_y += PINLENGTH / 2;

    } else {
      // FIXME
      printf("################### Error:\n");
      printf("Number of pins %u\n", m_module->package->number_of_pins);
      printf("pin_position %f\n", pin_position);
      printf("pin_position2 %f\n", m_module->package->get_pin_position(pin_number));
      printf("pin_number %u\n", pin_number);
      assert(0);
    }
  }

  pin->SetModulePosition(pin_x, pin_y);
  pin->SetLabelPosition(label_x, label_y);
  pin->SetOrientation(orientation);
  pin->Draw();
}


//------------------------------------------------------------------------
void GuiModule::Build()
{
  if (m_bIsBuilt || !m_bbw || !m_bbw->enabled) {
    return;
  }

  int nx, ny;
  BreadBoardXREF *cross_reference;
  m_width = 50;
  m_height = 18;
  Package *package = m_module->package;

  if (!package) {
    return;  // embedded module
  }

  m_module_widget = (GtkWidget *)m_module->get_widget();
  m_pin_count = m_module->get_pin_count();
  /*
  Value *xpos = m_module->get_attribute("xpos", false);
  Value *ypos = m_module->get_attribute("ypos", false);
  xpos->get(nx);
  ypos->get(ny);
  */
  GetPosition(nx, ny);
  GtkTreeIter module_titer, pin_titer;
  GtkTreeStore *tree_store;
  g_object_get(m_bbw->tree, "model", &tree_store, nullptr);
  gtk_tree_store_append(tree_store, &module_titer, nullptr);
  gtk_tree_store_set(tree_store, &module_titer,
                     0, m_module->name().c_str(),
                     1, this,
                     -1);
  hackPackageHeight = (float)((m_pin_count / 2 + (m_pin_count & 1) - 1) * pinspacing);
  // Find the length of the longest pin name in each direction
  // The directions are in the order of left, up , right, down.
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(m_bbw->window));
  PangoLayout *layout = pango_cairo_create_layout(cr);
  pango_layout_set_font_description(layout, m_bbw->pinnamefont);

  for (int i = 1; i <= m_pin_count; i++) {
    PinGeometry *pPinGeometry = m_module->package->getPinGeometry(i);
    pPinGeometry->convertToNew();
    int w = 0;
    std::string name = m_module->get_pin_name(i);

    if (!name.empty() && pPinGeometry->m_bShowPinname) {
      pango_layout_set_text(layout, name.c_str(), -1);
      pango_layout_get_size(layout, &w, nullptr);
      w /= PANGO_SCALE;
    }

    if (w > pinnameWidths[pPinGeometry->m_orientation]) {
      pinnameWidths[pPinGeometry->m_orientation] = w;
    }

    AddPin(i);
  }

  g_object_unref(layout);
  cairo_destroy(cr);

  if (!m_module_widget) {
    // Create a static representation.
    m_width =  pinnameWidths[0] + pinnameWidths[2] + 2 * FOORADIUS;
    m_width += 2 * CASELINEWIDTH + 2 * LABELPAD;
    m_height = m_module->get_pin_count() / 2 * pinspacing; // pin name height

    if (m_module->get_pin_count() % 2) {
      m_height += pinspacing;
    }

    m_height += 2 * CASELINEWIDTH + 2 * LABELPAD;
    m_pinLabel_widget = gtk_drawing_area_new();
    gtk_widget_set_size_request(m_pinLabel_widget, m_width, m_height);
    gtk_widget_show_all(m_pinLabel_widget);
    g_signal_connect(m_pinLabel_widget, "expose_event",
                     G_CALLBACK(module_expose), this);
    gtk_widget_show(m_pinLabel_widget);

  } else {
    // Get the [from the module] provided widget's size
    GtkRequisition req;
    gtk_widget_size_request(m_module_widget, &req);
    m_width = req.width;
    m_height = req.height;
    gtk_widget_show(m_module_widget);
  }

  // Create xref
  cross_reference = new BreadBoardXREF();
  cross_reference->parent_window = (gpointer) m_bbw;
  cross_reference->data = nullptr;
  m_module->xref->_add(cross_reference);
  // Create name_widget
  m_name_widget = new BB_ModuleLabel(m_module->name(), m_bbw->pinnamefont);

  for (auto &pin : m_pins) {
    AddPinGeometry(pin);
    gtk_layout_put(GTK_LAYOUT(m_bbw->layout), pin->m_pinDrawingArea, 0, 0);
    // Add pin to tree
    const char *name = pin->pinName();

    if (name) {
      gtk_tree_store_append(tree_store, &pin_titer, &module_titer);
      gtk_tree_store_set(tree_store, &pin_titer,
                         0, name,
                         1, pin,
                         -1);
    }
  }

  if (m_pinLabel_widget) {
    gtk_layout_put(GTK_LAYOUT(m_bbw->layout), m_pinLabel_widget, 0, 0);
  }

  if (m_module_widget) {
    gtk_layout_put(GTK_LAYOUT(m_bbw->layout), m_module_widget, 0, 0);
  }

  gtk_layout_put(GTK_LAYOUT(m_bbw->layout), m_name_widget->gobj(), 0, 0);
  SetPosition(nx, ny);
  m_bIsBuilt = true;
  m_bbw->update_board_matrix();
}


//========================================================================
//========================================================================

GuiModule::GuiModule(Module *_module, Breadboard_Window *_bbw)
  : GuiBreadBoardObject(_bbw, 0, 0), m_module(_module), m_module_widget(nullptr),
    m_pinLabel_widget(nullptr), m_module_x(0), m_module_y(0), m_name_widget(nullptr),
    m_pin_count(0)
{
  m_width = 0;
  m_height = 0;
  pinnameWidths[0] = 0;
  pinnameWidths[1] = 0;
  pinnameWidths[2] = 0;
  pinnameWidths[3] = 0;

  if (m_bbw) {
    m_bbw->modules.push_back(this);

    if (m_module) {
      Value *xpos = dynamic_cast<Value*>(m_module->findSymbol("xpos"));
      Value *ypos = dynamic_cast<Value*>(m_module->findSymbol("xpos"));

      if (!xpos || !ypos) {
        xpos = new PositionAttribute(m_bbw, "xpos", 80.0);
        ypos = new PositionAttribute(m_bbw, "ypos", 80.0);
        m_module->addSymbol(xpos);
        m_module->addSymbol(ypos);
      }
    }
  }
}


//========================================================================
GuiDipModule::GuiDipModule(Module *_module, Breadboard_Window *_bbw)
  : GuiModule(_module, _bbw)
{
}


void GuiDipModule::DrawCaseOutline(cairo_t *cr)
{
  cairo_line_to(cr, CASEOFFSET, CASEOFFSET);
  cairo_line_to(cr, CASEOFFSET, m_height - 2 * CASEOFFSET);
  cairo_line_to(cr, m_width - CASEOFFSET, m_height - 2 * CASEOFFSET);
  cairo_line_to(cr, m_width - CASEOFFSET, CASEOFFSET);
  cairo_arc(cr, m_width / 2 - CASEOFFSET, CASEOFFSET, FOORADIUS, 0, M_PI);
  cairo_close_path(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill_preserve(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_stroke(cr);
}


//========================================================================
void Breadboard_Window::Update()
{
  int x, y;

  // loop all modules and look for changes
  if (!enabled) {
    return;
  }

  if (!gtk_widget_get_visible(window)) {
    return;
  }

  for (const auto &p: modules) {
    if (p->IsBuilt()) {
      // Check if module has changed number of pins
      if (p->pin_count() != p->module()->get_pin_count())
        // If so, refresh the gui widget
      {
        p->Update();
      }

      // Check if module has changed its position
      p->GetPosition(x, y);

      if (p->x() != x || p->y() != y) {
        // If so, move the module
        p->SetPosition(x, y);
        p->bbw()->update_board_matrix();
      }

      // Check if pins have changed state
      p->UpdatePins();

    } else {
      p->Build();
      Update();
    }
  }
}


/* When a processor is created */
void Breadboard_Window::NewProcessor(GUI_Processor *)
{
  m_MainCpuModule = new GuiDipModule(gp->cpu, this);

  if (!enabled) {
    return;
  }

  m_MainCpuModule->Build();

  if (!gp || !gp->cpu) {
    return;
  }

  Update();
}


/* When a module is created */
void Breadboard_Window::NewModule(Module *module)
{
  auto p = new GuiModule(module, this);

  if (!enabled) {
    return;
  }

  p->Build();

  if (grab_next_module) {
    grab_module(p);
  }

  Update();
}


/* When a stimulus is being connected or disconnected, or a new node is created */
void Breadboard_Window::NodeConfigurationChanged(Stimulus_Node *node)
{
  if (std::find(nodes.begin(), nodes.end(), node) == nodes.end()) {
    nodes.push_back(node);
  }

  if (!node_iter) {
    return;
  }

  auto gn = static_cast<gui_node*>(g_object_get_data(G_OBJECT(tree), node->name().c_str()));
  GtkTreeStore *tree_store;
  g_object_get(tree, "model", &tree_store, nullptr);

  if (!gn) {
    GtkTreeIter parent_iter, iter;
    gn = new gui_node;
    gn->bbw = this;
    gn->node = node;
    g_object_set_data(G_OBJECT(tree), node->name().c_str(), gn);
    gtk_tree_model_get_iter_first((GtkTreeModel*) tree_store, &parent_iter);
    gtk_tree_store_append(tree_store, &iter, &parent_iter);
    gtk_tree_store_set(tree_store, &iter,
                       0, node->name().c_str(),
                       1, gn,
                       -1);
  }
}


GtkWidget *Breadboard_Window::bb_vbox()
{
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  return vbox;
}


GtkWidget *Breadboard_Window::bb_hbox()
{
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox);
  return hbox;
}


GtkWidget* Breadboard_Window::add_button(const char *label, GCallback f,
    GtkWidget *box)
{
  GtkWidget *btn = gtk_button_new_with_label(label);
  gtk_widget_show(btn);
  gtk_box_pack_start(GTK_BOX(box), btn, FALSE, FALSE, 0);
  g_signal_connect(btn, "clicked", G_CALLBACK(f), this);
  return btn;
}


void Breadboard_Window::Build()
{
  if (bIsBuilt || !enabled) {
    return;
  }

  GtkWidget *hpaned1;
  GtkWidget *vbox9;
  GtkWidget *vbox13;
  GtkWidget *scrolledwindow4;
  GtkWidget *tree1;
  GtkWidget *hbox12;
  GtkWidget *hbox15;
  GtkWidget *vbox11;
  GtkWidget *scrolledwindow2;
  GtkWidget *viewport7;
  GtkWidget *hbox10;
  GtkWidget *vbox14;
  GtkWidget *hbox13;
  GtkWidget *vbox10;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport6;
  GtkWidget *hbox9;
  GtkWidget *hbox14;
  GtkWidget *scrolledwindow5;
  GtkCellRenderer *renderer;
  GtkListStore *list_store;
  GtkTreeStore *tree_store;
  GtkTreeSelection *selection;
  gdk_color_parse("red", &high_output_color);
  gdk_color_parse("green", &low_output_color);
  //
  // Top level window
  //
  g_object_set_data(G_OBJECT(window), "window", window);
  gtk_window_set_title(GTK_WINDOW(window), "Breadboard");
  //
  // Status line
  //
  GtkWidget *base_box = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), base_box);
  status_line = gtk_label_new("");
  gtk_box_pack_end(GTK_BOX(base_box), status_line, FALSE, FALSE, 0);
  gtk_widget_show_all(base_box);
  //
  // Horizontal pane
  //
  hpaned1 = gtk_hpaned_new();
  gtk_widget_show(hpaned1);
  gtk_box_pack_end(GTK_BOX(base_box), hpaned1, TRUE, TRUE, 0);
  //gtk_container_add (GTK_CONTAINER (window), hpaned1);
  gtk_paned_set_position(GTK_PANED(hpaned1), 196);
  // vbox9 holds the left pane.
  vbox9 = bb_vbox();
  gtk_paned_pack1(GTK_PANED(hpaned1), vbox9, FALSE, TRUE);
  vbox13 = bb_vbox();
  gtk_box_pack_start(GTK_BOX(vbox9), vbox13, TRUE, TRUE, 2);
  scrolledwindow4 = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_widget_show(scrolledwindow4);
  gtk_box_pack_start(GTK_BOX(vbox13), scrolledwindow4, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow4),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  renderer = gtk_cell_renderer_text_new();
  tree_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  tree = tree1 = gtk_tree_view_new_with_model((GtkTreeModel*) tree_store);
  gtk_tree_view_insert_column_with_attributes((GtkTreeView*) tree1,
      0, "",
      renderer,
      "text", 0,
      nullptr);
  g_object_set(tree1,
               "headers-visible", FALSE,
               "enable-tree-lines", TRUE,
               nullptr);
  g_signal_connect(gtk_tree_view_get_selection((GtkTreeView*) tree1),
                   "changed", (GCallback) treeselect_cb,
                   nullptr);
  gtk_widget_show(tree1);
  gtk_container_add(GTK_CONTAINER(scrolledwindow4), tree1);
  hbox12 = bb_hbox();
  gtk_box_pack_start(GTK_BOX(vbox13), hbox12, FALSE, FALSE, 0);
  add_button("Add node", G_CALLBACK(add_new_snode), hbox12);
  add_button("Add module", G_CALLBACK(add_module), hbox12);
  add_button("Add library", G_CALLBACK(add_library), hbox12);
  hbox15 = bb_hbox();
  gtk_box_pack_start(GTK_BOX(vbox13), hbox15, FALSE, FALSE, 0);
  add_button("Trace all", G_CALLBACK(trace_all), hbox15);
  add_button("Clear traces", G_CALLBACK(clear_traces), hbox15);
  node_frame = gtk_frame_new("Node connections");
  gtk_box_pack_start(GTK_BOX(vbox9), node_frame, TRUE, TRUE, 0);
  vbox11 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox11);
  gtk_container_add(GTK_CONTAINER(node_frame), vbox11);
  scrolledwindow2 = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_widget_show(scrolledwindow2);
  gtk_box_pack_start(GTK_BOX(vbox11), scrolledwindow2, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  viewport7 = gtk_viewport_new(nullptr, nullptr);
  gtk_widget_show(viewport7);
  gtk_container_add(GTK_CONTAINER(scrolledwindow2), viewport7);
  renderer = gtk_cell_renderer_text_new();
  list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  node_clist = gtk_tree_view_new_with_model((GtkTreeModel*) list_store);
  gtk_tree_view_insert_column_with_attributes((GtkTreeView*) node_clist, 0,
      "Nodes", renderer,
      "text", 0, nullptr);
  g_object_set(node_clist, "headers-visible", FALSE, nullptr);
  g_object_ref(node_clist);
  g_object_set_data_full(G_OBJECT(window), "node_clist", node_clist,
                         (GDestroyNotify) g_object_unref);
  gtk_widget_show(node_clist);
  gtk_container_add(GTK_CONTAINER(viewport7), node_clist);
  hbox10 = bb_hbox();
  gtk_box_pack_start(GTK_BOX(vbox11), hbox10, FALSE, FALSE, 0);
  add_button("Remove stimulus", G_CALLBACK(remove_node_stimulus), hbox10);
  add_button("Remove node", G_CALLBACK(remove_node), hbox10);
  stimulus_frame = gtk_frame_new("Stimulus settings");
  gtk_box_pack_start(GTK_BOX(vbox9), stimulus_frame, FALSE, FALSE, 0);
  vbox14 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox14);
  gtk_container_add(GTK_CONTAINER(stimulus_frame), vbox14);
  stimulus_settings_label = gtk_label_new("");
  gtk_widget_show(stimulus_settings_label);
  gtk_box_pack_start(GTK_BOX(vbox14), stimulus_settings_label, FALSE, FALSE, 0);
  hbox13 = bb_hbox();
  gtk_box_pack_start(GTK_BOX(vbox14), hbox13, FALSE, FALSE, 0);
  add_button("Connect stimulus to node", G_CALLBACK(stimulus_add_node), hbox13);
  module_frame = gtk_frame_new("Module settings");
  gtk_box_pack_start(GTK_BOX(vbox9), module_frame, TRUE, TRUE, 0);
  vbox10 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox10);
  gtk_container_add(GTK_CONTAINER(module_frame), vbox10);
  scrolledwindow1 = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_widget_show(scrolledwindow1);
  gtk_box_pack_start(GTK_BOX(vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  viewport6 = gtk_viewport_new(nullptr, nullptr);
  gtk_widget_show(viewport6);
  gtk_container_add(GTK_CONTAINER(scrolledwindow1), viewport6);
  renderer = gtk_cell_renderer_text_new();
  list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  attribute_clist = gtk_tree_view_new_with_model((GtkTreeModel*) list_store);
  gtk_tree_view_insert_column_with_attributes((GtkTreeView*) attribute_clist, 0,
      "Attributes", renderer,
      "text", 0, nullptr);
  g_object_set(attribute_clist, "headers-visible", FALSE, nullptr);
  gtk_widget_show(attribute_clist);
  gtk_container_add(GTK_CONTAINER(viewport6), attribute_clist);
  selection = gtk_tree_view_get_selection((GtkTreeView*) attribute_clist);
  g_signal_connect(selection,
                   "changed", (GCallback) settings_clist_cb,
                   this);
  hbox9 = bb_hbox();
  gtk_box_pack_start(GTK_BOX(vbox10), hbox9, FALSE, FALSE, 0);
  attribute_entry = gtk_entry_new();
  gtk_widget_show(attribute_entry);
  gtk_box_pack_start(GTK_BOX(hbox9), attribute_entry, FALSE, FALSE, 0);
  g_signal_connect(attribute_entry,
                   "activate",
                   G_CALLBACK(settings_set_cb),
                   this);
  add_button("Set", G_CALLBACK(settings_set_cb), hbox9);
  hbox14 = bb_hbox();
  gtk_box_pack_start(GTK_BOX(vbox10), hbox14, FALSE, FALSE, 0);
  add_button("Remove module", G_CALLBACK(remove_module), hbox14);
  add_button("Save Configuration ...", G_CALLBACK(save_stc), vbox9);
  scrolledwindow5 = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_widget_show(scrolledwindow5);
  gtk_paned_pack2(GTK_PANED(hpaned1), scrolledwindow5, TRUE, TRUE);
  vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolledwindow5));
  hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolledwindow5));
  layout = gtk_layout_new(hadj, vadj);
  gtk_container_add(GTK_CONTAINER(scrolledwindow5), layout);
  gtk_layout_set_size(GTK_LAYOUT(layout), LAYOUTSIZE_X, LAYOUTSIZE_Y);
  gtk_widget_add_events(layout,
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
  g_signal_connect(layout, "motion-notify-event",
                   G_CALLBACK(pointer_cb), this);
  g_signal_connect(layout, "button_press_event",
                   G_CALLBACK(pointer_cb), this);
  g_signal_connect(layout, "button_release_event",
                   G_CALLBACK(pointer_cb), this);
  g_signal_connect(layout, "expose_event",
                   G_CALLBACK(layout_expose), this);
  GtkAdjustment *xadj = gtk_layout_get_hadjustment(GTK_LAYOUT(layout));
  gtk_adjustment_set_step_increment(xadj, 10.0);
  GtkAdjustment *yadj = gtk_layout_get_vadjustment(GTK_LAYOUT(layout));
  gtk_adjustment_set_step_increment(yadj, 10.0);
  gtk_widget_set_app_paintable(layout, TRUE);
  gtk_widget_show(layout);
  unsigned int rrx, rry;
  gtk_layout_get_size(GTK_LAYOUT(layout), &rrx, &rry);
  xsize = ((width < LAYOUTSIZE_X) ? LAYOUTSIZE_X : width) / ROUTE_RES;
  ysize = ((height < LAYOUTSIZE_Y) ? LAYOUTSIZE_Y : height) / ROUTE_RES;
  board_matrix = new unsigned char[xsize * ysize];
  mask_matrix = new unsigned short[xsize * ysize];
  gtk_widget_realize(window);
  pinstatefont = pango_font_description_from_string("Courier Bold 8");
  pinnamefont = pango_font_description_from_string("Courier Bold 8");
  cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(window));
  PangoLayout *pang_layout = pango_cairo_create_layout(cr);
  pango_layout_set_font_description(pang_layout, pinnamefont);
  pango_layout_set_text(pang_layout, "9y", -1);
  pango_layout_get_size(pang_layout, &pinnameheight, nullptr);
  pinnameheight /= PANGO_SCALE;
  g_object_unref(pang_layout);
  cairo_destroy(cr);

  if (pinspacing < pinnameheight) {
    pinspacing = pinnameheight + 2;
  }

  if (pinspacing % ROUTE_RES) {
    pinspacing -= pinspacing % ROUTE_RES;
    pinspacing += ROUTE_RES;
  }

  auto gn = new gui_node;
  gn->bbw = this;
  gn->node = nullptr; // indicates that this is the root node.
  GtkTreeIter iter;
  gtk_tree_store_append(tree_store, &iter, nullptr);
  gtk_tree_store_set(tree_store, &iter,
                     0, "nodes",
                     1, gn,
                     -1);
  node_iter = &iter;
  // Handle nodes added before breadboard GUI enabled

  for (auto &list : nodes) {
    Breadboard_Window::NodeConfigurationChanged(list);
  }

  bIsBuilt = true;
  UpdateMenuItem();
  draw_nodes();

  gtk_widget_show(window);
  Breadboard_Window::Update();
}


Breadboard_Window::~Breadboard_Window()
{
  delete [] mask_matrix;
  delete [] board_matrix;
  mask_matrix = nullptr;
  board_matrix = nullptr;
}


Breadboard_Window::Breadboard_Window(GUI_Processor *_gp)
  : GUI_Object("pinout"),
    pinstatefont(nullptr), pinnamefont(nullptr), node_clist(nullptr),
    stimulus_settings_label(nullptr), stimulus_add_node_button(nullptr),
    hadj(nullptr), vadj(nullptr), node_iter(nullptr),
    selected_pin(nullptr), selected_node(nullptr), selected_module(nullptr)
{
  menu = "/menu/Windows/Breadboard";
  mask_matrix = nullptr;
  board_matrix = nullptr;
  gp = _gp;

  if (enabled) {
    Breadboard_Window::Build();
  }
}


#endif // HAVE_GUI
