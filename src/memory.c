/*
 *  Project   : tin - a Usenet reader
 *  Module    : memory.c
 *  Author    : I. Lea & R. Skrenta
 *  Created   : 1991-04-01
 *  Updated   : 2007-12-30
 *  Notes     :
 *
 * Copyright (c) 1991-2008 Iain Lea <iain@bricbrac.de>, Rich Skrenta <skrenta@pbm.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef TIN_H
#	include "tin.h"
#endif /* !TIN_H */


/*
 * Dynamic arrays maximum (initialized in init_alloc()) & current sizes
 * num_* values are one past top of used part of array
 */
int max_active;
int num_active = -1;
int max_local_attributes;
int num_local_attributes = -1;
int max_newnews;
int num_newnews = 0;
int max_art;
int max_save;
int num_save = 0;

/*
 * Dynamic arrays
 */
int *my_group;				/* .newsrc --> active[] */
long *base;				/* base articles for each thread */
struct t_group *active;			/* active newsgroups */
struct t_attribute *local_attributes = NULL;	/* attributes stores in .tin/attributes */
struct t_newnews *newnews;		/* active file sizes on differnet servers */
struct t_article *arts;			/* articles headers in current group */
struct t_save *save;			/* sorts articles before saving them */

/*
 * Local prototypes
 */
static void free_active_arrays(void);
static void free_attributes(struct t_attribute *attributes);
static void free_newnews_array(void);
static void free_input_history(void);
static void free_global_arrays(void);


/*
 * Dynamic table management
 * These settings are memory conservative: small initial allocations
 * and a 50% expansion on table overflow. A fast vm system with
 * much memory might want to start with higher initial allocations
 * and a 100% expansion on overflow, especially for the arts[] array.
 */
void
init_alloc(
	void)
{
	/*
	 * active file arrays
	 */
	max_active = DEFAULT_ACTIVE_NUM;
	max_newnews = DEFAULT_NEWNEWS_NUM;

	active = my_malloc(sizeof(*active) * max_active);
	newnews = my_malloc(sizeof(*newnews) * max_newnews);
	my_group = my_calloc(1, sizeof(int) * max_active);

	/*
	 * article headers array
	 */
	max_art = DEFAULT_ARTICLE_NUM;

	arts = my_malloc(sizeof(*arts) * max_art);
	base = my_malloc(sizeof(long) * max_art);

	/*
	 * attributes array
	 */
	max_local_attributes = DEFAULT_LOCAL_ATTRIBUTES_NUM;
	expand_local_attributes();

	/*
	 * save file array
	 */
	max_save = DEFAULT_SAVE_NUM;

	save = my_malloc(sizeof(*save) * max_save);

#ifndef USE_CURSES
	screen = (struct t_screen *) 0;
#endif /* !USE_CURSES */
}


void
expand_art(
	void)
{
	max_art += max_art >> 1;		/* increase by 50% */
	arts = my_realloc(arts, sizeof(*arts) * max_art);
	base = my_realloc(base, sizeof(long) * max_art);
}


void
expand_active(
	void)
{
	max_active += max_active >> 1;		/* increase by 50% */
	if (active == NULL) {
		active = my_malloc(sizeof(*active) * max_active);
		my_group = my_calloc(1, sizeof(int) * max_active);
	} else {
		active = my_realloc(active, sizeof(*active) * max_active);
		my_group = my_realloc(my_group, sizeof(int) * max_active);
	}
}


void
expand_local_attributes(
	void)
{
	if ((local_attributes == NULL) || (num_local_attributes < 0)) {
		if (local_attributes == NULL)
			local_attributes = my_malloc(sizeof(*local_attributes) * max_local_attributes);
		num_local_attributes = 0;
	} else {
		max_local_attributes += max_local_attributes >> 1;	/* increase by 50 % */
		local_attributes = my_realloc(local_attributes, sizeof(*local_attributes) * max_local_attributes);
	}
}


void
expand_save(
	void)
{
	max_save += max_save >> 1;		/* increase by 50% */
	save = my_realloc(save, sizeof(struct t_save) * max_save);
}


void
expand_newnews(
	void)
{
	max_newnews += max_newnews >> 1;			/* increase by 50% */
	newnews = my_realloc(newnews, sizeof(struct t_newnews) * max_newnews);
}


#ifndef USE_CURSES
void
init_screen_array(
	t_bool allocate)
{
	int i;

	if (allocate) {
		screen = my_malloc(sizeof(struct t_screen) * cLINES + 1);

		for (i = 0; i < cLINES; i++)
#	if defined(MULTIBYTE_ABLE) && !defined(NO_LOCALE)
			screen[i].col = my_malloc((size_t) (cCOLS * MB_CUR_MAX + 2));
#	else
			screen[i].col = my_malloc((size_t) (cCOLS + 2));
#	endif /* MULTIBYTE_ABLE && !NO_LOCALE */
	} else {
		if (screen != NULL) {
			for (i = 0; i < cLINES; i++)
				FreeAndNull(screen[i].col);

			FreeAndNull(screen);
		}
	}
}
#endif /* !USE_CURSES */


