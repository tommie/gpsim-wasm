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

#include <config.h>
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <glib.h>

#include "gui.h"
#include "gui_object.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_src.h"
#include "gui_statusbar.h"
#include "gui_watch.h"

#include "../src/gpsim_classes.h"
#include "../src/gpsim_interface.h"
#include "../src/gpsim_object.h"
#include "../src/interface.h"
#include "../src/modules.h"
#include "../src/pic-instructions.h"
#include "../src/processor.h"
#include "../src/registers.h"
#include "../src/symbol.h"
#include "../src/ui.h"
#include "../src/value.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

//======================================================================
//
// When a user right-clicks in the source browser, a menu will popup.
// There can only be one menu active at any given time.


//
//  'aPopupMenu' pointer is a local pointer to a GtkMenu.
//

static GtkWidget *aPopupMenu;

//
// 'pViewContainingPopup' is a pointer to the GtkTextView that was active
// when the popup menu was opened.
//

static GtkTextView *pViewContainingPopup;

//------------------------------------------------------------------------
//
static char *strReverse(const char *start, char *dest, int nChars)
{
  *dest-- = '\0';
  while (nChars--)
    *dest-- = *start++;
  dest++;
  return dest;
}

//========================================================================

class SourceXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value) override
  {
    SourceWindow *sbaw = static_cast<SourceWindow *>(parent_window);

    if (sbaw->bSourceLoaded())
      sbaw->SetPC(new_value);
  }

  void Remove() override {}
};


#define BP_PIXEL_SIZE        10
#define PC_PIXEL_SIZE        10
#define MARGIN_WIDTH    (PC_PIXEL_SIZE + BP_PIXEL_SIZE)
#define PC_START  (MARGIN_WIDTH - PC_PIXEL_SIZE)
#define BP_START  (MARGIN_WIDTH - PC_PIXEL_SIZE-BP_PIXEL_SIZE)


/* This function is taken from gtk+/tests/testtext.c */
static void
gtk_source_view_get_lines(GtkTextView  *text_view,
  gint first_y,
  gint last_y,
  std::vector<gint> &buffer_coords,
  std::vector<gint> &numbers)
{
  GtkTextIter iter;
  gint last_line_num = -1;

  buffer_coords.clear();
  numbers.clear();

  /* Get iter at first y */
  gtk_text_view_get_line_at_y(text_view, &iter, first_y, nullptr);

  /* For each iter, get its location and add it to the arrays.
  * Stop when we pass last_y
  */
  while (!gtk_text_iter_is_end(&iter))
  {
    gint y, height;

    gtk_text_view_get_line_yrange(text_view, &iter, &y, &height);

    last_line_num = gtk_text_iter_get_line(&iter);

    buffer_coords.push_back(y);
    numbers.push_back(last_line_num);

    if ((y + height) >= last_y)
      break;

    gtk_text_iter_forward_line(&iter);
  }

  if (gtk_text_iter_is_end(&iter))
  {
    gint y, height;
    gint line_num;

    gtk_text_view_get_line_yrange(text_view, &iter, &y, &height);

    line_num = gtk_text_iter_get_line(&iter);

    if (line_num != last_line_num)
    {
      buffer_coords.push_back(y);
      numbers.push_back(line_num);
    }
  }
}


//------------------------------------------------------------------------
//
gboolean
NSourcePage::KeyPressHandler(GtkTextView *pView, GdkEventKey *key,
  NSourcePage *page)
{
  guint modifiers = gtk_accelerator_get_default_mod_mask();

  if ((key->state & modifiers) != 0) {
    return FALSE;
  }

  GtkTextBuffer *pBuffer = gtk_text_view_get_buffer(pView);
  GtkTextMark *pMark = gtk_text_buffer_get_insert(pBuffer);
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_mark(pBuffer, &iter, pMark);
  int line = gtk_text_iter_get_line(&iter);

  Dprintf(("Received key press for view. line=%d page%p\n", line, page));

  switch (key->keyval) {
  case 'b':
  case 'B':
    page->m_Parent->toggleBreak(page, line);
    break;

  default:
    return FALSE;
  }

  return TRUE;
}

gint
NSourcePage::ViewExposeEventHandler(GtkTextView *pView, GdkEventExpose *pEvent,
  NSourcePage *pPage)
{
  if (pEvent->window == gtk_text_view_get_window(pView,
    GTK_TEXT_WINDOW_LEFT))
  {
    //gtk_source_view_paint_margin (view, event);
    //event_handled = TRUE;
    Dprintf(("Expose event for view margin %p\n", pSW));

    gint y1 = pEvent->area.y;
    gint y2 = y1 + pEvent->area.height;

    gtk_text_view_window_to_buffer_coords(pView,
      GTK_TEXT_WINDOW_LEFT,
      0,
      y1,
      nullptr,
      &y1);

    gtk_text_view_window_to_buffer_coords(pView,
      GTK_TEXT_WINDOW_LEFT,
      0,
      y2,
      nullptr,
      &y2);

    pPage->updateMargin(y1,y2);

  }
  else {
    Dprintf(("Expose event for view %p\n",pSW));
  }
  return FALSE;
}

