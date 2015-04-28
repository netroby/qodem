/*
 * forms.c
 *
 * This module is licensed under the GNU General Public License
 * Version 2.  Please see the file "COPYING" in this directory for
 * more information about the GNU General Public License Version 2.
 *
 *     Copyright (C) 2015  Kevin Lamonte
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "common.h"

#include <string.h>                     /* strdup() */
#include <ctype.h>                      /* isspace() */
#include <errno.h>                      /* strerror() */
#include <stdlib.h>                     /* realloc() */
#include <libgen.h>                     /* dirname() */
#if defined(Q_PDCURSES_WIN32) && !defined(__BORLANDC__)
#include <shlwapi.h>                    /* PathMatchSpec() */
#else
#include <fnmatch.h>                    /* fnmatch() */
#endif /* Q_PDCURSES_WIN32 */
#include <assert.h>
#include "console.h"
#include "qodem.h"
#include "options.h"
#include "states.h"
#include "screen.h"
#include "field.h"
#include "help.h"
#include "netclient.h"
#include "forms.h"

/*
 * Prompt for a TCP listening port.
 */
Q_BOOL prompt_listen_port(char ** port) {
        int i;
        void * pick_window;
        struct field * field;
        struct fieldset * pick_form;
        int window_left;
        int window_top;
        int window_height;
        int window_length;
        char * status_string;
        int status_left_stop;
        int field_length;
        char * title;
        int title_left;
        int keystroke;
        Q_BOOL field_visible = Q_FALSE;
        Q_BOOL old_keyboard_blocks = q_keyboard_blocks;
        q_keyboard_blocks = Q_TRUE;

        /* We will use the cursor */
        q_cursor_on();

        window_length = 30;
#ifdef Q_UPNP
        window_height = 7;
#else
        window_height = 6;
#endif /* Q_UPNP */

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = (HEIGHT - STATUS_HEIGHT) / 2;
        if (window_top < 0) {
                window_top = 0;
        }

        /* Put up the status line */
        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);

        status_string = _(" Enter The TCP Listening Port   ESC/`-Exit ");
        status_left_stop = WIDTH - strlen(status_string);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }
        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);

        /* Draw the sub-window */
        pick_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(pick_window) == Q_FALSE) {
                q_cursor_off();
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                q_keyboard_blocks = old_keyboard_blocks;
                return Q_FALSE;
        }

        field_length = 5;
        /* width, toprow, leftcol, fixed, color_active, color_inactive */
        field = field_malloc(field_length, 2, window_length - field_length - 2, Q_TRUE,
                Q_COLOR_PHONEBOOK_FIELD_TEXT, Q_COLOR_WINDOW_FIELD_HIGHLIGHTED);
        pick_form = fieldset_malloc(&field, 1, pick_window);
        field->invisible = Q_TRUE;

        /* Draw the box */
        screen_win_draw_box(pick_window, 0, 0, window_length, window_height);

        /* Place the title */
        title = _("TCP Listen Port");
        title_left = window_length - (strlen(title) + 2);
        if (title_left < 0) {
                title_left = 0;
        } else {
                title_left /= 2;
        }
        screen_win_put_color_printf_yx(pick_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

        /* Place the prompt text */
        i = 1;
        screen_win_put_color_str_yx(pick_window, i, 2, _("1"), Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str_yx(pick_window, i, 4, _(" - Next Available"), Q_COLOR_MENU_TEXT);
        i++;
        screen_win_put_color_str_yx(pick_window, i, 2, _("2. "), Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str_yx(pick_window, i, 4, _(" - Enter Port"), Q_COLOR_MENU_TEXT);
        i++;
#ifdef Q_UPNP
        screen_win_put_color_str_yx(pick_window, i, 2, _("3. "), Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str_yx(pick_window, i, 4, _(" - UPnP"), Q_COLOR_MENU_TEXT);
        i++;
#endif /* Q_UPNP */

        /* Prompt */
        i++;
        screen_win_put_color_str_yx(pick_window, i, 5, _("Your Choice ? "), Q_COLOR_MENU_COMMAND);
        screen_win_move_yx(pick_window, i, 19);

        screen_flush();
        screen_win_flush(pick_window);

        for (;;) {
                qodem_win_getch(pick_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                switch (keystroke) {

                case '`':
                case KEY_ESCAPE:
                        if (field_visible == Q_TRUE) {
                                field_visible = Q_FALSE;
                                field->invisible = Q_TRUE;
                                fieldset_render(pick_form);
                                screen_win_put_color_hline_yx(pick_window, 2, window_length - field_length - 2, ' ', 5, Q_COLOR_WINDOW);
                                screen_win_flush(pick_window);
                                screen_win_move_yx(pick_window, i, 19);
                                break;
                        }

                        /* The abort exit point */
                        fieldset_free(pick_form);
                        screen_delwin(pick_window);

                        q_cursor_off();

                        /* Force a repaint */
                        q_screen_dirty = Q_TRUE;
                        q_keyboard_blocks = old_keyboard_blocks;
                        return Q_FALSE;
                case Q_KEY_BACKSPACE:
                case 0x08:
                        if (field_visible == Q_TRUE) {
                                fieldset_backspace(pick_form);
                        }
                        break;
                case Q_KEY_LEFT:
                        if (field_visible == Q_TRUE) {
                                fieldset_left(pick_form);
                        }
                        break;
                case Q_KEY_RIGHT:
                        if (field_visible == Q_TRUE) {
                                fieldset_right(pick_form);
                        }
                        break;
                case Q_KEY_HOME:
                        if (field_visible == Q_TRUE) {
                                fieldset_home_char(pick_form);
                        }
                        break;
                case Q_KEY_END:
                        if (field_visible == Q_TRUE) {
                                fieldset_end_char(pick_form);
                        }
                        break;
                case Q_KEY_IC:
                        if (field_visible == Q_TRUE) {
                                fieldset_insert_char(pick_form);
                        }
                        break;
                case Q_KEY_DC:
                        if (field_visible == Q_TRUE) {
                                fieldset_delete_char(pick_form);
                        }
                        break;
                case '1':
                        if (field_visible == Q_TRUE) {
                                fieldset_keystroke(pick_form, keystroke);
                        } else {
                                /* The OK exit point */
                                *port = NEXT_AVAILABLE_PORT_STRING;
                                fieldset_free(pick_form);
                                screen_delwin(pick_window);
                                q_cursor_off();

                                /* Force a repaint */
                                q_screen_dirty = Q_TRUE;
                                q_keyboard_blocks = old_keyboard_blocks;
                                return Q_TRUE;
                        }
                        break;
                case '2':
                        if (field_visible == Q_TRUE) {
                                fieldset_keystroke(pick_form, keystroke);
                        } else {
                                field_visible = Q_TRUE;
                                field->invisible = Q_FALSE;
                                fieldset_render(pick_form);
                                break;
                        }
                        break;
#ifdef Q_UPNP
                case '3':
                        if (field_visible == Q_TRUE) {
                                fieldset_keystroke(pick_form, keystroke);
                        } else {
                                /* The OK exit point */
                                *port = UPNP_PORT_STRING;
                                fieldset_free(pick_form);
                                screen_delwin(pick_window);
                                q_cursor_off();

                                /* Force a repaint */
                                q_screen_dirty = Q_TRUE;
                                q_keyboard_blocks = old_keyboard_blocks;
                                return Q_TRUE;
                        }
                        break;
#endif /* Q_UPNP */
                case Q_KEY_ENTER:
                case C_CR:
                        if (field_visible == Q_TRUE) {
                                /* The OK exit point */
                                *port = field_get_char_value(field);
                                if (strlen(*port) == 0) {
                                        /* User pressed enter but field was blank */
                                        break;
                                }
                                fieldset_free(pick_form);
                                screen_delwin(pick_window);
                                q_cursor_off();

                                /* Force a repaint */
                                q_screen_dirty = Q_TRUE;
                                q_keyboard_blocks = old_keyboard_blocks;
                                return Q_TRUE;
                        }
                default:
                        if (field_visible == Q_TRUE) {
                                if (!q_key_code_yes(keystroke)) {
                                        /* Pass normal keys to form driver */
                                        fieldset_keystroke(pick_form, keystroke);
                                }
                        }
                        break;

                }
        }

        /* Should never get here. */
        q_keyboard_blocks = old_keyboard_blocks;
        return Q_FALSE;
} /* ---------------------------------------------------------------------- */

/*
 * Compose key function
 */
int compose_key(Q_BOOL utf8) {
        int message_left;
        void * form_window;
        int window_left;
        int window_top;
        int window_height = 3;
        int window_length;
        int keystroke;
        char code[4] = { '-', '-', '-', '-' };
        int current_place = 0;
        int status_left_stop;
        Q_BOOL dirty = Q_TRUE;
        int keycode;
        int i;

        char * title;
        char * status_prompt;
        if (utf8 == Q_TRUE) {
                title = _("Compose Key (Unicode)");
                status_prompt = _(" DIGIT/HEX-Hexadecimal Keycode   Del/Bksp-Clear   ESC/`-Exit ");
                window_length = 25;
        } else {
                title = _("Compose Key");
                status_prompt = _(" DIGIT-Decimal Keycode   Del/BkSp-Clear   ESC/`-Exit ");
                window_length = 19;
        }

        /* Turn off the cursor */
        if (q_status.visible_cursor == Q_TRUE) {
                q_cursor_off();
        }

        /* Put up the status line */
        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
        status_left_stop = WIDTH - strlen(status_prompt);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }
        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_prompt, Q_COLOR_STATUS);

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                if (q_status.visible_cursor == Q_TRUE) {
                        q_cursor_on();
                } else {
                        q_cursor_off();
                }
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                return -1;
        }

        /* Draw box */
        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

        /* Place title */
        message_left = window_length - (strlen(title) + 2);
        if (message_left < 0) {
                message_left = 0;
        } else {
                message_left /= 2;
        }
        screen_win_put_color_printf_yx(form_window, 0, message_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

        for (;;) {
                if (dirty) {

                        /* Place the inside text */
                        if (utf8 == Q_TRUE) {
                                screen_win_put_color_printf_yx(form_window, 1, 2,
                                        Q_COLOR_MENU_COMMAND,
                                        _("Key code hex: %c %c %c %c"), code[0], code[1], code[2], code[3]);
                        } else {
                                screen_win_put_color_printf_yx(form_window, 1, 2,
                                        Q_COLOR_MENU_COMMAND,
                                        _("Key code: %c %c %c"), code[0], code[1], code[2]);
                        }

                        dirty = Q_FALSE;
                        screen_flush();
                }

                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                if ((keystroke == KEY_ESCAPE) || (keystroke == '`')) {
                        keycode = -1;
                        break;
                }
                if (keystroke != -1) {
                        if ((keystroke == Q_KEY_DC) || (keystroke == 0x08) || (keystroke == Q_KEY_BACKSPACE)) {
                                code[0] = '-';
                                code[1] = '-';
                                code[2] = '-';
                                code[3] = '-';
                                current_place = 0;
                        } else if (utf8 == Q_TRUE) {

                                if (isdigit(keystroke) || ((tolower(keystroke) >= 'a') && (tolower(keystroke) <= 'f'))) {
                                        code[current_place] = keystroke;
                                        current_place++;

                                        if (current_place == 4) {
                                                keycode = 0;
                                                for (i = 0; i < 4; i++) {
                                                        keycode *= 16;
                                                        if (isdigit(code[i])) {
                                                                keycode += (code[i] - '0');
                                                        } else {
                                                                keycode += (tolower(code[i]) - 'a') + 10;
                                                        }
                                                }
                                                break;
                                        }
                                }

                        } else {

                                if (isdigit(keystroke)) {
                                        if ((current_place == 0) && (keystroke > '2')) {
                                                /* Invalid key */
                                                continue;
                                        } else if ((current_place == 1) && (code[0] == '2') && (keystroke > '5')) {
                                                /* Invalid key */
                                                continue;
                                        } else if ((current_place == 2) && (code[0] == '2') && (code[1] == '5') && (keystroke > '5')) {
                                                /* Invalid key */
                                                continue;
                                        }

                                        code[current_place] = keystroke;
                                        current_place++;

                                        if (current_place == 3) {
                                                keycode = ((code[0] - '0') * 100) + ((code[1] - '0') * 10) + (code[2] - '0');
                                                break;
                                        }
                                }
                        }

                        dirty = Q_TRUE;
                }
        }

        /* The OK exit point */
        screen_delwin(form_window);

        if (q_status.visible_cursor == Q_TRUE) {
                q_cursor_on();
        } else {
                q_cursor_off();
        }

        /* Force a repaint */
        q_screen_dirty = Q_TRUE;

        return keycode;
} /* ---------------------------------------------------------------------- */

/* Pick a string for find/find again */
wchar_t * pick_find_string() {
        void * pick_window = NULL;
        struct field * field;
        struct fieldset * pick_form = NULL;
        wchar_t * return_string;
        int window_left;
        int window_top;
        int window_height;
        int window_length;
        char * status_string;
        int status_left_stop;
        int field_length;
        char * title;
        int title_left;
        int keystroke;
        int new_keystroke;
        int flags;
        Q_BOOL dirty = Q_TRUE;
        Q_BOOL old_keyboard_blocks = q_keyboard_blocks;
        q_keyboard_blocks = Q_TRUE;

        window_height = 3;
        window_length = 73;

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        /* ...but six rows above the status line */
        window_top = HEIGHT - STATUS_HEIGHT - 1 - 6;
        if (window_top < 0) {
                window_top = 0;
        }

        status_string = _(" Enter the text to scan for   ESC/`-Exit ");
        status_left_stop = WIDTH - strlen(status_string);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }

        /* Draw the sub-window */
        pick_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(pick_window) == Q_FALSE) {
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                q_keyboard_blocks = old_keyboard_blocks;
                return NULL;
        }

        field_length = window_length - strlen(_("Search for > ")) - 4;

        /* width, toprow, leftcol, fixed, color_active, color_inactive */
        field = field_malloc(field_length, 1, window_length - field_length - 2, Q_FALSE,
                Q_COLOR_PHONEBOOK_FIELD_TEXT, Q_COLOR_WINDOW_FIELD_HIGHLIGHTED);
        pick_form = fieldset_malloc(&field, 1, pick_window);

        title = _("Find Text");
        title_left = window_length - (strlen(title) + 2);
        if (title_left < 0) {
                title_left = 0;
        } else {
                title_left /= 2;
        }

        for (;;) {
                if (dirty == Q_TRUE) {
                        /* Put up the status line */
                        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);

                        /* Draw the box */
                        screen_win_draw_box(pick_window, 0, 0, window_length, window_height);

                        /* Place the title */
                        screen_win_put_color_printf_yx(pick_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

                        /* Place the prompt text */
                        screen_win_put_color_str_yx(pick_window, 1, 2, _("Search for > "), Q_COLOR_MENU_COMMAND);

                        screen_flush();
                        fieldset_render(pick_form);
                        dirty = Q_FALSE;

                } /* if (dirty == Q_TRUE) */

                qodem_win_getch(pick_window, &keystroke, &flags, Q_KEYBOARD_DELAY);
                switch (keystroke) {

                case '`':
                case KEY_ESCAPE:
                        /* The abort exit point */
                        fieldset_free(pick_form);
                        screen_delwin(pick_window);
                        /* Force a repaint */
                        q_screen_dirty = Q_TRUE;
                        q_keyboard_blocks = old_keyboard_blocks;
                        return NULL;
                case Q_KEY_BACKSPACE:
                case 0x08:
                        fieldset_backspace(pick_form);
                        break;
                case Q_KEY_LEFT:
                        fieldset_left(pick_form);
                        break;
                case Q_KEY_RIGHT:
                        fieldset_right(pick_form);
                        break;
                case Q_KEY_HOME:
                        fieldset_home_char(pick_form);
                        break;
                case Q_KEY_END:
                        fieldset_end_char(pick_form);
                        break;
                case Q_KEY_IC:
                        fieldset_insert_char(pick_form);
                        break;
                case Q_KEY_DC:
                        fieldset_delete_char(pick_form);
                        break;
                case Q_KEY_ENTER:
                case C_CR:
                        /* The OK exit point */
                        return_string = field_get_value(field);
                        fieldset_free(pick_form);
                        screen_delwin(pick_window);
                        /* Force a repaint */
                        q_screen_dirty = Q_TRUE;
                        q_keyboard_blocks = old_keyboard_blocks;

                        /* Don't return an empty string */
                        if (wcslen(return_string) == 0) {
                                Xfree(return_string, __FILE__, __LINE__);
                                return NULL;
                        }
                        return return_string;

                case '\\':
                        /* Alt-\ Compose key */
                        if (flags & KEY_FLAG_ALT) {
                                new_keystroke = compose_key(Q_TRUE);
                                if (new_keystroke > 0) {
                                        /* Pass normal keys to form driver */
                                        if (q_key_code_yes(new_keystroke) == 0) {
                                                fieldset_keystroke(pick_form, new_keystroke);
                                        }
                                }
                                q_screen_dirty = Q_TRUE;
                                refresh_handler();
                                dirty = Q_TRUE;
                        }
                        break;

                default:
                        if (!q_key_code_yes(keystroke)) {
                                /* Pass normal keys to form driver */
                                fieldset_keystroke(pick_form, keystroke);
                        }
                        break;

                }
        }

        /* Should never get here. */
        q_keyboard_blocks = old_keyboard_blocks;
        return NULL;
} /* ---------------------------------------------------------------------- */

