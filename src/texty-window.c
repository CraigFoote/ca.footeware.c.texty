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
  AdwHeaderBar *header_bar;
  GtkTextView *text_view;
  GtkButton *save_button;
  GtkLabel *cursor_pos;
  AdwToastOverlay *toast_overlay;

  /* instance variable */
  GFile *file;
};

G_DEFINE_FINAL_TYPE(TextyWindow, texty_window, ADW_TYPE_APPLICATION_WINDOW)

static void save_font_size(int value)
{
  GSettings *settings;

  settings = g_settings_new("ca.footeware.c.texty");
  g_settings_set_int(settings, "font-size", value);
  g_object_unref(settings);
}

static gboolean get_font_size(void)
{
  GSettings *settings;
  gboolean value;

  settings = g_settings_new("ca.footeware.c.texty");
  value = g_settings_get_int(settings, "font-size");
  g_object_unref(settings);

  return value;
}

static void save_text_wrap(gboolean value)
{
  GSettings *settings;

  settings = g_settings_new("ca.footeware.c.texty");
  g_settings_set_boolean(settings, "text-wrap", value);
  g_object_unref(settings);
}

static gboolean get_text_wrap(void)
{
  GSettings *settings;
  gboolean value;

  settings = g_settings_new("ca.footeware.c.texty");
  value = g_settings_get_boolean(settings, "text-wrap");
  g_object_unref(settings);

  return value;
}

static void
load_window_size(TextyWindow *self)
{
  GSettings *settings;
  int width, height;

  settings = g_settings_new("ca.footeware.c.texty");
  width = g_settings_get_int(settings, "window-width");
  height = g_settings_get_int(settings, "window-height");

  if (width > 0 && height > 0)
  {
    gtk_window_set_default_size(GTK_WINDOW(self), width, height);
  }
}

static void save_window_size(TextyWindow *self)
{
  GSettings *settings;
  int width, height;

  /* save_window_size */
  settings = g_settings_new("ca.footeware.c.texty");
  gtk_window_get_default_size(GTK_WINDOW(self), &width, &height);
  g_settings_set_int(settings, "window-width", width);
  g_settings_set_int(settings, "window-height", height);
  g_object_unref(settings);
}

/**********************************/
/* Preferences 👆️                 */
/**********************************/

static void
save_file_complete(GObject *source_object,
                   GAsyncResult *result,
                   gpointer user_data)
{
  g_autofree char *display_name;
  g_autoptr(GFileInfo) info;
  g_autofree char *msg;
  TextyWindow *self;
  GtkTextBuffer *buffer;
  g_autoptr(GError) error;

  self = user_data;

  error = NULL;
  /* the save function will prompt to replace if exists */
  g_file_replace_contents_finish(self->file, result, NULL, &error);

  self = TEXTY_WINDOW(user_data);
  /* store a reference to the file, duplicated */
  self->file = g_file_dup(self->file);

  /* mark textview as having no changes to save */
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->text_view));
  gtk_text_buffer_set_modified(buffer, FALSE);

  /* Query the file for its display name */
  display_name = NULL;
  info = g_file_query_info(self->file,
                           "standard::display-name",
                           G_FILE_QUERY_INFO_NONE,
                           NULL,
                           NULL);
  if (info != NULL)
    display_name =
        g_strdup(g_file_info_get_attribute_string(info,
                                                  "standard::display-name"));
  else
    display_name = g_file_get_basename(self->file);

  /* display name in window title */
  gtk_window_set_title(GTK_WINDOW(self), display_name);

  /* display toast */
  msg = NULL;
  if (error != NULL)
    msg = g_strdup_printf("Unable to save “%s”", display_name);
  else
    msg = g_strdup_printf("Saved “%s”", display_name);

  adw_toast_overlay_add_toast(self->toast_overlay, adw_toast_new(msg));
  /* put cursor in text_view */
  gtk_widget_grab_focus(GTK_WIDGET(self->text_view));
}