//------------------------------------------------------------------------
//
gboolean
SourceWindow::KeyPressHandler(GtkWidget *,
          GdkEventKey *key,
          SourceWindow *pSW)
{
  if (!pSW || !key)
    return FALSE;

  guint modifiers = gtk_accelerator_get_default_mod_mask();
  if ((key->state & modifiers) == GDK_CONTROL_MASK && key->keyval == 'f') {
    NSourcePage *page = pSW->pages[pSW->m_currentPage];
    if (!page)
        return FALSE;
    pViewContainingPopup = page->getView();
    pSW->findText();
    return TRUE;
  }

  if ((key->state & modifiers) != 0) {
    return FALSE;
  }

  switch (key->keyval) {
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    pSW->step(key->keyval - '0');
    break;
  case 's':
  case 'S':
  case GDK_KEY_F7:
    pSW->step();
    break;
  case 'o':
  case 'O':
  case GDK_KEY_F8:
    pSW->step_over();
    break;
  case 'r':
  case 'R':
  case GDK_KEY_F9:
    pSW->run();
    break;
  case 'f':
  case 'F':
    pSW->finish();
    break;
  case GDK_KEY_Escape:
    pSW->stop();
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

typedef enum {
  MENU_FIND_TEXT,
  MENU_FIND_PC,
  MENU_MOVE_PC,
  MENU_RUN_HERE,
  MENU_BP_HERE,
  MENU_SELECT_SYMBOL,
  MENU_STEP,
  MENU_STEP_OVER,
  MENU_RUN,
  MENU_STOP,
  MENU_FINISH,
  MENU_RESET,
  MENU_PROFILE_START_HERE,
  MENU_PROFILE_STOP_HERE,
  MENU_ADD_TO_WATCH,
} menu_id;


typedef struct _menu_item {
  const char *name;
  menu_id id;
} menu_item;

static const menu_item menu_items[] = {
  {"Find PC",         MENU_FIND_PC},
  {"Run to here",     MENU_RUN_HERE},
  {"Move PC here",    MENU_MOVE_PC},
  {"Breakpoint here", MENU_BP_HERE},
  {"Profile start here", MENU_PROFILE_START_HERE},
  {"Profile stop here", MENU_PROFILE_STOP_HERE},
  {"Add to watch",    MENU_ADD_TO_WATCH},
  {"Find text...",    MENU_FIND_TEXT},
};

static const menu_item submenu_items[] = {
  {"Step",            MENU_STEP},
  {"Step Over",       MENU_STEP_OVER},
  {"Run",             MENU_RUN},
  {"Stop",            MENU_STOP},
  {"Reset",           MENU_RESET},
  {"Finish",          MENU_FINISH},
};


//------------------------------------------------------------------------
// ButtonPressHandler
// Event handler for text view mouse clicks.
gint
NSourcePage::ButtonPressHandler(GtkTextView *pView, GdkEventButton *pButton,
  NSourcePage *pPage)
{
  if (pButton->window == gtk_text_view_get_window(pView,
    GTK_TEXT_WINDOW_LEFT))
  {
    // Margin
    gint y;

    gtk_text_view_window_to_buffer_coords(pView,
      GTK_TEXT_WINDOW_LEFT,
      (gint) pButton->x,
      (gint) pButton->y,
      nullptr,
      &y);
    GtkTextIter iter;

    gtk_text_view_get_line_at_y(pView, &iter, y, nullptr);
    gint line = gtk_text_iter_get_line(&iter);
    pPage->m_Parent->toggleBreak(pPage, line);

  } else {
    // Text (i.e. not the margin
    if (pButton->button == 3 && aPopupMenu && GTK_IS_TEXT_VIEW(pView)) {
      GtkTextIter iter;
      gint y;

      pViewContainingPopup = pView;
      gtk_text_view_window_to_buffer_coords(pView,
        GTK_TEXT_WINDOW_LEFT,
        (gint) pButton->x,
        (gint) pButton->y,
        nullptr,
        &y);

      gtk_text_view_get_line_at_y(pView, &iter, y, nullptr);
      pPage->getParent()->m_LineAtButtonClick = gtk_text_iter_get_line(&iter);

      gtk_menu_popup(GTK_MENU(aPopupMenu), nullptr, nullptr, nullptr, nullptr,
        3, pButton->time);

      GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(pView);
      gboolean selected = gtk_text_buffer_get_has_selection(text_buffer);

      // Move cursor if no text is selected
      if (!selected) {
        gtk_text_buffer_place_cursor(text_buffer, &iter);
      }
      return TRUE;
    }

  }

  return FALSE;
}

//========================================================================
// Helper functions for parsing
static int isString(const char *cP)
{
  int i = 0;

  if (isalpha(*cP) || *cP == '_')
    while (isalnum(cP[i]) || cP[i] == '_')
      i++;
  return i;
}

static int isWhiteSpace(const char *cP)
{
  int i = 0;

  while (cP[i] == ' ' || cP[i] == '\t')
    i++;
  return i;
}

static int isHexNumber(const char *c)
{
  const char *start = c;

  if (*c == '0') {
    ++c;
    if (*c == 'x' || *c == 'X')
      ++c;
    else
      return 1;
  } else if (*c == '$') {
    ++c;
  } else if (*c == 'H') {
    ++c;
    if (*c == '\'')
      ++c;
    else
      return 0;
  }

  if (!isxdigit(*c))
    return 0;

  while (isxdigit(*c)) {
    ++c;
  }
  return c - start;
}

static int isNumber(const char *cP)
{
  int i = isHexNumber(cP);
  if (!i)
    while (isdigit(cP[i]))
      i++;
  return i;
}

static bool isEnd(const char c)
{
  return c == '\n' || c == 0;
}

static int isComment(const char *cP)
{
  int i = (*cP == ';') ? 1 : 0;
  if (i)
    while (!isEnd(cP[i]))
      i++;
  return i;
}

//------------------------------------------------------------------------
class SearchDialog
{
public:
  SearchDialog();
  void Show(SourceWindow *);
  bool bDirection();
  bool bCase();

private:
  int  m_iStart;

  GtkWidget   *m_Window;           // The Search Dialog Window
  GtkWidget   *m_Entry;            // Widget that holds the search text.
  GtkWidget   *m_BackButton;       //
  GtkWidget   *m_CaseButton;       //

  SourceWindow *m_pSourceWindow; // The last source window that requested a search.

  void find(const char *);

  static void response(GtkDialog *dialog, gint response, SearchDialog *sd);
  static void activate(GtkEntry *entry, SearchDialog *sd);
  static void icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos,
  GdkEvent *event, gpointer user_data);
};

//------------------------------------------------------------------------
SearchDialog::SearchDialog()
  : m_iStart(0), m_pSourceWindow(nullptr)
{
  m_Window = gtk_dialog_new_with_buttons(
    "Find", nullptr,
    GtkDialogFlags(0),
    "_Find", 1,
    "_Close", GTK_RESPONSE_CLOSE,
    nullptr);

  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(m_Window));
  gtk_dialog_set_default_response(GTK_DIALOG(m_Window), 1);

  g_signal_connect(m_Window, "response", G_CALLBACK(response), this);
  g_signal_connect_swapped(m_Window,
    "delete_event", G_CALLBACK (gtk_widget_hide),
    GTK_OBJECT(m_Window));

  GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(content_area), hbox,
    FALSE, TRUE, 0);
  GtkWidget *label = gtk_label_new("Find:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  m_Entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), m_Entry, TRUE, TRUE, 0);
  gtk_widget_grab_focus(m_Entry);
  gtk_entry_set_icon_from_stock(GTK_ENTRY(m_Entry), GTK_ENTRY_ICON_PRIMARY,
    GTK_STOCK_FIND);
  gtk_entry_set_icon_from_stock(GTK_ENTRY(m_Entry), GTK_ENTRY_ICON_SECONDARY,
    GTK_STOCK_CLEAR);
  gtk_entry_set_icon_activatable(GTK_ENTRY(m_Entry), GTK_ENTRY_ICON_SECONDARY,
    TRUE);
  gtk_entry_set_icon_sensitive(GTK_ENTRY(m_Entry), GTK_ENTRY_ICON_SECONDARY,
    TRUE);
  gtk_entry_set_icon_tooltip_text(GTK_ENTRY(m_Entry), GTK_ENTRY_ICON_SECONDARY,
    "Clear text");
  g_signal_connect(m_Entry, "activate", G_CALLBACK(activate), this);
  g_signal_connect(m_Entry, "icon-press", G_CALLBACK(icon_press), nullptr);

  hbox = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(content_area), hbox, FALSE, TRUE, 0);
  m_CaseButton = gtk_check_button_new_with_label("Case Sensitive");
  gtk_box_pack_start(GTK_BOX(hbox), m_CaseButton, FALSE, FALSE, 0);
  m_BackButton = gtk_check_button_new_with_label("Find Backwards");
  gtk_box_pack_start(GTK_BOX(hbox), m_BackButton, FALSE, FALSE, 0);
}

bool SearchDialog::bDirection()
{
  return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_BackButton)) == TRUE;
}

bool SearchDialog::bCase()
{
  return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_CaseButton)) == TRUE;
}

void SearchDialog::find(const char *cpPattern)
{
  if (m_pSourceWindow)
    m_iStart = m_pSourceWindow->findText(cpPattern,m_iStart,!bDirection(), bCase());
}

void SearchDialog::response(GtkDialog *dialog, gint response, SearchDialog *sd)
{
  switch (response) {
  case 1:
    {
      const char *p = gtk_entry_get_text(GTK_ENTRY(sd->m_Entry));
      sd->find(p);
    }
    break;
  default:
    gtk_widget_hide(GTK_WIDGET(dialog));
  }
}

void SearchDialog::activate(GtkEntry *entry, SearchDialog *sd)
{
  const char *p = gtk_entry_get_text(entry);
  sd->find(p);
}

void SearchDialog::icon_press(GtkEntry *entry, GtkEntryIconPosition ,
  GdkEvent *, gpointer)
{
  gtk_entry_set_text(entry, "");
}

void SearchDialog::Show(SourceWindow *pSourceWindow)
{
  m_pSourceWindow = pSourceWindow;
  m_iStart = 0;

  gtk_widget_show_all(m_Window);
}

//========================================================================
//========================================================================
SourcePageMargin::SourcePageMargin()
: m_bShowLineNumbers(true), m_bShowAddresses(false), m_bShowOpcodes(true)
{
}

//========================================================================
//========================================================================
SourceBuffer::SourceBuffer(GtkTextTagTable *pTagTable, FileContext *pFC,
                           SourceBrowserParent_Window *pParent)
  : m_pParent(pParent), m_pFC(pFC), m_bParsed(false)
{
  assert(pTagTable);
  assert(pParent);
  m_buffer = gtk_text_buffer_new(pTagTable);
}

