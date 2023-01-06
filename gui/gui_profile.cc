/*
   Copyright (C) 1998,1999,2000,2001,2002,2003
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

// The code below is currently stubs which are unable to operate.
// It so far is unable to extract profiling data except for the
// routine profile tab

#include "../config.h"

#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include <math.h>

#include <cassert>
#include <cstdlib>
#include <string>

#include "../src/gpsim_time.h"
#include "../src/pic-instructions.h"
#include "../src/processor.h"
#include "../src/registers.h"
#include "../src/trace.h"
#include "../src/trigger.h"
#include "gui.h"
#include "gui_processor.h"
#include "gui_profile.h"

enum {
  PROFILE_ADDRESS,
  PROFILE_CYCLES,
  PROFILE_INSTRUCTION,
  PROFILE_COLUMNS
};

enum {
  PROFILE_REGISTER_ADDRESS,
  PROFILE_REGISTER_REGISTER,
  PROFILE_REGISTER_READ,
  PROFILE_REGISTER_WRITE,
  PROFILE_REGISTER_COLUMNS
};

enum {
  PROFILE_EXESTATS_FADDRESS,
  PROFILE_EXESTATS_TADDRESS,
  PROFILE_EXESTATS_EXECUTIONS,
  PROFILE_EXESTATS_MIN,
  PROFILE_EXESTATS_MAX,
  PROFILE_EXESTATS_MEDIAN,
  PROFILE_EXESTATS_AVERAGE,
  PROFILE_EXESTATS_STDDEV,
  PROFILE_EXESTATS_TOTAL,
  PROFILE_EXESTATS_COLUMNS
};

gint histogram_list_compare_func_cycles(gconstpointer a, gconstpointer b)
{
    const struct cycle_histogram_counter *h1 = (struct cycle_histogram_counter*)a;
    const struct cycle_histogram_counter *h2 = (struct cycle_histogram_counter*)b;

    if (h1->histo_cycles > h2->histo_cycles)
        return 1;
    if (h1->histo_cycles == h2->histo_cycles)
        return 0;
    return -1;
}

gint histogram_list_compare_func(gconstpointer a, gconstpointer b)
{
    const struct cycle_histogram_counter *h1 = (struct cycle_histogram_counter*)a;
    const struct cycle_histogram_counter *h2 = (struct cycle_histogram_counter*)b;

    if (h1->start_address > h2->start_address)
        return 1;
    if (h1->start_address == h2->start_address)
    {
        if (h1->stop_address > h2->stop_address)
            return 1;
        if (h1->stop_address == h2->stop_address)
        {
            if (h1->histo_cycles * h1->count > h2->histo_cycles * h2->count)
                return 1;
            if (h1->histo_cycles * h1->count == h2->histo_cycles * h2->count)
                return 0;
        }
    }
    return -1;
}

double calculate_median(GList *start, GList *stop)
{
    GList *sorted_list = nullptr;

    struct cycle_histogram_counter *chc_start, *chc_stop;//, *chc_result;
//    GList *result;
    int count_sum = 0;

    if (!start)
        return -4.2;

    if (!stop)
    {
        stop = start;
        while (stop->next)
            stop = stop->next;
    }

    // Copy list and sort it on cycles
    while (start != stop)
    {
        sorted_list = g_list_append(sorted_list, start->data);
        start = start->next;
    }
    sorted_list = g_list_append(sorted_list, start->data);

    sorted_list = g_list_sort(sorted_list, histogram_list_compare_func_cycles);

    start = sorted_list;
    stop = start;
    while (stop->next)
        stop = stop->next;

    chc_start = (struct cycle_histogram_counter*)start->data;
    chc_stop = (struct cycle_histogram_counter*)stop->data;

    while (start != stop)
    {
        if (count_sum >= 0)
        {
            // Move start to right
            start = start->next;
            count_sum -= chc_start->count;
            chc_start = (struct cycle_histogram_counter*)start->data;
            continue;
        }
        else
        {
            // Move stop to left
            stop = stop->prev;
            count_sum += chc_stop->count;
            chc_stop = (struct cycle_histogram_counter*)stop->data;
            continue;
        }
    }

    if (count_sum>(int)chc_start->count)
    {
        start = start->next;
        chc_start = (struct cycle_histogram_counter*)start->data;
        g_list_free(sorted_list);
        return (double)chc_start->histo_cycles;
    }
    if (-count_sum>(int)chc_start->count)
    {
        start = start->prev;
        chc_start = (struct cycle_histogram_counter*)start->data;
        g_list_free(sorted_list);
        return (double)chc_start->histo_cycles;
    }
    if (-count_sum == (int)chc_start->count)
    {
        stop = stop->prev;
        chc_stop = (struct cycle_histogram_counter*)stop->data;
        g_list_free(sorted_list);
        return (chc_start->histo_cycles + chc_stop->histo_cycles) / 2.0;
    }
    if (count_sum == (int)chc_start->count)
    {
        stop = stop->next;
        chc_stop = (struct cycle_histogram_counter*)stop->data;
        g_list_free(sorted_list);
        return (chc_start->histo_cycles + chc_stop->histo_cycles) / 2.0;
    }
    if ((unsigned int)abs(count_sum) < chc_start->count)
    {
        g_list_free(sorted_list);
        return (double)chc_start->histo_cycles;
    }

    assert(0);
    return 0.0;
}

double calculate_stddev(GList *start, GList *stop, double average)
{
    int count = 0;
    double sum = 0.0;
    struct cycle_histogram_counter *chc_start;

    if (start == stop)
        return 0.0;

    if (!stop)
    {
        stop = start;
        while (stop->next)
            stop = stop->next;
    }

    while (start != stop)
    {
        chc_start = (struct cycle_histogram_counter*)start->data;

        double diff = chc_start->histo_cycles - average;

        double diff2 = diff * diff;

        sum += diff2 * chc_start->count;

        count += chc_start->count;

        start = start->next;
    }

    double variance = sum / count;
    return sqrt(variance);
}


void Profile_Window::Update()
{
  if (!enabled)
    return;

  if (!gp || !gp->cpu)
  {
    return;
  }

  // Update profile list
  GtkTreeIter titer;
  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(profile_list), &titer)) {
    do {
      guint instruction_address;
      guint64 last_count;
      gtk_tree_model_get(GTK_TREE_MODEL(profile_list), &titer,
        PROFILE_ADDRESS, &instruction_address, PROFILE_CYCLES, &last_count, -1);

      guint64 count;
      count = gp->cpu->cycles_used(gp->cpu->map_pm_address2index(instruction_address));
      if (last_count != count) {
        gtk_list_store_set(profile_list, &titer,
          PROFILE_CYCLES, count, -1);
      }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(profile_list), &titer));
  }

  // Update register list

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(profile_register_list), &titer)) {
    do {
      guint register_address;
      guint64 last_count_read;
      guint64 last_count_write;
      gtk_tree_model_get(GTK_TREE_MODEL(profile_register_list), &titer,
        PROFILE_REGISTER_ADDRESS, &register_address,
        PROFILE_REGISTER_READ, &last_count_read,
        PROFILE_REGISTER_WRITE, &last_count_write,
        -1);

      Register *reg = gp->cpu->rma.get_register(register_address);
      guint64 count_read  = reg->read_access_count;
      guint64 count_write = reg->write_access_count;

      if (last_count_read != count_read || last_count_write != count_write) {

        gtk_list_store_set(profile_register_list, &titer,
          PROFILE_REGISTER_READ, count_read, -1);
        gtk_list_store_set(profile_register_list, &titer,
          PROFILE_REGISTER_WRITE, count_write, -1);
      }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(profile_register_list), &titer));
  }

  // Update cummulative statistics list
  histogram_profile_list = g_list_sort(histogram_profile_list,
                                           histogram_list_compare_func);
  // Remove all of list (for now)
  gtk_list_store_clear(profile_exestats_list);
  if (histogram_profile_list != 0)
  {
      struct cycle_histogram_counter *chc;
      int count_sum = 0;
      unsigned int start = 0xffffffff, stop = 0xffffffff;
      guint64 min = 0xffffffffffffffffULL, max = 0;
      guint64 cycles_sum = 0;
      GList *list_start = nullptr, *list_end = nullptr;

      GList *iter = histogram_profile_list;
      list_start = iter;
      while (iter)
      {
          chc = (struct cycle_histogram_counter*)iter->data;


          if (start == chc->start_address &&
             stop == chc->stop_address)
          {
              // Add data to statistics

              count_sum += chc->count;
              if (chc->histo_cycles < min)
                  min = chc->histo_cycles;
              if (chc->histo_cycles > max)
                  max = chc->histo_cycles;
              cycles_sum += chc->histo_cycles * chc->count;
          }
          else
          {
              if (count_sum != 0)
              {
                  // We have data, display it.
                  GtkTreeIter titer;
                  gtk_list_store_append(profile_exestats_list, &titer);
                  gtk_list_store_set(profile_exestats_list, &titer,
                    PROFILE_EXESTATS_FADDRESS, start,
                    PROFILE_EXESTATS_TADDRESS, stop,
                    PROFILE_EXESTATS_EXECUTIONS, count_sum,
                    PROFILE_EXESTATS_MIN, min,
                    PROFILE_EXESTATS_MAX, max,
                    PROFILE_EXESTATS_MEDIAN,
                      calculate_median(list_start , list_end),
                    PROFILE_EXESTATS_AVERAGE,
                      cycles_sum / (double)count_sum,
                    PROFILE_EXESTATS_STDDEV,
                      calculate_stddev(list_start , list_end , cycles_sum / (double)count_sum),
                    PROFILE_EXESTATS_TOTAL, cycles_sum,
                    -1);
              }

              // Start new calculation
              count_sum = chc->count;
              start = chc->start_address;
              stop = chc->stop_address;
              min = chc->histo_cycles;
              max = chc->histo_cycles;
              cycles_sum = chc->histo_cycles * chc->count;
              list_start = iter;

          }
          list_end = iter;
          iter = iter->next;
      }
      // add current to list

      GtkTreeIter titer;
      gtk_list_store_append(profile_exestats_list, &titer);
      gtk_list_store_set(profile_exestats_list, &titer,
        PROFILE_EXESTATS_FADDRESS, start,
        PROFILE_EXESTATS_TADDRESS, stop,
        PROFILE_EXESTATS_EXECUTIONS, count_sum,
        PROFILE_EXESTATS_MIN, min,
        PROFILE_EXESTATS_MAX, max,
        PROFILE_EXESTATS_MEDIAN, calculate_median(list_start , list_end),
        PROFILE_EXESTATS_AVERAGE, cycles_sum / (double)count_sum,
        PROFILE_EXESTATS_STDDEV,
          calculate_stddev(list_start, list_end, cycles_sum / (double)count_sum),
        PROFILE_EXESTATS_TOTAL, cycles_sum,
        -1);
  }
}

#define END_OF_TIME 0xFFFFFFFFFFFFFFFFULL
static guint64 startcycle = END_OF_TIME;
static unsigned int startaddress;
static guint64 stopcycle = END_OF_TIME;
static unsigned int stopaddress;

//------------------------------------------------------------------------
//
// ProfileStart class
//
class ProfileStart : public TriggerObject
{

public:
  ProfileStart(Profile_Window *_pw, int _address)
  {
    pw = _pw;
    address = _address;
  }

  void callback() override
  {
    if (!gpGuiProcessor || !gpGuiProcessor->cpu || !pw->gp->cpu)
      return;

    if (startcycle == END_OF_TIME) {
      startcycle   = get_cycles().get();
      startaddress = pw->gp->cpu->pma->get_PC();
    }
  }

private:
  Profile_Window *pw;
  int address;
};

//------------------------------------------------------------------------
//
// ProfileStop class
//
class ProfileStop : public TriggerObject
{
public:
  ProfileStop(Profile_Window *_pw, int _address)
  {
    pw = _pw;
    address = _address;
  }

  void callback() override
  {
    if (!gpGuiProcessor || !gpGuiProcessor->cpu || !pw->gp->cpu)
      return;

    if (stopcycle == END_OF_TIME && startcycle != END_OF_TIME) {

      stopcycle = get_cycles().get();
      if (startcycle == stopcycle) {
        // This was probably an attempt to measure the whole loop.
        // Set stopcycle to unset, and wait for the next one
        stopcycle = END_OF_TIME;

      } else {
        stopaddress = pw->gp->cpu->pma->get_PC();

        // We have a new measurement
        guint64 cycles_count = (int)stopcycle - (int)startcycle;

        // Search to see if there are an entry with this startaddress,
        // stopaddress and cycle count.
        GList *iter = pw->histogram_profile_list;
        while (iter != 0) {

          struct cycle_histogram_counter *chc;
          chc = (struct cycle_histogram_counter*)iter->data;
          if (chc->start_address == startaddress &&
             chc->stop_address == stopaddress &&
             chc->histo_cycles == cycles_count)
            {
              // If so then add 1 to the counter
              chc->count++;
              break;
            }
          iter = iter->next;
        }

        if (iter == 0) {

          // Else create a new struct, fill with values and add (sorted) to list
          struct cycle_histogram_counter *chc = new cycle_histogram_counter;
          chc->start_address = startaddress;
          chc->stop_address = stopaddress;
          chc->histo_cycles = cycles_count;
          chc->count = 1;

          pw->histogram_profile_list = g_list_append(pw->histogram_profile_list, chc);
        }

        startcycle=stopcycle = END_OF_TIME;
      }
    }
  }


private:
  Profile_Window *pw;
  int address;
};

/*****************************************************************
 * StartExe
 *
 * Create a 'profile start' object for the program memory.
 *
 */