/*
 * notify_form - present a notification to the user, waiting up to timeout seconds
 *               or for a key press.  Note ALWAYS TURNS OFF CURSOR.
 */
void notify_form(const char * message, const double timeout) {
        notify_prompt_form(_("Status"), message, NULL, Q_FALSE, timeout, "\033` \r");
} /* ---------------------------------------------------------------------- */

/*
 * notify_form_long - present a notification to the user, waiting up to timeout
 *                    seconds or for a key press.  Note ALWAYS TURNS OFF CURSOR.
 *                    lines is how many lines to assume the message will be.
 */
void notify_form_long(char ** message, const double timeout, const int lines) {
        notify_prompt_form_long(message, _("Status"), _(" Message "), Q_FALSE,
                0, "\033` \r", lines);
} /* ---------------------------------------------------------------------- */

/*
 * notify_prompt_form_long - present a notification to the user, waiting up
 *                    to timeout seconds or for a key press.
 *
 *                    lines is how many lines to assume the message will be.
 */
int notify_prompt_form_long(char ** message, const char * prompt,
        const char * status_prompt, const Q_BOOL visible_cursor,
        const double timeout, const char * allowed_chars, int lines) {

        int title_left;
        void * form_window;
        int window_left;
        int window_top;
        int window_height;
        int window_length = 0;
        time_t current_time;
        time_t start_time;
        int keystroke;
        int i;
        const char * title = prompt;
        window_height = 3 + lines - 1;

        for (i = 0; i < lines; i++) {
                if (strlen(message[i]) > window_length) {
                        window_length = strlen(message[i]);
                }
        }
        if (strlen(title) > window_length) {
                window_length = strlen(title);
        }

        if (status_prompt != NULL) {
                int status_left_stop;

                /* Put up the status line */
                screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                status_left_stop = WIDTH - strlen(status_prompt);
                if (status_left_stop <= 0) {
                        status_left_stop = 0;
                } else {
                        status_left_stop /= 2;
                }
                screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_prompt, Q_COLOR_STATUS);
        }

        /* Add room for border + 1 space on each side */
        window_length += 4;

        /* Truncate to fit on screen */
        if (window_length > WIDTH - 1) {
                window_length = WIDTH - 1;
        }

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                return -1;
        }

        /* Draw box */
        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

        /* Place title */
        title_left = window_length - (strlen(title) + 2);
        if (title_left < 0) {
                title_left = 0;
        } else {
                title_left /= 2;
        }
        screen_win_put_color_printf_yx(form_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

        /* Place the inside text */
        for (i = 0; i < lines; i++) {
                screen_win_put_color_str_yx(form_window, 1 + i, 2, message[i], Q_COLOR_MENU_COMMAND);
        }

        if (visible_cursor == Q_TRUE) {
                q_cursor_on();
        } else {
                q_cursor_off();
        }
        screen_flush();

        time(&start_time);
        for (;;) {
                time(&current_time);
                if (timeout != 0) {
                        if (difftime(current_time, start_time) > timeout) {
                                break;
                        }
                }
                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                for (i = 0; i < strlen(allowed_chars); i++) {
                        if (keystroke == allowed_chars[i]) {
                                goto notify_prompt_form_done;
                        }
                }
        }

notify_prompt_form_done:

        /* The OK exit point */
        screen_delwin(form_window);

        /* Force a repaint */
        q_screen_dirty = Q_TRUE;

        return keystroke;
} /* ---------------------------------------------------------------------- */

/*
 * notify_prompt_form - present a notification to the user, waiting up
 *               to timeout seconds or for a key press.  Return the key
 *               pressed.
 */
int notify_prompt_form(const char * message, const char * prompt, const char * status_prompt, const Q_BOOL visible_cursor, const double timeout, const char * allowed_chars) {
        int message_left;
        void * form_window;
        int window_left;
        int window_top;
        int window_height = 3;
        int window_length;
        time_t current_time;
        time_t start_time;
        int keystroke;
        int i;

        if (status_prompt != NULL) {
                int status_left_stop;

                /* Put up the status line */
                screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                status_left_stop = WIDTH - strlen(status_prompt);
                if (status_left_stop <= 0) {
                        status_left_stop = 0;
                } else {
                        status_left_stop /= 2;
                }
                screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_prompt, Q_COLOR_STATUS);
        }

        if (strlen(prompt) > strlen(message)) {
                window_length = strlen(prompt);
        } else {
                window_length = strlen(message);
        }

        /* Add room for border + 1 space on each side */
        if (visible_cursor == Q_TRUE) {
                /* Visible cursor means this is a prompt, so make it a tad bigger */
                window_length += 4 + 2;
        } else {
                window_length += 4;
        }

        /* Truncate to fit on screen */
        if (window_length > WIDTH - 1) {
                window_length = WIDTH - 1;
        }

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                return -1;
        }

        /* Draw box */
        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

        /* Place title */
        message_left = window_length - (strlen(message) + 2);
        if (message_left < 0) {
                message_left = 0;
        } else {
                message_left /= 2;
        }
        screen_win_put_color_printf_yx(form_window, 0, message_left, Q_COLOR_WINDOW_BORDER, " %s ", message);

        /* Place the inside text */
        screen_win_put_color_str_yx(form_window, 1, 2, prompt, Q_COLOR_MENU_COMMAND);

        if (visible_cursor == Q_TRUE) {
                q_cursor_on();
        } else {
                q_cursor_off();
        }
        screen_flush();

        time(&start_time);
        for (;;) {
                time(&current_time);
                if (timeout != 0) {
                        if (difftime(current_time, start_time) > timeout) {
                                break;
                        }
                }
                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                for (i = 0; i < strlen(allowed_chars); i++) {
                        if (keystroke == allowed_chars[i]) {
                                goto notify_prompt_form_done;
                        }
                }
        }

notify_prompt_form_done:

        /* The OK exit point */
        screen_delwin(form_window);

        /* Force a repaint */
        q_screen_dirty = Q_TRUE;

        return keystroke;
} /* ---------------------------------------------------------------------- */

/*
 * save_form - prompt the user for a save file location.  Return NULL
 *             if the user cancelled.  If directory_ok is Q_TRUE, then the
 *             returned value can be a directory.
 */