static void
save_file(TextyWindow *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;
  char *text;
  g_autoptr(GBytes) bytes;

  buffer = gtk_text_view_get_buffer(self->text_view);

  /* retrieve the iterators at the start and end of the buffer */
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  /* Retrieve all the visible text between the two bounds */
  text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

  bytes = g_bytes_new_take(text, strlen(text));

  /* start the asynchronous operation to save the data into the file */
  g_file_replace_contents_bytes_async(self->file,
                                      bytes,
                                      NULL,
                                      FALSE,
                                      G_FILE_CREATE_NONE,
                                      NULL,
                                      save_file_complete,
                                      self);
}

static void
on_save_response(GObject *source,
                 GAsyncResult *result,
                 gpointer user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  TextyWindow *self = user_data;

  /* get the selected file and save buffer contents into it */
  g_autoptr(GFile) file = gtk_file_dialog_save_finish(dialog, result, NULL);
  if (file != NULL)
  {
    self->file = file;
    save_file(self);
  }
}

static void
texty_window__save(GAction *action G_GNUC_UNUSED,
                   GVariant *param G_GNUC_UNUSED,
                   TextyWindow *self)
{
  /* check if we have a file yet */
  if (self->file != NULL)
  {
    save_file(self);
  }
  else
  {
    /* prompt user for file */
    g_autoptr(GtkFileDialog) dialog;
    dialog = gtk_file_dialog_new();

    /* present save file dialog */
    gtk_file_dialog_save(dialog,
                         GTK_WINDOW(self),
                         NULL,
                         on_save_response,
                         self);
  }
}

/**********************************/
/* Save File 👆️                     */
/**********************************/

static void
save_modified_file_complete(GObject *source_object,
                            GAsyncResult *result,
                            gpointer user_data)
{
  TextyWindow *self;
  g_autoptr(GError) error;
  GtkTextBuffer *buffer;
  g_autofree char *display_name;
  g_autoptr(GFileInfo) info;
  g_autofree char *msg;

  /* the selected file */
  GFile *file = G_FILE(source_object);
  self = TEXTY_WINDOW(user_data);
  error = NULL;

  /* the main save function will prompt to replace if exists */
  g_file_replace_contents_finish(file, result, NULL, &error);
  /* store a reference to the file, duplicated */
  self->file = g_file_dup(file);
  /* mark textview as having no changes to save */
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->text_view));
  gtk_text_buffer_set_modified(buffer, FALSE);

  /* get the display name of the file */
  display_name = NULL;
  info = g_file_query_info(file,
                           "standard::display-name",
                           G_FILE_QUERY_INFO_NONE,
                           NULL,
                           NULL);
  if (info != NULL)
  {
    display_name =
        g_strdup(g_file_info_get_attribute_string(info,
                                                  "standard::display-name"));
  }
  else
  {
    display_name = g_file_get_basename(file);
  }

  /* In case of error, show a toast */
  if (error != NULL)
  {
    msg = g_strdup_printf("Unable to save “%s”", display_name);
    adw_toast_overlay_add_toast(self->toast_overlay, adw_toast_new(msg));
    return;
  }
  msg = g_strdup_printf("Saved “%s”", display_name);
  adw_toast_overlay_add_toast(self->toast_overlay, adw_toast_new(msg));
  /* put cursor in textview */
  gtk_widget_grab_focus(GTK_WIDGET(self->text_view));
}

static void
save_modified_file(TextyWindow *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;
  char *text;
  g_autoptr(GBytes) bytes;

  buffer = gtk_text_view_get_buffer(self->text_view);

  /* Retrieve the iterators at the start and end of the buffer */
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  /* Retrieve all the visible text between the two bounds */
  text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
  bytes = g_bytes_new_take(text, strlen(text));

  /* Start the asynchronous operation to save the data into the file */
  g_file_replace_contents_bytes_async(self->file,
                                      bytes,
                                      NULL,
                                      FALSE,
                                      G_FILE_CREATE_NONE,
                                      NULL,
                                      save_modified_file_complete,
                                      self);
}

