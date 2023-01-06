/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg
   Copyright (C) 2011 Roy R Rankin

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

#include "gui.h"
#include "gui_src.h"
#include "preferences.h"
#include "../xpms/break.xpm"
#include "../xpms/pc.xpm"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>

#include "gtkextra/gtksheet.h"

#include <cstdio>
#include <string>

#define OPCODES_PER_ROW 16

//
// Notes:
// Changing the opcodes for a program dose not make sense for a high level
// language like C, JAL or even assembly source. Would be OK for a .HEX file.
// Doing this crashes within src/processor.cc so has been removed for now.
// Will reintroduce when both problems can be solved, as it can be usefull
// for some PICs which can write to their own program memory.
//
// Removed Add Watch as this is not implemented
// Removed "Set break on read" and "Set break on write" popup options/
// They are marked as not implemented and do not do the expected thing.
//
// Removed the different ASCII views. Missing out data is wrong and flipping
// data around is confusing.
// TODO: Fix the above.
// TODO: Use the systems (users theme) mono-space font? or one global setting


enum {
  ADDRESS_COLUMN,    // Address     - integer
  OPCODE_COLUMN,     // Opcode      - integer
  MNEMONIC_COLUMN,   // Mnemonic    - string
  PC_PIX_COLUMN,     // PC marker   - pixbuf
  BREAK_PIX_COLUMN,  // Break point - pixbuf
  NUM_COLUMNS
};

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_EXECUTE,
    MENU_ASM_BREAK_CLEAR,
    MENU_ASM_BREAK_EXECUTE,
    MENU_SETTINGS,
} menu_id;

typedef struct _menu_item {
    const char *name;
    const menu_id id;
} menu_item;

static const menu_item sheet_menu_items[] = {
    {"Set break points", MENU_BREAK_EXECUTE},
    {"Clear break points", MENU_BREAK_CLEAR},
    {"Settings...",MENU_SETTINGS}
};

static menu_item const list_menu_items[] = {
    {"Set break points", MENU_ASM_BREAK_EXECUTE},
    {"Clear break points", MENU_ASM_BREAK_CLEAR},
    {"Settings...",MENU_SETTINGS}
};

//========================================================================

class SourceOpcodeXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value) override
  {
    SourceBrowserOpcode_Window *sbow;

    sbow = static_cast<SourceBrowserOpcode_Window*>(parent_window);

    sbow->SetPC(new_value);
  }

  void Remove() override
  {
  }

};

//========================================================================

// update ascii column in sheet
void SourceBrowserOpcode_Window::update_ascii(gint row)
{
  gchar name[45];
  unsigned char byte;

  int i;
  for (i = 0; i < 32; i++) {
    if (i % 2)
      byte = memory[row * 16 + i / 2] & 0xff;
    else
      byte = (memory[row * 16 + i / 2] & 0xff00) >> 8;

    if (g_ascii_isprint(byte))
      name[i] = byte;
    else
      name[i] = '.';
  }
  name[i] = '\0';

  gtk_sheet_set_cell(GTK_SHEET(sheet), row, OPCODES_PER_ROW, GTK_JUSTIFY_RIGHT,
    name);
}

