/* texty-window.c
 *
 * Copyright 2024 Craig Foote
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "texty-window.h"

struct _TextyWindow
{
  AdwApplicationWindow parent_instance;

  /* Template widgets */
  AdwHeaderBar    *header_bar;
  GtkTextView     *main_text_view;
  GtkButton       *save_button;
  GtkLabel        *cursor_pos;
  AdwToastOverlay *toast_overlay;
  GFile           *file;
};

G_DEFINE_FINAL_TYPE (TextyWindow, texty_window, ADW_TYPE_APPLICATION_WINDOW)

static void
save_file_complete (GObject      *source_object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  g_autofree char *display_name;
  g_autoptr (GFileInfo) info;
  g_autofree char *msg;
  TextyWindow *self;
  GtkTextBuffer *buffer;

  // the selected file
  GFile *file = G_FILE (source_object);

  g_autoptr (GError) error =  NULL;
  // the main save function will prompt to replace if exists
  g_file_replace_contents_finish (file, result, NULL, &error);

  self = TEXTY_WINDOW (user_data);
  // store a reference to the file, duplicated
  self->file = g_file_dup (file);

  // mark editor as having no changes to save
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->main_text_view));
  gtk_text_buffer_set_modified(buffer, FALSE);

  // Query the file for its display name
  display_name = NULL;
  info = g_file_query_info (file,
                     "standard::display-name",
                     G_FILE_QUERY_INFO_NONE,
                     NULL,
                     NULL);
  if (info != NULL)
      display_name =
        g_strdup (g_file_info_get_attribute_string (info, "standard::display-name"));
  else
      display_name = g_file_get_basename (file);

  // display name in window title
  gtk_window_set_title (GTK_WINDOW (self), display_name);

  // display toast
  msg = NULL;
  if (error != NULL)
    msg = g_strdup_printf ("Unable to save as “%s”", display_name);
  else
    msg = g_strdup_printf ("Saved as “%s”", display_name);

  adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (msg));
}

static void
save_file (TextyWindow *self,
           GFile       *file)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;
  char *text;
  g_autoptr(GBytes) bytes;

  if (file == NULL)
    return;

  buffer = gtk_text_view_get_buffer (self->main_text_view);

  // Retrieve the iterators at the start and end of the buffer
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);

  // Retrieve all the visible text between the two bounds
  text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  // If there is nothing to save, return early
  if (text == NULL || file == NULL)
    return;

  bytes = g_bytes_new_take (text, strlen (text));

  // Start the asynchronous operation to save the data into the file
  g_file_replace_contents_bytes_async (file,
                                        bytes,
                                        NULL,
                                        FALSE,
                                        G_FILE_CREATE_NONE,
                                        NULL,
                                        save_file_complete,
                                        self);
}

static void
on_save_response (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source);
  TextyWindow *self = user_data;

  // get the selected file and save buffer contents into it
  g_autoptr (GFile) file = gtk_file_dialog_save_finish (dialog, result, NULL);
  if (file != NULL)
    {
      save_file (self, file);
    }
}

static void
text_viewer_window__save (GAction     *action G_GNUC_UNUSED,
                          GVariant    *param G_GNUC_UNUSED,
                          TextyWindow *self)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (self->main_text_view);
  gboolean modified = gtk_text_buffer_get_modified(buffer);
  if (!modified)
    {
      return;
    }

  // check if we have a file yet
  if (self->file == NULL )
    {
      // prompt user for file
      g_autoptr (GtkFileDialog) dialog;
      dialog = gtk_file_dialog_new ();

      // present save file dialog
      gtk_file_dialog_save (dialog,
                            GTK_WINDOW (self),
                            NULL,
                            on_save_response,
                            self);
    }
  else
    {
      save_file (self, self->file);
    }
}

