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
        AdwApplicationWindow  parent_instance;

        /* Template widgets */
        AdwHeaderBar    *header_bar;
        GtkTextView     *main_text_view;
        GtkButton       *save_button;
        GtkLabel        *cursor_pos;
        AdwToastOverlay *toast_overlay;
};

G_DEFINE_FINAL_TYPE (TextyWindow, texty_window, ADW_TYPE_APPLICATION_WINDOW)

// init
static void
texty_window_class_init (TextyWindowClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        gtk_widget_class_set_template_from_resource (widget_class, "/ca/footeware/c/texty/texty-window.ui");
        gtk_widget_class_bind_template_child (widget_class, TextyWindow, header_bar);
        gtk_widget_class_bind_template_child (widget_class, TextyWindow, main_text_view);
        gtk_widget_class_bind_template_child (widget_class, TextyWindow, save_button);
        gtk_widget_class_bind_template_child (widget_class, TextyWindow, cursor_pos);
        gtk_widget_class_bind_template_child (widget_class, TextyWindow, toast_overlay);
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
                          G_CALLBACK (text_viewer_window__save_file),
                          self);
        g_action_map_add_action (G_ACTION_MAP (self),
                                 G_ACTION (save_action));

        // new
        new_action = g_simple_action_new ("new", NULL);
        g_signal_connect (new_action,
                          "activate",
                          G_CALLBACK (text_viewer_window__new_file),
                          self);
        g_action_map_add_action (G_ACTION_MAP (self),
                                 G_ACTION (new_action));

        // open
        open_action = g_simple_action_new ("open", NULL);
        g_signal_connect (open_action,
                          "activate",
                          G_CALLBACK (text_viewer_window__open_file),
                          self);
        g_action_map_add_action (G_ACTION_MAP (self),
                                 G_ACTION (open_action));

        // save as
        save_as_action = g_simple_action_new ("save-as", NULL);
        g_signal_connect (save_as_action,
                          "activate",
                          G_CALLBACK (text_viewer_window__save_as_file),
                          self);
        g_action_map_add_action (G_ACTION_MAP (self),
                                 G_ACTION (save_as_action));

        buffer = gtk_text_view_get_buffer (self->main_text_view);
        g_signal_connect (buffer,
                          "notify::cursor-position",
                          G_CALLBACK (text_viewer_window__update_cursor_position),
                          self);
}

// save file
static void
text_viewer_window__save_file (GAction     *action G_GNUC_UNUSED,
                               GVariant    *param G_GNUC_UNUSED,
                               TextyWindow *self)
{
        g_autoptr (GtkFileDialog) dialog;
        dialog = gtk_file_dialog_new ();

        gtk_file_dialog_save (dialog,
                              GTK_WINDOW (self),
                              NULL,
                              on_save_response,
                              self);
}

static void
on_save_response (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data)
{
        GtkFileDialog *dialog = GTK_FILE_DIALOG (source);
        TextyWindow *self = user_data;

<<<<<<< Upstream, based on origin/master
        file = gtk_file_dialog_save_finish (dialog, result, NULL);
=======
      g_autoptr (GFile) file =
        gtk_file_dialog_save_finish (dialog, result, NULL);
>>>>>>> 206219f Revert "global scope file variable"

<<<<<<< Upstream, based on origin/master
        if (file != NULL)
          save_file (self);
=======
      if (file != NULL)
        save_file (self, file);
>>>>>>> 206219f Revert "global scope file variable"
}

static void
save_file (TextyWindow *self,
           GFile       *file)
 {
        GtkTextIter end;
        char *text;
        g_autoptr(GBytes) bytes;

        GtkTextBuffer *buffer = gtk_text_view_get_buffer (self->main_text_view);

        // Retrieve the iterator at the start of the buffer
        GtkTextIter start;
        gtk_text_buffer_get_start_iter (buffer, &start);

        gtk_text_buffer_get_end_iter (buffer, &end);

        // Retrieve all the visible text between the two bounds
        text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

        // If there is nothing to save, return early
        if (text == NULL)
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
save_file_complete (GObject      *source_object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
        g_autofree char *display_name;
        g_autoptr (GFileInfo) info;
        g_autofree char *msg;
        TextyWindow *self;

<<<<<<< Upstream, based on origin/master
        g_autoptr (GError) error =  NULL;
        g_file_replace_contents_finish (file, result, NULL, &error);
=======
  GFile *file = G_FILE (source_object);

  g_autoptr (GError) error =  NULL;
  g_file_replace_contents_finish (file, result, NULL, &error);
>>>>>>> 206219f Revert "global scope file variable"

        // Query the display name for the file
        display_name = NULL;
        info = g_file_query_info (file,
                           "standard::display-name",
                           G_FILE_QUERY_INFO_NONE,
                           NULL,
                           NULL);
        if (info != NULL)
          {
            display_name =
              g_strdup (g_file_info_get_attribute_string (info, "standard::display-name"));
          }
        else
          {
            display_name = g_file_get_basename (file);
          }

          msg = NULL;
          if (error != NULL)
            msg = g_strdup_printf ("Unable to save as “%s”", display_name);
          else
            msg = g_strdup_printf ("Saved as “%s”", display_name);

          self = TEXTY_WINDOW (user_data);
          adw_toast_overlay_add_toast (self->toast_overlay, adw_toast_new (msg));
}

// new file
static void
text_viewer_window__new_file (void)
{
<<<<<<< Upstream, based on origin/master
        g_print ("window new file\n");

        // check if text buffer has been changed = listen for event?
        if (file != NULL)
            save_file (self);
=======
  g_print ("window new file\n");
>>>>>>> 206219f Revert "global scope file variable"
}

// open file
static void
text_viewer_window__open_file (GAction          *action,
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
open_file (TextyWindow *self,
           GFile       *file)
{
        g_file_load_contents_async (file,
                                    NULL,
                                    (GAsyncReadyCallback) open_file_complete,
                                    self);
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

        // Complete the asynchronous operation; this function will either
        // give you the contents of the file as a byte array, or will
        // set the error argument
        g_file_load_contents_finish (file,
                                     result,
                                     &contents,
                                     &length,
                                     NULL,
                                     &error);

          // Query the display name for the file
          display_name = NULL;
          info = g_file_query_info (file,
                                    "standard::display-name",
                                    G_FILE_QUERY_INFO_NONE,
                                    NULL,
                                    NULL);
        if (info != NULL)
          {
            display_name =
              g_strdup (g_file_info_get_attribute_string (info, "standard::display-name"));
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
        // text displayed by the GtkTextView widget
        buffer = gtk_text_view_get_buffer (self->main_text_view);

        // Set the text using the contents of the file
        gtk_text_buffer_set_text (buffer, contents, length);

        // Reposition the cursor so it's at the start of the text
        gtk_text_buffer_get_start_iter (buffer, &start);
        gtk_text_buffer_place_cursor (buffer, &start);

        // Set the title using the display name
        gtk_window_set_title (GTK_WINDOW (self), display_name);
}

// save as file
static void
text_viewer_window__save_as_file (void)
{
        g_print ("window save-as file\n");
}

// cursor position
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
