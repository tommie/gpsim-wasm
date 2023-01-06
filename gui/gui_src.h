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

#ifndef GUI_GUI_SRC_H_
#define GUI_GUI_SRC_H_

#include "../src/modules.h"
#include "gui_object.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <map>
#include <string>
#include <vector>

// forward references
class SourceBrowserParent_Window;
class StatusBar_Window;
class Value;
class SourceWindow;
class FileContext;
class GUI_Processor;
class ProgramMemoryAccess;

//========================================================================
class SourceBuffer
{
public:
  SourceBuffer(GtkTextTagTable *,FileContext *,SourceBrowserParent_Window *);
  void parseLine(const char*, int parseStyle);

  void addTagRange(const char *pStyle, int start_index, int end_index);

  GtkTextBuffer *getBuffer();
  SourceBrowserParent_Window *m_pParent;
  FileContext   *m_pFC;

  bool IsParsed();
  void parse();
private:
  bool m_bParsed;
  GtkTextBuffer *m_buffer;
};


//========================================================================
// The SourcePageMargin holds configuration information for the left margin
// of a SourcePage. The left margin is where line numbers, addresses, opcodes,
// breakpoints, and current program counter are all shown.
class SourcePageMargin
{
public:
  SourcePageMargin();

  void enableLineNumbers(bool b)
  {
    m_bShowLineNumbers = b;
  }
  void enableAddresses(bool b)
  {
    m_bShowAddresses = b;
  }
  void enableOpcodes(bool b)
  {
    m_bShowOpcodes = b;
  }

  bool formatMargin(char *,int, int line,int addr,int opcode,bool bBreak);
  bool bLineNumbers() { return m_bShowLineNumbers; }
  bool bAddresses()   { return m_bShowAddresses; }
  bool bOpcodes()     { return m_bShowOpcodes; }
private:
  bool m_bShowLineNumbers;
  bool m_bShowAddresses;
  bool m_bShowOpcodes;
};

//========================================================================
// SourcePage
// A single source file.
//
// The SourcePage class associates a single file with a single GtkTextBuffer.
// manages
class NSourcePage
{
public:
  NSourcePage(SourceWindow *, SourceBuffer  *, int file_id, GtkWidget *);

  GtkTextBuffer *buffer();
  GtkTextView   *getView();
  SourceWindow  *getParent();

  void invalidateView();
  void updateMargin(int y1, int y2);

  void Close();
  void setFont(const char *);

  FileContext *getFC();

  unsigned int get_file_id() { return m_fileid; }
  int get_margin_width() { return m_marginWidth; }

private:
  // callbacks
  static gboolean KeyPressHandler(GtkTextView *pView, GdkEventKey *key,
    NSourcePage *page);
  static gint ButtonPressHandler(GtkTextView *pView, GdkEventButton *pButton,
    NSourcePage *pPage);
  static gint ViewExposeEventHandler(GtkTextView *pView, GdkEventExpose *pEvent,
    NSourcePage *pPage);

  GtkTextView   *m_view;
  SourceBuffer  *m_pBuffer;
  SourceWindow  *m_Parent;
  unsigned int   m_fileid;
  int            m_marginWidth;
  std::string    m_cpFont;
};

class SearchDialog;

class SourceWindow : public GUI_Object
{
public:
  SourceWindow(GUI_Processor *gp,
               SourceBrowserParent_Window *,
               bool bUseConfig,
               const char *newName = nullptr);

  void Build() override;
  virtual void SetTitle();

  void set_pma(ProgramMemoryAccess *new_pma);

  virtual void SelectAddress(int address);
  virtual void SelectAddress(Value *);
  void Update() override;
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void);
  virtual void NewSource(GUI_Processor *gp);

  virtual int getPCLine(int page);
  virtual int getAddress(NSourcePage *pPage, int line);
  virtual bool bAddressHasBreak(int address);
  virtual int getOpcode(int address);
  int  AddPage(SourceBuffer *pSourceBuffer, const std::string &fName);
  void step(int n = 1);
  void step_over();
  void stop();
  void run();
  void finish();
  void reset();
  void toggleBreak(NSourcePage *pPage, int line);
  void movePC(int line);
  bool bSourceLoaded() { return m_bSourceLoaded; }
  void findText();
  int  findText(const char *, int, bool bDir, bool bCase);

  GtkTextTagTable *getTagTable();

  SourcePageMargin &margin();
  const char *getFont();
  gint switch_page_cb(guint newPage);

  // do we need this:
  bool m_bLoadSource;
  bool m_bSourceLoaded;
  int  m_LineAtButtonClick;

private:
  int AddPage(SourceBuffer *pSourceBuffer);

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.
  StatusBar_Window *status_bar;  // display's PC, status, etc.
  SIMULATION_MODES last_simulation_mode;
  std::string sLastPmaName;
  unsigned int m_currentPage;          // Notebook page currently displayed.

  struct _PC {
    bool bIsActive;
    int  page;                // Notebook page containing the source
    int  line;                // Line within the page.
    GtkTextBuffer *pBuffer;   // Buffer containing the Program Counter
    GtkTextIter   iBegin;     // Start of where highlight begins
    GtkTextIter   iEnd;       // End of highlight
  } mProgramCounter;

  // Popup Menu
  SearchDialog *stPSearchDialog;
  GtkWidget * BuildPopupMenu();
  static void PopupMenuHandler(GtkWidget *widget, gpointer data);

  // Callbacks
  static gboolean KeyPressHandler(GtkWidget *widget,
                GdkEventKey *key,
                SourceWindow *pSW);
  static int DeleteEventHandler(GtkWidget *widget,
                GdkEvent  *event,
                SourceWindow *sw);
  static void cb_notebook_switchpage(GtkNotebook    *notebook,
                                    gpointer         page,
                                    guint            page_num,
                                    SourceWindow     *pSW);

  std::string m_name;