// Addtag range applies the tag state to a range of text in the buffer
// using a given text style (i.e. the style contains a gtkTextTag)
void SourceBuffer::addTagRange(const char *pStyle,
                               int start_index, int end_index)
{
  if (!pStyle)
    return;

  GtkTextIter    start;
  GtkTextIter    end;
  gtk_text_buffer_get_iter_at_offset(m_buffer, &start, start_index);
  gtk_text_buffer_get_iter_at_offset(m_buffer, &end, end_index);

  gtk_text_buffer_apply_tag_by_name(m_buffer, pStyle, &start, &end);
}

//------------------------------------------------------------------------
bool SourceBuffer::IsParsed()
{
  return m_bParsed;
}

//------------------------------------------------------------------------
void SourceBuffer::parse()
{
  if (IsParsed() || !m_pParent || !m_pFC)
    return;

  Dprintf(("parsing source buffer %s\n", m_pFC->name().c_str()));
  m_pParent->parseSource(this, m_pFC);
  m_bParsed = true;
}

GtkTextBuffer *SourceBuffer::getBuffer()
{
  parse();
  return m_buffer;
}

//------------------------------------------------------------------------
SourceWindow::SourceWindow(GUI_Processor *pgp,
  SourceBrowserParent_Window *pParent, bool bUseConfig,
  const char *newName)
  : GUI_Object(newName ? newName : "source_browser"),
    m_bLoadSource(false), m_bSourceLoaded(false),
    m_LineAtButtonClick(-1), pma(nullptr), status_bar(nullptr),
    last_simulation_mode(eSM_INITIAL), stPSearchDialog(nullptr), m_Notebook(nullptr),
    m_pParent(pParent)
{
  Dprintf(("Constructor \n"));

  gp = pgp;

  mProgramCounter.bIsActive = false;

  if (bUseConfig) {
    if (enabled)
      Build();
  }
}

//------------------------------------------------------------------------
void SourceWindow::step(int n)
{
  if (pma)
    pma->step(n);
}

//------------------------------------------------------------------------
void SourceWindow::step_over()
{
  if (pma)
    pma->step_over();
}

//------------------------------------------------------------------------
void SourceWindow::stop()
{
  if (pma)
    pma->stop();
}

//------------------------------------------------------------------------
void SourceWindow::run()
{
  // if (pma)
  //  pma->run();
  get_interface().start_simulation();
}

//------------------------------------------------------------------------
void SourceWindow::finish()
{
  if (pma)
    pma->finish();
}

//------------------------------------------------------------------------
void SourceWindow::reset()
{
  if (gp && gp->cpu)
    gp->cpu->reset(POR_RESET);
}

//------------------------------------------------------------------------
// toggleBreak
//
//
void SourceWindow::toggleBreak(NSourcePage *pPage, int line)
{
  if (pma && pPage) {
    int address = pma->find_address_from_line(pPage->getFC(), line + 1);
    if (address >= 0)
      pma->toggle_break_at_address(address);

  }
}

//------------------------------------------------------------------------
// movePC
//
//
void SourceWindow::movePC(int)
{
}

//------------------------------------------------------------------------
void SourceWindow::findText()
{
  if (!stPSearchDialog)
    stPSearchDialog = new SearchDialog();

  stPSearchDialog->Show(this);
}


//------------------------------------------------------------------------
// strcasestr is a non standard function
//
#if defined _MSC_VER || defined __MINGW32__ || defined __CYGWIN__
char *strcasestr(const char *searchee, const char *lookfor)
{
  if (*searchee == '\0')
  {
    if (*lookfor)
      return nullptr;
    return (char *) searchee;
  }

  while (*searchee != '\0')
  {
    size_t i;

    for (i = 0; ; ++i)
    {
      if (lookfor[i] == '\0')
        return (char *) searchee;

      if (tolower(lookfor[i]) != tolower(searchee[i]))
        break;
    }
    searchee++;
  }

  return nullptr;
}
#endif

//------------------------------------------------------------------------
// findText
//
//  Search for the pattern 'pText' in the source window.
//  if bDirection is true then search forward.
int SourceWindow::findText(const char *pText, int start, bool bDirection, bool bCase)
{
  if (!pText)
    return 0;

  unsigned int patternLen = strlen(pText);
  char buff[1024];
  patternLen = (patternLen < sizeof(buff)) ? patternLen : sizeof(buff) -1;
  const char *pattern = bDirection ? pText :
  strReverse(pText, &buff[patternLen], patternLen);

  //printf("findText %s view:%p\n",pattern,pViewContainingPopup);

  NSourcePage *pPage = pages[gtk_notebook_get_current_page(GTK_NOTEBOOK(m_Notebook))];

  if (!pPage)
    return 0;

  GtkTextIter iStart;
  GtkTextIter iEnd;
  int line = 0;
  int offset = 0;

  int totalLines = gtk_text_buffer_get_line_count(pPage->buffer());
  if (!start) {
    if (bDirection) {
      gtk_text_buffer_get_start_iter(pPage->buffer(), &iStart);
      gtk_text_buffer_get_iter_at_line(pPage->buffer(), &iEnd, line + 1);
    } else {

      gtk_text_buffer_get_end_iter(pPage->buffer(), &iEnd);
      gtk_text_buffer_get_end_iter(pPage->buffer(), &iStart);
      gtk_text_iter_backward_line(&iStart);
      line = totalLines - 2;
    }

  } else {
    gtk_text_buffer_get_iter_at_offset(pPage->buffer(), &iStart, start);
    line = gtk_text_iter_get_line (&iStart);

    if (bDirection) {
      if (line >= totalLines) {
        line = 0;
        gtk_text_buffer_get_iter_at_offset(pPage->buffer(), &iStart, 0);
      }
    } else {
      if (line <= 0) {
        line = totalLines - 1;
        gtk_text_buffer_get_iter_at_line(pPage->buffer(), &iStart, line--);
      }
    }

    gtk_text_buffer_get_iter_at_line(pPage->buffer(), &iEnd, line);

    offset = start - gtk_text_iter_get_offset(&iEnd);

    gtk_text_buffer_get_iter_at_line(pPage->buffer(), &iEnd, line + 1);
  }


  while (totalLines--) {

    char *str = gtk_text_buffer_get_text(pPage->buffer(),
      &iStart, &iEnd, FALSE);
    unsigned int srcLen = strlen(str);

    const char *cpSource = str;
    char buffer2[1024];
    if (!bDirection) {
      srcLen = (srcLen < sizeof(buffer2)) ? srcLen : sizeof(buffer2)-1;
      cpSource = strReverse(cpSource, &buffer2[srcLen], srcLen);
    }

    const char *pFound = bCase ? strstr(cpSource, pattern) : strcasestr(cpSource, pattern);

    if (pFound) {
      int pos = bDirection ? (pFound - cpSource) : (srcLen - (pFound - cpSource));
      pos += offset;
      //printf("Found %s in %s starting at %s, pos=%d\n",pattern, str, pFound,pos);

      gtk_text_view_scroll_to_iter(pViewContainingPopup,
        &iStart,
        0.0,
        TRUE,
        0.0, 0.3);

      gtk_text_buffer_get_iter_at_line_offset(pPage->buffer(),
        &iStart,
        line, pos);
      gtk_text_buffer_get_iter_at_line_offset(pPage->buffer(),
        &iEnd,
        line,
        pos + (bDirection ? patternLen : -patternLen));

      gtk_text_buffer_select_range (pPage->buffer(),
        &iStart,
        &iEnd);

      g_free(str);
      return gtk_text_iter_get_offset(bDirection ? &iEnd : &iStart);
    }
    g_free(str);

    // Now we'll search whole lines.
    offset = 0;

    if (bDirection) {
      if (gtk_text_iter_forward_line(&iStart) == FALSE)
        return 0;
      gtk_text_iter_forward_line(&iEnd);
      line++;
    } else {
      if (gtk_text_iter_backward_line(&iStart) == FALSE)
        return gtk_text_buffer_get_char_count(pPage->buffer()) - 1;

      gtk_text_iter_backward_line(&iEnd);
      line--;
    }
  }

  printf("Did not find %s\n", pattern);

  return 0;
}