void Profile_Window::StartExe(int address)
{
  if (!enabled)
    ChangeView(VIEW_SHOW);

  if (gp->cpu->pma->address_has_profile_start(address))
    gp->cpu->pma->clear_profile_start_at_address(address);
  else {

    if (gp->cpu->pma->address_has_profile_stop(address))
      // Can't have both start and stop at the same address
      // ..it becomes difficult to calculate the cycles
      gp->cpu->pma->clear_profile_stop_at_address(address);

    // FIXME -- memory leak...
    gp->cpu->pma->set_profile_start_at_address(address,
                                              new ProfileStart(this, address));
  }
}

/*****************************************************************
 * SopExe
 *
 * Create a 'profile stop' object for the program memory.
 *
 */
void Profile_Window::StopExe(int address)
{
  if (enabled)
    ChangeView(VIEW_SHOW);

  if (gp->cpu->pma->address_has_profile_stop(address))
    gp->cpu->pma->clear_profile_stop_at_address(address);
  else {

    if (gp->cpu->pma->address_has_profile_start(address))
      // Can't have both start and stop at the same address
      // ..it becomes difficult to calculate the cycles
      gp->cpu->pma->clear_profile_start_at_address(address);

    // FIXME -- memory leak...
    gp->cpu->pma->set_profile_stop_at_address(address,
                                              new ProfileStop(this, address));
  }
}

