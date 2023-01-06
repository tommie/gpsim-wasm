/*
   Copyright (C) 1998,1999,2000,2001
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

#include "../config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>

#include "gui.h"
#include "gui_processor.h"
#include "gui_statusbar.h"
#include "../src/processor.h"
#include "../src/registers.h"

#include <cstdlib>
#include <list>
#include <string>

class gpsimObject;

//========================================================================

class StatusBarXREF : public CrossReferenceToGUI {
public:
  void Update(int /* new_value */) override
  {
    StatusBar_Window *sbw;
    sbw  = static_cast<StatusBar_Window *>(parent_window);
    sbw->Update();
  }
};


//------------------------------------------------------------------------
static void LabeledEntry_callback(GtkWidget *, LabeledEntry *le)
{
  char *bad_position;

  if (!gpGuiProcessor || !gpGuiProcessor->cpu || !le || !le->entry) {
    return;
  }

  const char *text = gtk_entry_get_text(GTK_ENTRY(le->entry));
  unsigned long value = strtoul(text, &bad_position, 16);

  if (*bad_position != '\0') {
    return;  /* string contains an invalid number */
  }

  le->put_value(value);
}


//========================================================================
EntryWidget::EntryWidget()
{
  entry = gtk_entry_new();
  gtk_widget_show(entry);
}


EntryWidget::~EntryWidget()
{
}


void EntryWidget::SetEntryWidth(int string_width)
{
  gtk_entry_set_width_chars(GTK_ENTRY(entry), string_width);
}


void EntryWidget::set_editable(bool Editable)
{
  gtk_editable_set_editable(GTK_EDITABLE(entry), Editable ? TRUE : FALSE);
}


//========================================================================


LabeledEntry::LabeledEntry(GtkWidget *box, const char *clabel)
{
  label = gtk_label_new(clabel);
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
}


void LabeledEntry::Update()
{
}


void LabeledEntry::put_value(unsigned int)
{
}


//------------------------------------------------------------------------
RegisterLabeledEntry::RegisterLabeledEntry(GtkWidget *box,
    Register *new_reg,
    bool isEditable = false)
  : LabeledEntry(box, new_reg->name().c_str()), reg(new_reg)
{
  g_snprintf(pCellFormat, sizeof(pCellFormat), "0x%%0%dx", reg->register_size() * 2);
  SetEntryWidth(2 + reg->register_size() * 2);
  Update();
  set_editable(isEditable);
  g_signal_connect(entry, "activate", G_CALLBACK(LabeledEntry_callback), this);
}


RegisterLabeledEntry::~RegisterLabeledEntry()
{
}


void RegisterLabeledEntry::put_value(unsigned int new_value)
{
  reg->put_value(new_value);
}


void RegisterLabeledEntry::Update()
{
  char buffer[32];
  unsigned int value = reg->get_value();
  g_snprintf(buffer, sizeof(buffer), pCellFormat, value);
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
}


//------------------------------------------------------------------------
void StatusBar_Window::Update()
{
  //update the displayed values
  if (!gp || !gp->cpu) {
    return;
  }

  for (auto &RLE : entries) {
    RLE->Update();
  }
}


/*  NewProcessor
 *
 */

void StatusBar_Window::NewProcessor(GUI_Processor *_gp, MemoryAccess *_ma)
{
  if (!_gp  || !_gp->cpu || !_ma) {
    return;
  }

  if (ma) {
    return;
  }

  gp = _gp;
  ma = _ma;
  Dprintf((" %s\n", __FUNCTION__));
  std::list<Register *>::iterator iReg;

  for (iReg = ma->SpecialRegisters.begin();
       iReg != ma->SpecialRegisters.end();
       ++iReg) {
    entries.push_back(new RegisterLabeledEntry(hbox, *iReg));
  }

  /* Now create a cross-reference link that the simulator can use to
   * send information back to the gui
   */
  ProgramMemoryAccess* pPMA = dynamic_cast<ProgramMemoryAccess*>(ma);

  if (gp->cpu && gp->cpu->pc) {
    Program_Counter *pPC = pPMA ? pPMA->GetProgramCounter() : gp->cpu->pc;
    StatusBarXREF *cross_reference;
    cross_reference = new StatusBarXREF();
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpsimObject *) this;
    pPC->add_xref((gpointer) cross_reference);
  }

  Update();
}


StatusBar_Window::StatusBar_Window(GtkWidget *vbox_main)
  : gp(nullptr), ma(nullptr)
{
  /* --- Create h-box for holding the status line --- */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_end(GTK_BOX(vbox_main), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
}


#endif // HAVE_GUI