//------------------------------------------------------------------------
void SourceWindow::cb_notebook_switchpage(GtkNotebook *,
  gpointer, guint page_num, SourceWindow *pSW)
{
  pSW->switch_page_cb(page_num);
}

gint SourceWindow::switch_page_cb(guint newPage)
{
  Dprintf((" Switch page call back-- page=%d\n", newPage));
  if (m_currentPage != newPage) {
    m_currentPage = newPage;

    NSourcePage *pPage = pages[m_currentPage];

    if (!pPage || !gp->cpu->files[pPage->get_file_id()])
      return TRUE;
    if (gp->cpu->files[pPage->get_file_id()]->IsHLL())
      pma->set_hll_mode(ProgramMemoryAccess::HLL_MODE);
    else
      pma->set_hll_mode(ProgramMemoryAccess::ASM_MODE);
    pPage->invalidateView();

  }
  return TRUE;
}

//------------------------------------------------------------------------
static Register *findRegister(std::string text)
{
  Register *pReg = dynamic_cast<Register *>(globalSymbolTable().find(text));
  if (!pReg)
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text + std::string("_")));
  if (!pReg) {
    (void) toupper(text);
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text));
  }
  if (!pReg)
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text + std::string("_")));
  if (!pReg) {
    transform(text.begin(), text.end(), text.begin(), ::tolower);
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text));
  }
  return pReg;
}

//------------------------------------------------------------------------

void
SourceWindow::PopupMenuHandler(GtkWidget *widget, gpointer data)
{
  SourceWindow *This = static_cast<SourceWindow *>(data);
  unsigned int address;

  SourceWindow *pSW = nullptr;
  NSourcePage *pPage = nullptr;

  // pViewContainingPopup is initialized when the view_button_press()
  // event handler is called. That function also initiates the event
  // that invokes this callback.

  if (!pViewContainingPopup) {
    printf("Warning popup without a textview\n");
  } else {

    pPage = This->pages[gtk_notebook_get_current_page(GTK_NOTEBOOK(This->m_Notebook))];
    pSW = pPage ? pPage->getParent() : nullptr;
  }

  if (!pSW) {
    printf("Warning (bug?): popup cannot be associate with any source\n");
    return;
  }

  switch (GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(widget), "item"))) {

  case MENU_FIND_TEXT:
    pSW->findText();
    break;

  case MENU_FIND_PC:
    address=pSW->pma->get_PC();
    pSW->SetPC(address);
    break;

  case MENU_MOVE_PC:
    if (-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
        pPage->get_file_id(), pSW->m_LineAtButtonClick + 1);
      if (address != INVALID_VALUE) {
        pSW->pma->set_PC(address);
        pSW->SetPC(pSW->pma->get_PC());
      }
    }
    break;

  case MENU_RUN_HERE:
    if (-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
          pPage->get_file_id(), pSW->m_LineAtButtonClick + 1);
      if (address != INVALID_VALUE) {
        pSW->gp->cpu->run_to_address(address);
        pSW->SetPC(pSW->pma->get_PC());     // RP - update GUI after running
        get_interface().simulation_has_stopped();   // RP - update GUI after running
      }
    }
    break;

  case MENU_BP_HERE:
    if (-1 != pSW->m_LineAtButtonClick) {
      pSW->toggleBreak(pPage, pSW->m_LineAtButtonClick);
    }
    break;
  case MENU_PROFILE_START_HERE:
    if (-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
        pPage->get_file_id(), pSW->m_LineAtButtonClick + 1);

      pSW->gp->profile_window->StartExe(address);
    }
    break;

  case MENU_PROFILE_STOP_HERE:
    if (-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
        pPage->get_file_id(), pSW->m_LineAtButtonClick + 1);

      pSW->gp->profile_window->StopExe(address);
    }
    break;

  case MENU_SELECT_SYMBOL:
  case MENU_ADD_TO_WATCH:
  {
    GtkTextBuffer *pBuffer = pPage->buffer();
    GtkTextIter itBegin, itEnd;
    if (gtk_text_buffer_get_selection_bounds(pBuffer, &itBegin, &itEnd)) {
      gchar *text = gtk_text_buffer_get_text(pBuffer, &itBegin, &itEnd, FALSE);

      TrimWhiteSpaceFromString(text);

      if (text[0] != '\0') {
        Register *pReg = findRegister(std::string(text));

        if(!pReg) {
          GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(pSW->window),
          GTK_DIALOG_MODAL,
          GTK_MESSAGE_WARNING,
          GTK_BUTTONS_OK,
          "The symbol '%s' does not exist as a register symbol.\n"
          "Only register based symbols may be added to the Watch window.",
          text);
          gtk_dialog_run(GTK_DIALOG (dialog));
          gtk_widget_destroy(dialog);
        } else {
          pSW->gp->watch_window->Add(pReg);
        }

      }
      g_free(text);
    }
    }
    break;
  case MENU_STEP:
    pSW->step();
    break;
  case MENU_STEP_OVER:
    pSW->step_over();
    break;
  case MENU_RUN:
    pSW->run();
    break;
  case MENU_STOP:
    pSW->stop();
    break;
  case MENU_RESET:
    pSW->reset();
    break;
  case MENU_FINISH:
    pSW->finish();
    break;
  default:
    puts("Unhandled menuitem?");
    break;
  }

}


//------------------------------------------------------------------------
GtkWidget *
SourceWindow::BuildPopupMenu()
{
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *item;
  unsigned int i;

  menu = gtk_menu_new();
  for (i = 0; i < G_N_ELEMENTS(menu_items); ++i) {
    item = gtk_menu_item_new_with_label(menu_items[i].name);
    g_object_set_data(G_OBJECT(item), "item", GSIZE_TO_POINTER(menu_items[i].id));
    g_signal_connect(item, "activate",
      G_CALLBACK(PopupMenuHandler), this);

    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  submenu = gtk_menu_new();

  for (i = 0; i < G_N_ELEMENTS(submenu_items); ++i) {
    item = gtk_menu_item_new_with_label(submenu_items[i].name);
    g_object_set_data(G_OBJECT(item), "item", GSIZE_TO_POINTER(submenu_items[i].id));
    g_signal_connect(item, "activate",
      G_CALLBACK (PopupMenuHandler), this);

    gtk_widget_set_can_focus(item, TRUE);

    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
  }
  item = gtk_menu_item_new_with_label("Controls");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  gtk_widget_show(item);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

  return menu;
}

//------------------------------------------------------------------------
// Build
//
//
void SourceWindow::Build()
{
  Dprintf((" \n"));

  if (bIsBuilt)
    return;

  Dprintf((" \n"));

  g_signal_connect(window,"key_press_event",
    G_CALLBACK (KeyPressHandler),
    (gpointer) this);

  gtk_container_set_border_width(GTK_CONTAINER(window), 0);

  SetTitle();

  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  m_Notebook = gtk_notebook_new();
  m_currentPage = 0;
  g_signal_connect(m_Notebook, "switch-page",
    G_CALLBACK(cb_notebook_switchpage),
    (gpointer) this);

  gtk_notebook_set_tab_pos((GtkNotebook*)m_Notebook, GTK_POS_LEFT);
  gtk_notebook_set_scrollable((GtkNotebook*)m_Notebook, TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), m_Notebook, TRUE, TRUE, 0);

  status_bar = new StatusBar_Window(vbox);

  gtk_widget_show_all(window);
  gtk_widget_show_all(vbox);
  gtk_widget_show_all(m_Notebook);

  aPopupMenu = BuildPopupMenu();

  bIsBuilt = true;

  menu = "/menu/Windows/Source";
  gtk_window_set_title(GTK_WINDOW(window), "Source Browser");
  UpdateMenuItem();
  if (m_bLoadSource) {
    Dprintf((" \n"));

    NewSource(gp);
  }

}