static void
on_save_modified_response(AdwAlertDialog *dialog,
                          GAsyncResult *result,
                          TextyWindow *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;

  const char *response = adw_alert_dialog_choose_finish(dialog, result);

  if (g_str_equal(response, "cancel"))
  {
    return;
  }
  if (g_str_equal(response, "discard"))
  {
    /* clear buffer */
    buffer = gtk_text_view_get_buffer(self->text_view);
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_set_modified(buffer, FALSE);

    /* set window title */
    gtk_window_set_title(GTK_WINDOW(self), "Texty");

    /* clear file ref */
    self->file = NULL;
  }
  else if (g_str_equal(response, "save"))
  {
    save_modified_file(self);
  }
}

/**********************************/
/* Save Modified 👆️               */
/**********************************/

static void
texty_window__new(GAction *action,
                  GVariant *parameter,
                  TextyWindow *self)
{
  GtkTextBuffer *buffer;
  gboolean modified;
  GtkTextIter start;
  GtkTextIter end;

  /* check if text buffer has been changed */
  buffer = gtk_text_view_get_buffer(self->text_view);
  modified = gtk_text_buffer_get_modified(buffer);
  if (modified)
  {
    /* prompt user to save file */
    AdwDialog *dialog = adw_alert_dialog_new(
        "Save Changes?",
        "There are unsaved modifications.\nDo you want to save them?");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "yes");
    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                                   "cancel", "_Cancel",
                                   "discard", "_Discard",
                                   "save", "_Save",
                                   NULL);
    adw_alert_dialog_choose(ADW_ALERT_DIALOG(dialog),
                            GTK_WIDGET(self),
                            NULL,
                            (GAsyncReadyCallback)on_save_modified_response,
                            self);
  }
  else
  {
    /* clear buffer and ref to file */
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_window_set_title(GTK_WINDOW(self), "Texty");
    gtk_text_buffer_set_modified(buffer, FALSE);
    self->file = NULL;
  }

  /* put cursor in textview */
  gtk_widget_grab_focus(GTK_WIDGET(self->text_view));
}

/**********************************/
/* New 👆️                         */
/**********************************/

static void
open_file_complete(GObject *source_object,
                   GAsyncResult *result,
                   TextyWindow *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  g_autofree char *display_name;
  g_autoptr(GFileInfo) info;

  GFile *file = G_FILE(source_object);

  g_autofree char *contents = NULL;
  gsize length = 0;

  g_autoptr(GError) error = NULL;

  /*
   * Complete the asynchronous operation; this function will either
   * give you the contents of the file as a byte array, or will
   * set the error argument.
   */
  g_file_load_contents_finish(file,
                              result,
                              &contents,
                              &length,
                              NULL,
                              &error);

  /* get the display name of the file */
  display_name = NULL;
  info = g_file_query_info(file,
                           "standard::display-name",
                           G_FILE_QUERY_INFO_NONE,
                           NULL,
                           NULL);
  if (info != NULL)
  {
    display_name =
        g_strdup(g_file_info_get_attribute_string(info,
                                                  "standard::display-name"));
  }
  else
  {
    display_name = g_file_get_basename(file);
  }

  /* In case of error, show a toast */
  if (error != NULL)
  {
    g_autofree char *msg =
        g_strdup_printf("Unable to open “%s”", display_name);

    adw_toast_overlay_add_toast(self->toast_overlay, adw_toast_new(msg));
    return;
  }

  /* Ensure that the file is encoded with UTF-8 */
  if (!g_utf8_validate(contents, length, NULL))
  {
    g_autofree char *msg =
        g_strdup_printf("Invalid text encoding for “%s”", display_name);

    adw_toast_overlay_add_toast(self->toast_overlay, adw_toast_new(msg));
    return;
  }

  /*
   * Retrieve the GtkTextBuffer instance that stores the
   * file's text displayed by the GtkTextView widget.
   */
  buffer = gtk_text_view_get_buffer(self->text_view);

  /* Set the text using the contents of the file */
  gtk_text_buffer_set_text(buffer, contents, length);

  /* Reposition the cursor so it's at the start of the text */
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_place_cursor(buffer, &start);
  /* set 'modified' bit to indicate it does not yet need saving */
  gtk_text_buffer_set_modified(buffer, FALSE);
  /* keep a pointer to the file */
  self->file = g_file_dup(file);

  /* Set the title using the display name */
  gtk_window_set_title(GTK_WINDOW(self), display_name);
}