/*****************************************************************
 * ProfileWindow_new_program
 *
 *
 */

void Profile_Window::NewProgram(GUI_Processor *_gp)
{
  unsigned int uPMIndex;

  if (!_gp)
    return;

  gp = _gp;

  if (!gp->cpu)
    return;

  program = 1;

  if (!enabled)
    return;

  profile_keeper.enable_profiling();

  // Instruction list store
  Processor *pProcessor = gp->cpu;
  ProgramMemoryAccess *pPMA = pProcessor->pma;
  for (uPMIndex = 0; uPMIndex < pProcessor->program_memory_size(); uPMIndex++) {

    instruction * pInstruction = pProcessor->pma->getFromIndex(uPMIndex);
    guint uAddress = pProcessor->map_pm_index2address(uPMIndex);
    if (pPMA->hasValid_opcode_at_index(uPMIndex)) {
      guint64 cycles_used = pProcessor->cycles_used(uPMIndex);
      GtkTreeIter iter;
      gtk_list_store_append(profile_list, &iter);

      gtk_list_store_set(profile_list, &iter,
        PROFILE_ADDRESS, uAddress,
        PROFILE_CYCLES, cycles_used,
        PROFILE_INSTRUCTION, pInstruction->name().c_str(),
        -1);
    }
  }

  // Register list
  for (unsigned int i = 0; i < pProcessor->rma.get_size(); i++) {
    Register *reg = pProcessor->rma.get_register(i);

    //
    // If the register is valid, but it's not aliased and it's not a special function
    // register, then we can profile it.
    //

    if (reg && reg->isa() != Register::INVALID_REGISTER    // i.e. the register is valid
          &&
       !((reg->isa() == Register::SFR_REGISTER) || (i != reg->address)) ) {

        guint64 read_cycles = reg->read_access_count;
        guint64 write_cycles = reg->write_access_count;

        GtkTreeIter iter;
        gtk_list_store_append(profile_register_list, &iter);

        gtk_list_store_set(profile_register_list, &iter,
          PROFILE_REGISTER_ADDRESS, i,
          PROFILE_REGISTER_REGISTER, reg->name().c_str(),
          PROFILE_REGISTER_READ, read_cycles,
          PROFILE_REGISTER_WRITE, write_cycles,
          -1);
    }
  }
}