//------------------------------------------------------------------------
//
void SourceWindow::SetTitle()
{
  if (!gp || !gp->cpu || !pma)
    return;

  if (last_simulation_mode != eSM_INITIAL &&
    ((last_simulation_mode == eSM_RUNNING &&
    gp->cpu->simulation_mode == eSM_RUNNING) ||
    (last_simulation_mode != eSM_RUNNING &&
    gp->cpu->simulation_mode != eSM_RUNNING)) &&
    sLastPmaName == pma->name()) {
      return;
  }

  last_simulation_mode = gp->cpu->simulation_mode;
  const char * sStatus;
  if (gp->cpu->simulation_mode == eSM_RUNNING)
    sStatus = "Run";
  else // if (gp->cpu->simulation_mode == eSM_STOPPED)
    sStatus = "Stopped";
  char buffer[256];
  g_snprintf(buffer, sizeof(buffer), "Source Browser: [%s] %s", sStatus, pma != nullptr ?
    pma->name().c_str() : "" );
  sLastPmaName = pma->name();
  gtk_window_set_title(GTK_WINDOW(window), buffer);
}


//------------------------------------------------------------------------
//
void SourceWindow::set_pma(ProgramMemoryAccess *new_pma)
{
  Dprintf((" \n"));

  pma = new_pma;

  if (window && pma) {
    SetTitle();
  }

  if (status_bar)
    status_bar->NewProcessor(gp, pma);
}

void SourceWindow::SelectAddress(int)
{
  Dprintf((" \n"));
}

void SourceWindow::SelectAddress(Value *)
{
  Dprintf((" \n"));
}

//------------------------------------------------------------------------
// Update
//
// Called whenever the source window needs to be updated (like after break points).
void SourceWindow::Update()
{
  Dprintf((" \n"));
  if (!window || !enabled)
    return;

  if (m_Notebook &&
    ((gtk_notebook_get_show_tabs(GTK_NOTEBOOK(m_Notebook)) == false
    && m_pParent->getTabPosition() < 0) ||
    (m_pParent->getTabPosition() != gtk_notebook_get_tab_pos(GTK_NOTEBOOK(m_Notebook))))) {

      if (m_pParent->getTabPosition() < 0) {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook), FALSE);
      } else {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook), TRUE);
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(m_Notebook), (GtkPositionType) m_pParent->getTabPosition());
      }
    }

    if (m_Notebook) {
      gint currPage = gtk_notebook_get_current_page(GTK_NOTEBOOK(m_Notebook));

      if (currPage >= 0) {
        pages[currPage]->setFont(m_pParent->getFont());
      }
    }

    if (!gp || !pma || ! window)
      return;

    SetTitle();
    SetPC(pma->get_PC());
    if (status_bar)
      status_bar->Update();
}

//------------------------------------------------------------------------
void SourceWindow::UpdateLine(int address)
{
  assert(address >= 0);

  Dprintf((" UpdateLine at address=%d\n", address));

  if (!bSourceLoaded() || !pma || !enabled)
    return;

  gint currPage = gtk_notebook_get_current_page(GTK_NOTEBOOK(m_Notebook));
  if (currPage < 0)
      return;

  NSourcePage *pPage = pages[currPage];

  if (!pPage)
    return;

  int line = (pPage->getFC()->IsList()) ?
    pma->getFromAddress(address)->get_lst_line() :
  pma->get_src_line(address);

  //int line  = pma->get_src_line(address) - 1;

  line -= 1;

  GtkTextIter iBegin;
  gtk_text_buffer_get_iter_at_line
    (gtk_text_view_get_buffer(pPage->getView()),
    &iBegin,
    line);

  int y, h;

  gtk_text_view_get_line_yrange (pPage->getView(),
    &iBegin,
    &y,
    &h);
  if (pPage->get_margin_width()) {

    GdkRectangle vRect;

    gtk_text_view_buffer_to_window_coords
      (pPage->getView(),
      GTK_TEXT_WINDOW_LEFT,
      0,
      y,
      nullptr,
      &y);

    vRect.x = 0;
    vRect.y = y;
    vRect.width = pPage->get_margin_width();
    vRect.height = h;

    Dprintf((" UpdateLine line=%d invalidating region %d,%d  %d,%d\n",line,0,y,vRect.width,h));
    // Send an expose event to repaint the whole margin
    gdk_window_invalidate_rect
      (gtk_text_view_get_window (pPage->getView(), GTK_TEXT_WINDOW_LEFT), &vRect, TRUE);
  }
}

//------------------------------------------------------------------------
//
int SourceWindow::getPCLine(int page)
{
  if (mProgramCounter.bIsActive && mProgramCounter.page == page)
    return mProgramCounter.line;

  NSourcePage *pPage = pages[page];

  return (pPage->getFC()->IsList()) ?
    pma->getFromAddress(pma->get_PC())->get_lst_line() :
  pma->get_src_line(pma->get_PC());
}

int SourceWindow::getAddress(NSourcePage *pPage, int line)
{
  return pma->find_address_from_line(pPage->getFC(),line);
}

bool SourceWindow::bAddressHasBreak(int address)
{
  return address >= 0 && pma->address_has_break(address);
}

int SourceWindow::getOpcode(int address)
{
  return (address >= 0) ? gp->cpu->pma->get_opcode(address) : address;
}

//------------------------------------------------------------------------
bool SourcePageMargin::formatMargin(char *str, int len, int line, int addr, int opcode, bool bBreak)
{
  if (str) {
    int pos = 0;
    int npos = 0;

    *str = '\0';

    npos = bBreak ? g_snprintf(&str[pos], len, "<span foreground=\"red\"><b>") : 0;
    pos += npos;
    len -= npos;

    npos = m_bShowLineNumbers ? g_snprintf(&str[pos], len, "%d",line) : 0;
    pos += npos;
    len -= npos;

    npos = (m_bShowAddresses && addr >= 0) ? g_snprintf(&str[pos], len, " %04X", addr) : 0;
    pos += npos;
    len -= npos;


    npos = (m_bShowOpcodes && opcode >= 0) ?
      g_snprintf(&str[pos], len, "%c%04X  ", m_bShowAddresses ? ':' : ' ', opcode)
      : 0;
    pos += npos;
    len -= npos;

    pos += bBreak ? g_snprintf(&str[pos], len, "</b></span>") : 0;

    return pos != 0;
  }

  return false;
}

//========================================================================
NSourcePage::NSourcePage(SourceWindow *pParent, SourceBuffer *pBuffer,
  int file_id, GtkWidget *pContainer)

  : m_pBuffer(pBuffer), m_Parent(pParent), m_fileid(file_id), m_marginWidth(0)
{
  if (!m_pBuffer || !pContainer || !m_Parent)
    return;

  m_pBuffer->parse();

  m_view = (GtkTextView *)gtk_text_view_new_with_buffer(m_pBuffer->getBuffer());
  Dprintf(("NSourcePage::setSource() - view=%p\n", m_view));
  gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(m_view),
    GTK_TEXT_WINDOW_LEFT,
    MARGIN_WIDTH);

  g_signal_connect(GTK_OBJECT(m_view), "key_press_event",
    G_CALLBACK(KeyPressHandler), this);

  g_signal_connect(GTK_OBJECT(m_view), "button_press_event",
    G_CALLBACK(ButtonPressHandler), this);

  g_signal_connect(GTK_OBJECT(m_view), "expose_event",
    G_CALLBACK(ViewExposeEventHandler), this);

  GtkWidget *pSW = gtk_scrolled_window_new(0, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (pSW),
    GTK_POLICY_AUTOMATIC,
    GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(pContainer), pSW);
  gtk_container_add(GTK_CONTAINER(pSW), GTK_WIDGET(m_view));

  gtk_text_view_set_wrap_mode(m_view, GTK_WRAP_NONE);
  gtk_text_view_set_editable(m_view, FALSE);

  setFont(m_Parent->getFont());

  gtk_widget_show_all(pContainer);
}