char * save_form(const char * title, char * initial_value, const Q_BOOL is_directory, const Q_BOOL warn_overwrite) {

#ifdef Q_PDCURSES_WIN32
        const char pathsep = '\\';
#else
        const char pathsep = '/';
#endif /* Q_PDCURSES_WIN32 */

        struct field * field;
        struct fieldset * save_form;
        void * form_window;
        char * return_string;
        int status_left_stop;
        char * status_string;
        int window_left;
        int window_top;
        int window_height = 5;
        int window_length = 74;
        int length = 64;
        int title_left;
        int keystroke;
        struct stat fstats;
        struct file_info * view_directory_return;
        Q_BOOL local_dirty;
        Q_BOOL old_keyboard_blocks = q_keyboard_blocks;
        q_keyboard_blocks = Q_TRUE;

        /* Window will be 2/3 down the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = (HEIGHT - 1 - window_height) * 2;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 3;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                q_keyboard_blocks = old_keyboard_blocks;
                return NULL;
        }

        /* width, toprow, leftcol, fixed, color_active, color_inactive */
        field = field_malloc(length, 2, 4, Q_FALSE,
                Q_COLOR_WINDOW_FIELD_TEXT_HIGHLIGHTED, Q_COLOR_WINDOW_FIELD_HIGHLIGHTED);
        save_form = fieldset_malloc(&field, 1, form_window);

        /* Avoid memory leak */
        initial_value = Xstrdup(initial_value, __FILE__, __LINE__);

        local_dirty = Q_TRUE;
        for (;;) {

save_form_top:

                q_keyboard_blocks = Q_TRUE;
                if (local_dirty == Q_TRUE) {

                        /* Put up the status line */
                        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);

                        status_string = _(" Edit File/Pathname   ESC/`-Exit ");
                        status_left_stop = WIDTH - strlen(status_string);
                        if (status_left_stop <= 0) {
                                status_left_stop = 0;
                        } else {
                                status_left_stop /= 2;
                        }
                        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);

                        /* Weird...have to define the fields and THEN draw the windows contents */
                        /* Draw box */
                        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

                        /* Place title */
                        title_left = window_length - (strlen(title) + 2);
                        if (title_left < 0) {
                                title_left = 0;
                        } else {
                                title_left /= 2;
                        }
                        screen_win_put_color_printf_yx(form_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

                        /* Draw prompt */
                        screen_win_put_color_char_yx(form_window, 2, 2, '>', Q_COLOR_MENU_COMMAND);

                        local_dirty = Q_FALSE;
                }

                field_set_char_value(field, initial_value);
                fieldset_end_char(save_form);
                screen_flush();
                fieldset_render(save_form);

                for (;;) {
                        qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                        switch (keystroke) {

                        case '`':
                        case KEY_ESCAPE:
                                /* The abort exit point */
                                fieldset_free(save_form);
                                screen_delwin(form_window);

                                /* Force a repaint */
                                q_screen_dirty = Q_TRUE;
                                Xfree(initial_value, __FILE__, __LINE__);
                                q_keyboard_blocks = old_keyboard_blocks;
                                return NULL;
                        case Q_KEY_BACKSPACE:
                        case 0x08:
                                fieldset_backspace(save_form);
                                break;
                        case Q_KEY_LEFT:
                                fieldset_left(save_form);
                                break;
                        case Q_KEY_RIGHT:
                                fieldset_right(save_form);
                                break;
                        case Q_KEY_HOME:
                                fieldset_home_char(save_form);
                                break;
                        case Q_KEY_END:
                                fieldset_end_char(save_form);
                                break;
                        case Q_KEY_DC:
                                fieldset_delete_char(save_form);
                                break;
                        case Q_KEY_IC:
                                fieldset_insert_char(save_form);
                                break;
                        case Q_KEY_ENTER:
                        case C_CR:
                                /*
                                 * If the file exists and is a directory, pop up
                                 * a directory pick box on it.
                                 */
                                q_keyboard_blocks = old_keyboard_blocks;
                                return_string = field_get_char_value(field);

                                /* Strip off any terminating path separators (except the first one) */
                                while ( (strlen(return_string) > 1) &&
                                        (return_string[strlen(return_string) - 1] == pathsep)
                                ) {
                                        return_string[strlen(return_string) - 1] = 0;
                                }

                                if (stat(return_string, &fstats) == 0) {
                                        /* See if this is a directory */

                                        if (S_ISDIR(fstats.st_mode)) {

                                                /* This is an existing directory */
                                                if (is_directory == Q_FALSE) {

                                                        /* Popup window */
                                                        q_cursor_off();
                                                        view_directory_return = view_directory(return_string, "");
                                                        q_cursor_on();
                                                        if (view_directory_return != NULL) {

                                                                Xfree(initial_value, __FILE__, __LINE__);
                                                                initial_value = Xstrdup(view_directory_return->name, __FILE__, __LINE__);

                                                                Xfree(view_directory_return->name, __FILE__, __LINE__);
                                                                Xfree(view_directory_return, __FILE__, __LINE__);
                                                                Xfree(return_string, __FILE__, __LINE__);
                                                        }

                                                        local_dirty = Q_TRUE;
                                                        goto save_form_top;
                                                } else {

                                                        /* It is OK to exit here */
                                                        fieldset_free(save_form);
                                                        screen_delwin(form_window);

                                                        /* Force a repaint */
                                                        q_screen_dirty = Q_TRUE;
                                                        Xfree(initial_value, __FILE__, __LINE__);
                                                        q_keyboard_blocks = old_keyboard_blocks;
                                                        return return_string;
                                                }

                                        } else {
                                                /* This is an existing filename */

                                                /* See if the user wants to overwrite */
                                                if (warn_overwrite == Q_TRUE) {
                                                        screen_win_put_color_str_yx(form_window, 3, 2,
                                                                _("File exists, overwrite? "), Q_COLOR_MENU_TEXT);
                                                        keystroke = -1;
                                                        while (keystroke == -1) {
                                                                q_keyboard_blocks = Q_TRUE;
                                                                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                                                                switch (keystroke) {
                                                                case -1:
                                                                        /* Keep waiting for input */
                                                                        /* Fall through... */
                                                                case 'Y':
                                                                case 'y':
                                                                        /* Overwrite ONLY on Y or y */
                                                                        break;
                                                                default:
                                                                        /* User decided NOT to overwrite the file */
                                                                        local_dirty = Q_TRUE;
                                                                        goto save_form_top;
                                                                }
                                                        }
                                                        /*
                                                         * At this point keystroke contains
                                                         * only 'Y' or 'y'
                                                         */
                                                        assert((keystroke == 'y') || (keystroke == 'Y'));
                                                }

                                                /* It is OK to exit here */
                                                fieldset_free(save_form);
                                                screen_delwin(form_window);

                                                /* Force a repaint */
                                                q_screen_dirty = Q_TRUE;
                                                Xfree(initial_value, __FILE__, __LINE__);
                                                q_keyboard_blocks = old_keyboard_blocks;
                                                return return_string;

                                        } /* if (S_ISDIR(fstats.st_mode)) */

                                } else {
                                        /* New filename */
                                        if (is_directory == Q_FALSE) {
                                                /* It is OK to exit here */
                                                fieldset_free(save_form);
                                                screen_delwin(form_window);

                                                /* Force a repaint */
                                                q_screen_dirty = Q_TRUE;
                                                Xfree(initial_value, __FILE__, __LINE__);
                                                q_keyboard_blocks = old_keyboard_blocks;
                                                return return_string;
                                        }
                                } /* if (stat(return_string, &fstats) == 0) */

                                /*
                                 * Should only get here if stat() failed
                                 * and is_directory == Q_TRUE
                                 */
                                assert(is_directory == Q_TRUE);
                                break;
                        default:
                                if (keystroke != ERR) {
                                        if (!q_key_code_yes(keystroke)) {
                                                /* Pass normal keys to
                                                 * form driver */
                                                fieldset_keystroke(save_form,
                                                        keystroke);
                                        }
                                }
                                break;
                        }
                }
        }

        /* Should never get here. */
        q_keyboard_blocks = old_keyboard_blocks;
        return NULL;
} /* ---------------------------------------------------------------------- */

/*
 * shorten_string - Reduce string to size length.
 */
static void shorten_string(char * string, int length) {

        if (strlen(string) < length - 4) {
                return;
        }

        string[length] = '\0';
        string[length-1] = '.';
        string[length-2] = '.';
        string[length-3] = '.';

} /* ---------------------------------------------------------------------- */

/* The string returned by file_mode_string */
static char file_mode_string_buffer[11];

/*
 * file_mode_string - return a string like "drwx--x-wt" for a file mode
 */
char * file_mode_string(mode_t mode) {
        memset(file_mode_string_buffer, '-', 10);

        if (S_ISDIR(mode)) {
                file_mode_string_buffer[0] = 'd';
#ifndef Q_PDCURSES_WIN32
        } else if (S_ISLNK(mode)) {
                file_mode_string_buffer[0] = 'l';
#endif /* Q_PDCURSES_WIN32 */
        } else if (S_ISCHR(mode)) {
                file_mode_string_buffer[0] = 'c';
        } else if (S_ISBLK(mode)) {
                file_mode_string_buffer[0] = 'b';
        } else if (S_ISFIFO(mode)) {
                file_mode_string_buffer[0] = 'p';
#ifndef Q_PDCURSES_WIN32
        } else if (S_ISSOCK(mode)) {
                file_mode_string_buffer[0] = 's';
#endif /* Q_PDCURSES_WIN32 */
        }

        if (mode & S_IRUSR) {
                file_mode_string_buffer[1] = 'r';
        }
        if (mode & S_IWUSR) {
                file_mode_string_buffer[2] = 'w';
        }

#ifndef Q_PDCURSES_WIN32

        if ((mode & S_IXUSR) && (mode & S_ISUID)) {
                file_mode_string_buffer[3] = 's';
        } else if ((mode & S_IXUSR) && !(mode & S_ISUID)) {
                file_mode_string_buffer[3] = 'x';
        } else if (!(mode & S_IXUSR) && (mode & S_ISUID)) {
                file_mode_string_buffer[3] = 'S';
        }

        if (mode & S_IRGRP) {
                file_mode_string_buffer[4] = 'r';
        }
        if (mode & S_IWGRP) {
                file_mode_string_buffer[5] = 'w';
        }

        if ((mode & S_IXGRP) && (mode & S_ISGID)) {
                file_mode_string_buffer[6] = 's';
        } else if ((mode & S_IXGRP) && !(mode & S_ISGID)) {
                file_mode_string_buffer[6] = 'x';
        } else if (!(mode & S_IXGRP) && (mode & S_ISGID)) {
                file_mode_string_buffer[6] = 'S';
        }

        if (mode & S_IROTH) {
                file_mode_string_buffer[7] = 'r';
        }
        if (mode & S_IWOTH) {
                file_mode_string_buffer[8] = 'w';
        }

        if ((mode & S_IXOTH) && (mode & S_ISVTX)) {
                file_mode_string_buffer[9] = 't';
        } else if ((mode & S_IXOTH) && !(mode & S_ISVTX)) {
                file_mode_string_buffer[9] = 'x';
        } else if (!(mode & S_IXOTH) && (mode & S_ISVTX)) {
                file_mode_string_buffer[9] = 'T';
        }
#endif /* Q_PDCURSES_WIN32 */

        file_mode_string_buffer[10] = '\0';
        return file_mode_string_buffer;
} /* ---------------------------------------------------------------------- */

/*
 * Swap two entries in a file_info array
 */
static void swap_file_info(struct file_info * file_list, int i, int j) {
        struct file_info temp;
        memcpy(&temp, &file_list[i], sizeof(struct file_info));
        memcpy(&file_list[i], &file_list[j], sizeof(struct file_info));
        memcpy(&file_list[j], &temp, sizeof(struct file_info));
} /* ---------------------------------------------------------------------- */

/*
 * GNU uses FNM_FILE_NAME instead of FNM_PATHNAME.  I prefer that too.
 */
#ifndef FNM_FILE_NAME
#define FNM_FILE_NAME FNM_PATHNAME
#endif /* FNM_FILE_NAME */

/*
 * Check a filename against a wildcard filter string
 */
static Q_BOOL match_by_filename(const char * filename, struct stat * fstats, const char * filter) {
#ifdef Q_PDCURSES_WIN32
        BOOL rc = FALSE;
#else
        int rc = 0;
#endif /* Q_PDCURSES_WIN32 */

        /* Directories always "match" the filename filter */
        if (S_ISDIR(fstats->st_mode)) {
                return Q_TRUE;
        }

        if ((strlen(filter) == 0) || (strcmp(filter, "*") == 0)) {
                /* No filter -> always match */
                return Q_TRUE;
        }
#if defined(Q_PDCURSES_WIN32) && !defined(__BORLANDC__)

        /* Check the filename itself */
        rc = PathMatchSpec(filename, filter);
        if (rc == TRUE) {
                /* Match */
                return Q_TRUE;
        }
#else
        /* Check the filename itself */
        rc = fnmatch(filter, filename, FNM_FILE_NAME | FNM_PERIOD);
        if ((rc != 0) && (rc != FNM_NOMATCH)) {
                fprintf(stderr, "fnmatch() for pattern %s in string %s failed: %s\n", filter, filename, strerror(errno));
                return Q_FALSE;
        }

        if (rc == 0) {
                /* Match */
                return Q_TRUE;
        }

#endif /* Q_PDCURSES_WIN32 */

        /* Did not match */
        return Q_FALSE;
}

