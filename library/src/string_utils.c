/*
 * Copyright (c) 2017-2022 Digi International Inc.
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
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "string_utils.h"

/*
 * delete_quotes() - Delete quotes from the given string.
 *
 * @str:	String to delete quotes from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without the quotes.
 */
char *delete_quotes(char *str)
{
	int len = 0;

	if (str == NULL)
		return str;

	len = strlen(str);
	if (len == 0)
		return str;

	if (str[len - 1] == '"')
		str[len - 1] = 0;

	if (str[0] == '"')
		memmove(str, str + 1, len);

	return str;
}

/*
 * delete_leading_spaces() - Delete leading spaces from the given string.
 *
 * @str:	String to delete leading spaces from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without leading white spaces.
 */
char *delete_leading_spaces(char *str)
{
	int len = 0;
	char *p = str;

	if (str == NULL || strlen(str) == 0)
		return str;

	while (isspace(*p) || !isprint(*p))
		++p;

	len = strlen(p);
	memmove(str, p, len);
	str[len] = 0;

	return str;
}

/*
 * delete_trailing_spaces() - Delete trailing spaces from the given string.
 *
 * Trailing spaces also include new line '\n' and carriage return '\r' chars.
 *
 * @str:	String to delete trailing spaces from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without trailing white spaces.
 */
char *delete_trailing_spaces(char *str)
{
	char *p = NULL;

	if (str == NULL || strlen(str) == 0)
		return str;

	p = str + strlen(str) - 1;

	while ((isspace(*p) || !isprint(*p) || *p == 0) && p >= str)
		--p;

	*++p = 0;

	return str;
}

/*
 * trim() - Trim the given string removing leading and trailing spaces.
 *
 * Trailing spaces also include new line '\n' and carriage return '\r' chars.
 *
 * @str:	String to delete leading and trailing spaces from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without leading nor trailing white spaces.
 */
char *trim(char *str)
{
	return delete_leading_spaces(delete_trailing_spaces(str));
}

/*
 * delete_newline_character() - Remove new line character '\n' from the end of
 *                              the given string.
 *
 * @str:	String to delete ending new line character '\n' from.
 *
 * This function modifies the original string.
 *
 * Return: The original string without the final new line.
 */
char *delete_newline_character(char *str)
{
	int len = 0;

	if (str == NULL)
		return str;

	len = strlen(str);
	if (len == 0)
		return str;

	if (str[len - 1] == '\n')
		str[len - 1] = '\0';

	return str;
}
