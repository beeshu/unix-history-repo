/*-
 * Copyright (c) 1995
 *	Paul Richards.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer, 
 *    verbatim and that no modifications are made prior to this 
 *    point in the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Paul Richards.
 * 4. The name Paul Richards may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PAUL RICHARDS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL PAUL RICHARDS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <ctype.h>
#include <err.h>
#include <ncurses.h>
#include <forms.h>
#include <string.h>
#include <stdlib.h>

#include "internal.h"

int done=0;

int
init_field(struct Field *field)
{
	struct Tuple *tuple;
	struct Field *def;
	int i;
	int len, lim;
	
	tuple = form_get_tuple(field->defname, FT_FIELD_DEF);
	if (!tuple) {
		warnx("Field definition not found -- skipping field");
		return (-1);
	}

	def = (struct Field *)tuple->addr;
	field->height = def->height;
	field->width = def->width;
	field->attr = def->attr;
	field->selattr = def->selattr;
	field->type = def->type;
	switch (field->type) {
		case FF_INPUT:
			field->field.input = malloc(sizeof (struct InputField));
			if (!field->field.input) {
				warnx("Couldn't allocate memory for input field");
				return (-1);
			}
			field->field.input->limit = def->field.input->limit;

			/* Force height to one regardless, at least for now :-) */
			field->height = 1;
			if (!field->width && !field->field.input->limit) {
				field->width = strlen(def->field.input->label);
				field->field.input->limit = field->width;
			} else if (!field->width)
				field->width = field->field.input->limit;
			else if (!field->field.input->limit)
				field->field.input->limit = field->width;
			if (field->field.input->limit < field->width)
				field->width = field->field.input->limit;

			field->field.input->input = malloc(field->field.input->limit + 1);
			if (!field->field.input->input) {
				warnx("Couldn't allocate memory for input field text");
				return (-1);
			}
			field->field.input->label = malloc(strlen(def->field.input->label)+1);
			if (!field->field.input->label) {
				warnx("Couldn't allocate memory for input field label");
				return (-1);
			}
			strncpy(field->field.input->label,
					def->field.input->label,
					strlen(def->field.input->label) + 1);
			field->field.input->lbl_flag = def->field.input->lbl_flag;

			/*
			* If it's a label then clear the input string
			* otherwise copy the default there.
			*/
			if (field->field.input->lbl_flag)
				field->field.input->input[0] = '\0';
			else if (field->field.input->label) {
				strncpy(field->field.input->input,
				field->field.input->label,
				field->field.input->limit);
				field->field.input->input[field->field.input->limit] = 0;
			}
			break;
		case FF_TEXT:
			field->field.text = malloc(sizeof (struct TextField));
			if (!field->field.text) {
				warnx("Couldn't allocate memory for text field");
				return (FS_ERROR);
			}
			if (!field->width)
				field->width = strlen(def->field.text->text);
			field->field.text->text = malloc(field->width + 1);
			if (!field->field.text->text) {
				warnx("Couldn't allocate memory for text field text");
				return (FS_ERROR);
			} else
				strncpy(field->field.text->text,
						def->field.text->text,
						field->width + 1);
			if (!field->height)
				calc_field_height(field, field->field.text->text);
			break;
		case FF_MENU:
			field->field.menu = malloc(sizeof (struct MenuField));
			if (!field->field.menu) {
				warnx("Couldn't allocate memory for menu field");
				return (FS_ERROR);
			}
			field->field.menu->no_options = 0;
			field->height = 1;
			lim = 0;
			for (i=0; i < def->field.menu->no_options; i++) {
				field->field.menu->no_options =
						add_menu_option(field->field.menu,
										def->field.menu->options[i]);
				if (!field->field.menu->no_options) {
					warnx("Couldn't add menu option");
					return (FS_ERROR);
				}
				len = strlen(def->field.menu->options[i]);
				if (len > lim)
					lim = len;
			}
			if (!field->width)
				field->width = lim;
			break;
		case FF_ACTION:
			field->field.action = malloc(sizeof (struct ActionField));
			if (!field->field.action) {
				warnx("Couldn't allocate memory for action field");
				return (FS_ERROR);
			}
			if (!field->width)
				field->width = strlen(def->field.action->text);
			field->field.action->text = malloc(field->width + 1);
			if (!field->field.action->text) {
				warnx("Couldn't allocate memory for text field text");
				return (FS_ERROR);
			} else
				strncpy(field->field.action->text,
						def->field.action->text,
						field->width + 1);
			if (!field->height)
				calc_field_height(field, field->field.action->text);
			field->field.action->fn = def->field.action->fn;
			break;
		default:
			break;
	}
	return (0);
}