// called when user has selected a menu item
void
SourceBrowserOpcode_Window::popup_activated(GtkWidget *widget,
  SourceBrowserOpcode_Window *sbow)
{
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  int i,j;

  if(!sbow->gp || !sbow->gp->cpu) {
    return;
  }

  GtkSheet *sheet = GTK_SHEET(sbow->sheet);
  GtkSheetRange range = sheet->range;

  gsize data = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(widget), "item"));
  switch(data)
    {
    case MENU_BREAK_EXECUTE:

      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++) {
          unsigned address = sbow->gp->cpu->map_pm_index2address(j*16+i);
          if (!sbow->gp->cpu->pma->address_has_break(address))
            sbow->gp->cpu->pma->set_break_at_address(address);
        }
      break;
    case MENU_BREAK_CLEAR:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++) {
          unsigned address = sbow->gp->cpu->map_pm_index2address(j*16+i);
          sbow->gp->cpu->pma->clear_break_at_address(address,
            instruction::BREAKPOINT_INSTRUCTION);
        }
      break;
    case MENU_ASM_BREAK_CLEAR:
      sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(sbow->tree));

      if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gint break_row;
        gtk_tree_model_get(model, &iter, gint(ADDRESS_COLUMN), &break_row, -1);

        unsigned address = sbow->gp->cpu->map_pm_index2address(break_row);
        sbow->gp->cpu->pma->clear_break_at_address(address,
            instruction::BREAKPOINT_INSTRUCTION);
      }
      break;
    case MENU_ASM_BREAK_EXECUTE:
      sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(sbow->tree));

      if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gint break_row;
        gtk_tree_model_get(model, &iter, gint(ADDRESS_COLUMN), &break_row, -1);

        unsigned address = sbow->gp->cpu->map_pm_index2address(break_row);
        if (!sbow->gp->cpu->pma->address_has_break(address))
          sbow->gp->cpu->pma->set_break_at_address(address);
      }
      break;
    case MENU_SETTINGS:
      sbow->settings_dialog();
      break;
    default:
      break;
    }
}


GtkWidget *
SourceBrowserOpcode_Window::build_menu_for_sheet()
{
  GtkWidget *menu = gtk_menu_new();

  for (gsize i = 0; i < G_N_ELEMENTS(sheet_menu_items); i++) {
    GtkWidget *item = gtk_menu_item_new_with_label(sheet_menu_items[i].name);

    g_signal_connect(item,"activate", G_CALLBACK (popup_activated), this);

    g_object_set_data(G_OBJECT(item), "item",
      GSIZE_TO_POINTER(sheet_menu_items[i].id));

    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
  }

  return menu;
}

GtkWidget *
SourceBrowserOpcode_Window::build_menu_for_list()
{
  GtkWidget *menu = gtk_menu_new();

  for (gsize i = 0; i < G_N_ELEMENTS(list_menu_items); i++) {
    GtkWidget *item = gtk_menu_item_new_with_label(list_menu_items[i].name);

    g_signal_connect(item, "activate", G_CALLBACK(popup_activated), this);

    g_object_set_data(G_OBJECT(item), "item",
      GSIZE_TO_POINTER(list_menu_items[i].id));

    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
  }

  return menu;
}

void SourceBrowserOpcode_Window::do_popup_menu(GtkWidget *widget,
  GdkEventButton *event)
{
  int button, event_time;

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
  }

    if (GTK_IS_TREE_VIEW(GTK_OBJECT(widget))) {
      gtk_menu_popup(GTK_MENU(list_popup_menu), nullptr, nullptr, nullptr, nullptr,
        button, event_time);
    } else {
      gtk_menu_popup(GTK_MENU(sheet_popup_menu), nullptr, nullptr, nullptr, nullptr,
        button, event_time);
    }
}

// button press signal handler (static)

gint SourceBrowserOpcode_Window::button_press(GtkWidget *widget,
  GdkEventButton *event, SourceBrowserOpcode_Window *sbow)
{
  if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
    sbow->do_popup_menu(widget, event);
    return TRUE;
  }

  return FALSE;
}

// Popup menu keyboard signal handler (static)

gboolean
SourceBrowserOpcode_Window::popup_menu_handler(GtkWidget *widget,
  SourceBrowserOpcode_Window *sbw)
{
  sbw->do_popup_menu(widget, nullptr);
  return TRUE;
}

// Row selection handle (static)

void SourceBrowserOpcode_Window::row_selected(GtkTreeView *tree_view,
  GtkTreePath *path, GtkTreeViewColumn *,
  SourceBrowserOpcode_Window *sbow)
{
  GtkTreeIter iter;

  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gint break_row;
    gtk_tree_model_get(model, &iter, gint(ADDRESS_COLUMN), &break_row, -1);

    unsigned address = sbow->gp->cpu->map_pm_index2address(break_row);
    sbow->gp->cpu->pma->toggle_break_at_address(address);
  }
}

