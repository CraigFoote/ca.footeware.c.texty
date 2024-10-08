/* texty-application.c
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
#include <glib/gi18n.h>

#include "texty-application.h"
#include "texty-window.h"

struct _TextyApplication
{
  AdwApplication parent_instance;
};

G_DEFINE_FINAL_TYPE (TextyApplication, texty_application, ADW_TYPE_APPLICATION)

TextyApplication *
texty_application_new (const char *application_id,
                       GApplicationFlags flags)
{
  g_return_val_if_fail (application_id != NULL, NULL);

  return g_object_new (TEXTY_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

static void
texty_application_activate (GApplication *app)
{
  GtkWindow *window;

  g_assert (TEXTY_IS_APPLICATION (app));
  window = g_object_new (TEXTY_TYPE_WINDOW,
                         "application", app,
                         NULL);
  gtk_window_present (window);
}

static void
texty_application_class_init (TextyApplicationClass *klass)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  app_class->activate = texty_application_activate;
}

static void
texty_application_about_action (GSimpleAction *action,
                                GVariant *parameter,
                                gpointer user_data)
{
  static const char *developers[] = {
    "Footeware.ca http://Footeware.ca",
    NULL
  };
  TextyApplication *self = user_data;
  GtkWindow *window = NULL;

  g_assert (TEXTY_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  adw_show_about_dialog (GTK_WIDGET (window),
                         "application-name", "texty",
                         "application-icon", "ca.footeware.c.texty",
                         "developer-name", "Another fine mess by Footeware.ca",
                         "translator-credits", _ ("translator-credits"),
                         "version", "1.10.0",
                         "developers", developers,
                         "copyright", "©2024 Craig Foote",
                         NULL);
}

static void
texty_application_new_window_action (GSimpleAction *action,
                                     GVariant *parameter,
                                     gpointer user_data)
{
  GtkWindow *window;
  TextyApplication *self = user_data;

  g_assert (TEXTY_IS_APPLICATION (self));
  window = g_object_new (TEXTY_TYPE_WINDOW,
                         "application",
                         self,
                         NULL);
  gtk_window_present (window);
}

static void
texty_application_quit_action (GSimpleAction *action,
                               GVariant *parameter,
                               gpointer user_data)
{
  TextyApplication *self = user_data;
  g_assert (TEXTY_IS_APPLICATION (self));
  g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
  { "quit", texty_application_quit_action },
  { "about", texty_application_about_action },
  { "new-window", texty_application_new_window_action }
};

static void
texty_application_init (TextyApplication *self)
{
  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   app_actions,
                                   G_N_ELEMENTS (app_actions),
                                   self);

  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.quit",
                                         (const char *[]){
                                             "<Ctrl>q",
                                             NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "win.save",
                                         (const char *[]){
                                             "<Ctrl>s",
                                             NULL,
                                         });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "win.open",
                                         (const char *[]){
                                             "<Ctrl>o",
                                             NULL,
                                         });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "win.new",
                                         (const char *[]){
                                             "<Ctrl>n",
                                             NULL,
                                         });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "win.save-as",
                                         (const char *[]){
                                             "<Ctrl><Shift>s",
                                             NULL,
                                         });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "win.toggle-text-wrap",
                                         (const char *[]){
                                             "<Ctrl><Shift>w",
                                             NULL,
                                         });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.new-window",
                                         (const char *[]){
                                             "<Ctrl><Shift>n",
                                             NULL,
                                         });
}