static void
on_save_modified_response(AdwAlertDialog *dialog,
                          GAsyncResult   *result,
                          TextyWindow    *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;

  const char *response = adw_alert_dialog_choose_finish (dialog, result);

  if (g_str_equal(response, "discard")) // do not save
    {
      // clear buffer
      buffer = gtk_text_view_get_buffer (self->main_text_view);
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);
      gtk_text_buffer_delete (buffer, &start, &end);
    }
  else if (g_str_equal(response, "save"))
    {
      // save then clear
      save_file (self, self->file);
      // clear buffer
      buffer = gtk_text_view_get_buffer (self->main_text_view);
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);
      gtk_text_buffer_delete (buffer, &start, &end);
    }

  // set window title
  gtk_window_set_title (GTK_WINDOW (self), "texty");
}

static void
text_viewer_window__new (GAction     *action,
                         GVariant    *parameter,
                         TextyWindow *self)
{
  GtkTextBuffer *buffer;
  gboolean modified;
  GtkTextIter start;
  GtkTextIter end;

  // check if text buffer has been changed
  buffer = gtk_text_view_get_buffer (self->main_text_view);
  modified = gtk_text_buffer_get_modified (buffer);
  if (modified)
    {
      // prompt user to save file
      AdwDialog *dialog = adw_alert_dialog_new (
          "Save Changes?",
          "There are unsaved modifications.\nDo you want to save them?");
      adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
      adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "yes");
      adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                      "cancel", "_Cancel",
                                      "discard", "_Discard",
                                      "save", "_Save",
                                      NULL);
      adw_alert_dialog_choose (ADW_ALERT_DIALOG (dialog),
                               GTK_WIDGET (self),
                               NULL,
                               (GAsyncReadyCallback) on_save_modified_response,
                               self);
    }
  else
    {
      // clear buffer
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);
      gtk_text_buffer_delete (buffer,
                              &start,
                              &end);
      gtk_window_set_title (GTK_WINDOW (self), "texty");
    }
}

static void
open_file_complete (GObject          *source_object,
                    GAsyncResult     *result,
                    TextyWindow      *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  g_autofree char *display_name;
  g_autoptr (GFileInfo) info;

  GFile *file = G_FILE (source_object);

  g_autofree char *contents = NULL;
  gsize length = 0;

  g_autoptr (GError) error = NULL;

  /*
   * Complete the asynchronous operation; this function will either
   * give you the contents of the file as a byte array, or will
   * set the error argument.
   */
  g_file_load_contents_finish (file,
                               result,
                               &contents,
                               &length,
                               NULL,
                               &error);

  // get the display name of the file
  display_name = NULL;
  info = g_file_query_info (file,
                            "standard::display-name",
                            G_FILE_QUERY_INFO_NONE,
                            NULL,
                            NULL);
  if (info != NULL)
    {
      display_name =
        g_strdup (g_file_info_get_attribute_string (info,
                                                    "standard::display-name"));
    }
  else
    {
      display_name = g_file_get_basename (file);
    }

  // In case of error, show a toast
  if (error != NULL)
    {
      g_autofree char *msg =
        g_strdup_printf ("Unable to open “%s”", display_name);

      adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (msg));
      return;
    }

  // Ensure that the file is encoded with UTF-8
  if (!g_utf8_validate (contents, length, NULL))
    {
      g_autofree char *msg =
        g_strdup_printf ("Invalid text encoding for “%s”", display_name);

      adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (msg));
      return;
    }

  // Retrieve the GtkTextBuffer instance that stores the
  // file's text displayed by the GtkTextView widget
  buffer = gtk_text_view_get_buffer (self->main_text_view);

  // Set the text using the contents of the file
  gtk_text_buffer_set_text (buffer, contents, length);

  // Reposition the cursor so it's at the start of the text
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_place_cursor (buffer, &start);
  // set 'modified' bit to indicate it does not yet need saving
  gtk_text_buffer_set_modified (buffer, FALSE);
  // keep a pointer to the file
  self->file = g_file_dup (file);

  // Set the title using the display name
  gtk_window_set_title (GTK_WINDOW (self), display_name);
}

static void
open_file (TextyWindow *self,
           GFile       *file)
{
  g_file_load_contents_async (file,
                              NULL,
                              (GAsyncReadyCallback) open_file_complete,
                              self);
}