static void
open_file(TextyWindow *self,
          GFile *file)
{
  g_file_load_contents_async(file,
                             NULL,
                             (GAsyncReadyCallback)open_file_complete,
                             self);
}

static void
on_open_response(GObject *source,
                 GAsyncResult *result,
                 gpointer user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  TextyWindow *self = user_data;

  g_autoptr(GFile) file = gtk_file_dialog_open_finish(dialog,
                                                      result,
                                                      NULL);

  /* If the user selected a file, open it */
  if (file != NULL)
    open_file(self, file);
}

static void
texty_window__open(GAction *action,
                   GVariant *parameter,
                   TextyWindow *self)
{
  GtkTextBuffer *buffer;
  gboolean modified;

  /* check if text buffer has been changed */
  buffer = gtk_text_view_get_buffer(self->text_view);
  modified = gtk_text_buffer_get_modified(buffer);
  if (modified)
  {
    /* prompt user to save file */
    AdwDialog *dialog = adw_alert_dialog_new(
        "Save Changes?",
        "There are unsaved modifications.\nDo you want to save them?");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "yes");
    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                                   "cancel", "_Cancel",
                                   "discard", "_Discard",
                                   "save", "_Save",
                                   NULL);
    adw_alert_dialog_choose(ADW_ALERT_DIALOG(dialog),
                            GTK_WIDGET(self),
                            NULL,
                            (GAsyncReadyCallback)on_save_modified_response,
                            self);
  }
  else
  {
    g_autoptr(GtkFileDialog) dialog = gtk_file_dialog_new();
    gtk_file_dialog_open(dialog,
                         GTK_WINDOW(self),
                         NULL,
                         on_open_response,
                         self);
  }
}

/**********************************/
/* Open File 👆️                   */
/**********************************/

static void
save_file_as_complete(GObject *source_object,
                      GAsyncResult *result,
                      gpointer user_data)
{
  g_autofree char *display_name;
  g_autoptr(GFileInfo) info;
  g_autofree char *msg;
  TextyWindow *self;
  GtkTextBuffer *buffer;

  /* the selected file */
  GFile *file = G_FILE(source_object);

  g_autoptr(GError) error = NULL;
  /* the main save function will prompt to replace if exists */
  g_file_replace_contents_finish(file, result, NULL, &error);

  self = TEXTY_WINDOW(user_data);
  /* store a reference to the file, duplicated */
  self->file = g_file_dup(file);

  /* mark textview as having no changes to save */
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->text_view));
  gtk_text_buffer_set_modified(buffer, FALSE);

  /* Query the file for its display name */
  display_name = NULL;
  info = g_file_query_info(file,
                           "standard::display-name",
                           G_FILE_QUERY_INFO_NONE,
                           NULL,
                           NULL);
  if (info != NULL)
    display_name =
        g_strdup(g_file_info_get_attribute_string(info,
                                                  "standard::display-name"));
  else
    display_name = g_file_get_basename(file);

  /* display name in window title */
  gtk_window_set_title(GTK_WINDOW(self), display_name);

  /* display toast */
  msg = NULL;
  if (error != NULL)
    msg = g_strdup_printf("Unable to save “%s”", display_name);
  else
    msg = g_strdup_printf("Saved “%s”", display_name);

  adw_toast_overlay_add_toast(self->toast_overlay, adw_toast_new(msg));
  /* put cursor in textview */
  gtk_widget_grab_focus(GTK_WIDGET(self->text_view));
}

