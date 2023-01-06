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

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include <cstring>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "gui.h"
#include "gui_breadboard.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_regwin.h"
#include "gui_scope.h"
#include "gui_src.h"
#include "gui_stack.h"
#include "gui_stopwatch.h"
#include "gui_symbols.h"
#include "gui_trace.h"
#include "gui_watch.h"

#include "../cli/input.h"  // for gpsim_open()
#include "../src/exports.h"
#include "../src/gpsim_classes.h"
#include "../src/gpsim_interface.h"
#include "../src/gpsim_time.h"
#include "../src/processor.h"

GtkUIManager *ui;

extern GUI_Processor *gpGuiProcessor;

static void
do_quit_app(GtkWidget *)
{
  exit_gpsim(0);
}


//========================================================================
static const gchar * const authors[] = {
  "Scott Dattalo - <scott@dattalo.com>",
  "Ralf Forsberg - <rfg@home.se>",
  "Borut Ra" "\xc5\xbe" "em - <borut.razem@gmail.com>",
  nullptr
};

static void
about_cb(GtkAction *, gpointer)
{
  gtk_show_about_dialog(nullptr,
    "authors", authors,
    "comments", "A simulator for Microchip PIC microcontrollers.",
    "version", VERSION,
    "website", "http://gpsim.sourceforge.net/gpsim.html",
    "program-name", "The GNUPIC Simulator",
    nullptr);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//========================================================================
//
// class ColorButton
//
// Creates a GtkColorButton and places it into a parent widget.
// When the color button is clicked and changed, the signal will
// call back into this class and keep track of the selected
// color state.
class SourceBrowserPreferences;
class ColorButton
{
public:
  ColorButton (GtkWidget *pParent,
               GtkTextTag *pStyle,
               const char *label,
               SourceBrowserPreferences *
               );
  ~ColorButton();

  void cancel();
private:
  static void setColor_cb(GtkColorButton *widget,
                          ColorButton    *This);

  GtkTextTag *m_pStyle;
  GdkColor *old_color;
};

//========================================================================
//
// class MarginButton
//
// Creates a GtkCheckButton that is used to select whether line numbers,
// addresses or opcodes will be displayed in the source browser margin.

class MarginButton
{
public:
  enum eMarginType {
    eLineNumbers,
    eAddresses,
    eOpcodes
  };
  MarginButton (GtkWidget *pParent,
                const char *pName,
                eMarginType id,
                SourceBrowserPreferences *
               );
  static void toggle_cb(GtkToggleButton *widget,
                        MarginButton    *This);
  void set_active();
private:
  GtkWidget *m_button;
  SourceBrowserPreferences *m_prefs;
  eMarginType m_id;
};

//========================================================================
//
// class TabButton
//
// Creates a GtkCheckButton that is used to select whether line numbers,
// addresses or opcodes will be displayed in the source browser margin.

class TabButton
{
public:
  TabButton (GtkWidget *pParent, GtkWidget *pButton,
             int id,
             SourceBrowserPreferences *
             );
  static void toggle_cb(GtkToggleButton *widget,
                        TabButton    *This);
  void set_active();
private:
  GtkWidget *m_button;
  SourceBrowserPreferences *m_prefs;
  int m_id;
};

//========================================================================
//
// class FontSelection
//

class FontSelection
{
public:
  FontSelection (GtkWidget *pParent,
                 SourceBrowserPreferences *
                 );
  static void setFont_cb(GtkFontButton *widget,
                        FontSelection  *This);
  void setFont();
private:
  SourceBrowserPreferences *m_prefs;
  GtkWidget *m_fontButton;
};

//========================================================================
//
class SourceBrowserPreferences : public SourceWindow
{
public:
  explicit SourceBrowserPreferences(GtkWidget *pParent);
  SourceBrowserPreferences(const SourceBrowserPreferences &) = delete;
  SourceBrowserPreferences& operator =(const SourceBrowserPreferences &) = delete;
  ~SourceBrowserPreferences();

  void apply();
  void cancel();
  void update();
  void toggleBreak(int line);
  void movePC(int line);

  int getPCLine(int page) override;
  int getAddress(NSourcePage *pPage, int line) override;
  bool bAddressHasBreak(int address) override;
  int getOpcode(int address) override;
  void setTabPosition(int);
  void setFont(const char *);
  const char *getFont();

private:
  ColorButton * m_LabelColor;
  ColorButton * m_MnemonicColor;
  ColorButton * m_SymbolColor;
  ColorButton * m_CommentColor;
  ColorButton * m_ConstantColor;

  MarginButton * m_LineNumbers;
  MarginButton * m_Addresses;
  MarginButton * m_Opcodes;

  int m_currentTabPosition;
  int m_originalTabPosition;
  TabButton *m_Up;
  TabButton *m_Left;
  TabButton *m_Down;
  TabButton *m_Right;
  TabButton *m_None;

  FontSelection  *m_FontSelector;
};

//------------------------------------------------------------------------
class gpsimGuiPreferences
{
public:
  gpsimGuiPreferences();
  gpsimGuiPreferences(const gpsimGuiPreferences&) = delete;  // Non-copiable
  gpsimGuiPreferences& operator =(const gpsimGuiPreferences &) = delete;
  ~gpsimGuiPreferences();

  static void setup(GtkAction *action, gpointer user_data);

private:
  SourceBrowserPreferences *m_SourceBrowser;

  static void response_cb(GtkDialog *dialog, gint response_id,
    gpsimGuiPreferences *Self);
  void apply() { m_SourceBrowser->apply(); }
  void cancel() { m_SourceBrowser->cancel(); }
  GtkWidget *window;
};


void gpsimGuiPreferences::setup(GtkAction *, gpointer)
{
  new gpsimGuiPreferences();
}

void gpsimGuiPreferences::response_cb(GtkDialog *, gint response_id,
  gpsimGuiPreferences *Self)
{
  if (response_id == gint(GTK_RESPONSE_CANCEL))
    Self->cancel();
  if (response_id == gint(GTK_RESPONSE_APPLY))
    Self->apply();
  delete Self;
}


//------------------------------------------------------------------------
// ColorButton Constructor
ColorButton::ColorButton(GtkWidget *pParent, GtkTextTag *pStyle,
                         const char *colorName, SourceBrowserPreferences *)
  : m_pStyle(pStyle)
{
  GtkWidget *hbox = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX (pParent), hbox, FALSE, TRUE, 0);

  g_object_get(m_pStyle, "foreground-gdk", &old_color, nullptr);

  GtkWidget *colorButton = gtk_color_button_new_with_color(old_color);
  gtk_color_button_set_title(GTK_COLOR_BUTTON(colorButton), colorName);
  gtk_box_pack_start(GTK_BOX(hbox), colorButton, FALSE, FALSE, 0);

  g_signal_connect(colorButton,
                      "color-set",
                      G_CALLBACK(setColor_cb),
                      this);

  GtkWidget *label = gtk_label_new(colorName);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

  gtk_widget_show_all(hbox);
}