static void filter(std::string &clean, const char *dirty)
{
  if (!dirty)
    return;

  for (int i = 0 ; *dirty; ++dirty, ++i) {
    if (*dirty == '\t') {
      for (int j = 0; j < 8 && i % 8; ++j, ++i)
        clean += ' ';
    } else if (g_ascii_isprint(*dirty)) {
      clean += *dirty;
    }
  }
}

void SourceBrowserOpcode_Window::update_styles(int address)
{
  GtkSheetRange range;

  int index = address;

  if (gp->cpu)
    index = gp->cpu->map_pm_address2index(address);

  int row = index / 16;
  int column = index % 16;

  range.row0 = row;
  range.rowi = row;
  range.col0 = column;
  range.coli = column;

  if (!gp->cpu) {
    gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, gColors.normal_bg());
    return;
  }

  GtkTreeIter iter;
  GdkPixbuf *pix = nullptr;

  if (gp->cpu && gp->cpu->pma->address_has_break(address)) {
    pix = break_pix;

    gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, gColors.breakpoint());

  } else {
    if (gp->cpu->pma->isModified(address))
      gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, gColors.sfr_bg());
    else
      gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, gColors.normal_bg());
  }

  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, nullptr, index)) {
    gtk_list_store_set(list, &iter, gint(BREAK_PIX_COLUMN), pix, -1);
  }
}


void SourceBrowserOpcode_Window::update_label(int address)
{
  std::string labeltext;
  char entrytext[128];
  GtkEntry *sheet_entry;

  if (!gp || !gp->cpu)
    return;

  if (address < 0) {
    entrytext[0] = '\0';
    labeltext = "ASCII";
  } else {
    unsigned int oc = gp->cpu->pma->get_opcode(address);

    filter(labeltext,
           gp->cpu->pma->get_opcode_name(address,entrytext,sizeof(entrytext)));

    g_snprintf(entrytext, sizeof(entrytext), "0x%04X", oc);
  }

  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(GTK_SHEET(sheet)));
  gtk_label_set_text(GTK_LABEL(label), labeltext.c_str());
  gtk_entry_set_max_length(GTK_ENTRY(entry),
    gtk_entry_buffer_get_max_length(gtk_entry_get_buffer(sheet_entry)));

  gtk_entry_set_text(GTK_ENTRY(entry), entrytext);
}

void SourceBrowserOpcode_Window::update_values(int address)
{
  if (!gp || !gp->cpu || !memory)
    return;

  unsigned uMemoryIndex = gp->cpu->map_pm_address2index(address);
  int row = uMemoryIndex / 16;
  int column = uMemoryIndex % 16;

  unsigned int oc = gp->cpu->pma->get_opcode(address);

  if (oc != memory[uMemoryIndex]) {
    memory[address] = oc;
    // Put new values, in case they changed
    char oc_buf[128];
    std::string mn_buf;
    char buf[128];

    g_snprintf(oc_buf, sizeof(oc_buf), "%04X", oc);
    filter(mn_buf,
           gp->cpu->pma->get_opcode_name(address, buf, sizeof(buf)));

    GtkTreeIter iter;

    gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list),
      &iter, nullptr, address);
    gtk_list_store_set(list, &iter,
      OPCODE_COLUMN, oc,
      MNEMONIC_COLUMN, mn_buf.c_str(),
      -1);

    gtk_sheet_set_cell(GTK_SHEET(sheet),
                       row, column,
                       GTK_JUSTIFY_RIGHT, oc_buf);
  }
}

void SourceBrowserOpcode_Window::update(int address)
{
  if (!gp->cpu)
    return;

  update_values(address);
  update_styles(address);
}

void SourceBrowserOpcode_Window::load_styles()
{
  PangoFontDescription *font =
    pango_font_description_from_string(normalfont_string.c_str());
  gtk_widget_modify_font(tree, font);

  pango_font_description_free(normalPFD);
  normalPFD = pango_font_description_copy(font);
}