void NSourcePage::invalidateView()
{
  if (m_view) {
    GdkRectangle vRect;

    vRect.x = 0;
    vRect.y = 0;
    vRect.width = 100;
    vRect.height = 100;
    gdk_window_invalidate_rect
      (gtk_text_view_get_window(m_view,
                                 GTK_TEXT_WINDOW_LEFT),
                                 &vRect,
                                 TRUE);
  }
}

SourceWindow *NSourcePage::getParent()
{
  return m_Parent;
}

GtkTextBuffer *NSourcePage::buffer()
{
  return m_pBuffer ? m_pBuffer->getBuffer() : nullptr;
}

FileContext * NSourcePage::getFC()
{
  return m_pBuffer ? m_pBuffer->m_pFC : nullptr;
}

//------------------------------------------------------------------------
GtkTextView *NSourcePage::getView()
{
  return m_view;
}

//------------------------------------------------------------------------
void NSourcePage::updateMargin(int y1, int y2)
{
  Dprintf((" updateMargin y1=%d y2=%d\n", y1, y2));

  GtkTextView * text_view = m_view;
  std::vector<gint> numbers;
  std::vector<gint> pixels;

  int PCline = m_Parent->getPCLine(m_fileid);

  GdkWindow *win = gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_LEFT);

  /* get the line numbers and y coordinates. */
  gtk_source_view_get_lines(text_view,
    y1,
    y2,
    pixels,
    numbers);

  /* set size. */
  gchar str [256];
  PangoLayout *layout = nullptr;
  gint text_width = 0;
  FileContext *pFC = getFC();
  gint addr_opcode = (pFC && !pFC->IsList()) ? 0x9999 : -1;
  if ( m_Parent->margin().formatMargin(str, sizeof(str),
    MAX (99, gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(text_view))),
    addr_opcode, addr_opcode, false) ) {

      layout = gtk_widget_create_pango_layout(GTK_WIDGET(text_view), str);

      pango_layout_get_pixel_size(layout, &text_width, nullptr);
      text_width += 2;
    }

    m_marginWidth = text_width + MARGIN_WIDTH;
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_view),
      GTK_TEXT_WINDOW_LEFT,
      m_marginWidth);

    for (size_t i = 0; i < numbers.size(); ++i) {

      gint pos;
      gint line = numbers[i] + 1;

      gtk_text_view_buffer_to_window_coords (text_view,
        GTK_TEXT_WINDOW_LEFT,
        0,
        pixels[i],
        nullptr,
        &pos);

      int address    = pFC && !pFC->IsList() ? m_Parent->getAddress(this, line) : - 1;
      int opcode     = pFC && !pFC->IsList() && !pFC->IsHLL() ? m_Parent->getOpcode(address) : -1;
      bool bHasBreak = m_Parent->bAddressHasBreak(m_Parent->getAddress(this, line));


      if (layout) {

        if ( m_Parent->margin().formatMargin(str, sizeof(str),
          line, address, opcode, bHasBreak)) {

            pango_layout_set_markup(layout, str, -1);

            gtk_paint_layout(gtk_widget_get_style(GTK_WIDGET(text_view)),
              win,
              GTK_STATE_NORMAL,
              FALSE,
              nullptr,
              GTK_WIDGET (text_view),
              nullptr,
              2, //text_width + 2,
              pos,
              layout);

          }

      }


      if (line == PCline) {
        gtk_paint_arrow(
          gtk_widget_get_style(GTK_WIDGET(text_view)),
          win,
          GTK_STATE_NORMAL,
          GTK_SHADOW_OUT,    // GtkShadowType shadow_type,
          nullptr,
          GTK_WIDGET(text_view),
          nullptr,
          GTK_ARROW_RIGHT,   //GtkArrowType arrow_type,
          TRUE,              //gboolean fill,
          text_width + PC_START, pos, PC_PIXEL_SIZE, 15);
        Dprintf((" updating PC at line %d\n", line));
      }

      if (m_Parent->getAddress(this,line) >= 0) {
        // There is code associated with this line.

        gtk_paint_diamond(
          gtk_widget_get_style(GTK_WIDGET(text_view)),
          win,
          GTK_STATE_NORMAL,
          bHasBreak ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
          nullptr,
          GTK_WIDGET(text_view),
          nullptr,
          text_width + BP_START,
          pos,
          BP_PIXEL_SIZE,
          BP_PIXEL_SIZE);
      }
    }

    if (layout)
      g_object_unref(layout);
}

//------------------------------------------------------------------------
void NSourcePage::setFont(const char *cp_newFont)
{
  if (m_view && cp_newFont) {

    if (m_cpFont == cp_newFont)
      return;

    m_cpFont = cp_newFont;

    /* Change default font throughout the widget */
    PangoFontDescription *font_desc;
    font_desc = pango_font_description_from_string(m_cpFont.c_str());
    gtk_widget_modify_font(GTK_WIDGET(m_view), font_desc);
    pango_font_description_free(font_desc);

  }
}

//------------------------------------------------------------------------
// SetPC
//
// Highlight the line corresponding to the current program counter.
//

void SourceWindow::SetPC(int address)
{
  Dprintf((" \n"));

  if (!bSourceLoaded() || !pma)
    return;

  int currPage = m_Notebook ?
    gtk_notebook_get_current_page(GTK_NOTEBOOK(m_Notebook)) : -1;

  // Get the file id associated with the program counter address
  unsigned int sbawFileId  = pma->get_file_id(address);
  if (sbawFileId == 0xffffffff)
    return;

  int id = -1;
  int PCline = -1;

  if (currPage >= 0  && pages[currPage]->getFC()->IsList()) {
    // Don't automatically switch away from a page if it is a list file
    id = currPage;
    PCline = pma->getFromAddress(address)->get_lst_line();
  } else {
    std::map<int, NSourcePage *>::iterator i = pages.begin();
    for ( ; i != pages.end(); ++i) {
      if (i->second->get_file_id() == sbawFileId) {
        id = i->first;
        break;
      }
    }

    if (id < 0)
      return;   // page was not found.

    // Switch to the source browser page that contains the program counter.
    if (currPage != id)
      gtk_notebook_set_current_page(GTK_NOTEBOOK(m_Notebook), id);

    // Get the source line number associated with the program counter address.
    PCline = pma->get_src_line(address);
    if (PCline == (int)INVALID_VALUE)
      return;
    //PCline--;
  }

  bool bFirstUpdate = true;
  if (mProgramCounter.bIsActive) {
    bFirstUpdate=false;
  } else {
    while (gtk_events_pending())
      gtk_main_iteration();
  }

  mProgramCounter.page = id;
  mProgramCounter.line = PCline;

  // Get a pointer to text_view margin window.
  GdkWindow *win = gtk_text_view_get_window (pages[id]->getView(), GTK_TEXT_WINDOW_LEFT);
  GdkRectangle PCloc;

  mProgramCounter.bIsActive = true;
  mProgramCounter.pBuffer = pages[id]->buffer();
  gtk_text_buffer_get_iter_at_line(mProgramCounter.pBuffer,
    &mProgramCounter.iBegin,
    PCline);

  // Now we're going to check if the program counter is in view or not.

  // Get the program counter location
  gtk_text_view_get_iter_location (pages[id]->getView(),
    &mProgramCounter.iBegin,
    &PCloc);
  // Get the viewable region of the text buffer. The region is in buffer coordinates.
  GdkRectangle vRect;
  gtk_text_view_get_visible_rect  (pages[id]->getView(),
    &vRect);

  // Now normalize the program counter's location. If yloc is between
  // 0 and 1.0 then the program counter is viewable. If not, then we
  // we need to scroll the text view so that the program counter is
  // viewable.
  double yloc = (PCloc.y - vRect.y) / (double) (vRect.height);

  if ( yloc < 0.05  || yloc > 0.95 || bFirstUpdate) {
    gtk_text_view_scroll_to_iter(pages[id]->getView(),
      &mProgramCounter.iBegin,
      0.0,
      TRUE,
      0.0, 0.3);
    gtk_text_view_get_visible_rect(pages[id]->getView(), &vRect);

  }

  // If there is a margin, then invalidate it so gtk will go off and redraw it.
  if (pages[id]->get_margin_width()) {
    vRect.x = 0;
    vRect.y = 0;
    vRect.width = pages[id]->get_margin_width();
    // Send an expose event to repaint the whole margin
    gdk_window_invalidate_rect(win, &vRect, TRUE);
  }
}