ColorButton::~ColorButton()
{
  gdk_color_free(old_color);
}

//------------------------------------------------------------------------
void ColorButton::setColor_cb(GtkColorButton *widget,
                              ColorButton    *This)
{
  GdkColor newColor;
  gtk_color_button_get_color(widget, &newColor);
  g_object_set(This->m_pStyle, "foreground-gdk", &newColor, nullptr);
}

void ColorButton::cancel()
{
  g_object_set(m_pStyle, "foreground-gdk", old_color, nullptr);
}

//------------------------------------------------------------------------
MarginButton::MarginButton(GtkWidget *pParent, const char *pName,
                           eMarginType id,
                           SourceBrowserPreferences *prefs)
  : m_prefs(prefs), m_id(id)
{
  m_button = gtk_check_button_new_with_label(pName);
  bool bState = false;

  switch (m_id) {
  case eLineNumbers:
    bState = m_prefs->margin().bLineNumbers();
    break;

  case eAddresses:
    bState = m_prefs->margin().bAddresses();
    break;

  case eOpcodes:
    bState = m_prefs->margin().bOpcodes();
    break;
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_button),
                                bState);
  gtk_box_pack_start(GTK_BOX(pParent), m_button, FALSE, TRUE, 10);

  g_signal_connect(m_button,
                      "toggled",
                      G_CALLBACK(toggle_cb),
                      this);
}

//------------------------------------------------------------------------
void MarginButton::toggle_cb(GtkToggleButton *, MarginButton *This)
{
  This->set_active();
}

void MarginButton::set_active()
{
  bool bNewState = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_button)) ? true : false;
  switch (m_id) {
  case eLineNumbers:
    m_prefs->margin().enableLineNumbers(bNewState);
    break;

  case eAddresses:
    m_prefs->margin().enableAddresses(bNewState);
    break;

  case eOpcodes:
    m_prefs->margin().enableOpcodes(bNewState);
    break;
  }
}

//------------------------------------------------------------------------
TabButton::TabButton(GtkWidget *pParent, GtkWidget *pButton,
                     int id,
                     SourceBrowserPreferences *prefs)
  : m_button(pButton), m_prefs(prefs), m_id(id)
{
  gtk_box_pack_start(GTK_BOX(pParent), m_button, FALSE, TRUE, 5);

  g_signal_connect(m_button,
                      "toggled",
                      G_CALLBACK(toggle_cb),
                      this);
}

//------------------------------------------------------------------------
void TabButton::toggle_cb(GtkToggleButton *,
                          TabButton    *This)
{
  This->set_active();
}

void TabButton::set_active()
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_button)))
    m_prefs->setTabPosition(m_id);
}

//------------------------------------------------------------------------
FontSelection::FontSelection(GtkWidget *pParent,
                              SourceBrowserPreferences *pPrefs)
  : m_prefs(pPrefs)
{
  GtkWidget *frame = gtk_frame_new("Font");

  gtk_box_pack_start(GTK_BOX(pParent), frame, FALSE, TRUE, 0);
  GtkWidget *hbox = gtk_hbox_new(0, 0);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  m_fontButton = gtk_font_button_new_with_font(m_prefs->getFont());
  const char *fontDescription = "Font Selector";
  gtk_font_button_set_title(GTK_FONT_BUTTON(m_fontButton), fontDescription);
  gtk_box_pack_start(GTK_BOX(hbox), m_fontButton, FALSE, FALSE, 0);
  gtk_widget_show(m_fontButton);

  g_signal_connect(m_fontButton,
                      "font-set",
                      G_CALLBACK(setFont_cb),
                      this);

  GtkWidget *label = gtk_label_new("font");
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 10);
  gtk_widget_show(label);

  gtk_widget_show(hbox);
}