void
free_all_arrays(
	void)
{
	hash_reclaim();

#ifndef USE_CURSES
	if (!batch_mode)
		init_screen_array(FALSE);
#endif /* !USE_CURSES */

	free_art_array();
	free_msgids();
	FreeAndNull(arts);
	free_filter_array(&glob_filter);
	free_active_arrays();
	free_global_arrays();

#ifdef HAVE_COLOR
	FreeIfNeeded(quote_regex.re);
	FreeIfNeeded(quote_regex.extra);
	FreeIfNeeded(quote_regex2.re);
	FreeIfNeeded(quote_regex2.extra);
	FreeIfNeeded(quote_regex3.re);
	FreeIfNeeded(quote_regex3.extra);
#endif /* HAVE_COLOR */
	FreeIfNeeded(slashes_regex.re);
	FreeIfNeeded(slashes_regex.extra);
	FreeIfNeeded(stars_regex.re);
	FreeIfNeeded(stars_regex.extra);
	FreeIfNeeded(strokes_regex.re);
	FreeIfNeeded(strokes_regex.extra);
	FreeIfNeeded(underscores_regex.re);
	FreeIfNeeded(underscores_regex.extra);
	FreeIfNeeded(strip_re_regex.re);
	FreeIfNeeded(strip_re_regex.extra);
	FreeIfNeeded(strip_was_regex.re);
	FreeIfNeeded(strip_was_regex.extra);
	FreeIfNeeded(uubegin_regex.re);
	FreeIfNeeded(uubegin_regex.extra);
	FreeIfNeeded(uubody_regex.re);
	FreeIfNeeded(uubody_regex.extra);
	FreeIfNeeded(verbatim_begin_regex.re);
	FreeIfNeeded(verbatim_begin_regex.extra);
	FreeIfNeeded(verbatim_end_regex.re);
	FreeIfNeeded(verbatim_end_regex.extra);
	FreeIfNeeded(url_regex.re);
	FreeIfNeeded(url_regex.extra);
	FreeIfNeeded(mail_regex.re);
	FreeIfNeeded(mail_regex.extra);
	FreeIfNeeded(news_regex.re);
	FreeIfNeeded(news_regex.extra);
	FreeIfNeeded(shar_regex.re);
	FreeIfNeeded(shar_regex.extra);

	if (!batch_mode) {
		free_keymaps();
		free_input_history();
	}

	FreeAndNull(base);

	if (save != NULL) {
		free_save_array();
		FreeAndNull(save);
	}

	if (newnews != NULL) {
		free_newnews_array();
		FreeAndNull(newnews);
	}

	FreeAndNull(nntp_caps.implementation);

	tin_fgets(NULL, FALSE);
}


void
free_art_array(
	void)
{
	int i;

	for_each_art(i) {
		arts[i].artnum = 0L;
		arts[i].date = (time_t) 0;
		FreeAndNull(arts[i].xref);

		/* .refs & .msgid are free()d in build_references() */
		arts[i].refs = (char *) '\0';
		arts[i].msgid = (char *) '\0';

		if (arts[i].archive) {
			/* ->name is hashed */
			FreeAndNull(arts[i].archive->partnum);
			FreeAndNull(arts[i].archive);
		}
		arts[i].tagged = 0;
		arts[i].thread = ART_EXPIRED;
		arts[i].prev = ART_NORMAL;
		arts[i].status = ART_UNREAD;
		arts[i].killed = ART_NOTKILLED;
		arts[i].selected = FALSE;
	}
}


/*
 * Use this only for attributes that have a fixed default of a static string
 * in tinrc
 */
void
free_if_not_default(
	char **attrib,
	char *deflt)
{
	if (*attrib != deflt)
		FreeAndNull(*attrib);
}


/*
 * Free memory of one attributes struct only
 */
static void
free_attributes(
	struct t_attribute *attributes)
{
	FreeAndNull(attributes->scope);

	free_if_not_default(&attributes->maildir, tinrc.maildir);
	free_if_not_default(&attributes->savedir, tinrc.savedir);

	FreeAndNull(attributes->savefile);

	free_if_not_default(&attributes->sigfile, tinrc.sigfile);
	free_if_not_default(&attributes->organization, default_organization);

	FreeAndNull(attributes->followup_to);

	FreeAndNull(attributes->fcc);

	FreeAndNull(attributes->mailing_list);
	FreeAndNull(attributes->x_headers);
	FreeAndNull(attributes->x_body);