protected:
  std::map<int, NSourcePage *> pages;
  GtkWidget *m_Notebook;
  GtkPositionType m_TabPosition;
  SourceBrowserParent_Window *m_pParent;

public:
  const char *name_pub() { return name(); }
};

class SourceBrowser_Window : public GUI_Object {
 public:
  explicit SourceBrowser_Window(const char *name);

  GtkWidget *vbox;               // for children to put widgets in

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.
  SIMULATION_MODES last_simulation_mode;
  std::string sLastPmaName;

  void set_pma(ProgramMemoryAccess *new_pma);

  void Create();
  void NewProcessor(GUI_Processor *gp) override = 0;
  virtual void SetTitle();
  virtual void SelectAddress(int address) = 0;
  virtual void SelectAddress(Value *);
  void Update() override;
  virtual void UpdateLine(int address) = 0;
  virtual void SetPC(int address) = 0;
  virtual void CloseSource() {}
  virtual void NewSource(GUI_Processor *) {}
};

//
// The Source Opcode Browser Data
//
class SourceBrowserOpcode_Window : public SourceBrowser_Window
{
 public:
  explicit SourceBrowserOpcode_Window(GUI_Processor *gp);
  ~SourceBrowserOpcode_Window();

  void Build() override;
  void NewProcessor(GUI_Processor *gp) override;
  void SelectAddress(int address) override;
  void SetPC(int address) override;
  void NewSource(GUI_Processor *gp) override;
  void UpdateLine(int address) override;

  virtual void Fill();

  void update_ascii(gint row);
  void load_styles();
  void settings_dialog();

private:
  GtkWidget *build_menu_for_sheet();
  GtkWidget *build_menu_for_list();

  void update_values(int address);
  void update_styles(int address);
  void update(int address);
  void update_label(int address);

  void do_popup_menu(GtkWidget *my_widget, GdkEventButton *event);

  static void popup_activated(GtkWidget *widget, SourceBrowserOpcode_Window *sbow);
  static void cell_renderer(GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell, GtkTreeModel *tree_model,
    GtkTreeIter *iter, gpointer data);
  static void show_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow);
  static gint activate_sheet_cell(GtkWidget *widget,
    gint row, gint column, SourceBrowserOpcode_Window *sbow);
  static gint button_press(GtkWidget *widget, GdkEventButton *event,
    SourceBrowserOpcode_Window *sbow);
  static gboolean popup_menu_handler(GtkWidget *widget,
    SourceBrowserOpcode_Window *sbw);
  static void row_selected(GtkTreeView *tree_view, GtkTreePath *path,
    GtkTreeViewColumn *column, SourceBrowserOpcode_Window *sbow);

  GtkListStore *list;
  GtkWidget *tree;

  unsigned int current_address;   // current PC

  std::string normalfont_string;
  PangoFontDescription *normalPFD;

  GtkWidget *notebook;
  GtkWidget *sheet;
  GtkWidget *entry;
  GtkWidget *label;

  GtkWidget *sheet_popup_menu;
  GtkWidget *list_popup_menu;

  GdkPixbuf *break_pix;
  GdkPixbuf *pc_pix;

  unsigned int *memory;
};

//
// The Source Browser Child window.
//
// The gui supports "context" debugging, where a context
// may be code written for interrupt routines, non-interrupt
// routines, high versus low interrupt priorities, etc. Each
// context has a dedicated source browser. The SourceBrowserChild_Window
// class manages this.

class SourceBrowserParent_Window : public GUI_Object
{
public:
  explicit SourceBrowserParent_Window(GUI_Processor *gp);

  void Build() override;
  void NewProcessor(GUI_Processor *gp) override;
  virtual void SelectAddress(int address);
  virtual void SelectAddress(Value *);
  void Update() override;
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource();
  virtual void NewSource(GUI_Processor *gp);
  void ChangeView(int view_state) override;
  int set_config() override;

  GtkTextTagTable *getTagTable() { return mpTagTable; }
  void CreateSourceBuffers(GUI_Processor *gp);

  void parseSource(SourceBuffer *pBuffer,FileContext *pFC);
  SourcePageMargin &margin();
  void setTabPosition(int tt);
  int getTabPosition() { return m_TabType; }
  void setFont(const char *);
  const char *getFont();

private:
  gchar *get_color_string(const char *tag_name);

  GtkTextTagTable *mpTagTable;
  std::vector<SourceWindow *> children;

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.

private:
  SourcePageMargin m_margin;
  int m_TabType;
  std::string m_FontDescription;

public:
  std::vector<SourceBuffer *> ppSourceBuffers;
};

#endif // GUI_GUI_SRC_H_