//------------------------------------------------------------------------
void FontSelection::setFont_cb(GtkFontButton *, FontSelection *This)
{
  This->setFont();
}

void FontSelection::setFont()
{
  m_prefs->setFont(gtk_font_button_get_font_name(GTK_FONT_BUTTON(m_fontButton)));
}

//------------------------------------------------------------------------

void SourceBrowserPreferences::toggleBreak(int)
{
}


void SourceBrowserPreferences::movePC(int)
{
}


//========================================================================
SourceBrowserPreferences::SourceBrowserPreferences(GtkWidget *pParent)
  : SourceWindow(nullptr, nullptr, false, nullptr)
{
  if (!gpGuiProcessor || !gpGuiProcessor->source_browser)
    return;

  GtkWidget *notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos((GtkNotebook*)notebook,GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX (pParent), notebook, TRUE, TRUE, 0);
  gtk_widget_show(notebook);

  m_pParent = gpGuiProcessor->source_browser;
  GtkWidget *label;

  {
    // Color Frame for Source Browser configuration

    GtkWidget *vbox = gtk_vbox_new(0, 0);

    GtkWidget *colorFrame = gtk_frame_new("Colors");
    gtk_box_pack_start(GTK_BOX(vbox), colorFrame, FALSE, TRUE, 0);

    GtkWidget *colorVbox = gtk_vbox_new(0, 0);
    gtk_container_add(GTK_CONTAINER(colorFrame), colorVbox);

    GtkTextTagTable *tag_table = m_pParent->getTagTable();

    m_LabelColor    = new ColorButton(colorVbox,
			gtk_text_tag_table_lookup(tag_table, "Label"),
      			"Label", this);
    m_MnemonicColor = new ColorButton(colorVbox,
			gtk_text_tag_table_lookup(tag_table, "Mnemonic"),
      			"Mnemonic", this);
    m_SymbolColor   = new ColorButton(colorVbox,
			gtk_text_tag_table_lookup(tag_table, "Symbols"),
      			"Symbols", this);
    m_ConstantColor = new ColorButton(colorVbox,
			gtk_text_tag_table_lookup(tag_table, "Constants"),
      			"Constants", this);
    m_CommentColor  = new ColorButton(colorVbox,
			gtk_text_tag_table_lookup(tag_table, "Comments"),
      			"Comments", this);

    // Font selector
    m_FontSelector = new FontSelection(vbox, this);

    label = gtk_label_new("Font");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,label);
  }

  {
    // Tab Frame for the Source browser
    m_currentTabPosition = m_pParent->getTabPosition();
    m_originalTabPosition = m_currentTabPosition;

    GtkWidget *hbox = gtk_hbox_new(0, 0);
    GtkWidget *tabFrame = gtk_frame_new("Notebook Tabs");
    gtk_box_pack_start(GTK_BOX(hbox), tabFrame, FALSE, TRUE, 0);

    GtkWidget *radioUp  = gtk_radio_button_new_with_label(nullptr,"up");
    GtkRadioButton *rb  = GTK_RADIO_BUTTON(radioUp);

    GtkWidget *tabVbox = gtk_vbox_new(0, 0);
    gtk_container_add(GTK_CONTAINER(tabFrame), tabVbox);

    m_Up    = new TabButton(tabVbox, radioUp, GTK_POS_TOP, this);
    m_Left  = new TabButton(tabVbox,
		gtk_radio_button_new_with_label_from_widget(rb,"left"),
      		GTK_POS_LEFT, this);
    m_Down  = new TabButton(tabVbox,
		gtk_radio_button_new_with_label_from_widget(rb,"down"),
      		GTK_POS_BOTTOM, this);
    m_Right = new TabButton(tabVbox,
		gtk_radio_button_new_with_label_from_widget(rb,"right"),
      		GTK_POS_RIGHT, this);
    m_None  = new TabButton(tabVbox,
		gtk_radio_button_new_with_label_from_widget(rb,"none"),
      		-1, this);


    // Source browser margin
    GtkWidget *marginFrame = gtk_frame_new("Margin");
    gtk_box_pack_start(GTK_BOX(hbox), marginFrame, FALSE, TRUE, 0);
    GtkWidget *marginVbox = gtk_vbox_new(0, 0);
    gtk_container_add(GTK_CONTAINER(marginFrame), marginVbox);

    m_LineNumbers = new MarginButton(marginVbox, "Line Numbers",
      			MarginButton::eLineNumbers, this);
    m_Addresses   = new MarginButton(marginVbox, "Addresses",
      			MarginButton::eAddresses, this);
    m_Opcodes     = new MarginButton(marginVbox, "Opcodes",
			MarginButton::eOpcodes, this);

    label = gtk_label_new("Margins");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),hbox,label);
  }

  {
    SourceBuffer *pBuffer = new SourceBuffer(m_pParent->getTagTable(), 0, m_pParent);


    GtkWidget *frame = gtk_frame_new("Sample");
    gtk_box_pack_start(GTK_BOX(pParent), frame, TRUE, TRUE, 0);

    m_Notebook = gtk_notebook_new();
    //m_currentTabPosition = m_pParent->getTabPosition();
    //gtk_notebook_set_tab_pos((GtkNotebook*)m_Notebook,m_TabPosition);
    setTabPosition(m_pParent->getTabPosition());

    gtk_container_add(GTK_CONTAINER(frame), m_Notebook);

    bIsBuilt = true;

    AddPage(pBuffer, "file1.asm");

    pBuffer->parseLine("        MOVLW   0x34       ; Comment\n", 1);
    pBuffer->parseLine("; Comment only\n", 1);
    pBuffer->parseLine("Label:  ADDWF  Variable,F  ; Comment\n", 1);

    gtk_widget_show_all(frame);

    label = gtk_label_new("file2.asm");
    GtkWidget *emptyBox = gtk_hbox_new(0, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_Notebook),emptyBox,label);
  }

  gtk_widget_show_all(notebook);
}

