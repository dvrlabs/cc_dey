/*
 * Copyright (c) 2017 Digi International Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "string_utils.h"

/*
 * delete_quotes() - Delete quotes from the given string.
 *
 * @str:	String to delete quotes from.
 */
void delete_quotes(char *str)
{
	if (str[0] == '"' && str[strlen(str) - 1] == '"') {
		strcpy(str, str + 1);
		str[strlen(str) - 1] = '\0';
	}
}

/*
 * delete_leading_spaces() - Delete leading spaces from the given string.
 *
 * @str:	String to delete leading spaces from.
 */
void delete_leading_spaces(char *str)
{
	unsigned int i;
	char *p = str;

	if (str != NULL) {
		for (i = 0; i < strlen(str); i++) {
			if (isspace(str[i]))
				p++;
			else
				break;
		}
	}
	if (p != str)
		strcpy(str, p);
}

/*
 * delete_trailing_spaces() - Delete trailing spaces from the given string.
 *
 * Trailing spaces also include new line '\n' and carriage return '\r' chars.
 *
 * @str:	String to delete trailing spaces from.
 */
void delete_trailing_spaces(char *str)
{
	int i;

	if (str != NULL) {
		for (i = strlen(str) - 1; i >= 0; i--) {
			if ((str[i] == '\n') || (str[i] == ' ') || (str[i] == '\r'))
				str[i] = '\0';
			else
				break;
		}
	}
}

/*
 * trim() - Trim the given string removing leading and trailing spaces.
 *
 * Trailing spaces also include new line '\n' and carriage return '\r' chars.
 *
 * @str:	String to delete leading and trailing spaces from.
 */
void trim(char *str)
{
	delete_leading_spaces(str);
	delete_trailing_spaces(str);
}

/*
 * delete_newline_character() - Remove new line character '\n' from the end of
 *                              the given string.
 *
 * @str:	String to delete ending new line character '\n' from.
 */
void delete_newline_character(char *str)
{
	if (str != NULL) {
		if (str[strlen(str) - 1] == '\n')
			str[strlen(str) - 1] = '\0';
	}
}
