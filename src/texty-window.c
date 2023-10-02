/* texty-window.c
 *
 * Copyright 2023 Craig
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "texty-window.h"

struct _TextyWindow
{
  AdwApplicationWindow  parent_instance;

  /* Template widgets */
  AdwHeaderBar  *header_bar;
  GtkButton     *new_button;
  GtkButton     *open_button;
  GtkButton     *save_button;
  GtkButton     *save_as_button;
  GtkTextView   *text_view;

};

G_DEFINE_FINAL_TYPE (TextyWindow, texty_window, ADW_TYPE_APPLICATION_WINDOW)

static void
texty_window_new_action(GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer      user_data)
{
  g_print ("new");
}

/*

static void
open_file_complete (GObject       *source_object,
                    GAsyncResult  *result,
                    TextyWindow   *self)
{
  GtkTextBuffer *buffer;
  GtkTextIter start;
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

  // In case of error, print a warning to the standard error output
  if (error != NULL)
    {
      g_printerr ("Unable to open “%s”: %s\n",
                  g_file_peek_path (file),
                  error->message);
      return;
    }

  // Ensure that the file is encoded with UTF-8
  if (!g_utf8_validate (contents, length, NULL))
    {
      g_printerr ("Unable to load the contents of “%s”: "
                  "the file is not encoded with UTF-8\n",
                  g_file_peek_path (file));
      return;
    }

  // Retrieve the GtkTextBuffer instance that stores the
  // text displayed by the GtkTextView widget
  buffer = gtk_text_view_get_buffer (self->text_view);

  // Set the text using the contents of the file
  gtk_text_buffer_set_text (buffer, contents, length);

  // Reposition the cursor so it's at the start of the text
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_place_cursor (buffer, &start);
}

static void
open_callback (GObject      *source,
               GAsyncResult *result,
               void         *data)
{
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source);
  GFile *file;
  GtkRoot *root;
  GtkWindow *window;

  file = gtk_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      g_print ("File selected: %s", g_file_peek_path (file));
      root = gtk_widget_get_root (dialog);
      window = GTK_WINDOW (root);
      g_file_load_contents_async (file,
                                  NULL,
                                  (GAsyncReadyCallback) open_file_complete,
                                  window);
      g_object_unref (file);
    }
}

static void
texty_window_open_action(GAction      *action G_GNUC_UNUSED,
                         GVariant     *parameter G_GNUC_UNUSED,
                         TextyWindow  *self)
{
  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_open (dialog, GTK_WINDOW (self), NULL, open_callback, self);
  g_object_unref (dialog);
}

static void
texty_window_save_action(GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer      user_data)
{
  TextyWindow *win = TEXTY_WINDOW (user_data);
  GtkTextView *tv = win->text_view;

  GtkTextBuffer *tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));

  /*
  if (! gtk_text_buffer_get_modified (tb))
    return; // no need to save it
  else if (tv->file == NULL)
    tfe_text_view_saveas (tv);
  else
    save_file (tv->file, tb, GTK_WINDOW (win));
  */

}

static void
texty_window_save_as_action(GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer      user_data)
{
  g_print ("save_as");
}

static void
texty_window_class_init (TextyWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ca/footeware/c/texty/texty-window.ui");
  gtk_widget_class_bind_template_child (widget_class, TextyWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, TextyWindow, new_button);
  gtk_widget_class_bind_template_child (widget_class, TextyWindow, open_button);
  gtk_widget_class_bind_template_child (widget_class, TextyWindow, save_button);
  gtk_widget_class_bind_template_child (widget_class, TextyWindow, save_as_button);
}

static void
texty_window_init (TextyWindow *self)
{
  g_autoptr (GSimpleAction) new_action;
  g_autoptr (GSimpleAction) open_action;
  g_autoptr (GSimpleAction) save_action;
  g_autoptr (GSimpleAction) save_as_action;

  gtk_widget_init_template (GTK_WIDGET (self));

  new_action = g_simple_action_new ("new", NULL);
  g_signal_connect (new_action, "activate", G_CALLBACK (texty_window_new_action), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (new_action));

  open_action = g_simple_action_new ("open", NULL);
  g_signal_connect (open_action, "activate", G_CALLBACK (texty_window_open_action), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (open_action));

  save_action = g_simple_action_new ("save", NULL);
  g_signal_connect (save_action, "activate", G_CALLBACK (texty_window_save_action), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (save_action));

  save_as_action = g_simple_action_new ("save_as", NULL);
  g_signal_connect (save_as_action, "activate", G_CALLBACK (texty_window_save_as_action), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (save_as_action));
}