SourceBrowserPreferences::~SourceBrowserPreferences()
{
  delete m_Left;
  delete m_Down;
  delete m_Right;
  delete m_None;
  delete m_Up;
  delete m_LabelColor;
  delete m_MnemonicColor;
  delete m_SymbolColor;
  delete m_CommentColor;
  delete m_ConstantColor;

  delete m_LineNumbers;
  delete m_Addresses;
  delete m_Opcodes;
  delete m_FontSelector;
}

void SourceBrowserPreferences::setTabPosition(int tabPosition)
{
  m_currentTabPosition = tabPosition;
  m_pParent->setTabPosition(tabPosition);
  if (tabPosition >= 0) {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook), TRUE);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(m_Notebook), (GtkPositionType) m_currentTabPosition);
  } else {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook), FALSE);
  }
  Update();
}

void SourceBrowserPreferences::setFont(const char *cpFont)
{
  m_pParent->setFont(cpFont);
}

const char *SourceBrowserPreferences::getFont()
{
  return m_pParent->getFont();
}

void SourceBrowserPreferences::apply()
{
  m_pParent->setTabPosition(m_currentTabPosition);
}

void SourceBrowserPreferences::cancel()
{

  m_LabelColor->cancel();
  m_MnemonicColor->cancel();
  m_SymbolColor->cancel();
  m_ConstantColor->cancel();
  m_CommentColor->cancel();

  m_pParent->setTabPosition(m_originalTabPosition);
}

int SourceBrowserPreferences::getPCLine(int)
{
  return 1;
}

int SourceBrowserPreferences::getAddress(NSourcePage *, int)
{
  return 0x1234;
}

bool SourceBrowserPreferences::bAddressHasBreak(int)
{
  return false;
}

int SourceBrowserPreferences::getOpcode(int)
{
  return 0xABCD;
}

//========================================================================
gpsimGuiPreferences::gpsimGuiPreferences()
{
  window = gtk_dialog_new_with_buttons("Source Browser configuration",
    nullptr,
    GTK_DIALOG_MODAL,
    GTK_STOCK_CANCEL, gint(GTK_RESPONSE_CANCEL),
    GTK_STOCK_APPLY, gint(GTK_RESPONSE_APPLY),
    nullptr);

  g_signal_connect(window, "response",
    G_CALLBACK(gpsimGuiPreferences::response_cb), this);

  GtkWidget *box = gtk_dialog_get_content_area(GTK_DIALOG(window));

  m_SourceBrowser = new SourceBrowserPreferences(box);

  gtk_widget_show_all(window);
}

gpsimGuiPreferences::~gpsimGuiPreferences()
{
  gtk_widget_destroy(window);

  delete m_SourceBrowser;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//========================================================================

void gui_message(const char *message);

static void update_preview(GtkFileChooser *file_chooser, gpointer)
{
  gchar *file = gtk_file_chooser_get_preview_filename(file_chooser);
  gboolean show = FALSE;
  if (file) {
    size_t len = strlen(file);
    if (len >= 4) {
      gchar *p = file + len - 4;
      if (!strcmp(p, ".hex") || !strcmp(p, ".HEX")) {
        show = TRUE;
      }
    }
  }
  g_free(file);
  gtk_file_chooser_set_preview_widget_active(file_chooser, show);
}

static void file_selection_changed(GtkTreeSelection *sel, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *processor;
  gchar **d = (gchar **)data;

  if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
    gtk_tree_model_get(model, &iter, 0, &processor, -1);
    gchar *p = *d;
    g_free(p);
    *d = processor;
  }
}