/********************** Settings dialog ***************************/

void SourceBrowserOpcode_Window::settings_dialog()
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
    "Opcode browser settings",
    GTK_WINDOW(window),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    nullptr
    );

  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *table = gtk_table_new(3, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 6);
  gtk_container_add(GTK_CONTAINER(content_area), table);
  gtk_container_set_border_width(GTK_CONTAINER(table), 18);

  // Normal font
  GtkWidget *label = gtk_label_new("Normal font");
  GtkWidget *normal_font_btn = gtk_font_button_new_with_font(normalfont_string.c_str());
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), normal_font_btn, 1, 2, 0, 1);

  gtk_widget_show_all(dialog);

  int retval = gtk_dialog_run(GTK_DIALOG(dialog));
  if (retval == GTK_RESPONSE_OK) {
    const char *fontname =
      gtk_font_button_get_font_name(GTK_FONT_BUTTON(normal_font_btn));
    normalfont_string = fontname;
    config_set_string(name(), "normalfont", fontname);
    load_styles();
    Fill();
  }

  gtk_widget_destroy(dialog);
}

//
// Signal handler
// we get here when the entry in a cell is changed (typed a digit), we
// copy it to the entry above the sheet.
//
void SourceBrowserOpcode_Window::show_entry(GtkWidget *widget,
  SourceBrowserOpcode_Window *sbow)
{
  if (!gtk_widget_has_focus(widget)) return;

  GtkSheet *sheet = GTK_SHEET(sbow->sheet);
  GtkWidget *sheet_entry = gtk_sheet_get_entry(sheet);
  const char *text = gtk_entry_get_text(GTK_ENTRY(sheet_entry));

  if (text)
    gtk_entry_set_text(GTK_ENTRY(sbow->entry), text);
}

// Signal handler - static
// when a cell is activated, we set the label and entry above the sheet
//
gint SourceBrowserOpcode_Window::activate_sheet_cell(GtkWidget *,
  gint row, gint column, SourceBrowserOpcode_Window *sbow)
{
  GtkSheetCellAttr attributes;

  if (!sbow->gp || ! sbow->gp->cpu)
    return 0;

  GtkSheet *sheet = GTK_SHEET(sbow->sheet);

  if (row>sheet->maxrow || row < 0 || column>sheet->maxcol || column < 0)
  {
    printf("Warning activate_sheet_cell(%x,%x)\n", row, column);
    return 0;
  }

  if (column < 16)
  {
    sbow->update_label(sbow->gp->cpu->map_pm_index2address(row * 16 + column));
  }
  else
    sbow->update_label(-1);

  gtk_sheet_get_attributes(sheet,sheet->active_cell.row,
                           sheet->active_cell.col, &attributes);
  //gtk_editable_set_editable(GTK_EDITABLE(sbow->entry), attributes.is_editable);
  gtk_sheet_range_set_justification(sheet, &sheet->range, GTK_JUSTIFY_RIGHT);

  return TRUE;
}

void SourceBrowserOpcode_Window::SelectAddress(int address)
{
  if (!enabled)
    return;
  unsigned int row = address;
  if (gp->cpu)
    row = gp->cpu->map_pm_address2index(address);

  GtkTreeIter iter;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  gtk_tree_selection_unselect_all(sel);
  gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, nullptr, row);
  gtk_tree_selection_select_iter(sel, &iter);

  GtkTreePath *path
    = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);

  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree),
    path, nullptr, FALSE, 0.5, 0.0);
  gtk_tree_path_free(path);
}

void SourceBrowserOpcode_Window::UpdateLine(int address)
{
  if (!enabled)
    return;

  if (address >= 0)
    update(address);
}