/*
 * Popup the directory pick box
 */
struct file_info * view_directory(const char * initial_directory, const char * filter) {
        void * pick_window;
        int window_left;
        int window_top;
        int window_height;
        int window_length;
        char * title;
        char selection_buffer[FILENAME_SIZE];
        int status_left_stop = 0;
        char * status_string = "";
        int title_left;
        int keystroke;
        int i;
        int selected_field;
#ifdef Q_PDCURSES_WIN32
        const char pathsep = '\\';
#else
        const char pathsep = '/';
#endif /* Q_PDCURSES_WIN32 */

        DIR * directory = NULL;
        struct dirent * dir_entry;
        char * current_directory_name;
        int files_n = 0;
        int page_size;
        char * full_filename;
        struct file_info * return_file_info;
        struct stat fstats;

        /* Deliberate: NULL to the first call to realloc() means malloc() */
        struct file_info * file_list = NULL;
        int file_list_idx;
        int swaps;
        Q_BOOL reload;
        Q_BOOL skip_hidden = Q_TRUE;

        Q_BOOL old_keyboard_blocks = q_keyboard_blocks;
        q_keyboard_blocks = Q_TRUE;

        current_directory_name = Xstrdup(initial_directory, __FILE__, __LINE__);

        /* Nix the trailing '/' */
        while ((strlen(current_directory_name) > 1) && (current_directory_name[strlen(current_directory_name) - 1] == pathsep)) {
                current_directory_name[strlen(current_directory_name) - 1] = 0;
        }

        /* Display the filter string */
        if (strlen(filter) > 0) {
                title = (char *)Xmalloc(strlen(current_directory_name) + strlen(filter) + 3, __FILE__, __LINE__);
                snprintf(title, strlen(current_directory_name) + strlen(filter) + 2, "%s%c%s", current_directory_name, pathsep, filter);
        } else {
                title = (char *)Xstrdup(current_directory_name, __FILE__, __LINE__);
        }
        shorten_string(title, sizeof(selection_buffer));

        for (;;) {

                /* Cleanup file_list if I reload */
                if (file_list != NULL) {
                        for (i = 0; i < files_n; i++) {
                                Xfree(file_list[i].name, __FILE__, __LINE__);
                        }
                        Xfree(file_list, __FILE__, __LINE__);
                        file_list = NULL;
                }

                /* Read directory */
                assert(directory == NULL);
                directory = opendir(current_directory_name);
                if (directory == NULL) {
                        snprintf(selection_buffer, sizeof(selection_buffer), _("Error opening directory %s: %s"), current_directory_name, strerror(errno));
                        notify_form(selection_buffer, 0);
                        q_keyboard_blocks = old_keyboard_blocks;
                        return NULL;
                }
                files_n = 0;

                dir_entry = readdir(directory);
                while (dir_entry != NULL) {
                        char * full_filename;

                        /*
                         * Get the full filename
                         */
                        full_filename = (char *)Xmalloc(strlen(dir_entry->d_name) + strlen(current_directory_name) + 2, __FILE__, __LINE__);
                        memset(full_filename, 0, strlen(dir_entry->d_name) + strlen(current_directory_name) + 2);
                        memcpy(full_filename, current_directory_name, strlen(current_directory_name));
                        full_filename[strlen(current_directory_name)] = pathsep;
                        memcpy(full_filename + strlen(current_directory_name) + 1, dir_entry->d_name, strlen(dir_entry->d_name));
                        full_filename[strlen(dir_entry->d_name) + strlen(current_directory_name) + 1] = '\0';

                        /* Get the file stats */
#ifdef Q_PDCURSES_WIN32
                        if (stat(full_filename, &fstats) < 0) {
#else
                        if (lstat(full_filename, &fstats) < 0) {
#endif /* Q_PDCURSES_WIN32 */
                                fprintf(stderr, "Can't stat %s: %s\n", full_filename, strerror(errno));
                        }

                        /*
                         * Skip over files that don't meet the filter
                         */
                        if (match_by_filename(dir_entry->d_name, &fstats, filter) == Q_FALSE) {
                                dir_entry = readdir(directory);
                                Xfree(full_filename, __FILE__, __LINE__);
                                full_filename = NULL;
                                continue;
                        }

                        /*
                         * Skip over hidden files
                         */
                        if (skip_hidden == Q_TRUE) {
                                if (dir_entry->d_name[0] == '.') {
                                        if ((strcmp(dir_entry->d_name, ".") != 0) && (strcmp(dir_entry->d_name, "..") != 0)) {
                                                dir_entry = readdir(directory);
                                                Xfree(full_filename, __FILE__, __LINE__);
                                                full_filename = NULL;
                                                continue;
                                        }
                                }
                        }

                        /*
                         * Save this filename
                         */
                        file_list = (struct file_info *)Xrealloc(file_list, sizeof(struct file_info) * (files_n+1), __FILE__, __LINE__);
                        memset(&file_list[files_n], 0, sizeof(struct file_info));
                        file_list[files_n].name = Xstrdup(dir_entry->d_name, __FILE__, __LINE__);

                        /* Save the file stats */
                        memcpy(&file_list[files_n].fstats, &fstats, sizeof(struct stat));

                        /* Increment file_list size */
                        files_n++;

                        /* Get next entry */
                        dir_entry = readdir(directory);
                        Xfree(full_filename, __FILE__, __LINE__);
                        full_filename = NULL;
                }
                assert(directory != NULL);
                closedir(directory);
                directory = NULL;

                /* Sort by filename, but put directories before files */
                swaps = 1;
                while (swaps > 0) {
                        swaps = 0;
                        for (file_list_idx = 0; file_list_idx < files_n-1; file_list_idx++) {

                                if (!S_ISDIR(file_list[file_list_idx].fstats.st_mode) && S_ISDIR(file_list[file_list_idx+1].fstats.st_mode)) {
                                        /* Swap */
                                        swap_file_info(file_list, file_list_idx, file_list_idx + 1);
                                        swaps++;
                                        file_list_idx -= 2;
                                        if (file_list_idx < 0) {
                                                file_list_idx = 0;
                                        }
                                        continue;
                                }
                                if (S_ISDIR(file_list[file_list_idx].fstats.st_mode) && S_ISDIR(file_list[file_list_idx+1].fstats.st_mode)) {
                                        if (strcmp(file_list[file_list_idx].name, file_list[file_list_idx+1].name) > 0) {
                                                /* Swap */
                                                swap_file_info(file_list, file_list_idx, file_list_idx + 1);
                                                swaps++;
                                                file_list_idx -= 2;
                                                if (file_list_idx < 0) {
                                                        file_list_idx = 0;
                                                }
                                        }
                                        continue;
                                }
                                if (!S_ISDIR(file_list[file_list_idx].fstats.st_mode) && !S_ISDIR(file_list[file_list_idx+1].fstats.st_mode)) {
                                        if (strcmp(file_list[file_list_idx].name, file_list[file_list_idx+1].name) > 0) {
                                                /* Swap */
                                                swap_file_info(file_list, file_list_idx, file_list_idx + 1);
                                                swaps++;
                                                file_list_idx -= 2;
                                                if (file_list_idx < 0) {
                                                        file_list_idx = 0;
                                                }
                                        }
                                        continue;
                                }
                        }
                }


                /* Window will be at (2,2) */
                window_left = 2;
                window_top = 2;

                page_size = files_n;
                if (page_size > HEIGHT - STATUS_HEIGHT - window_top - 2) {
                        /* Reduce page_size to fit the screen */
                        page_size = HEIGHT - STATUS_HEIGHT - window_top - 2;
                }

                window_height = page_size + 2;
                window_length = 70;

                /* Draw the sub-window */
                pick_window = screen_subwin(window_height, window_length, window_top, window_left);
                if (check_subwin_result(pick_window) == Q_FALSE) {
                        Xfree(current_directory_name, __FILE__, __LINE__);
                        current_directory_name = NULL;
                        Xfree(title, __FILE__, __LINE__);
                        title = NULL;
                        /* Refresh the underlying screen before returning */
                        q_screen_dirty = Q_TRUE;
                        if ((q_program_state != Q_STATE_DOWNLOAD_PATHDIALOG) && (q_program_state != Q_STATE_UPLOAD_PATHDIALOG)) {
                                if (q_program_state != Q_STATE_PHONEBOOK) {
                                        console_refresh(Q_FALSE);
                                }
                                refresh_handler();
                                screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                                screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);
                                screen_flush();
                        } else {
                                console_refresh(Q_FALSE);
                        }

                        /* No leak */
                        if (file_list != NULL) {
                                for (i = 0; i < files_n; i++) {
                                        Xfree(file_list[i].name, __FILE__, __LINE__);
                                }
                                Xfree(file_list, __FILE__, __LINE__);
                                file_list = NULL;
                        }

                        q_keyboard_blocks = old_keyboard_blocks;
                        return NULL;
                }

                /* Draw box */
                screen_win_draw_box(pick_window, 0, 0, window_length, window_height);

                /* Place title */
                title_left = window_length - (strlen(title) + 2);
                if (title_left < 0) {
                        title_left = 0;
                } else {
                        title_left /= 2;
                }
                screen_win_put_color_printf_yx(pick_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

                /* Put up the status line */
                screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);

                status_string = _(""
                        " <dir> Chdir  F4-Hidden  Arrows-Scroll  Alpha-Search  Enter-Selects  ESC/`-"
                        "Exit ");

                status_left_stop = WIDTH - strlen(status_string);
                if (status_left_stop <= 0) {
                        status_left_stop = 0;
                } else {
                        status_left_stop /= 2;
                }
                screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);
                screen_flush();

                selected_field = 0;