static void
fileopen_dialog(GtkAction *, gpointer)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Open file",
    nullptr,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    nullptr);

  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Gpsim");
  gtk_file_filter_add_pattern(filter, "*.cod");
  gtk_file_filter_add_pattern(filter, "*.COD");
  gtk_file_filter_add_pattern(filter, "*.stc");
  gtk_file_filter_add_pattern(filter, "*.STC");
  gtk_file_filter_add_pattern(filter, "*.hex");
  gtk_file_filter_add_pattern(filter, "*.HEX");

  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern(filter, "*");
  gtk_file_filter_set_name(filter, "All files");

  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  GtkListStore *pic_list = gtk_list_store_new(1, G_TYPE_STRING);

  ProcessorConstructorList::iterator processor_iterator;
  ProcessorConstructorList *pl = ProcessorConstructor::GetList();
  for (processor_iterator = pl->begin(); processor_iterator != pl->end();
    ++processor_iterator) {
    ProcessorConstructor *p = *processor_iterator;
    GtkTreeIter iter;
    gtk_list_store_append(pic_list, &iter);
    gtk_list_store_set(pic_list, &iter, 0, p->names[1], -1);
  }

  GtkWidget *scroll_window = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pic_list));

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column
    = gtk_tree_view_column_new_with_attributes("Processor", renderer, "text",
    0, nullptr);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), TRUE);
  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

  gtk_container_add(GTK_CONTAINER(scroll_window), tree);

  gtk_widget_show_all(scroll_window);

  gchar *processor = nullptr;

  gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog), scroll_window);
  gtk_file_chooser_set_use_preview_label(GTK_FILE_CHOOSER(dialog), FALSE);
  g_signal_connect(selection, "changed", G_CALLBACK(file_selection_changed), (gpointer)&processor);
  g_signal_connect(dialog, "update-preview", G_CALLBACK(update_preview), nullptr);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gchar *use_processor = nullptr;
    if (filename) {
      size_t len = strlen(filename);
      if (len >= 4) {
        gchar *p = filename + len - 4;
        if (!strcmp(p, ".hex") || !strcmp(p, ".HEX")) {
          use_processor = processor;
        }
      }
    }
    if (!gpsim_open(gpGuiProcessor->cpu, filename, use_processor, 0)) {
      gchar *msg = g_strdup_printf(
        "Open failed. Could not open \"%s\"", filename);
      gui_message(msg);
      g_free(msg);
    } else {
      GtkAction *menu_item = gtk_ui_manager_get_action(ui, "/menu/FileMenu/Open");
      gtk_action_set_sensitive(menu_item, FALSE);
    }
    g_free(filename);
  }

  g_free(processor);
  g_object_unref(pic_list);
  gtk_widget_destroy(tree);
  gtk_widget_destroy(dialog);
}




// Menuhandler for Windows menu buttons
static void
toggle_window(GtkToggleAction *action, gpointer)
{
  if (gpGuiProcessor) {
    std::string item = gtk_action_get_name(GTK_ACTION(action));
    gboolean view_state = gtk_toggle_action_get_active(action);

    if (item == "Program memory") {
      gpGuiProcessor->program_memory->ChangeView(view_state);
    } else if (item == "Source") {
      gpGuiProcessor->source_browser->ChangeView(view_state);
    } else if (item == "Ram") {
      gpGuiProcessor->regwin_ram->ChangeView(view_state);
    } else if (item == "EEPROM") {
      gpGuiProcessor->regwin_eeprom->ChangeView(view_state);
    } else if (item == "Watch") {
      gpGuiProcessor->watch_window->ChangeView(view_state);
    } else if (item == "Symbols") {
      gpGuiProcessor->symbol_window->ChangeView(view_state);
    } else if (item == "Breadboard") {
      gpGuiProcessor->breadboard_window->ChangeView(view_state);
    } else if (item == "Stack") {
      gpGuiProcessor->stack_window->ChangeView(view_state);
    } else if (item == "Trace") {
      gpGuiProcessor->trace_window->ChangeView(view_state);
    } else if (item == "Profile") {
      gpGuiProcessor->profile_window->ChangeView(view_state);
    } else if (item == "Stopwatch") {
      gpGuiProcessor->stopwatch_window->ChangeView(view_state);
    } else if (item == "Scope") {
      gpGuiProcessor->scope_window->ChangeView(view_state);
    }
  }
}

//========================================================================
// Button callbacks
static void
runbutton_cb(GtkWidget *)
{
  get_interface().start_simulation();
}