void SourceWindow::CloseSource()
{
  Dprintf((" \n"));
}

SourcePageMargin &SourceWindow::margin()
{
  return m_pParent->margin();
}

const char *SourceWindow::getFont()
{
  return m_pParent->getFont();
}

void SourceWindow::NewSource(GUI_Processor *gp)
{
  Dprintf((" \n"));

  unsigned int address;

  if (!gp || !gp->cpu || !gp->cpu->pma)
    return;
  Dprintf((" \n"));

  Processor * pProc = gp->cpu;
  if (!enabled)
  {
    m_bLoadSource=true;
    return;
  }
  Dprintf((" \n"));

  if (!pma)
    pma = pProc->pma;

  CloseSource();

  m_bLoadSource = true;

  Dprintf(("NewSource\n"));

  /* Now create a cross-reference link that the
  * simulator can use to send information back to the gui
  */
  if (pProc->pc) {
    SourceXREF *cross_reference = new SourceXREF();
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = 0;

    pProc->pc->add_xref((gpointer) cross_reference);
    if(pProc->pc != pma->GetProgramCounter()) {
      pma->GetProgramCounter()->add_xref((gpointer) cross_reference);
    }
  }

  std::vector<SourceBuffer *>::iterator i = m_pParent->ppSourceBuffers.begin();
  std::vector<SourceBuffer *>::iterator iend = m_pParent->ppSourceBuffers.end();

  for ( ; i != iend; ++i) {
    AddPage(*i);
  }

  m_bSourceLoaded = 1;


  // update breakpoint widgets
  unsigned uPMMaxIndex = pProc->program_memory_size();
  for (unsigned int uPMIndex = 0; uPMIndex < uPMMaxIndex; uPMIndex++) {
    int addr = pProc->map_pm_index2address(uPMIndex);
    if (pma->address_has_break(addr))
      UpdateLine(addr);
  }

  address = pProc->pma->get_PC();
  if (address == INVALID_VALUE)
    puts("Warning, PC is invalid?");
  else
    SetPC(address);

  Dprintf((" Source is loaded\n"));
}


//------------------------------------------------------------------------
// AddPage
// Adds a page to the notebook, and returns notebook-id for that page.
//
int SourceWindow::AddPage(SourceBuffer *pSourceBuffer)
{
  if (pSourceBuffer && pSourceBuffer->m_pFC)
    return AddPage(pSourceBuffer,  pSourceBuffer->m_pFC->name());
  return -1;
}

int SourceWindow::AddPage(SourceBuffer *pSourceBuffer, const std::string &fName)
{
  if (!bIsBuilt || !pSourceBuffer)
    return -1;

  GtkWidget *label;

  std::string::size_type pos = fName.find_last_of("/\\");
  if (pos != std::string::npos)
    label = gtk_label_new(fName.substr(pos + 1).c_str());
  else
    label = gtk_label_new(fName.c_str());

  GtkWidget *pFrame = gtk_frame_new(nullptr);

  int id = gtk_notebook_append_page(GTK_NOTEBOOK(m_Notebook),pFrame,label);

  pages[id] = new NSourcePage(this, pSourceBuffer, id, pFrame);

  gtk_widget_show_all(pFrame);

  return id;
}

/*********************** gui message dialog *************************/

void gui_message(const char *message)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
    "", nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
    "_OK", GTK_RESPONSE_OK,
    nullptr
    );

  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label = gtk_label_new(message);
  gtk_container_add(GTK_CONTAINER(content_area), label);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

// gui question dialog
// modal dialog, asking a yes/no question
int gui_question(const char *question, const char *a, const char *b)
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
    "", nullptr, GTK_DIALOG_DESTROY_WITH_PARENT,
    a, TRUE,
    b, FALSE,
    nullptr
    );

  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label = gtk_label_new(question);
  gtk_container_add(GTK_CONTAINER(content_area), label);

  gtk_widget_show_all(dialog);
  int retval = gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);

  return retval;
}

void SourceBrowser_Window::set_pma(ProgramMemoryAccess *new_pma)
{
  pma = new_pma;

  if (window && pma) {
    SetTitle();
  }
}

//========================================================================
//
// SourceBrowserParent_Window
//
// Here is some experimental code that allows multiple source browser
// windows.

SourceBrowserParent_Window::SourceBrowserParent_Window(GUI_Processor *_gp)
  : GUI_Object("source_browser_parent")
{
  gp = _gp;

  pma = nullptr;
  m_TabType = GTK_POS_BOTTOM;
  mpTagTable = gtk_text_tag_table_new();

  const char *sName = "source_config";
  char *fg = nullptr;
  GdkColor color;
  GtkTextTag *tag;

  tag = gtk_text_tag_new("Label");
  gdk_color_parse(config_get_string(sName, "label_fg", &fg)
    ? fg : "orange" , &color);
  g_object_set(tag, "foreground-gdk", &color, nullptr);
  gtk_text_tag_table_add(mpTagTable, tag);

  tag = gtk_text_tag_new("Mnemonic");
  gdk_color_parse(config_get_string(sName, "mnemonic_fg", &fg)
    ? fg : "red" , &color);
  g_object_set(tag, "foreground-gdk", &color, nullptr);
  gtk_text_tag_table_add(mpTagTable, tag);

  tag = gtk_text_tag_new("Symbols");
  gdk_color_parse(config_get_string(sName, "symbol_fg", &fg)
    ? fg : "dark green" , &color);
  g_object_set(tag, "foreground-gdk", &color, nullptr);
  gtk_text_tag_table_add(mpTagTable, tag);

  tag = gtk_text_tag_new("Comments");
  gdk_color_parse(config_get_string(sName, "comment_fg", &fg)
    ? fg : "dim gray" , &color);
  g_object_set(tag, "foreground-gdk", &color, nullptr);
  gtk_text_tag_table_add(mpTagTable, tag);

  tag = gtk_text_tag_new("Constants");
  gdk_color_parse(config_get_string(sName, "constant_fg", &fg)
    ? fg : "blue" , &color);
  g_object_set(tag, "foreground-gdk", &color, nullptr);
  gtk_text_tag_table_add(mpTagTable, tag);

  if (!config_get_variable(sName, "tab_position", &m_TabType))
    m_TabType = GTK_POS_LEFT;

  int b = 1;
  config_get_variable(sName, "line_numbers", &b);
  margin().enableLineNumbers(b != 0);

  config_get_variable(sName, "addresses", &b);
  margin().enableAddresses(b != 0);

  config_get_variable(sName, "opcodes", &b);
  margin().enableOpcodes(b != 0);

  if (config_get_string(sName, "font", &fg))
    setFont(fg);
  else
    setFont("Serif 8");

  children.push_back(new SourceWindow(_gp, this, true));
}

void SourceBrowserParent_Window::Build()
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->Build();

  UpdateMenuItem();
}