	free_if_not_default(&attributes->from, tinrc.mail_address);
	free_if_not_default(&attributes->news_quote_format, tinrc.news_quote_format);
	free_if_not_default(&attributes->quote_chars, tinrc.quote_chars);

	FreeAndNull(attributes->mime_types_to_save);

#ifdef HAVE_ISPELL
	FreeAndNull(attributes->ispell);
#endif /* HAVE_ISPELL */

	FreeAndNull(attributes->quick_kill_scope);
	FreeAndNull(attributes->quick_select_scope);

#ifdef CHARSET_CONVERSATION
	FreeAndNull(attributes->undeclared_charset);
#endif /* CHARSET_CONVERSATION */
}


/*
 * TODO: fix the leaks in the global case
 *       Are there any leaks left? If there are, they would also be there for
 *       every group, wouldn't they? 2006-11-12, -dn
 */
void
free_attributes_array(
	void)
{
	int i;
	struct t_group *group;

	for_each_group(i) {
		group = &active[i];
		/*
		 * Check for bogus group removed to prevent memory leaks because
		 * attrib.c:do_set_attrib() also doesn't check but allocates memory
		 * unconditionally. Groups may become bogus on a resync of the active
		 * file (after attributes are "applied"), too. 2006-11-12, -dn
		 */
		if (/* !group->bogus && */ group->attribute && !group->attribute->global) {
			free_attributes(group->attribute);
			free(group->attribute);
		}
		group->attribute = (struct t_attribute *) 0;
	}

	/* free the local attributes array */
	while (num_local_attributes > 0)
		free_attributes(&local_attributes[--num_local_attributes]);

	/* free the global attributes array */
	free_attributes(&glob_attributes);
}


static void
free_global_arrays(
	void)
{
	if (news_headers_to_display_array)
		FreeIfNeeded(*news_headers_to_display_array);
	FreeAndNull(news_headers_to_display_array);

	if (news_headers_to_not_display_array)
		FreeIfNeeded(*news_headers_to_not_display_array);
	FreeAndNull(news_headers_to_not_display_array);
}


static void
free_active_arrays(
	void)
{
	int i;

	FreeAndNull(my_group);	/* my_group[] */

	if (active != NULL) {	/* active[] */
		for_each_group(i) {
			FreeAndNull(active[i].name);
			FreeAndNull(active[i].description);
			FreeAndNull(active[i].aliasedto);
			if (active[i].type == GROUP_TYPE_MAIL) {
				FreeAndNull(active[i].spooldir);
			}
			FreeAndNull(active[i].newsrc.xbitmap);
		}
		free_attributes_array();
		FreeAndNull(active);
	}
	FreeAndNull(local_attributes);
	num_local_attributes = -1;
	num_active = -1;
}


void
free_save_array(
	void)
{
	int i;

	for (i = 0; i < num_save; i++) {
		FreeAndNull(save[i].path);
		/* file does NOT need to be freed */
		save[i].file = NULL;
		save[i].mailbox = FALSE;
	}
	num_save = 0;
}


static void
free_newnews_array(
	void)
{
	int i;

	for (i = 0; i < num_newnews; i++)
		FreeAndNull(newnews[i].host);

	num_newnews = 0;
}


static void
free_input_history(
	void)
{
	int his_w, his_e;

	for (his_w = 0; his_w <= HIST_MAXNUM; his_w++) {
		for (his_e = 0; his_e < HIST_SIZE; his_e++) {
			FreeIfNeeded(input_history[his_w][his_e]);
		}
	}
}


void *
my_malloc1(
	const char *file,
	int line,
	size_t size)
{
	void *p;

#ifdef DEBUG
	debug_print_malloc(TRUE, file, line, size);
#endif /* DEBUG */

	if ((p = malloc(size)) == NULL) {
		error_message(txt_out_of_memory, tin_progname, size, file, line);
		giveup();
	}
	return p;
}


/*
 * TODO: add fallback code with malloc(nmemb*size);memset(0,nmemb*size)?
 */
void *
my_calloc1(
	const char *file,
	int line,
	size_t nmemb,
	size_t size)
{
	void *p;

#ifdef DEBUG
	debug_print_malloc(TRUE, file, line, nmemb * size);
#endif /* DEBUG */

	if ((p = calloc(nmemb, size)) == NULL) {
		error_message(txt_out_of_memory, tin_progname, nmemb * size, file, line);
		giveup();
	}
	return p;
}


void *
my_realloc1(
	const char *file,
	int line,
	void *p,
	size_t size)
{
#ifdef DEBUG
	debug_print_malloc(FALSE, file, line, size);
#endif /* DEBUG */

	if (!size) {
		if (p)
			free(p);

		return NULL;
	}

	p = ((!p) ? (malloc(size)) : realloc(p, size));

	if (p == NULL) {
		error_message(txt_out_of_memory, tin_progname, size, file, line);
		giveup();
	}
	return p;
}