void SourceBrowserOpcode_Window::SetPC(int address)
{
  gint last_address = current_address;

  if (!enabled)
    return;

  current_address = address;

  if (address != last_address) {
      UpdateLine(last_address);

    int index = last_address;
    if (gp->cpu)
      index = gp->cpu->map_pm_address2index(last_address);

    GtkTreeIter iter;
    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, nullptr, index)) {
      gtk_list_store_set(list, &iter, gint(PC_PIX_COLUMN), nullptr, -1);
    }
  }

  UpdateLine(address);

  int index = address;
  if (gp->cpu)
    index = gp->cpu->map_pm_address2index(address);

  GtkTreeIter iter;
  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, nullptr, index)) {
    gtk_list_store_set(list, &iter, gint(PC_PIX_COLUMN), pc_pix, -1);

    GtkTreePath *path
      = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);

    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree),
      path, nullptr, FALSE, 0.5, 0.0);
    gtk_tree_path_free(path);
  }

  GtkSheetRange range;

  range.row0 = range.rowi = index / 16;
  range.col0 = range.coli = index % 16;

  gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, gColors.sfr_bg());
}

//========================================================================
// Fill()
//
// copy the processor's program memory contents to the both the disassembly
// and opcode windows.

void SourceBrowserOpcode_Window::Fill()
{
  GtkSheetRange range;

  if (!bIsBuilt)
    Build();

  if (!gp || !gp->cpu)
    return;

  char buf[128];
  gint i;
  int pm_size;
  int pc;
  gint column_width, char_width;

  // Clearing list
  gtk_list_store_clear(list);

  pm_size = gp->cpu->program_memory_size();

  delete[] memory;
  memory = new unsigned int[pm_size];

  gchar name_buf[10];
  gtk_sheet_freeze(GTK_SHEET(sheet));

  PangoRectangle rect;
  PangoLayout *layout;

  layout = gtk_widget_create_pango_layout(GTK_WIDGET(sheet), "A");
  pango_layout_set_font_description(layout, normalPFD);

  pango_layout_get_extents(layout, nullptr, &rect);

  char_width = PANGO_PIXELS(rect.width);
  column_width = 4 * char_width + 5;
  g_object_unref(layout);

  for (i = 0; i < GTK_SHEET(sheet)->maxcol; i++) {
    g_snprintf(name_buf, sizeof(name_buf), "%02x", gp->cpu->map_pm_index2address(i));
    gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, name_buf);
    gtk_sheet_set_column_title(GTK_SHEET(sheet), i, name_buf);
    gtk_sheet_set_column_width(GTK_SHEET(sheet), i, column_width);
  }

  for (i = 0; i < pm_size; i++) {
    int address = gp->cpu->map_pm_index2address(i);
    unsigned int opcode = gp->cpu->pma->get_opcode(address);
    memory[i] = opcode;

    char oc_buf[128];
    std::string mn_buf;
    g_snprintf(oc_buf, sizeof(oc_buf), "%04X", opcode);
    filter(mn_buf,
           gp->cpu->pma->get_opcode_name(address, buf, sizeof(buf)));

    if (GTK_SHEET(sheet)->maxrow < i / 16)
    {
        int j = i / 16;
        gtk_sheet_add_row(GTK_SHEET(sheet), 1);

        g_snprintf(name_buf, sizeof(name_buf), "%04x", gp->cpu->map_pm_index2address(i));
        gtk_sheet_row_button_add_label(GTK_SHEET(sheet), j, name_buf);
        gtk_sheet_set_row_title(GTK_SHEET(sheet), j, name_buf);
    }

    gtk_sheet_set_cell(GTK_SHEET(sheet),
                       i / 16,
                       i % 16,
                       GTK_JUSTIFY_RIGHT, oc_buf);

    GtkTreeIter iter;
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
      gint(ADDRESS_COLUMN), address,
      gint(OPCODE_COLUMN), opcode,
      gint(MNEMONIC_COLUMN), mn_buf.c_str(),
      -1);

    update_styles(address);
  }

  for (i = 0; i < pm_size / 16; i++)
    update_ascii(i);

  range.row0 = 0;
  range.col0 = 0;
  range.rowi = GTK_SHEET(sheet)->maxrow;
  range.coli = GTK_SHEET(sheet)->maxcol;
  gtk_sheet_range_set_font(GTK_SHEET(sheet), &range, normalPFD);

  gtk_sheet_thaw(GTK_SHEET(sheet));

  pc=gp->cpu->pma->get_PC();
  SetPC(pc);
  update_label(pc);
}