static void
on_open_response (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source);
  TextyWindow *self = user_data;

  g_autoptr (GFile) file = gtk_file_dialog_open_finish (dialog,
                                                        result,
                                                        NULL);

  // If the user selected a file, open it
  if (file != NULL)
    open_file (self, file);
}

static void
text_viewer_window__open (GAction          *action,
                          GVariant         *parameter,
                          TextyWindow      *self)
{
  g_autoptr (GtkFileDialog) dialog = gtk_file_dialog_new ();

  gtk_file_dialog_open (dialog,
                        GTK_WINDOW (self),
                        NULL,
                        on_open_response,
                        self);
}

static void
text_viewer_window__save_as (void)
{
  g_print ("window save-as\n"); // TODO
}

static void
text_viewer_window__update_cursor_position (GtkTextBuffer *buffer,
                                            GParamSpec *pspec,
                                            TextyWindow *self)
{
  GtkTextIter iter;
  g_autofree char *cursor_str;

  int cursor_pos = 0;

  // Retrieve the value of the "cursor-position" property
  g_object_get (buffer, "cursor-position", &cursor_pos, NULL);

  // Construct the text iterator for the position of the cursor
  gtk_text_buffer_get_iter_at_offset (buffer, &iter, cursor_pos);

  // Set the new contents of the label
  cursor_str = g_strdup_printf ("Ln %d, Col %d",
                            gtk_text_iter_get_line (&iter) + 1,
                            gtk_text_iter_get_line_offset (&iter) + 1);

  gtk_label_set_text (self->cursor_pos, cursor_str);
}

static void
texty_window_class_init (TextyWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                      "/ca/footeware/c/texty/texty-window.ui");
  gtk_widget_class_bind_template_child (widget_class,
                                        TextyWindow,
                                        header_bar);
  gtk_widget_class_bind_template_child (widget_class,
                                        TextyWindow,
                                        main_text_view);
  gtk_widget_class_bind_template_child (widget_class,
                                        TextyWindow,
                                        save_button);
  gtk_widget_class_bind_template_child (widget_class,
                                        TextyWindow,
                                        cursor_pos);
  gtk_widget_class_bind_template_child (widget_class,
                                        TextyWindow,
                                        toast_overlay);
}

static void
texty_window_init (TextyWindow *self)
{
  g_autoptr (GSimpleAction) save_action;
  g_autoptr (GSimpleAction) new_action;
  g_autoptr (GSimpleAction) open_action;
  g_autoptr (GSimpleAction) save_as_action;
  GtkTextBuffer *buffer;

  gtk_widget_init_template (GTK_WIDGET (self));

  // save
  save_action = g_simple_action_new ("save", NULL);
  g_signal_connect (save_action,
                    "activate",
                    G_CALLBACK (text_viewer_window__save),
                    self);
  g_action_map_add_action (G_ACTION_MAP (self),
                           G_ACTION (save_action));

  // new
  new_action = g_simple_action_new ("new", NULL);
  g_signal_connect (new_action,
                    "activate",
                    G_CALLBACK (text_viewer_window__new),
                    self);
  g_action_map_add_action (G_ACTION_MAP (self),
                           G_ACTION (new_action));

  // open
  open_action = g_simple_action_new ("open", NULL);
  g_signal_connect (open_action,
                    "activate",
                    G_CALLBACK (text_viewer_window__open),
                    self);
  g_action_map_add_action (G_ACTION_MAP (self),
                           G_ACTION (open_action));

  // save as
  save_as_action = g_simple_action_new ("save-as", NULL);
  g_signal_connect (save_as_action,
                    "activate",
                    G_CALLBACK (text_viewer_window__save_as),
                    self);
  g_action_map_add_action (G_ACTION_MAP (self),
                           G_ACTION (save_as_action));

  buffer = gtk_text_view_get_buffer (self->main_text_view);
  g_signal_connect (buffer,
                    "notify::cursor-position",
                    G_CALLBACK (text_viewer_window__update_cursor_position),
                    self);
}