/*****************************************************************
 * ProfileWindow_new_processor
 *
 *
 */

void Profile_Window::NewProcessor(GUI_Processor *)
{
  if (!gp)
    return;

  if (!enabled)
    return;
}

// Call-back to format an address for the cell renderer
static void address_data_func(GtkTreeViewColumn *,
  GtkCellRenderer *renderer,
  GtkTreeModel *tree_model,
  GtkTreeIter *iter,
  gpointer data)
{
  guint addr;
  gchar buf[64];
  int position = GPOINTER_TO_INT(data);

  gtk_tree_model_get(tree_model, iter, position, &addr, -1);

  g_snprintf(buf, sizeof(buf), "0x%04x", addr);

  g_object_set(renderer, "text", buf, nullptr);
}

// Call-back to format the count for the cell renderer
static void cycles_data_func(GtkTreeViewColumn *,
  GtkCellRenderer *renderer,
  GtkTreeModel *tree_model,
  GtkTreeIter *iter,
  gpointer data)
{
  guint64 addr;
  gchar buf[64];
  int position = GPOINTER_TO_INT(data);

  gtk_tree_model_get(tree_model, iter, position, &addr, -1);

  g_snprintf(buf, sizeof(buf), "0x%" PRINTF_GINT64_MODIFIER "x", addr);

  g_object_set(renderer, "text", buf, nullptr);
}