                reload = Q_FALSE;
                while (reload == Q_FALSE) {
                        int page_offset = (selected_field / page_size) * page_size;

                        for (i=0; (i<page_size) && (i + page_offset < files_n); i++) {

                                snprintf(selection_buffer, sizeof(selection_buffer), " %s", file_list[page_offset + i].name);

                                /* Name */
                                if (strlen(selection_buffer) >= 20) {
                                        selection_buffer[20] = ' ';
                                        selection_buffer[21] = '\0';
                                } else if (strlen(selection_buffer) < 20) {
                                        memset(selection_buffer + strlen(selection_buffer), ' ', 21 - strlen(selection_buffer));
                                        selection_buffer[21] = '\0';
                                }

                                /* Size or <dir>*/
                                if (S_ISDIR(file_list[page_offset + i].fstats.st_mode)) {
                                        /* Directory */
                                        snprintf(selection_buffer + strlen(selection_buffer), sizeof(selection_buffer), _("       <dir>"));
                                } else {
                                        /* File, show size */
                                        snprintf(selection_buffer + strlen(selection_buffer), sizeof(selection_buffer), "%12lu", (unsigned long)file_list[page_offset + i].fstats.st_size);
                                }

                                /* Time */
                                strftime(selection_buffer + strlen(selection_buffer), sizeof(selection_buffer), "  %d/%b/%Y %H:%M:%S", localtime(&file_list[page_offset + i].fstats.st_mtime));

                                /* Mask */
                                snprintf(selection_buffer + strlen(selection_buffer), sizeof(selection_buffer), " %s", file_mode_string(file_list[page_offset + i].fstats.st_mode));

                                if (strlen(selection_buffer) < window_length - 3) {
                                        memset(selection_buffer + strlen(selection_buffer), ' ', window_length - 2 - strlen(selection_buffer));
                                        selection_buffer[window_length - 2] = '\0';
                                }

                                if (selected_field == page_offset + i) {
                                        screen_win_put_color_str_yx(pick_window, i + 1, 1, selection_buffer, Q_COLOR_PHONEBOOK_SELECTED);
                                } else {
                                        screen_win_put_color_str_yx(pick_window, i + 1, 1, selection_buffer, Q_COLOR_PHONEBOOK_ENTRY);
                                }
                        }
                        /* Pad out last page with blanks */
                        for (; i<page_size; i++) {
                                screen_win_put_color_hline_yx(pick_window, i + 1, 1, ' ', window_length - 2, Q_COLOR_PHONEBOOK_ENTRY);
                        }

                        /* Replace "<UPARROW> for more" */
                        screen_win_put_color_str_yx(pick_window, window_height - 1, window_length - 15,
                                _("    for more "), Q_COLOR_WINDOW_BORDER);

                        if (selected_field < files_n - 1) {
                                /* Place down arrow since we can go down */
                                screen_win_put_color_char_yx(pick_window, window_height - 1, window_length - 13, cp437_chars[DOWNARROW], Q_COLOR_WINDOW_BORDER);
                        }

                        if (selected_field > 0) {
                                /* Place up arrow since we can go up */
                                screen_win_put_color_char_yx(pick_window, window_height - 1, window_length - 14, cp437_chars[UPARROW], Q_COLOR_WINDOW_BORDER);
                        }

                        screen_win_flush(pick_window);
                        screen_flush();

                        qodem_win_getch(pick_window, &keystroke, NULL, Q_KEYBOARD_DELAY);
                        switch (keystroke) {

                        case '`':
                        case KEY_ESCAPE:
                                /* The abort exit point */
                                screen_delwin(pick_window);
                                Xfree(current_directory_name, __FILE__, __LINE__);
                                current_directory_name = NULL;
                                Xfree(title, __FILE__, __LINE__);
                                title = NULL;

                                /* Refresh the underlying screen before returning */
                                q_screen_dirty = Q_TRUE;
                                if ((q_program_state != Q_STATE_DOWNLOAD_PATHDIALOG) && (q_program_state != Q_STATE_UPLOAD_PATHDIALOG)) {
                                        if (q_program_state != Q_STATE_PHONEBOOK) {
                                                console_refresh(Q_FALSE);
                                        }
                                        refresh_handler();
                                        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                                        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);
                                        screen_flush();
                                } else {
                                        console_refresh(Q_FALSE);
                                }

                                /* No leak */
                                if (file_list != NULL) {
                                        for (i = 0; i < files_n; i++) {
                                                Xfree(file_list[i].name, __FILE__, __LINE__);
                                        }
                                        Xfree(file_list, __FILE__, __LINE__);
                                        file_list = NULL;
                                }

                                q_keyboard_blocks = old_keyboard_blocks;
                                return NULL;
                        case Q_KEY_F(4):
                                if (skip_hidden == Q_TRUE) {
                                        skip_hidden = Q_FALSE;
                                } else {
                                        skip_hidden = Q_TRUE;
                                }
                                screen_delwin(pick_window);
                                reload = Q_TRUE;
                                /* Refresh the underlying screen */
                                q_screen_dirty = Q_TRUE;
                                if ((q_program_state != Q_STATE_DOWNLOAD_PATHDIALOG) && (q_program_state != Q_STATE_UPLOAD_PATHDIALOG)) {
                                        if (q_program_state != Q_STATE_PHONEBOOK) {
                                                console_refresh(Q_FALSE);
                                        }
                                        refresh_handler();
                                        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                                        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);
                                        screen_flush();
                                } else {
                                        console_refresh(Q_FALSE);
                                }
                                break;
                        case Q_KEY_DOWN:
                                selected_field++;
                                if (selected_field == files_n) {
                                        selected_field = 0;
                                }
                                break;
                        case Q_KEY_UP:
                                selected_field--;
                                if (selected_field < 0) {
                                        selected_field = files_n - 1;
                                }
                                break;
                        case Q_KEY_HOME:
                                selected_field = 0;
                                break;
                        case Q_KEY_END:
                                selected_field = files_n - 1;
                                break;
                        case Q_KEY_NPAGE:
                                selected_field += page_size;
                                if (selected_field > files_n - 1) {
                                        selected_field = files_n - 1;
                                }
                                break;
                        case Q_KEY_PPAGE:
                                selected_field -= page_size;
                                if (selected_field < 0) {
                                        selected_field = 0;
                                }
                                break;

                        case Q_KEY_ENTER:
                        case C_CR:

                                if (strcmp(file_list[selected_field].name, ".") == 0) {
                                        /* Special case: '.' */
                                        full_filename = Xstrdup(current_directory_name, __FILE__, __LINE__);
                                } else if (strcmp(file_list[selected_field].name, "..") == 0) {
                                        /* Special case: '..' */
                                        full_filename = (char *)Xstrdup(dirname(current_directory_name), __FILE__, __LINE__);
                                } else {

                                        /* Normal case: a subdirectory or filename */
#ifdef Q_PDCURSES_WIN32
                                        if (strcmp(current_directory_name, "\\") == 0) {
#else
                                        if (strcmp(current_directory_name, "/") == 0) {
#endif /* Q_PDCURSES_WIN32 */

                                                /* Special case: the root directory */
                                                full_filename = (char *)Xmalloc(strlen(file_list[selected_field].name) + 2, __FILE__, __LINE__);
                                                memset(full_filename, 0, strlen(file_list[selected_field].name) + 2);
                                                full_filename[0] = pathsep;
                                                memcpy(full_filename + 1, file_list[selected_field].name, strlen(file_list[selected_field].name));
                                                full_filename[strlen(file_list[selected_field].name) + 1] = '\0';

                                        } else {

                                                full_filename = (char *)Xmalloc(strlen(file_list[selected_field].name) + strlen(current_directory_name) + 2, __FILE__, __LINE__);
                                                memset(full_filename, 0, strlen(file_list[selected_field].name) + strlen(current_directory_name) + 2);
                                                memcpy(full_filename, current_directory_name, strlen(current_directory_name));
                                                full_filename[strlen(current_directory_name)] = pathsep;
                                                memcpy(full_filename + strlen(current_directory_name) + 1, file_list[selected_field].name, strlen(file_list[selected_field].name));
                                                full_filename[strlen(file_list[selected_field].name) + strlen(current_directory_name) + 1] = '\0';

                                        }
                                }

#ifndef Q_PDCURSES_WIN32
                                if (S_ISLNK(file_list[selected_field].fstats.st_mode)) {
                                        /* Follow symlink to underlying file or directory */
                                        if (stat(full_filename, &file_list[selected_field].fstats) < 0) {
                                                goto exit_view_directory;
                                        }
                                }
#endif /* Q_PDCURSES_WIN32 */

                                if (S_ISDIR(file_list[selected_field].fstats.st_mode)) {
                                        /* Switch directory */
                                        Xfree(current_directory_name, __FILE__, __LINE__);
                                        current_directory_name = NULL;
                                        current_directory_name = full_filename;
                                        full_filename = NULL;
                                        Xfree(title, __FILE__, __LINE__);
                                        title = NULL;

                                        /* Display the filter string */
                                        if (strlen(filter) > 0) {
                                                title = (char *)Xmalloc(strlen(current_directory_name) + strlen(filter) + 3, __FILE__, __LINE__);
                                                snprintf(title, strlen(current_directory_name) + strlen(filter) + 2, "%s%c%s", current_directory_name, pathsep, filter);
                                        } else {
                                                title = Xstrdup(current_directory_name, __FILE__, __LINE__);
                                        }

                                        shorten_string(title, sizeof(selection_buffer));
                                        screen_delwin(pick_window);
                                        reload = Q_TRUE;
                                        break;

                                } else {

#ifndef Q_PDCURSES_WIN32
                                exit_view_directory:
#endif /* Q_PDCURSES_WIN32 */

                                        /* Choose file and return */
                                        screen_delwin(pick_window);
                                        Xfree(current_directory_name, __FILE__, __LINE__);
                                        current_directory_name = NULL;
                                        Xfree(title, __FILE__, __LINE__);
                                        title = NULL;
                                        return_file_info = (struct file_info *)Xmalloc(sizeof(struct file_info), __FILE__, __LINE__);
                                        memset(return_file_info, 0, sizeof(struct file_info));
                                        memcpy(return_file_info, &file_list[selected_field], sizeof(struct file_info));
                                        return_file_info->name = full_filename;

                                        /* Refresh the underlying screen before returning */
                                        q_screen_dirty = Q_TRUE;
                                        if ((q_program_state != Q_STATE_DOWNLOAD_PATHDIALOG) && (q_program_state != Q_STATE_UPLOAD_PATHDIALOG)) {
                                                if (q_program_state != Q_STATE_PHONEBOOK) {
                                                        console_refresh(Q_FALSE);
                                                }
                                                refresh_handler();
                                                screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                                                screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);
                                                screen_flush();
                                        } else {
                                                console_refresh(Q_FALSE);
                                        }

                                        /* No leak */
                                        if (file_list != NULL) {
                                                for (i = 0; i < files_n; i++) {
                                                        Xfree(file_list[i].name, __FILE__, __LINE__);
                                                }
                                                Xfree(file_list, __FILE__, __LINE__);
                                                file_list = NULL;
                                        }

                                        q_keyboard_blocks = old_keyboard_blocks;
                                        return return_file_info;
                                }
                                break;
                        default:
                                /* Handle the alpha-search case */
                                if (isalpha(keystroke & 0x7F)) {
                                        /* Start search at one place below selected */
                                        i = selected_field + 1;

                                        while (i != selected_field) {
                                                if (i > files_n - 1) {
                                                        /* Wrap around for circular search */
                                                        i = 0;
                                                        continue;
                                                }
                                                if ((strcmp(file_list[i].name, ".") == 0) || (strcmp(file_list[i].name, "..") == 0)) {
                                                        /* Don't look at '.' or '..' */
                                                        i++;
                                                        continue;
                                                }
                                                if (tolower(file_list[i].name[0]) == tolower(keystroke & 0x7F)) {
                                                        /* Found match on first character */
                                                        break;
                                                }
                                                if (strlen(file_list[i].name) >= 2) {
                                                        if ((file_list[i].name[0] == '.') && (tolower(file_list[i].name[1]) == tolower(keystroke & 0x7F))) {
                                                                /* Found match on first character past dot */
                                                                break;
                                                        }
                                                }
                                                i++;
                                        }
                                        /* Switch selected_field to either the next match or itself if no match */
                                        selected_field = i;
                                }
                                /* Done */
                                break;
                        }
                } /* while (reload == Q_FALSE) */

                q_screen_dirty = Q_TRUE;
                if ((q_program_state != Q_STATE_DOWNLOAD_PATHDIALOG) && (q_program_state != Q_STATE_UPLOAD_PATHDIALOG)) {
                        if (q_program_state != Q_STATE_PHONEBOOK) {
                                console_refresh(Q_FALSE);
                        }
                        refresh_handler();
                        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);
                        screen_flush();
                } else {
                        console_refresh(Q_FALSE);
                }


        } /* for (;;) */

        /* Should never get here. */
        q_keyboard_blocks = old_keyboard_blocks;
        return NULL;
} /* ---------------------------------------------------------------------- */

#define BATCH_ENTRY_FILES_N             20
#define BATCH_ENTRY_FILENAME_LENGTH     30

/*
 * Popup the batch entry window.  If zmodem is Q_TRUE, display the
 * Zmodem upload box, else display the console batch entry window.
 */
struct file_info * batch_entry_window(const char * initial_directory, const Q_BOOL zmodem) {
        struct fieldset * batch_entry_form;
        void * form_window;
        int window_left;
        int window_top;
        int window_height = BATCH_ENTRY_FILES_N + 4;
        /* File name + file size + two spaces + border */
        int window_length = BATCH_ENTRY_FILENAME_LENGTH + 13 + 4 + 2;
        struct field * fields[BATCH_ENTRY_FILES_N];
        char * title;
        int status_left_stop;
        char * status_string;
        int title_left;
        int keystroke;
        int flags;
        int field_number;
        int i;
        int total_size;
        Q_BOOL local_dirty;
        Q_BOOL real_dirty;
        struct file_info * return_file_list;
        int return_file_list_n;
        struct file_info file_info_list[BATCH_ENTRY_FILES_N];
        struct file_info * file_selection;
        char * new_field_value;
        FILE * bew_file;
        char * bew_filename;
        char * filename;
        char bew_line[FILENAME_SIZE];
        char notify_message[DIALOG_MESSAGE_SIZE];
        Q_BOOL old_keyboard_blocks = q_keyboard_blocks;
        q_keyboard_blocks = Q_TRUE;