static void
save_file_as(TextyWindow *self,
             GFile *file)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;
  char *text;
  g_autoptr(GBytes) bytes;

  buffer = gtk_text_view_get_buffer(self->text_view);

  /* Retrieve the iterators at the start and end of the buffer */
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  /* Retrieve all the visible text between the two bounds */
  text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

  bytes = g_bytes_new_take(text, strlen(text));

  /* Start the asynchronous operation to save the data into the file */
  g_file_replace_contents_bytes_async(file,
                                      bytes,
                                      NULL,
                                      FALSE,
                                      G_FILE_CREATE_NONE,
                                      NULL,
                                      save_file_as_complete,
                                      self);
}

static void
on_save_as_response(GObject *source,
                    GAsyncResult *result,
                    gpointer user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  TextyWindow *self = user_data;

  /* get the selected file and write the buffer contents into it */
  g_autoptr(GFile) file = gtk_file_dialog_save_finish(dialog, result, NULL);
  if (file != NULL)
  {
    save_file_as(self, file);
  }
}

static void
texty_window__save_as(GAction *action,
                      GVariant *parameter,
                      TextyWindow *self)
{
  g_autoptr(GtkFileDialog) dialog = gtk_file_dialog_new();
  /* present save file dialog */
  gtk_file_dialog_save(dialog,
                       GTK_WINDOW(self),
                       NULL,
                       on_save_as_response,
                       self);
}

/**********************************/
/* Save As 👆️                     */
/**********************************/

static void
texty_window__update_cursor_position(GtkTextBuffer *buffer,
                                     GParamSpec *pspec,
                                     TextyWindow *self)
{
  GtkTextIter iter;
  g_autofree char *cursor_str;

  int cursor_pos = 0;

  /* Retrieve the value of the "cursor-position" property */
  g_object_get(buffer, "cursor-position", &cursor_pos, NULL);

  /* Construct the text iterator for the position of the cursor */
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, cursor_pos);

  /* Set the new contents of the label */
  cursor_str = g_strdup_printf("Ln %d, Col %d",
                               gtk_text_iter_get_line(&iter) + 1,
                               gtk_text_iter_get_line_offset(&iter) + 1);

  gtk_label_set_text(self->cursor_pos, cursor_str);
}

/**********************************/
/* Update Cursor Position 👆️      */
/**********************************/

static void
texty_window__toggle_text_wrap(GSimpleAction *action,
                               GVariant *parameter,
                               TextyWindow *self)
{
  GVariant *state;
  gboolean current_state;

  state = g_action_get_state(G_ACTION(action));
  current_state = g_variant_get_boolean(state);
  g_variant_unref(state);

  current_state = !current_state;

  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->text_view),
                              current_state ? GTK_WRAP_WORD : GTK_WRAP_NONE);

  g_simple_action_set_state(action, g_variant_new_boolean(current_state));
  save_text_wrap(current_state);
}

/**********************************/
/* Toggle text wrap 👆️            */
/**********************************/