void
display_field(WINDOW *window, struct Field *field)
{
	wattrset(window, field->attr);
	wmove(window, field->y, field->x);
	switch (field->type) {
		case FF_TEXT:
			display_text(window, field);
			break;
		case FF_MENU:
			display_menu(window, field);
			break;
		case FF_INPUT:
			display_input(window, field);
			break;
		case FF_ACTION:
			display_action(window, field);
			break;
		case FF_UNKNOWN:
		default:
			break;
	}
	wattrset(window, 0);
	wrefresh(window);
}

void
display_text(WINDOW *window, struct Field *field)
{

	if (print_string(window, field->y, field->x, field->height,
	             field->width, field->field.text->text) == ERR)
		print_status("Illegal scroll in print_string");
}

void
display_input(WINDOW *window, struct Field *field)
{
	if (field->field.input->lbl_flag) {
		if (print_string(window, field->y, field->x, field->height,
						 field->width, field->field.input->label) == ERR)
			print_status("Illegal scroll in print_string");
	} else 
		if (print_string(window, field->y, field->x, field->height,
						 field->width, field->field.input->input) == ERR)
			print_status("Illegal scroll in print_string");
}

void
display_menu(WINDOW *window, struct Field *field)
{
	if (print_string(window, field->y, field->x,
					 field->height, field->width,
			field->field.menu->options[field->field.menu->selected]) == ERR)
		print_status("Illegal scroll in print_string");
}

void
display_action(WINDOW *window, struct Field *field)
{
	if (print_string(window, field->y, field->x, field->height,
				field->width,
				field->field.action->text) == ERR)
		print_status("Illegal scroll in print_string");
}

int
do_action(struct Form *form)
{
	struct Field *field = form->current_field;
	struct Tuple *tuple;
	int ch;
	void (* fn)();

	display_action(form->window, field);
	wmove(form->window, field->y, field->x);

	for (;;) {

		ch = wgetch(form->window);

		if (ch == FK_ACCEPT) {
			tuple = form_get_tuple(field->field.action->fn, FT_FUNC);
			if (!tuple) {
				print_status("No function bound to action");
				beep();
				continue;
			} else {
				fn = tuple->addr;
				(*fn)(form);
				return (FS_OK);
			}
		} else
			ch = do_key_bind(form, ch);

		if (ch == FS_OK)
			return (FS_OK);
		else if (ch == FS_ERROR)
			continue;
	}
}

int
do_menu(struct Form *form)
{
	struct Field *field = form->current_field;
	int ch;


	for (;;) {

		display_menu(form->window, field);
		wmove(form->window, field->y, field->x);

		ch = wgetch(form->window);

		switch (ch) {
			case ' ':
				print_status("");
				field->field.menu->selected++;
				if (field->field.menu->selected >= field->field.menu->no_options)
					field->field.menu->selected = 0;
				break;
			default:
				ch = do_key_bind(form, ch);
				break;
		}

		if (ch == FS_OK)
			return (FS_OK);
		else if (ch == FS_ERROR) {
			beep();
			continue;
		} else {
			print_status("Hit the space bar to toggle through options");
			beep();
			continue;
		}
	}
}

int
do_field(struct Form *form)
{
	struct Field *field = form->current_field;
	int status;

	switch (field->type) {
		case FF_TEXT:
			status = FS_OK;
			display_text(form->window, field);
			break;
		case FF_INPUT:
			status = do_input(form);
			break;
		case FF_MENU:
			status = do_menu(form);
			break;
		case FF_ACTION:
			status = do_action(form);
			break;
		default:
			status = FF_UNKNOWN;
			beep();
			print_status("Unknown field type");
			form->current_field = form->prev_field;
			break;
	}

	return (status);
}

