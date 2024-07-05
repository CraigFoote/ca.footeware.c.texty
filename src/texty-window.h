/* texty-window.h
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

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define TEXTY_TYPE_WINDOW (texty_window_get_type())

G_DECLARE_FINAL_TYPE (TextyWindow, texty_window, TEXTY, WINDOW, AdwApplicationWindow)

G_END_DECLS

// save file
static void
text_viewer_window__save_file (GAction     *action G_GNUC_UNUSED,
                               GVariant    *param G_GNUC_UNUSED,
                               TextyWindow *self);

static void
on_save_response (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data);

static void
save_file (TextyWindow *self);

static void
save_file_complete (GObject      *source_object,
                    GAsyncResult *result,
                    gpointer      user_data);

// new file
static void
text_viewer_window__new_file (TextyWindow *self);

// open file
static void
text_viewer_window__open_file (GAction      *action,
                               GVariant     *parameter,
                               TextyWindow  *self);

static void
on_open_response (GObject      *source,
                  GAsyncResult *result,
                  gpointer      user_data);

static void
open_file (TextyWindow *self);

static void
open_file_complete (GObject       *source_object,
                    GAsyncResult  *result,
                    TextyWindow   *self);

// save as file
static void
text_viewer_window__save_as_file (void);

// cursor position
static void
text_viewer_window__update_cursor_position (GtkTextBuffer *buffer,
                                            GParamSpec    *pspec,
                                            TextyWindow   *self);