static void
texty_window__set_font_size(GSimpleAction *action,
                            GVariant *parameter,
                            TextyWindow *self)
{
  gint font_size;
  char *css_class_name;
  const char *css_classes[2];
  GtkCssProvider *provider;
  char *css;
  GdkDisplay *display;

  font_size = g_variant_get_int32(parameter);

  css_class_name = g_strdup_printf("font-size-%d", font_size);
  css_classes[0] = css_class_name;
  css_classes[1] = NULL; // Null-terminate the array
  gtk_widget_set_css_classes(GTK_WIDGET(self->text_view), css_classes);
  provider = gtk_css_provider_new();
  css = g_strdup_printf("textview.%s { font-size: %dpx; font-family: monospace; }", css_class_name, font_size);
  gtk_css_provider_load_from_string(provider, css);
  display = gdk_display_get_default();
  gtk_style_context_add_provider_for_display(display,
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free(css);
  g_free(css_class_name);
  g_object_unref(provider);

  g_simple_action_set_state(action, parameter);
  save_font_size(g_variant_get_int32(parameter));
}

/**********************************/
/* Set font size 👆️               */
/**********************************/

static void
on_close_save_response(GObject *source,
                       GAsyncResult *result,
                       gpointer user_data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  TextyWindow *self = user_data;

  /* get the selected file and save buffer contents into it */
  g_autoptr(GFile) file = gtk_file_dialog_save_finish(dialog, result, NULL);
  if (file != NULL)
  {
    self->file = file;
    save_file(self);
  }
}

static void
on_close_save(TextyWindow *self)
{
  /* check if we have a file yet */
  if (self->file != NULL)
  {
    save_file(self);
  }
  else
  {
    /* prompt user for file */
    g_autoptr(GtkFileDialog) dialog;
    dialog = gtk_file_dialog_new();

    /* present save file dialog */
    gtk_file_dialog_save(dialog,
                         GTK_WINDOW(self),
                         NULL,
                         on_close_save_response,
                         self);
  }
}

static void
on_close_response(AdwAlertDialog *dialog,
                  GAsyncResult *result,
                  TextyWindow *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GtkTextIter end;

  const char *response = adw_alert_dialog_choose_finish(dialog, result);

  if (g_str_equal(response, "cancel"))
  {
    return;
  }
  if (g_str_equal(response, "discard"))
  {
    /* clear buffer */
    buffer = gtk_text_view_get_buffer(self->text_view);
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_set_modified(buffer, FALSE);

    /* set window title */
    gtk_window_set_title(GTK_WINDOW(self), "Texty");

    /* clear file ref */
    self->file = NULL;

    /* close window */
    gtk_window_close(GTK_WINDOW(self));
  }
  else if (g_str_equal(response, "save"))
  {
    on_close_save(self);
  }
}

static void
on_close_request(TextyWindow *self,
                 gpointer user_data)
{
  GtkTextBuffer *buffer;
  gboolean modified;

  save_window_size(self);

  /* check if text buffer has been changed */
  buffer = gtk_text_view_get_buffer(self->text_view);
  modified = gtk_text_buffer_get_modified(buffer);
  if (modified)
  {
    /* prompt user to save file */
    AdwDialog *dialog = adw_alert_dialog_new(
        "Save Changes?",
        "There are unsaved modifications.\nDo you want to save them?");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "yes");
    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                                   "cancel", "_Cancel",
                                   "discard", "_Discard",
                                   "save", "_Save",
                                   NULL);
    adw_alert_dialog_choose(ADW_ALERT_DIALOG(dialog),
                            GTK_WIDGET(self),
                            NULL,
                            (GAsyncReadyCallback)on_close_response,
                            self);
  }
}

/**********************************/
/* Closing 👆️                     */
/**********************************/

static void
texty_window_class_init(TextyWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  gtk_widget_class_set_template_from_resource(widget_class,
                                              "/ca/footeware/c/texty/texty-window.ui");
  gtk_widget_class_bind_template_child(widget_class,
                                       TextyWindow,
                                       header_bar);
  gtk_widget_class_bind_template_child(widget_class,
                                       TextyWindow,
                                       text_view);
  gtk_widget_class_bind_template_child(widget_class,
                                       TextyWindow,
                                       save_button);
  gtk_widget_class_bind_template_child(widget_class,
                                       TextyWindow,
                                       cursor_pos);
  gtk_widget_class_bind_template_child(widget_class,
                                       TextyWindow,
                                       toast_overlay);
}