int
print_string(WINDOW *window, int y, int x,
             int height, int fwidth, char *string)
{
	int len;
	int width;

	if (!string)
		len = -1;

	len = strlen(string);

	if (wmove(window, y, x) == ERR)
		return (ERR);
	while (height--) {
		width = fwidth;
		while (width--) {
			if (len-- > 0) {
				if (waddch(window, *string++) == ERR)
					return (ERR);
			} else
				if (waddch(window, ' ') == ERR)
					return (ERR);
		}
		if (wmove(window, ++y, x) == ERR)
			return (ERR);

	}
	return (OK);
}	

void
print_status(char *msg)
{
	if (wmove(stdscr, LINES-1, 0) == ERR) {
		endwin();
		exit(1);
	}

	wclrtoeol(stdscr);

	wstandout(stdscr);
	if (wprintw(stdscr, "%s",
				msg) == ERR) {
		endwin();
		exit(1);
	}
	wstandend(stdscr);
	wrefresh(stdscr);
}


int
do_input(struct Form *form)
{
	struct Field *field = form->current_field;
	int len;
	int disp_off=0, abspos=0, cursor = 0;
	unsigned int ch;

#define DISPOFF ((len < field->width) ? 0 : len - field->width)
#define CURSPOS ((len < field->width) ? len : field->width)

	len = strlen(field->field.input->input);
	display_input(form->window, field);

	cursor = CURSPOS;
	abspos = cursor;

	for (;;) {

		wmove(form->window, field->y, field->x+cursor);
		wrefresh(form->window);

		ch = wgetch(form->window);
		ch = do_key_bind(form, ch);

		/*
		 * If there was a valid motion command then we've
		 * moved to a new field so just return. If the motion
		 * command was invalid then just go around and get another
		 * keystroke. Otherwise, it was not a motion command.
		 */

		if (ch == FS_OK)
			return (FS_OK);
		else if (ch == FS_ERROR)
			continue;

		print_status("");

		if (field->field.input->lbl_flag) {
			field->field.input->lbl_flag = 0;
		}
		if ((ch == FK_CHOME) || (ch == '')) {
				disp_off = 0;
				cursor = 0;
				abspos = 0;
		} else if ((ch == FK_CEND) || (ch == '')) {
				disp_off = DISPOFF;
				abspos = len;
				cursor = CURSPOS;
		} else if (ch == FK_CDEL) {
			if (!(len-abspos))
				beep();
			else {
				bcopy(field->field.input->input+abspos+1,
						field->field.input->input+abspos,
						len - abspos);
				--len;
			}
		} else if ((ch == FK_CLEFT) || (ch == FK_CBS) || (ch == '')) {
			if (!abspos)
				beep();
			else {
				if (ch == FK_CBS) {
					bcopy(field->field.input->input+abspos,
							field->field.input->input+abspos-1,
							len-abspos+1);
					--len;
				}
				--abspos;
				--cursor;
				if ((disp_off) && (cursor < 0)) {
					--disp_off;
					++cursor;
				}
			}
		} else if ((ch == FK_CRIGHT) || (ch == '')) {
			if (abspos == len)
				beep();
			else {
				++abspos;
				if (++cursor >= field->width) {
					++disp_off;
					--cursor;
				}
			}
		} else if ((isprint(ch)) && (len < field->field.input->limit)){ 
			bcopy(field->field.input->input+abspos,
					 field->field.input->input+abspos+1, len-abspos+1);
			field->field.input->input[abspos++] = ch;
			len++;
			if (++cursor > field->width) {
				++disp_off;
				--cursor;
			}
		} else 
				beep();
		print_string(form->window, field->y, field->x, field->height,
					 field->width, field->field.input->input+disp_off);
	}
}

void
exit_form(struct Form *form)
{
	form->status = FS_EXIT;
}

void
cancel_form(struct Form *form)
{
	form->status = FS_CANCEL;
}