static void
stopbutton_cb(GtkWidget *)
{
  if (gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->stop();
}

static void
stepbutton_cb(GtkWidget *)
{
  if (gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->step(1);
}

static void
overbutton_cb(GtkWidget *)
{
  if (gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->step_over();
}

static void
finishbutton_cb(GtkWidget *)
{
  if (gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->finish();
}

static void
resetbutton_cb(GtkWidget *)
{
  if (gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->reset(POR_RESET);
}


int gui_animate_delay; // in milliseconds


//========================================================================
//========================================================================
//
// UpdateRateMenuItem

//========================================================================
//
// Class declaration -- probaby should move to a .h file.

class UpdateRateMenuItem {
public:
  UpdateRateMenuItem(GtkWidget *,char, const char *, int update_rate = 0,
      bool _bRealTime = false, bool _bWithGui = false);

  void Select();

private:
  int update_rate;
public:
  char id;
private:
  bool bAnimate;
  bool bRealTime;
  bool bWithGui;
};

UpdateRateMenuItem::UpdateRateMenuItem(GtkWidget *parent,
                                       char _id,
                                       const char *label,
                                       int _update_rate,
                                       bool _bRealTime,
                                       bool _bWithGui)
  : update_rate(_update_rate), id(_id), bRealTime(_bRealTime), bWithGui(_bWithGui)
{
  if (update_rate < 0) {
    bAnimate = true;
    update_rate = -update_rate;
  } else
    bAnimate = false;

  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(parent), label);
}

void UpdateRateMenuItem::Select()
{
  EnableRealTimeMode(bRealTime);
  EnableRealTimeModeWithGui(bWithGui);

  if (bAnimate) {
    gui_animate_delay = update_rate;
    get_interface().set_update_rate(1);
  } else {
    gui_animate_delay = 0;
    get_interface().set_update_rate(update_rate);
  }

  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->stop();

  config_set_variable("dispatcher", "SimulationMode", id);
}


//========================================================================
//========================================================================
class TimeWidget;
class TimeFormatter
{
public:
  enum eMenuID {
    eCyclesHex = 0,
    eCyclesDec,
    eMicroSeconds,
    eMilliSeconds,
    eSeconds,
    eHHMMSS
  } time_format;

  TimeFormatter(TimeWidget *_tw, GtkWidget *menu, const char *menu_text)
   : tw(_tw)
  {
    AddToMenu(menu, menu_text);
  }

  virtual ~TimeFormatter()
  {
  }

  void ChangeFormat();
  void AddToMenu(GtkWidget *menu, const char*menu_text);
  virtual void Format(char *, int) = 0;
  TimeWidget *tw;
};

class TimeWidget : public EntryWidget
{
public:
  TimeWidget();
  void Create(GtkWidget *);
  void Update() override;
  void NewFormat(TimeFormatter *tf);
  TimeFormatter *current_format;

  GtkWidget *menu;
};


class TimeMicroSeconds : public TimeFormatter
{
public:
  TimeMicroSeconds(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw, menu, "MicroSeconds") {}

  virtual ~TimeMicroSeconds()
  {
  }

  void Format(char *buf, int size) override
  {
    double time_db = 0.0;
    if (gpGuiProcessor && gpGuiProcessor->cpu)
        time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get() * 1e6;
    g_snprintf(buf, size, "%19.2f us", time_db);
  }
};

class TimeMilliSeconds : public TimeFormatter
{
public:
  TimeMilliSeconds(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw, menu, "MilliSeconds") {}

  virtual ~TimeMilliSeconds()
  {
  }

  void Format(char *buf, int size) override
  {
    double time_db = 0.0;
    if (gpGuiProcessor && gpGuiProcessor->cpu)
        time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get() * 1e3;
    g_snprintf(buf, size, "%19.3f ms", time_db);
  }
};

class TimeSeconds : public TimeFormatter
{
public:
  TimeSeconds(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw, menu, "Seconds") {}

  virtual ~TimeSeconds()
  {
  }

  void Format(char *buf, int size) override
  {
    double time_db = 0.0;
    if (gpGuiProcessor && gpGuiProcessor->cpu)
       time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get();
    g_snprintf(buf, size, "%19.3f Sec", time_db);
  }
};

class TimeHHMMSS : public TimeFormatter
{
public:
  TimeHHMMSS(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw, menu, "HH:MM:SS.mmm") {}

  virtual ~TimeHHMMSS()
  {
  }

  void Format(char *buf, int size) override
  {
    double time_db = 0.0;
    if (gpGuiProcessor && gpGuiProcessor->cpu)
        time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get();
    double v = time_db + 0.005;	// round msec
    int hh = (int)(v / 3600);
    v -= hh * 3600.0;
    int mm = v / 60;
    v -= mm * 60.0;
    int ss = v;
    int cc = (v - ss) * 100.0;
    g_snprintf(buf, size, "    %02d:%02d:%02d.%02d", hh, mm, ss, cc);
  }
};

class TimeCyclesHex : public TimeFormatter
{
public:
  TimeCyclesHex(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw, menu, "Cycles (Hex)") {}

  virtual ~TimeCyclesHex()
  {
  }

  void Format(char *buf, int size) override
  {
    g_snprintf(buf, size, "0x%016" PRINTF_GINT64_MODIFIER "x", get_cycles().get());
  }
};

class TimeCyclesDec : public TimeFormatter
{
public:
  TimeCyclesDec(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw, menu, "Cycles (Dec)") {}

  virtual ~TimeCyclesDec()
  {
  }

  void Format(char *buf, int size) override
  {
    g_snprintf(buf, size, "%016" PRINTF_GINT64_MODIFIER "d", get_cycles().get());
  }
};

//========================================================================
// called when user has selected a menu item from the time format menu
static void
cbTimeFormatActivated(GtkWidget *widget, gpointer data)
{
  if (!widget || !data)
    return;

  TimeFormatter *tf = static_cast<TimeFormatter *>(data);
  tf->ChangeFormat();
}

// button press handler
static gint
cbTimeFormatPopup(GtkWidget *widget, GdkEventButton *event, TimeWidget *tw)
{
  if (!widget || !event || !tw)
    return 0;

  if (event->type == GDK_BUTTON_PRESS) {

    gtk_menu_popup(GTK_MENU(tw->menu), nullptr, nullptr, nullptr, nullptr,
                   3, event->time);
    // It looks like we need it to avoid a selection in the entry.
    // For this we tell the entry to stop reporting this event.
    g_signal_stop_emission_by_name(tw->entry, "button_press_event");
  }
  return FALSE;
}


void TimeFormatter::ChangeFormat()
{
  if (tw)
    tw->NewFormat(this);
}

void TimeFormatter::AddToMenu(GtkWidget *menu,
                              const char*menu_text)
{
  GtkWidget *item = gtk_menu_item_new_with_label(menu_text);
  g_signal_connect(item, "activate",
                     G_CALLBACK(cbTimeFormatActivated),
                     this);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

void TimeWidget::Create(GtkWidget *container)
{
  set_editable(false);

  gtk_container_add(GTK_CONTAINER(container), entry);

  SetEntryWidth(18);

  menu = gtk_menu_new();

  // Create an entry for each item in the formatter pop up window.

  new TimeMicroSeconds(this, menu);
  new TimeMilliSeconds(this, menu);
  new TimeSeconds(this, menu);
  new TimeHHMMSS(this, menu);
  new TimeCyclesHex(this, menu);
  NewFormat(new TimeCyclesDec(this, menu));

  // Associate a callback with the user button-click actions
  g_signal_connect(entry,
                     "button_press_event",
                     G_CALLBACK(cbTimeFormatPopup),
                     this);
}


void TimeWidget::NewFormat(TimeFormatter *tf)
{
  if (tf && tf != current_format) {
    current_format = tf;
    Update();
  }
}

void TimeWidget::Update()
{
  if (!current_format)
    return;

  char buffer[32];

  current_format->Format(buffer, sizeof(buffer));
  gtk_entry_set_text(GTK_ENTRY(entry), buffer);
}

TimeWidget::TimeWidget()
{
  menu = nullptr;
  current_format = nullptr;
}


//========================================================================
static int dispatcher_delete_event(GtkWidget *,
                                   GdkEvent  *,
                                   gpointer)
{
  do_quit_app(0);

  return 0;
}

static const GtkActionEntry entries[] = {
  {"FileMenu", nullptr, "_File", nullptr, nullptr, nullptr},
  {"Open", GTK_STOCK_OPEN, "_Open", "<control>O", nullptr, G_CALLBACK(fileopen_dialog)},
  {"Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK(do_quit_app)},
  {"Windows", nullptr, "_Windows", nullptr, nullptr, nullptr},
  {"Edit", nullptr, "_Edit", nullptr, nullptr, nullptr},
  {"Preferences", GTK_STOCK_PREFERENCES, "Preferences", nullptr, nullptr, G_CALLBACK(gpsimGuiPreferences::setup)},
  {"Help", nullptr, "_Help", nullptr, nullptr, nullptr},
  {"About", GTK_STOCK_ABOUT, "_About", nullptr, nullptr, G_CALLBACK(about_cb)}
};

static const GtkToggleActionEntry toggle_entries[] = {
  {"Program memory", nullptr, "Program _memory", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Source", nullptr, "_Source", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Ram", nullptr, "_Ram", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"EEPROM", nullptr, "_EEPROM", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Watch", nullptr, "_Watch", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Stack", nullptr, "Sta_ck", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Symbols", nullptr, "Symbo_ls", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Breadboard", nullptr, "_Breadboard", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Trace", nullptr, "_Trace", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Profile", nullptr, "Pro_file", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Stopwatch", nullptr, "St_opwatch", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE},
  {"Scope", nullptr, "Sco_pe", nullptr, nullptr, G_CALLBACK(toggle_window), FALSE}
};

static const gchar *ui_info =
"<ui>"
"  <menubar name='menu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='Open'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='Windows'>"
"      <menuitem action='Program memory'/>"
"      <menuitem action='Source'/>"
"      <separator/>"
"      <menuitem action='Ram'/>"
"      <menuitem action='EEPROM'/>"
"      <menuitem action='Watch'/>"
"      <menuitem action='Stack'/>"
"      <separator/>"
"      <menuitem action='Symbols'/>"
"      <menuitem action='Breadboard'/>"
"      <separator/>"
"      <menuitem action='Trace'/>"
"      <menuitem action='Profile'/>"
"      <menuitem action='Stopwatch'/>"
"      <menuitem action='Scope'/>"
"    </menu>"
"    <menu action='Edit'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='Help'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

GtkWidget *dispatcher_window = nullptr;
//========================================================================
//========================================================================

class MainWindow
{
public:
  void Update();

  MainWindow();
  ~MainWindow();

private:
  static void gui_update_cb(GtkWidget *widget, gpointer data);

  TimeWidget timeW;
  std::vector<UpdateRateMenuItem> rate_menu_items;
};

MainWindow * TheWindow;

MainWindow::~MainWindow()
{
}

void MainWindow::Update()
{
   timeW.Update();
}

void MainWindow::gui_update_cb(GtkWidget *widget, gpointer data)
{
  gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  MainWindow *main_window = static_cast<MainWindow *>(data);

  main_window->rate_menu_items[index].Select();
}

MainWindow::MainWindow()
{
  GtkWidget *box1;

  GtkWidget *buttonbox;
  GtkWidget *separator;
  GtkWidget *button;
  GtkWidget *frame;
  int x,y,width,height;

  GtkWidget *update_rate_menu;

  int SimulationMode;

  dispatcher_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  if (!config_get_variable("dispatcher", "x", &x))
    x = 10;
  if (!config_get_variable("dispatcher", "y", &y))
    y = 10;
  if (!config_get_variable("dispatcher", "width", &width))
    width = 1;
  if (!config_get_variable("dispatcher", "height", &height))
    height = 1;
  gtk_window_resize(GTK_WINDOW(dispatcher_window), width, height);
  gtk_window_move(GTK_WINDOW(dispatcher_window), x, y);


  g_signal_connect(dispatcher_window, "delete-event",
                      G_CALLBACK(dispatcher_delete_event),
                      0);

  GtkActionGroup *actions = gtk_action_group_new("Actions");
  gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), nullptr);
  gtk_action_group_add_toggle_actions(actions, toggle_entries,
    G_N_ELEMENTS(toggle_entries), nullptr);

  ui = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group(ui, actions, 0);
  g_object_unref(actions);
  gtk_window_add_accel_group(GTK_WINDOW(dispatcher_window),
    gtk_ui_manager_get_accel_group(ui));

  if (!gtk_ui_manager_add_ui_from_string(ui, ui_info, -1, nullptr)) {
    g_error("building menus failed");
  }

  gtk_window_set_title(GTK_WINDOW(dispatcher_window), VERSION);
  gtk_container_set_border_width(GTK_CONTAINER(dispatcher_window), 0);

  box1 = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER (dispatcher_window), box1);

  gtk_box_pack_start(GTK_BOX(box1),
                      gtk_ui_manager_get_widget(ui, "/menu"),
                      FALSE, FALSE, 0);


  buttonbox = gtk_hbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(buttonbox), 1);
  gtk_box_pack_start(GTK_BOX(box1), buttonbox, TRUE, TRUE, 0);


  // Buttons
  button = gtk_button_new_with_label("step");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(stepbutton_cb), 0);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label("over");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(overbutton_cb), 0);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label("finish");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(finishbutton_cb), 0);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label("run");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(runbutton_cb), 0);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label("stop");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(stopbutton_cb), 0);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label("reset");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(resetbutton_cb), 0);
  gtk_box_pack_start(GTK_BOX(buttonbox), button, TRUE, TRUE, 0);


  //
  // Simulation Mode Frame
  //

  frame = gtk_frame_new("Simulation mode");
  if (!config_get_variable("dispatcher", "SimulationMode", &SimulationMode))
    {
      SimulationMode = '4';
    }

  //
  // Gui Update Rate
  //

  update_rate_menu = gtk_combo_box_text_new();
  gtk_container_add(GTK_CONTAINER(frame),update_rate_menu);

  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, '5', "Without gui (fastest simulation)", 0));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, '4', "2000000 cycles/gui update", 2000000));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, '3', "100000 cycles/gui update", 100000));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, '2', "1000 cycles/gui update", 1000));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, '1', "Update gui every cycle", 1));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, 'b', "100ms animate", -100));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, 'c', "300ms animate", -300));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, 'd', "700ms animate", -700));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, 'r', "Realtime without gui", 0, true));
  rate_menu_items.push_back(
    UpdateRateMenuItem(update_rate_menu, 'R', "Realtime with gui", 0, true, true));

  for (size_t i = 0 ; i < rate_menu_items.size(); ++i) {
    if (rate_menu_items[i].id == SimulationMode) {
      rate_menu_items[i].Select();
      gtk_combo_box_set_active(GTK_COMBO_BOX(update_rate_menu), i);
    }
  }

  g_signal_connect(update_rate_menu, "changed", G_CALLBACK(gui_update_cb), this);

  gtk_box_pack_start(GTK_BOX(buttonbox), frame, FALSE, FALSE, 5);

  //
  // Simulation Time Frame
  //

  frame = gtk_frame_new("Simulation Time");
  gtk_box_pack_start(GTK_BOX(buttonbox), frame, FALSE, FALSE, 5);

  timeW.Create(frame);
  timeW.Update();

  //gtk_box_pack_start (GTK_BOX (buttonbox), frame, TRUE, TRUE, 5);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(box1), separator, FALSE, TRUE, 5);
  button = gtk_button_new_with_label("Quit gpsim");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(do_quit_app), 0);

  gtk_box_pack_start(GTK_BOX(box1), button, FALSE, TRUE, 5);
  gtk_widget_show_all(dispatcher_window);
}

//========================================================================

void create_dispatcher ()
{
  if (!TheWindow)
    TheWindow = new MainWindow;
}

void dispatch_Update()
{
  if (TheWindow && gpGuiProcessor && gpGuiProcessor->cpu) {
    TheWindow->Update();
  }
}

#endif // HAVE_GUI