void SourceBrowserParent_Window::NewProcessor(GUI_Processor *gp)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();
  std::list <ProgramMemoryAccess *>::iterator pma_iterator
    = gp->cpu->pma_context.begin();

  CreateSourceBuffers(gp);

  int child = 1;
  SourceWindow *sbaw = nullptr;
  while ( (sbaw_iterator != children.end()) ||
    (pma_iterator != gp->cpu->pma_context.end()))
  {
    if (sbaw_iterator == children.end())
    {
      char child_name[64];
      child++;
      g_snprintf(child_name, sizeof(child_name), "source_browser%d", child);
      sbaw = new SourceWindow(gp, this, true, child_name);
      children.push_back(sbaw);
    }
    else
      sbaw = *sbaw_iterator++;

    if (pma_iterator != gp->cpu->pma_context.end())
    {
      sbaw->set_pma(*pma_iterator);
      ++pma_iterator;
    }
    else
    {
      sbaw->set_pma(gp->cpu->pma);
    }

  }
}

void SourceBrowserParent_Window::SelectAddress(int address)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->SelectAddress(address);
}

void SourceBrowserParent_Window::SelectAddress(Value *addrSym)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->SelectAddress(addrSym);
}

void SourceBrowserParent_Window::Update()
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->Update();
}

void SourceBrowserParent_Window::UpdateLine(int address)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->UpdateLine(address);
}

void SourceBrowserParent_Window::SetPC(int address)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->SetPC(address);
}

void SourceBrowserParent_Window::CloseSource()
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->CloseSource();
}

void SourceBrowserParent_Window::NewSource(GUI_Processor *gp)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  CreateSourceBuffers(gp);
  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->NewSource(gp);
}

void SourceBrowserParent_Window::ChangeView(int view_state)
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->ChangeView(view_state);
}

gchar *SourceBrowserParent_Window::get_color_string(const char *tag_name)
{
  GdkColor *gdk_color;
  g_object_get(gtk_text_tag_table_lookup(mpTagTable, tag_name),
    "foreground-gdk", &gdk_color, nullptr);
  gchar *color_string = gdk_color_to_string(gdk_color);
  gdk_color_free(gdk_color);
  return color_string;
}

int SourceBrowserParent_Window::set_config()
{
  std::vector<SourceWindow *>::iterator sbaw_iterator = children.begin();

  for ( ; sbaw_iterator != children.end(); ++sbaw_iterator)
    (*sbaw_iterator)->set_config();

  const char *sName = "source_config";
  char *buff;

  buff = get_color_string("Mnemonic");
  config_set_string(sName, "mnemonic_fg", buff);
  g_free(buff);
  buff = get_color_string("Label");
  config_set_string(sName, "label_fg", buff);
  g_free(buff);
  buff = get_color_string("Symbols");
  config_set_string(sName, "symbol_fg", buff);
  g_free(buff);
  buff = get_color_string("Comments");
  config_set_string(sName, "comment_fg", buff);
  g_free(buff);
  buff = get_color_string("Constants");
  config_set_string(sName, "constant_fg", buff);
  g_free(buff);

  config_set_string(sName, "font", getFont());

  config_set_variable(sName, "tab_position", getTabPosition());
  config_set_variable(sName, "line_numbers", margin().bLineNumbers());
  config_set_variable(sName, "addresses", margin().bAddresses());
  config_set_variable(sName, "opcodes", margin().bOpcodes());

  return 0;
}

//------------------------------------------------------------------------
// parseLine
//
// Added a line of text to the source buffer. Apply syntax highlighting.
//

void SourceBuffer::parseLine(const char *cP, int parseStyle)
{
  GtkTextIter iEnd;
  GtkTextBuffer *pTextBuffer = m_buffer;
  gtk_text_buffer_get_end_iter(pTextBuffer, &iEnd);

  int offset = gtk_text_iter_get_offset(&iEnd);

  gtk_text_buffer_insert(pTextBuffer, &iEnd, cP, -1);

  if (parseStyle < 0) {
    addTagRange("Comments", offset, offset + strlen(cP));
    return;
  }

  int i = 0;
  int j = 0;
  bool bHaveMnemonic = false;

  if (i != (j = isString(cP))) {
    addTagRange("Label", i + offset, j + offset);
    i = j;
  }

  while (!isEnd(cP[i])) {

    if ( (j = isWhiteSpace(&cP[i])) != 0) {
      i += j;
    } else if ( (j = isString(&cP[i])) != 0) {
      if (bHaveMnemonic)
        addTagRange("Symbols", i + offset, i + j + offset);
      else
        addTagRange("Mnemonic", i + offset, i + j + offset);
      bHaveMnemonic = true;
      i += j;
    } else if ( (j = isNumber(&cP[i])) != 0) {
      addTagRange("Constants", i + offset, i + j + offset);
      i += j;
    } else if ( (j = isComment(&cP[i])) != 0) {
      addTagRange("Comments", i + offset, i + j + offset);
      // i += j;
      return;
    } else
      i++;
  }
}

//------------------------------------------------------------------------
SourcePageMargin &SourceBrowserParent_Window::margin()
{
  return m_margin;
}

//------------------------------------------------------------------------
void SourceBrowserParent_Window::setTabPosition(int tt)
{
  m_TabType = tt;
  Update();
}

//------------------------------------------------------------------------
void SourceBrowserParent_Window::setFont(const char *cpNewFont)
{
  if (cpNewFont) {
    m_FontDescription = cpNewFont;
    Update();
  }
}

const char *SourceBrowserParent_Window::getFont()
{
  return m_FontDescription.c_str();
}

//------------------------------------------------------------------------
// parseSource
void SourceBrowserParent_Window::parseSource(SourceBuffer *pBuffer,FileContext *pFC)
{
  pFC->rewind();

  char text_buffer[256];
  int line = 1;

  while (pFC->gets(text_buffer, sizeof(text_buffer))) {

    int address;
    // The syntax highlighting doesn't work on list files or hll files
    address = pFC->IsList()||pFC->IsHLL() ? -1 : 1;//gp->cpu->pma->find_address_from_line(pFC,line);

    // check if text_buffer in uft8 character set
    if (!g_utf8_validate(text_buffer, -1, nullptr))
    {
	gsize bytes_read, bytes_written;
	gchar *new_buffer;

	// try to convert to uft8 using current locale
	// if we succeed, do normal processing on converted text
	if ((new_buffer = g_locale_to_utf8((const gchar *)text_buffer,
		-1, &bytes_read, &bytes_written, nullptr)))
	{
        	pBuffer->parseLine(new_buffer,address);
		g_free(new_buffer);
	}
	// Conversion based on locale did not work
	else
	{
	    // replace comment which may have unknown character set
	    if ((new_buffer = strchr(text_buffer, ';')))
	    {
		*new_buffer = 0;
		strcat(text_buffer, "; comment stripped, characters from unknown locale\n");
	    }
	    // if still not OK replace entire line so line numbering still OK
    	    if (!g_utf8_validate(text_buffer, -1, nullptr))
	    {
		strcpy(text_buffer, "; non-comment characters from unknow locale\n");
	    }
            pBuffer->parseLine(text_buffer,address);
	}
    }
    else
        pBuffer->parseLine(text_buffer,address);

    line++;
  }

}

//------------------------------------------------------------------------
void SourceBrowserParent_Window::CreateSourceBuffers(GUI_Processor *gp)
{
  Dprintf((" \n"));

  if (!gp || !gp->cpu || !gp->cpu->pma)
    return;
  Dprintf((" \n"));

  Processor * pProc = gp->cpu;
  if (!pma)
    pma = pProc->pma;

  CloseSource();

  Dprintf(("NewSource\n"));

  if (pProc->files.nsrc_files() != 0) {

    for (int i = 0; i < pProc->files.nsrc_files(); i++) {
      FileContext *fc = pProc->files[i];

      int pos = fc->name().size() - 4;
      if (pos > 0 && fc->name().compare(pos, 4, ".cod")
        && fc->name().compare(pos, 4, ".COD")) {

        ppSourceBuffers.push_back(new SourceBuffer(mpTagTable, fc, this));

      } else {
        if (verbose)
          printf("SourceBrowserAsm_new_source: skipping file: <%s>\n",
          fc->name().c_str());
      }
    }

  }

  Dprintf((" Source is loaded\n"));
}


#endif // HAVE_GUI