        title = _("Batch Upload File Entry");

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                        if (file_info_list[i].name != NULL) {
                                Xfree(file_info_list[i].name, __FILE__, __LINE__);
                        }
                }
                q_keyboard_blocks = old_keyboard_blocks;
                return NULL;
        }

        for (i = 0; i < BATCH_ENTRY_FILES_N; i++) {
                /* width, toprow, leftcol, fixed, color_active, color_inactive */
                fields[i] = field_malloc(BATCH_ENTRY_FILENAME_LENGTH, 2 + i, 2, Q_FALSE,
                        Q_COLOR_WINDOW_FIELD_TEXT_HIGHLIGHTED, Q_COLOR_WINDOW_FIELD_HIGHLIGHTED);
        }
        batch_entry_form = fieldset_malloc(fields, BATCH_ENTRY_FILES_N, form_window);

        for (i = 0; i < BATCH_ENTRY_FILES_N; i++) {
                file_info_list[i].name = Xstrdup("", __FILE__, __LINE__);
                file_info_list[i].fstats.st_size = 0;
        }

        /* If BEW file exists, populate the window with it's data */
        bew_filename = get_option(Q_OPTION_BATCH_ENTRY_FILE);
        bew_file = fopen(bew_filename, "r");
        if (bew_file == NULL) {
                /* File doesn't exist, don't use it */
        } else {
                i = 0;
                memset(bew_line, 0, sizeof(bew_line));
                while (!feof(bew_file) && (i < BATCH_ENTRY_FILES_N)) {
                        if (fgets(bew_line, sizeof(bew_line), bew_file) == NULL) {
                                /* This should cause the outer while's feof() check to fail */
                                continue;
                        }
                        bew_line[sizeof(bew_line) - 1] = 0;

                        if ((strlen(bew_line) == 0) || (bew_line[0] == '#')) {
                                /* Empty or comment line */
                                continue;
                        }

                        /* Nix trailing whitespace */
                        while (isspace(bew_line[strlen(bew_line) - 1])) {
                                bew_line[strlen(bew_line) - 1] = 0;
                        }
                        filename = bew_line;
                        while ((isspace(*filename)) && (strlen(filename) > 0)) {
                                filename++;
                        }

                        if (file_info_list[i].name != NULL) {
                                Xfree(file_info_list[i].name, __FILE__, __LINE__);
                        }
                        file_info_list[i].name = Xstrdup(filename, __FILE__, __LINE__);
                        if (stat(file_info_list[i].name, &file_info_list[i].fstats) == -1) {
                                Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                file_info_list[i].name = Xstrdup("", __FILE__, __LINE__);
                                file_info_list[i].fstats.st_size = 0;
                        } else {
                                i++;
                        }
                }

                fclose(bew_file);
        }

        field_number = 0;

        real_dirty = Q_TRUE;
        local_dirty = Q_TRUE;
        for (;;) {

                if (local_dirty == Q_TRUE) {

                        if (real_dirty == Q_TRUE) {

                                /* Refresh background */
                                q_screen_dirty = Q_TRUE;
                                refresh_handler();

                                /* Weird...have to define the fields and THEN draw the windows contents */
                                /* Draw box */
                                screen_win_draw_box(form_window, 0, 0, window_length, window_height);

                                /* Add the "F1 Help" part */
                                screen_win_put_color_str_yx(form_window, window_height - 1, window_length - 10, _("F1 Help"), Q_COLOR_WINDOW_BORDER);

                                /* Place the title */
                                title_left = window_length - (strlen(title) + 2);
                                if (title_left < 0) {
                                        title_left = 0;
                                } else {
                                        title_left /= 2;
                                }
                                screen_win_put_color_printf_yx(form_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

                                /* Headings */
                                screen_win_put_color_str_yx(form_window, 1, 2, _("Filename"), Q_COLOR_MENU_COMMAND);
                                screen_win_put_color_str_yx(form_window, 1, 3 + BATCH_ENTRY_FILENAME_LENGTH, _("Size (kbytes)"), Q_COLOR_MENU_COMMAND);
                                screen_win_put_color_str_yx(form_window, window_height - 2, 2, _("Total size (kbytes):"), Q_COLOR_MENU_COMMAND);

                                total_size = 0;
                                for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                        field_set_char_value(fields[i], file_info_list[i].name);
                                        if (strlen(file_info_list[i].name) > 0) {
                                                screen_win_put_color_printf_yx(form_window, 2 + i, 3 + BATCH_ENTRY_FILENAME_LENGTH, Q_COLOR_MENU_TEXT, " %12lu", file_info_list[i].fstats.st_size);
                                                total_size += file_info_list[i].fstats.st_size;
                                        }
                                }
                                screen_win_put_color_printf_yx(form_window, window_height - 2, 3 + BATCH_ENTRY_FILENAME_LENGTH, Q_COLOR_MENU_TEXT, " %12lu", total_size);

                                real_dirty = Q_FALSE;
                        } /* if (real_dirty == Q_TRUE) */

                        /* If any other drawing needed to be done that didn't overwrite
                           the main window, such as a status line update, this is the
                           place to do it. */

                        /* Put up the status line */
                        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
                        if (zmodem == Q_TRUE) {
                                status_string = _(" F2/Enter-Pick List   F4-Clear   F10/Alt-Enter-Upload   ESC/`-Exit ");
                        } else {
                                status_string = _(" F2/Enter-Pick List   F4-Clear   F10/Alt-Enter-Save to Disk   ESC/`-Exit ");
                        }
                        status_left_stop = WIDTH - strlen(status_string);
                        if (status_left_stop <= 0) {
                                status_left_stop = 0;
                        } else {
                                status_left_stop /= 2;
                        }
                        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);

                        local_dirty = Q_FALSE;

                        screen_flush();

                } /* if (local_dirty == Q_TRUE) */

                fieldset_render(batch_entry_form);
                screen_win_flush(form_window);
                screen_flush();

                qodem_win_getch(form_window, &keystroke, &flags, Q_KEYBOARD_DELAY);

                /* Support alternate keys */
                if ((keystroke == Q_KEY_ENTER) || (keystroke == 0x0D)) {
                        if (flags & KEY_FLAG_ALT) {
                                keystroke = Q_KEY_F(10);
                        } else {
                                keystroke = Q_KEY_F(2);
                        }
                }

                switch (keystroke) {

                case '`':
                case KEY_ESCAPE:
                        /* The abort exit point */

                        for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                if (file_info_list[i].name != NULL) {
                                        Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                }
                        }

                        fieldset_free(batch_entry_form);
                        screen_delwin(form_window);
                        q_keyboard_blocks = old_keyboard_blocks;
                        return NULL;

                case Q_KEY_F(1):
                        launch_help(Q_HELP_BATCH_ENTRY_WINDOW);

                        /* Refresh the whole screen */
                        local_dirty = Q_TRUE;
                        real_dirty = Q_TRUE;
                        break;

                case Q_KEY_DOWN:
                        if (field_number < BATCH_ENTRY_FILES_N - 1) {
                                /* Call this FIRST so the data is available */
                                fieldset_next_field(batch_entry_form);
                                if (file_info_list[field_number].name != NULL) {
                                        Xfree(file_info_list[field_number].name, __FILE__, __LINE__);
                                }
                                file_info_list[field_number].name = field_get_char_value(fields[field_number]);
                                if (stat(file_info_list[field_number].name, &file_info_list[field_number].fstats) != -1) {
                                        screen_win_put_color_printf_yx(form_window, 2 + field_number, 3 + BATCH_ENTRY_FILENAME_LENGTH, Q_COLOR_MENU_TEXT, " %12lu", file_info_list[field_number].fstats.st_size);

                                } else {
                                        file_info_list[field_number].fstats.st_size = 0;
                                        screen_win_put_color_str_yx(form_window, 2 + field_number, 3 + BATCH_ENTRY_FILENAME_LENGTH, "             ", Q_COLOR_MENU_TEXT);
                                }
                                /* Recompute total */
                                total_size = 0;
                                for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                        total_size += file_info_list[i].fstats.st_size;
                                }
                                screen_win_put_color_printf_yx(form_window, window_height - 2, 3 + BATCH_ENTRY_FILENAME_LENGTH, Q_COLOR_MENU_TEXT, " %12lu", total_size);

                                field_number++;

                        }
                        break;

                case Q_KEY_UP:
                        if (field_number > 0) {
                                /* Call this FIRST so the data is available */
                                fieldset_prev_field(batch_entry_form);

                                if (file_info_list[field_number].name != NULL) {
                                        Xfree(file_info_list[field_number].name, __FILE__, __LINE__);
                                }
                                file_info_list[field_number].name = field_get_char_value(fields[field_number]);
                                if (stat(file_info_list[field_number].name, &file_info_list[field_number].fstats) != -1) {
                                        screen_win_put_color_printf_yx(form_window, 2 + field_number, 3 + BATCH_ENTRY_FILENAME_LENGTH, Q_COLOR_MENU_TEXT, " %12lu", file_info_list[field_number].fstats.st_size);

                                } else {
                                        file_info_list[field_number].fstats.st_size = 0;
                                        screen_win_put_color_str_yx(form_window, 2 + field_number, 3 + BATCH_ENTRY_FILENAME_LENGTH, "             ", Q_COLOR_MENU_TEXT);
                                }

                                /* Recompute total */
                                total_size = 0;
                                for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                        total_size += file_info_list[i].fstats.st_size;
                                }
                                screen_win_put_color_printf_yx(form_window, window_height - 2, 3 + BATCH_ENTRY_FILENAME_LENGTH, Q_COLOR_MENU_TEXT, " %12lu", total_size);

                                field_number--;
                        }
                        break;
                case Q_KEY_BACKSPACE:
                case 0x08:
                        fieldset_backspace(batch_entry_form);
                        break;
                case Q_KEY_LEFT:
                        fieldset_left(batch_entry_form);
                        break;
                case Q_KEY_RIGHT:
                        fieldset_right(batch_entry_form);
                        break;
                case Q_KEY_HOME:
                        fieldset_home_char(batch_entry_form);
                        break;
                case Q_KEY_END:
                        fieldset_end_char(batch_entry_form);
                        break;
                case Q_KEY_DC:
                        fieldset_delete_char(batch_entry_form);
                        break;
                case Q_KEY_IC:
                        fieldset_insert_char(batch_entry_form);
                        break;

                case Q_KEY_F(2):
                        q_cursor_off();
                        file_selection = view_directory(initial_directory, "");
                        q_cursor_on();

                        /* Refresh the whole screen */
                        local_dirty = Q_TRUE;
                        real_dirty = Q_TRUE;

                        if (file_selection != NULL) {
                                /* view_directory uses lstat().  For accurate readings we
                                   need to use stat(). */
                                if (stat(file_selection->name, &(file_selection->fstats)) != -1) {
                                        if (file_info_list[field_number].name != NULL) {
                                                Xfree(file_info_list[field_number].name, __FILE__, __LINE__);
                                        }
                                        memcpy(&file_info_list[field_number], file_selection, sizeof(struct file_info));
                                }
                                Xfree(file_selection, __FILE__, __LINE__);
                        }
                        for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                if (i != field_number) {
                                        if (file_info_list[i].name != NULL) {
                                                Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                        }
                                        file_info_list[i].name = field_get_char_value(fields[i]);
                                }
                        }
                        break;

                case Q_KEY_F(4):
                        while (field_number > 0) {
                                fieldset_prev_field(batch_entry_form);
                                field_number--;
                        }
                        for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                if (file_info_list[i].name != NULL) {
                                        Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                }
                                file_info_list[i].name = Xstrdup("", __FILE__, __LINE__);
                                file_info_list[i].fstats.st_size = 0;
                                field_set_char_value(fields[i], file_info_list[i].name);
                        }
                        local_dirty = Q_TRUE;
                        real_dirty = Q_TRUE;
                        break;

                case Q_KEY_F(10):

                        /* The OK exit point */

                        /*
                         * Wow.  Turns out if the entire window is empty this
                         * code will return NULL.  Exactly like I need it to
                         * do.  And I didn't remember structuring it that way.
                         */
                        return_file_list = NULL;
                        return_file_list_n = 0;
                        /* Update the names based on the form */
                        for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                new_field_value = field_get_char_value(fields[i]);
                                if (file_info_list[i].name != NULL) {
                                        Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                        file_info_list[i].name = NULL;
                                }
                                if (strlen(new_field_value) > 0) {
                                        file_info_list[i].name = field_get_char_value(fields[i]);
                                }
                                /* No leak */
                                Xfree(new_field_value, __FILE__, __LINE__);
                        }
                        /* Scan for valid files and build the return list */
                        for (i=0; i<BATCH_ENTRY_FILES_N; i++) {
                                if (stat(file_info_list[i].name, &file_info_list[i].fstats) != -1) {
                                        /* Make sure the file is readable */
#ifndef Q_PDCURSES_WIN32
                                        if ((geteuid() == 0) ||
                                            ((file_info_list[i].fstats.st_uid == geteuid()) && (file_info_list[i].fstats.st_mode & S_IRUSR)) ||
                                            ((file_info_list[i].fstats.st_gid == getegid()) && (file_info_list[i].fstats.st_mode & S_IRGRP)) ||
#else
                                        if (
#endif /* Q_PDCURSES_WIN32 */
                                                (file_info_list[i].fstats.st_mode & S_IRUSR)) {

                                                /* Readable */

                                                /* Add this one to the list we're returning */
                                                return_file_list_n++;
                                                return_file_list = (struct file_info *)Xrealloc(return_file_list, return_file_list_n * sizeof(struct file_info), __FILE__, __LINE__);
                                                memcpy(&return_file_list[return_file_list_n-1], &file_info_list[i], sizeof(struct file_info));
                                        } else {
                                                /* No leak */
                                                Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                        }
                                } else {
                                        /* No leak */
                                        Xfree(file_info_list[i].name, __FILE__, __LINE__);
                                }
                        }
                        if (return_file_list != NULL) {
                                /* Add the terminator entry */
                                return_file_list = (struct file_info *)Xrealloc(return_file_list, (return_file_list_n+1) * sizeof(struct file_info), __FILE__, __LINE__);
                                memset(&return_file_list[return_file_list_n], 0, sizeof(struct file_info));

                                /* Save to disk */
                                bew_file = fopen(bew_filename, "w");
                                if (bew_file == NULL) {
                                        snprintf(notify_message, sizeof(notify_message), _("Error opening file \"%s\" for writing: %s"), bew_filename, strerror(errno));
                                        notify_form(notify_message, 0);
                                        q_cursor_on();
                                } else {
                                        /* Save to disk */
                                        for (i = 0; return_file_list[i].name != NULL; i++) {
                                                fprintf(bew_file, "%s\n", return_file_list[i].name);
                                        }
                                        fclose(bew_file);
                                }

                                /*
                                 * Since I used memcpy on return_file_list, the pointers in file_info_list
                                 * will be directly returned to the caller, so I can't free them here like
                                 * I do on Q_KEY_F(4) and KEY_ESCAPE.
                                 */
                        }

                        fieldset_free(batch_entry_form);
                        screen_delwin(form_window);
                        q_keyboard_blocks = old_keyboard_blocks;
                        return return_file_list;

                default:

                        if (!q_key_code_yes(keystroke)) {
                                /* Pass normal keys to form driver */
                                fieldset_keystroke(batch_entry_form, keystroke);
                        }
                        break;
                }

        } /* for (;;) */

        /* Should never get here. */
        q_keyboard_blocks = old_keyboard_blocks;
        return NULL;

} /* ---------------------------------------------------------------------- */