// Call-back to formation floating point information
static void float_data_func(GtkTreeViewColumn *,
  GtkCellRenderer *renderer,
  GtkTreeModel *tree_model,
  GtkTreeIter *iter,
  gpointer data)
{
  gdouble floating_data;
  gchar buf[64];
  int position = GPOINTER_TO_INT(data);

  gtk_tree_model_get(tree_model, iter, position, &floating_data, -1);

  g_snprintf(buf, sizeof(buf), "%.1f", floating_data);

  g_object_set(renderer, "text", buf, nullptr);
}

void Profile_Window::Build()
{
  if (bIsBuilt)
    return;

  GtkWidget *label;
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 0);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "profile viewer");

  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);

  gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);


  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  // Instruction profile list
  profile_list = gtk_list_store_new(PROFILE_COLUMNS,
    G_TYPE_UINT, G_TYPE_UINT64, G_TYPE_STRING);

  profile_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Address",
    renderer, "text", PROFILE_ADDRESS, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    address_data_func,
    GINT_TO_POINTER(PROFILE_ADDRESS), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Cycles",
    renderer, "text", PROFILE_CYCLES, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    cycles_data_func,
    GINT_TO_POINTER(PROFILE_CYCLES), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Instruction",
    renderer, "text", PROFILE_INSTRUCTION, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_tree), column);

  scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_tree);

  gtk_widget_show(profile_tree);

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label = gtk_label_new("Instruction profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);
  ///////////////////////////////////////////////////

  // Register profile list
  profile_register_list = gtk_list_store_new(PROFILE_REGISTER_COLUMNS,
    G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64);

  profile_register_tree
    = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_register_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Address",
    renderer, "text", PROFILE_REGISTER_ADDRESS, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    address_data_func,
    GINT_TO_POINTER(PROFILE_REGISTER_ADDRESS), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Register",
    renderer, "text", PROFILE_REGISTER_REGISTER, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Read count",
    renderer, "text", PROFILE_REGISTER_READ, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    cycles_data_func,
    GINT_TO_POINTER(PROFILE_REGISTER_READ), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Write count",
    renderer, "text", PROFILE_REGISTER_WRITE, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    cycles_data_func,
    GINT_TO_POINTER(PROFILE_REGISTER_WRITE), nullptr);

  scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_register_tree);

  gtk_widget_show(profile_register_tree);

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label = gtk_label_new("Register profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_window, label);


  // Execution time statistics tab
  profile_exestats_list = gtk_list_store_new(PROFILE_EXESTATS_COLUMNS,
    G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_UINT64, G_TYPE_UINT64,
    G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_UINT64);

  profile_exestats_tree
    = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_exestats_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("From address",
    renderer, "text", PROFILE_EXESTATS_FADDRESS, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    address_data_func,
    GINT_TO_POINTER(PROFILE_EXESTATS_FADDRESS), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("To address",
    renderer, "text", PROFILE_EXESTATS_TADDRESS, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    address_data_func,
    GINT_TO_POINTER(PROFILE_EXESTATS_TADDRESS), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Executions",
    renderer, "text", PROFILE_EXESTATS_EXECUTIONS, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Min",
    renderer, "text", PROFILE_EXESTATS_MIN, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Max",
    renderer, "text", PROFILE_EXESTATS_MAX, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Median",
    renderer, "text", PROFILE_EXESTATS_MEDIAN, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    float_data_func,
    GINT_TO_POINTER(PROFILE_EXESTATS_MEDIAN), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Average",
    renderer, "text", PROFILE_EXESTATS_AVERAGE, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    float_data_func,
    GINT_TO_POINTER(PROFILE_EXESTATS_AVERAGE), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Std. Dev.",
    renderer, "text", PROFILE_EXESTATS_STDDEV, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);
  gtk_tree_view_column_set_cell_data_func(
    column, renderer,
    float_data_func,
    GINT_TO_POINTER(PROFILE_EXESTATS_STDDEV), nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Total",
    renderer, "text", PROFILE_EXESTATS_TOTAL, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_exestats_tree);

  gtk_widget_show(profile_exestats_tree);

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Routine profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_window,label);
  ///////////////////////////////////////////////////

  gtk_widget_show(window);

  bIsBuilt = true;

  NewProcessor(gp);

  if (program)
    NewProgram(gp);

  Update();

  UpdateMenuItem();
}

Profile_Window::Profile_Window(GUI_Processor *_gp)
  : GUI_Object("profile"),
    program(0), histogram_profile_list(nullptr)
{
  menu = "/menu/Windows/Profile";

  gp = _gp;

  if (enabled)
      Build();
}


#endif // HAVE_GUI