static void
texty_window_init(TextyWindow *self)
{
  g_autoptr(GSimpleAction) save_action;
  g_autoptr(GSimpleAction) new_action;
  g_autoptr(GSimpleAction) open_action;
  g_autoptr(GSimpleAction) save_as_action;
  g_autoptr(GSimpleAction) toggle_text_wrap_action;
  g_autoptr(GSimpleAction) set_font_size_action;
  GtkTextBuffer *buffer;
  GtkCssProvider *css_provider;
  gboolean text_wrap;
  int font_size;
  char *css_class_name;
  const char *css_classes[2];
  char *css;
  GdkDisplay *display;

  gtk_widget_init_template(GTK_WIDGET(self));

  /* save */
  save_action = g_simple_action_new("save", NULL);
  g_signal_connect(save_action,
                   "activate",
                   G_CALLBACK(texty_window__save),
                   self);
  g_action_map_add_action(G_ACTION_MAP(self),
                          G_ACTION(save_action));

  /* new */
  new_action = g_simple_action_new("new", NULL);
  g_signal_connect(new_action,
                   "activate",
                   G_CALLBACK(texty_window__new),
                   self);
  g_action_map_add_action(G_ACTION_MAP(self),
                          G_ACTION(new_action));

  /* open */
  open_action = g_simple_action_new("open", NULL);
  g_signal_connect(open_action,
                   "activate",
                   G_CALLBACK(texty_window__open),
                   self);
  g_action_map_add_action(G_ACTION_MAP(self),
                          G_ACTION(open_action));

  /* save as */
  save_as_action = g_simple_action_new("save-as", NULL);
  g_signal_connect(save_as_action,
                   "activate",
                   G_CALLBACK(texty_window__save_as),
                   self);
  g_action_map_add_action(G_ACTION_MAP(self),
                          G_ACTION(save_as_action));

  /* wrap text */
  toggle_text_wrap_action = g_simple_action_new_stateful("toggle-text-wrap",
                                                         NULL,
                                                         g_variant_new_boolean(FALSE));
  g_signal_connect(toggle_text_wrap_action,
                   "activate",
                   G_CALLBACK(texty_window__toggle_text_wrap),
                   self);
  g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(toggle_text_wrap_action));
  text_wrap = get_text_wrap();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->text_view),
                              text_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE);
  g_simple_action_set_state(toggle_text_wrap_action, g_variant_new_boolean(text_wrap));

  /* cursor position */
  buffer = gtk_text_view_get_buffer(self->text_view);
  g_signal_connect(buffer,
                   "notify::cursor-position",
                   G_CALLBACK(texty_window__update_cursor_position),
                   self);

  /* set font size */
  set_font_size_action = g_simple_action_new_stateful("set-font-size",
                                                      g_variant_type_new("i"),
                                                      g_variant_new_int32(22));
  g_signal_connect(set_font_size_action,
                   "activate",
                   G_CALLBACK(texty_window__set_font_size),
                   self);
  g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(set_font_size_action));
  /* init font-size */
  font_size = get_font_size();
  g_simple_action_set_state(set_font_size_action, g_variant_new_int32(font_size));
  css_class_name = g_strdup_printf("font-size-%d", font_size);
  css_classes[0] = css_class_name;
  css_classes[1] = NULL; /* Null-terminate the array */
  gtk_widget_set_css_classes(GTK_WIDGET(self->text_view), css_classes);
  css_provider = gtk_css_provider_new();
  css = g_strdup_printf("textview.%s { font-size: %dpx; font-family: monospace; }", css_class_name, font_size);
  gtk_css_provider_load_from_string(css_provider, css);
  display = gdk_display_get_default();
  gtk_style_context_add_provider_for_display(display,
                                             GTK_STYLE_PROVIDER(css_provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free(css);
  g_free(css_class_name);
  g_object_unref(css_provider);
  /* init window-size */
  load_window_size(self);

  /* Listen for window close and prompt if modified */
  g_signal_connect(self, "close-request", G_CALLBACK(on_close_request), NULL);
}