void SourceBrowserOpcode_Window::NewSource(GUI_Processor *)
{
  if (!gp)
    return;

  current_address = 0;

  if (!enabled)
    return;
  if (!bIsBuilt)
    Build();

  /* Now create a cross-reference link that the
   * simulator can use to send information back to the gui
   */
  if (gp->cpu && gp->cpu->pc) {
    SourceOpcodeXREF *cross_reference;

    cross_reference = new SourceOpcodeXREF();
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpsimObject *) this;

    gp->cpu->pc->add_xref((gpointer) cross_reference);
  }

  Fill();
}

void SourceBrowserOpcode_Window::NewProcessor(GUI_Processor *)
{
  GtkSheetRange range;

  if (!gp || !gp->cpu)
    return;

  current_address = 0;

  if (!enabled)
    return;

  if (!bIsBuilt)
    Build();

  pma = gp->cpu->pma;

  range.row0 = 0;
  range.col0 = 0;
  range.rowi = GTK_SHEET(sheet)->maxrow;
  range.coli = GTK_SHEET(sheet)->maxcol;
  gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, gColors.normal_bg());

  range.row0 = range.rowi = 0;
  range.col0 = range.coli = 0;
  gtk_sheet_select_range(GTK_SHEET(sheet), &range);

  update_label(0);
}

void
SourceBrowserOpcode_Window::cell_renderer(GtkTreeViewColumn *,
  GtkCellRenderer *cell, GtkTreeModel *tree_model,
  GtkTreeIter *iter, gpointer)
{
  gint addr;
  gchar buf[64];

  gtk_tree_model_get(tree_model, iter,
    GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "col")), &addr, -1);

  g_snprintf(buf, sizeof(buf), "0x%04x", addr);

  g_object_set(cell, "text", buf, nullptr);
}