#ifndef Q_NO_SERIAL

/* Prompt for new communication settings */
Q_BOOL comm_settings_form(const char * title, Q_BAUD_RATE * baud, DATA_BITS * data_bits, Q_PARITY * parity, STOP_BITS * stop_bits, Q_BOOL * xonxoff, Q_BOOL * rtscts) {
        void * form_window;
        int status_left_stop;
        char * status_string;
        int window_left;
        int window_top;
        int window_height = 20;
        int window_length = 37;
        int title_left;
        int keystroke;
        Q_BOOL local_dirty;

        /* The new version */
        Q_BAUD_RATE new_baud = *baud;
        DATA_BITS new_data_bits = *data_bits;
        Q_PARITY new_parity = *parity;
        STOP_BITS new_stop_bits = *stop_bits;
        Q_BOOL new_xonxoff = *xonxoff;
        Q_BOOL new_rtscts = *rtscts;

        Q_BOOL old_keyboard_blocks = q_keyboard_blocks;
        q_keyboard_blocks = Q_TRUE;

        /* Window will be 1/3 down the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = (HEIGHT - 1 - window_height);
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 3;
        }

        /* Put up the status line */
        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
        status_string = _(" LETTER-Select a Comm Parameter   ENTER-Done   ESC/`-Exit ");
        status_left_stop = WIDTH - strlen(status_string);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }
        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_string, Q_COLOR_STATUS);

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                q_keyboard_blocks = old_keyboard_blocks;
                return Q_FALSE;
        }

        /* Place the title */
        title_left = window_length - (strlen(title) + 2);
        if (title_left < 0) {
                title_left = 0;
        } else {
                title_left /= 2;
        }

        local_dirty = Q_TRUE;

        for (;;) {
                if (local_dirty == Q_TRUE) {
                        /* Re-draw the screen*/
                        /* Draw box */
                        screen_win_draw_box(form_window, 0, 0, window_length, window_height);
                        screen_win_put_color_printf_yx(form_window, 0, title_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

                        /* Add the "F1 Help" part */
                        screen_win_put_color_str_yx(form_window, window_height - 1, window_length - 10, _("F1 Help"), Q_COLOR_WINDOW_BORDER);

                        /* Show currect parameters */
                        screen_win_put_color_str_yx(form_window,  2, 8, _("CURRENT: "), Q_COLOR_MENU_COMMAND);

                        screen_win_put_color_printf_yx(form_window,  2, 9 + 8, Q_COLOR_MENU_COMMAND, "%6s %s%s%s", baud_string(new_baud),
                                data_bits_string(new_data_bits), parity_string(new_parity, Q_TRUE), stop_bits_string(new_stop_bits));
                        screen_win_put_color_printf_yx(form_window,  3, 9 + 5, Q_COLOR_MENU_COMMAND, "%s %s", (new_xonxoff == Q_TRUE ? "XON/XOFF" : "        "),
                                (new_rtscts == Q_TRUE ? "RTS/CTS" : "       "));

                        /* Baud rates */
                        screen_win_put_color_str_yx(form_window,  4, 7, _("BAUD"), Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  6, 4, "A.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  6, 6, "    300", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  7, 4, "B.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  7, 6, "   1200", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  8, 4, "C.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  8, 6, "   2400", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  9, 4, "D.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  9, 6, "   4800", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 10, 4, "E.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 10, 6, "   9600", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 11, 4, "F.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 11, 6, "  19200", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 12, 4, "G.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 12, 6, "  38400", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 13, 4, "H.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 13, 6, "  57600", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 14, 4, "I.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 14, 6, " 115200", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 15, 4, "J.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 15, 6, " 230400", Q_COLOR_MENU_TEXT);

                        /* Data bits */
                        screen_win_put_color_str_yx(form_window,  4, 16, _("DATA"), Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  6, 16, "K.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  6, 18, " 8", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  7, 16, "L.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  7, 18, " 7", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  8, 16, "M.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  8, 18, " 6", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  9, 16, "N.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  9, 18, " 5", Q_COLOR_MENU_TEXT);

                        /* Stop bits */
                        screen_win_put_color_str_yx(form_window, 12, 16, _("STOP"), Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 14, 16, "O.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 14, 18, " 1", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 15, 16, "P.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 15, 18, " 2", Q_COLOR_MENU_TEXT);

                        /* Parity */
                        screen_win_put_color_str_yx(form_window,  4, 23, _("PARITY"), Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  6, 23, "Q.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  6, 25, " NONE", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  7, 23, "R.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  7, 25, " ODD", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  8, 23, "S.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  8, 25, " EVEN", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  9, 23, "T.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  9, 25, " MARK", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window, 10, 23, "U.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window, 10, 25, " SPACE", Q_COLOR_MENU_TEXT);

                        /* Flow control */
                        screen_win_put_color_str_yx(form_window,  12, 23, _("FLOW"), Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  14, 23, "V.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  14, 25, " XON/XOFF", Q_COLOR_MENU_TEXT);
                        screen_win_put_color_str_yx(form_window,  15, 23, "W.", Q_COLOR_MENU_COMMAND);
                        screen_win_put_color_str_yx(form_window,  15, 25, " RTS/CTS", Q_COLOR_MENU_TEXT);

                        q_cursor_on();
                        screen_win_put_color_str_yx(form_window, 17, 9, _("Your Choice ? "), Q_COLOR_MENU_COMMAND);
                        /* Place prompt */
                        screen_win_move_yx(form_window, 17, 23);

                        screen_flush();
                        screen_win_flush(form_window);

                        local_dirty = Q_FALSE;
                }

                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);

                if (!q_key_code_yes(keystroke)) {
                        /* Regular character, process and refresh */
                        switch (keystroke) {

                        case 'A':
                        case 'a':
                                new_baud = Q_BAUD_300;
                                break;

                        case 'B':
                        case 'b':
                                new_baud = Q_BAUD_1200;
                                break;

                        case 'C':
                        case 'c':
                                new_baud = Q_BAUD_2400;
                                break;

                        case 'D':
                        case 'd':
                                new_baud = Q_BAUD_4800;
                                break;

                        case 'E':
                        case 'e':
                                new_baud = Q_BAUD_9600;
                                break;

                        case 'F':
                        case 'f':
                                new_baud = Q_BAUD_19200;
                                break;

                        case 'G':
                        case 'g':
                                new_baud = Q_BAUD_38400;
                                break;

                        case 'H':
                        case 'h':
                                new_baud = Q_BAUD_57600;
                                break;

                        case 'I':
                        case 'i':
                                new_baud = Q_BAUD_115200;
                                break;

                        case 'J':
                        case 'j':
                                new_baud = Q_BAUD_230400;
                                break;

                        case 'K':
                        case 'k':
                                if ((new_parity != Q_PARITY_MARK) && (new_parity != Q_PARITY_SPACE)) {
                                        new_data_bits = DATA_BITS_8;
                                }
                                break;

                        case 'L':
                        case 'l':
                                new_data_bits = DATA_BITS_7;
                                break;

                        case 'M':
                        case 'm':
                                if ((new_parity != Q_PARITY_MARK) && (new_parity != Q_PARITY_SPACE)) {
                                        new_data_bits = DATA_BITS_6;
                                }
                                break;

                        case 'N':
                        case 'n':
                                if ((new_parity != Q_PARITY_MARK) && (new_parity != Q_PARITY_SPACE)) {
                                        new_data_bits = DATA_BITS_5;
                                }
                                break;

                        case 'O':
                        case 'o':
                                new_stop_bits = STOP_BITS_1;
                                break;

                        case 'P':
                        case 'p':
                                new_stop_bits = STOP_BITS_2;
                                break;

                        case 'Q':
                        case 'q':
                                new_parity = Q_PARITY_NONE;
                                break;

                        case 'R':
                        case 'r':
                                new_parity = Q_PARITY_ODD;
                                break;

                        case 'S':
                        case 's':
                                new_parity = Q_PARITY_EVEN;
                                break;

                        case 'T':
                        case 't':
                                new_parity = Q_PARITY_MARK;
                                new_data_bits = DATA_BITS_7;
                                break;

                        case 'U':
                        case 'u':
                                new_parity = Q_PARITY_SPACE;
                                new_data_bits = DATA_BITS_7;
                                break;

                        case 'V':
                        case 'v':
                                if (new_xonxoff == Q_TRUE) {
                                        new_xonxoff = Q_FALSE;
                                } else {
                                        new_xonxoff = Q_TRUE;
                                }
                                break;

                        case 'W':
                        case 'w':
                                if (new_rtscts == Q_TRUE) {
                                        new_rtscts = Q_FALSE;
                                } else {
                                        new_rtscts = Q_TRUE;
                                }
                                break;

                        default:
                                /* Disregard */
                                break;
                        }

                        /* Refresh form window */
                        local_dirty = Q_TRUE;
                }

                switch (keystroke) {

                case Q_KEY_F(1):
                        launch_help(Q_HELP_COMM_PARMS);

                        /* Refresh the whole screen */
                        local_dirty = Q_TRUE;
                        q_screen_dirty = Q_TRUE;
                        refresh_handler();
                        break;

                case '`':
                case KEY_ESCAPE:
                        /* Force a repaint */
                        q_screen_dirty = Q_TRUE;
                        q_keyboard_blocks = old_keyboard_blocks;
                        return Q_FALSE;

                case Q_KEY_ENTER:
                case C_CR:

                        /* The OK exit point */

                        /* Save new values */
                        *baud = new_baud;
                        *data_bits = new_data_bits;
                        *parity = new_parity;
                        *stop_bits = new_stop_bits;
                        *xonxoff = new_xonxoff;
                        *rtscts = new_rtscts;

                        /* Force a repaint */
                        q_screen_dirty = Q_TRUE;
                        q_keyboard_blocks = old_keyboard_blocks;
                        return Q_TRUE;

                default:
                        /* Disregard keystroke */
                        break;
                }
        }

        /* Should never get here. */
        q_keyboard_blocks = old_keyboard_blocks;
        return Q_FALSE;
} /* ---------------------------------------------------------------------- */

#endif /* Q_NO_SERIAL */

/*
 * Prompt to determine what kind of capture file to generate.  Returns
 * Q_CAPTURE_TYPE_ASK if the user cancelled.
 */
Q_CAPTURE_TYPE ask_capture_type() {
        int message_left;
        void * form_window;
        int window_left;
        int window_top;
        int window_height = 8;
        int window_length;
        int keystroke;
        int status_left_stop;
        Q_CAPTURE_TYPE capture_type;
        Q_BOOL done = Q_FALSE;
        int i;

        char * title;
        char * status_prompt;
        title = _("Choose Capture Type");
        status_prompt = _(" LETTER-Select a Capture Type   ESC/`-Exit ");
        window_length = 25;

        /* Use the cursor */
        if (q_status.visible_cursor == Q_FALSE) {
                q_cursor_on();
        }

        /* Put up the status line */
        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
        status_left_stop = WIDTH - strlen(status_prompt);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }
        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_prompt, Q_COLOR_STATUS);

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                capture_type = Q_CAPTURE_TYPE_ASK;
                if (q_status.visible_cursor == Q_TRUE) {
                        q_cursor_on();
                } else {
                        q_cursor_off();
                }
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                return capture_type;
        }

        /* Draw box */
        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

        /* Place title */
        message_left = window_length - (strlen(title) + 2);
        if (message_left < 0) {
                message_left = 0;
        } else {
                message_left /= 2;
        }
        screen_win_put_color_printf_yx(form_window, 0, message_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

        /* Draw the menu */
        i = 2;
        screen_win_put_color_str_yx(form_window, i, 7, "N", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - Normal"), Q_COLOR_MENU_TEXT);
        i++;
        screen_win_put_color_str_yx(form_window, i, 7, "H", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - HTML"), Q_COLOR_MENU_TEXT);
        i++;
        screen_win_put_color_str_yx(form_window, i, 7, "R", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - Raw"), Q_COLOR_MENU_TEXT);
        i++;
        i++;

        /* Prompt */
        screen_win_put_color_str_yx(form_window, i, 5, _("Your Choice ? "), Q_COLOR_MENU_COMMAND);
        screen_win_move_yx(form_window, i, 19);

        screen_flush();
        screen_win_flush(form_window);

        while (done == Q_FALSE) {
                /* Handle keystroke */
                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);

                switch (keystroke) {

                case 'R':
                case 'r':
                        capture_type = Q_CAPTURE_TYPE_RAW;
                        done = Q_TRUE;
                        break;

                case 'H':
                case 'h':
                        capture_type = Q_CAPTURE_TYPE_HTML;
                        done = Q_TRUE;
                        break;

                case 'N':
                case 'n':
                        capture_type = Q_CAPTURE_TYPE_NORMAL;
                        done = Q_TRUE;
                        break;

                case '`':
                        /* Backtick works too */
                case KEY_ESCAPE:
                        capture_type = Q_CAPTURE_TYPE_ASK;
                        done = Q_TRUE;
                        break;

                default:
                        /* Ignore keystroke */
                        break;

                }
        } /* while (done == Q_FALSE) */

        /* The OK exit point */
        screen_delwin(form_window);

        if (q_status.visible_cursor == Q_TRUE) {
                q_cursor_on();
        } else {
                q_cursor_off();
        }

        /* Force a repaint */
        q_screen_dirty = Q_TRUE;

        return capture_type;
} /* ---------------------------------------------------------------------- */

/*
 * Prompt to determine what kind of capture file to generate.  Returns
 * Q_CAPTURE_TYPE_ASK if the user cancelled.
 */
Q_CAPTURE_TYPE ask_save_type() {
        int message_left;
        void * form_window;
        int window_left;
        int window_top;
        int window_height = 7;
        int window_length;
        int keystroke;
        int status_left_stop;
        Q_CAPTURE_TYPE capture_type;
        Q_BOOL done = Q_FALSE;
        int i;

        char * title;
        char * status_prompt;
        title = _("Choose Save Type");
        status_prompt = _(" LETTER-Select a Save File Type   ESC/`-Exit ");
        window_length = 25;

        /* Use the cursor */
        if (q_status.visible_cursor == Q_FALSE) {
                q_cursor_on();
        }

        /* Put up the status line */
        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
        status_left_stop = WIDTH - strlen(status_prompt);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }
        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_prompt, Q_COLOR_STATUS);

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                capture_type = Q_CAPTURE_TYPE_ASK;
                if (q_status.visible_cursor == Q_TRUE) {
                        q_cursor_on();
                } else {
                        q_cursor_off();
                }

                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                return capture_type;
        }

        /* Draw box */
        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

        /* Place title */
        message_left = window_length - (strlen(title) + 2);
        if (message_left < 0) {
                message_left = 0;
        } else {
                message_left /= 2;
        }
        screen_win_put_color_printf_yx(form_window, 0, message_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

        /* Draw the menu */
        i = 2;
        screen_win_put_color_str_yx(form_window, i, 7, "N", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - Normal"), Q_COLOR_MENU_TEXT);
        i++;
        screen_win_put_color_str_yx(form_window, i, 7, "H", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - HTML"), Q_COLOR_MENU_TEXT);
        i++;
        i++;

        /* Prompt */
        screen_win_put_color_str_yx(form_window, i, 5, _("Your Choice ? "), Q_COLOR_MENU_COMMAND);
        screen_win_move_yx(form_window, i, 19);

        screen_flush();
        screen_win_flush(form_window);

        while (done == Q_FALSE) {
                /* Handle keystroke */
                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);

                switch (keystroke) {

                case 'H':
                case 'h':
                        capture_type = Q_CAPTURE_TYPE_HTML;
                        done = Q_TRUE;
                        break;

                case 'N':
                case 'n':
                        capture_type = Q_CAPTURE_TYPE_NORMAL;
                        done = Q_TRUE;
                        break;

                case '`':
                        /* Backtick works too */
                case KEY_ESCAPE:
                        capture_type = Q_CAPTURE_TYPE_ASK;
                        done = Q_TRUE;
                        break;

                default:
                        /* Ignore keystroke */
                        break;

                }
        } /* while (done == Q_FALSE) */

        /* The OK exit point */
        screen_delwin(form_window);

        if (q_status.visible_cursor == Q_TRUE) {
                q_cursor_on();
        } else {
                q_cursor_off();
        }

        /* Force a repaint */
        q_screen_dirty = Q_TRUE;

        return capture_type;
} /* ---------------------------------------------------------------------- */

/*
 * Asks what kind of host mode to switch to.  Returns Q_FALSE if the user
 * cancelled.
 */
Q_BOOL ask_host_type(Q_HOST_TYPE * type) {
        int message_left;
        void * form_window;
        int window_left;
        int window_top;
        int window_height = 9;
        int window_length;
        int keystroke;
        int status_left_stop;
        Q_HOST_TYPE host_type = Q_HOST_TYPE_SOCKET;
        Q_BOOL done = Q_FALSE;
        Q_BOOL abort;
        int i;

        char * title;
        char * status_prompt;
        title = _("Choose Host Mode Type");
        status_prompt = _(" LETTER-Select a Host Mode Type   ESC/`-Exit ");
        window_length = 25;

        /* Use the cursor */
        if (q_status.visible_cursor == Q_FALSE) {
                q_cursor_on();
        }

        /* Put up the status line */
        screen_put_color_hline_yx(HEIGHT - 1, 0, cp437_chars[HATCH], WIDTH, Q_COLOR_STATUS);
        status_left_stop = WIDTH - strlen(status_prompt);
        if (status_left_stop <= 0) {
                status_left_stop = 0;
        } else {
                status_left_stop /= 2;
        }
        screen_put_color_str_yx(HEIGHT - 1, status_left_stop, status_prompt, Q_COLOR_STATUS);

        /* Window will be centered on the screen */
        window_left = WIDTH - 1 - window_length;
        if (window_left < 0) {
                window_left = 0;
        } else {
                window_left /= 2;
        }
        window_top = HEIGHT - 1 - window_height;
        if (window_top < 0) {
                window_top = 0;
        } else {
                window_top /= 2;
        }

        /* Draw the sub-window */
        form_window = screen_subwin(window_height, window_length, window_top, window_left);
        if (check_subwin_result(form_window) == Q_FALSE) {
                if (q_status.visible_cursor == Q_TRUE) {
                        q_cursor_on();
                } else {
                        q_cursor_off();
                }
                /* Force a repaint */
                q_screen_dirty = Q_TRUE;
                *type = host_type;
                return Q_FALSE;
        }

        /* Draw box */
        screen_win_draw_box(form_window, 0, 0, window_length, window_height);

        /* Place title */
        message_left = window_length - (strlen(title) + 2);
        if (message_left < 0) {
                message_left = 0;
        } else {
                message_left /= 2;
        }
        screen_win_put_color_printf_yx(form_window, 0, message_left, Q_COLOR_WINDOW_BORDER, " %s ", title);

        /* Draw the menu */
        i = 2;

#ifndef Q_NO_SERIAL
        screen_win_put_color_str_yx(form_window, i, 7, "1", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - Modem"), Q_COLOR_MENU_TEXT);
        i++;
        screen_win_put_color_str_yx(form_window, i, 7, "2", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - Serial Port"), Q_COLOR_MENU_TEXT);
        i++;
#endif /* Q_NO_SERIAL */

        screen_win_put_color_str_yx(form_window, i, 7, "3", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - Socket"), Q_COLOR_MENU_TEXT);
        i++;
        screen_win_put_color_str_yx(form_window, i, 7, "4", Q_COLOR_MENU_COMMAND);
        screen_win_put_color_str(form_window, _(" - telnetd"), Q_COLOR_MENU_TEXT);
        i++;
        i++;

        /* Prompt */
        screen_win_put_color_str_yx(form_window, i, 5, _("Your Choice ? "), Q_COLOR_MENU_COMMAND);
        screen_win_move_yx(form_window, i, 19);

        screen_flush();
        screen_win_flush(form_window);

        while (done == Q_FALSE) {
                /* Handle keystroke */
                qodem_win_getch(form_window, &keystroke, NULL, Q_KEYBOARD_DELAY);

                switch (keystroke) {

#ifndef Q_NO_SERIAL
                case '1':
                        host_type = Q_HOST_TYPE_MODEM;
                        done = Q_TRUE;
                        abort = Q_FALSE;
                        break;

                case '2':
                        host_type = Q_HOST_TYPE_SERIAL;
                        done = Q_TRUE;
                        abort = Q_FALSE;
                        break;
#endif /* Q_NO_SERIAL */

                case '3':
                        host_type = Q_HOST_TYPE_SOCKET;
                        done = Q_TRUE;
                        abort = Q_FALSE;
                        break;

                case '4':
                        host_type = Q_HOST_TYPE_TELNETD;
                        done = Q_TRUE;
                        abort = Q_FALSE;
                        break;

                case '`':
                        /* Backtick works too */
                case KEY_ESCAPE:
                        done = Q_TRUE;
                        abort = Q_TRUE;
                        break;

                default:
                        /* Ignore keystroke */
                        break;

                }
        } /* while (done == Q_FALSE) */

        /* The OK exit point */
        screen_delwin(form_window);

        if (q_status.visible_cursor == Q_TRUE) {
                q_cursor_on();
        } else {
                q_cursor_off();
        }

        /* Force a repaint */
        q_screen_dirty = Q_TRUE;
        *type = host_type;
        if (abort == Q_TRUE) {
                return Q_FALSE;
        }
        return Q_TRUE;
} /* ---------------------------------------------------------------------- */

/* Display error message if the screen is too small */
Q_BOOL check_subwin_result(void * window) {
        if (window == NULL) {
                char * message[3];
                message[0] = _("Qodem cannot display this dialog box.");
                message[1] = _("Please increase the terminal size to");
                message[2] = _("at least 80 columns by 25 rows.");
                notify_form_long(message, 0, 3);
                return Q_FALSE;
        }
        return Q_TRUE;
} /* ---------------------------------------------------------------------- */