void SourceBrowserOpcode_Window::Build()
{
  if (bIsBuilt)
    return;

  GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkRequisition request;

  gchar _name[10];
  gint column_width,char_width;
  gint i;

  SourceBrowser_Window::Create();

  gtk_window_set_title(GTK_WINDOW(window), "Program memory");

  notebook = gtk_notebook_new();

  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

  //
  // create list
  //
  scrolled_win = gtk_scrolled_window_new(0, 0);
  gtk_container_set_border_width(GTK_CONTAINER(scrolled_win), 6);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  list = gtk_list_store_new(NUM_COLUMNS,
    G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF);

  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
  // TODO: Add selecting multiple lines of code
  //GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  //gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  column = gtk_tree_view_column_new();
  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column, renderer,
    "pixbuf", BREAK_PIX_COLUMN, nullptr);

  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column, renderer,
    "pixbuf", PC_PIX_COLUMN, nullptr);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("address",
    renderer, "text", ADDRESS_COLUMN, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  g_object_set_data(G_OBJECT(renderer),
    "col", GINT_TO_POINTER(gint(ADDRESS_COLUMN)));

  gtk_tree_view_column_set_cell_data_func(column, renderer,
    cell_renderer, nullptr, nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("opcode",
    renderer, "text", OPCODE_COLUMN, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set_data(G_OBJECT(renderer),
    "col", GINT_TO_POINTER(gint(OPCODE_COLUMN)));

  gtk_tree_view_column_set_cell_data_func(column, renderer,
    cell_renderer, nullptr, nullptr);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("instruction",
    renderer, "text", MNEMONIC_COLUMN, nullptr);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  /**************************** load fonts *********************************/
  char *fontstring;
  GtkStyle *style  = gtk_widget_get_default_style();

  normalfont_string = pango_font_description_to_string(style->font_desc);

  if (config_get_string(name(), "normalfont", &fontstring))
    normalfont_string = fontstring;

  load_styles();

  gtk_container_add(GTK_CONTAINER(scrolled_win), tree);

  /* Add a signal handler for button press events. This will capture
   * commands for setting and/or clearing break points
   */
  g_signal_connect(tree, "button_press_event",
                     G_CALLBACK (button_press),
                     (gpointer) this);
  g_signal_connect(tree, "popup-menu", G_CALLBACK(popup_menu_handler), this);

  g_signal_connect(tree, "row-activated",
    G_CALLBACK(row_selected), gpointer(this));

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           scrolled_win,
                           gtk_label_new("Assembly"));

  //
  // create sheet
  //
  vbox = gtk_vbox_new(FALSE, 1);

  // Create entry bar
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new(nullptr);

  gtk_widget_modify_font(label, normalPFD);

  gtk_widget_size_request(label, &request);
  gtk_widget_set_size_request(label, 160, request.height);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

  entry = gtk_entry_new();

  gtk_widget_modify_font(entry, normalPFD);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

  // Create sheet
  scrolled_win = gtk_scrolled_window_new(0, 0);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

  GtkSheetRange sheet_range = {0, 0, 16, 0};

  sheet = gtk_sheet_new(1, 17, "where does this string go?");
  gtk_sheet_range_set_editable(GTK_SHEET(sheet), &sheet_range, FALSE);
  gtk_container_add(GTK_CONTAINER(scrolled_win), sheet);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           vbox,
                           gtk_label_new("Opcodes"));

  PangoRectangle rect;
  PangoLayout *layout;

  layout = gtk_widget_create_pango_layout(GTK_WIDGET(sheet), "A");
  pango_layout_set_font_description(layout, normalPFD);

  pango_layout_get_extents(layout, nullptr, &rect);

  char_width = PANGO_PIXELS(rect.width);
  column_width = 4 * char_width + 5;
  g_object_unref(layout);

  for (i = 0; i < GTK_SHEET(sheet)->maxcol; i++) {
    g_snprintf(_name, sizeof(_name), "%02x", i);
    gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, _name);
    gtk_sheet_set_column_title(GTK_SHEET(sheet), i, _name);
    gtk_sheet_set_column_width (GTK_SHEET(sheet), i, column_width);
  }

  const char * ascii_name = "ASCII";
  gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, ascii_name);
  gtk_sheet_set_column_title(GTK_SHEET(sheet), i, ascii_name);
  gtk_sheet_set_row_titles_width(GTK_SHEET(sheet), column_width);
  gtk_sheet_set_column_width(GTK_SHEET(sheet), i, 32 * char_width + 5);


  g_signal_connect(sheet, "button_press_event", G_CALLBACK(button_press), this);
  g_signal_connect(sheet, "popup-menu", G_CALLBACK(popup_menu_handler), this);

  g_signal_connect(gtk_sheet_get_entry(GTK_SHEET(sheet)),
                     "changed", G_CALLBACK (show_entry), this);

  g_signal_connect(sheet,
                     "activate", G_CALLBACK (activate_sheet_cell),
                     (gpointer) this);


  /////////////////////////////////////////////////////////////////

  gtk_widget_show_all(window);

  bIsBuilt = true;

  NewProcessor(gp);
  NewSource(gp);

  // create popupmenu for sheet
  sheet_popup_menu = build_menu_for_sheet();

  // create popupmenu for clist
  list_popup_menu = build_menu_for_list();

  UpdateMenuItem();
}

SourceBrowserOpcode_Window::SourceBrowserOpcode_Window(GUI_Processor *_gp)
  : SourceBrowser_Window("program_memory"),
    current_address(0), normalPFD(nullptr), memory(nullptr)
{
  menu = "/menu/Windows/Program memory";

  pma = nullptr;

  gp = _gp;

  break_pix = gdk_pixbuf_new_from_xpm_data(break_xpm);
  pc_pix = gdk_pixbuf_new_from_xpm_data(pc_xpm);

  if (enabled)
    Build();
}

SourceBrowserOpcode_Window::~SourceBrowserOpcode_Window()
{
  pango_font_description_free(normalPFD);
}

#endif // HAVE_GUI
