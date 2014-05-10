/*
 *  Project   : tin - a Usenet reader
 *  Module    : art.c
 *  Author    : I.Lea & R.Skrenta
 *  Created   : 1991-04-01
 *  Updated   : 2011-01-29
 *  Notes     :
 *
 * Copyright (c) 1991-2011 Iain Lea <iain@bricbrac.de>, Rich Skrenta <skrenta@pbm.com>
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
#ifndef TCURSES_H
#	include "tcurses.h"
#endif /* !TCURSES_H */
#ifndef NEWSRC_H
#	include "newsrc.h"
#endif /* !NEWSRC_H */

#ifndef STPWATCH_H
#	include "stpwatch.h"
#endif /* !STPWATCH_H */

/*
 * TODO: fixup to remove CURR_GROUP dependency in all sort funcs
 */
#define SortBy(func)	qsort(arts, (size_t) top_art, sizeof(struct t_article), func);

int top_art = 0;				/* # of articles in arts[] */


/*
 * Local prototypes
 */
static FILE *open_art_header(char *groupname, long art, long *next);
static FILE *open_xover_fp(struct t_group *group, const char *mode, long min, long max, t_bool local);
static char *find_nov_file(struct t_group *group, int mode);
static char *print_date(time_t secs);
static char *print_from(struct t_group *group, struct t_article *article);
static int artnum_comp(t_comptype p1, t_comptype p2);
static int base_comp(t_comptype p1, t_comptype p2);
static int date_comp_asc(t_comptype p1, t_comptype p2);
static int date_comp_desc(t_comptype p1, t_comptype p2);
static int from_comp_asc(t_comptype p1, t_comptype p2);
static int from_comp_desc(t_comptype p1, t_comptype p2);
static int global_get_multiparts(int aindex, MultiPartInfo **malloc_and_setme_info);
static int global_look_for_multipart_info(int aindex, MultiPartInfo *setme, char start, char stop, int *offset);
static int last_date_comp_base_asc(t_comptype p1, t_comptype p2);
static int last_date_comp_base_desc(t_comptype p1, t_comptype p2);
static int lines_comp_asc(t_comptype p1, t_comptype p2);
static int lines_comp_desc(t_comptype p1, t_comptype p2);
static int read_art_headers(struct t_group *group, int total, long top);
static int read_overview(struct t_group *group, long min, long max, long *top, t_bool local);
static int score_comp_asc(t_comptype p1, t_comptype p2);
static int score_comp_desc(t_comptype p1, t_comptype p2);
static int score_comp_base(t_comptype p1, t_comptype p2);
static int subj_comp_asc(t_comptype p1, t_comptype p2);
static int subj_comp_desc(t_comptype p1, t_comptype p2);
static int valid_artnum(long art);
static long find_first_unread(struct t_group *group);
static long setup_hard_base(struct t_group *group);
static t_bool parse_headers(FILE *fp, struct t_article *h);
static t_compfunc eval_sort_arts_func(unsigned int sort_art_type);
static time_t get_last_posting_date(long n);
static void sort_base(unsigned int sort_threads_type);
static void thread_by_multipart(void);
static void thread_by_percentage(struct t_group *group);
static void thread_by_subject(void);
static void write_overview(struct t_group *group);


/*
 * Display a suitable 'entering group' message if screen needs redrawing
 * Allow for the non-printing %s, and the %-age counter
 */
void
show_art_msg(
	const char *group)
{
/* what if cCOLS < (strlen)+18? */
	wait_message(0, _(txt_group), cCOLS - strlen(_(txt_group)) + 2 - 3, group);
}


/*
 * Construct the pointers to the first (base) article in each thread.
 * If we are showing only unread, then point to the first unread. I have
 * no idea why this should be so, it causes problems elsewhere [which_response]
 */
void
find_base(
	struct t_group *group)
{
	int i, j;

	grpmenu.max = 0;

#ifdef DEBUG
	debug_print_arts();
#endif /* DEBUG */

	for_each_art(i) {
		/*
		 * .prev will be set on each article that is after the first article in
		 * the thread. Invalid articles which have been expired will have
		 * .thread set to ART_EXPIRED
		 */
		if (arts[i].prev >= 0 || arts[i].thread == ART_EXPIRED || (arts[i].killed && tinrc.kill_level == KILL_NOTHREAD))
			continue;

		if (grpmenu.max >= max_art)
			expand_art();

		if (group->attribute->show_only_unread_arts) {
			if (arts[i].status != ART_READ || arts[i].keep_in_base)
				base[grpmenu.max++] = i;
			else {
				/* Find 1st unread art in thread */
				for (j = i; j >= 0; j = arts[j].thread) {
					if (arts[j].status != ART_READ || arts[j].keep_in_base) {
						base[grpmenu.max++] = i;
						break;
					}
				}
			}
		} else
			base[grpmenu.max++] = i;
	}
	/* sort base[] */
	if (group->attribute->sort_threads_type > SORT_THREADS_BY_NOTHING)
		sort_base(group->attribute->sort_threads_type);
}


/*
 * Longword comparison routine for the qsort()
 */
static int
base_comp(
	t_comptype p1,
	t_comptype p2)
{
	const long *a = (const long *) p1;
	const long *b = (const long *) p2;

	if (*a < *b)
		return -1;

	if (*a > *b)
		return 1;

	return 0;
}


/*
 * via NNTP:
 *   Issue a LISTGROUP command
 *   Read the article numbers existing in the group into base[]
 *   If the LISTGROUP failed, issue a GROUP command. Use the results to
 *   create a less accurate version of base[]
 *   This data will already be sorted
 *
 * on local spool:
 *   Read the spool dir to populate base[] as above. Sort it.
 *
 * Grow the arts[] and bitmaps as needed.
 * NB: the output will be sorted on artnum
 *
 * grpmenu.max is one past top.
 * Returns total number of articles in group, or -1 on error
 */
static long
setup_hard_base(
	struct t_group *group)
{
	long total = 0;

	grpmenu.max = 0;

	/*
	 * If reading with NNTP, issue a LISTGROUP
	 */
	if (read_news_via_nntp && !read_saved_news && group->type == GROUP_TYPE_NEWS) {
#ifdef NNTP_ABLE
		char buf[NNTP_STRLEN];
		FILE *fp;

		/*
		 * Some nntp servers are broken and need an extra GROUP command
		 * (reported by reorx@irc.pl). This affects (old?) versions of
		 * nntpcache, leafnode and SurgeNews. Usually this should not be
		 * needed.
		 */
		if (nntp_caps.broken_listgroup) {
			snprintf(buf, sizeof(buf), "GROUP %s", group->name);
			if (nntp_command(buf, OK_GROUP, NULL, 0) == NULL)
				return -1;
		}

		/*
		 * See if LISTGROUP works
		 *
		 * think about something like:
		 * if (nntp_caps.type == CAPABILITIES && tinrc.getart_limit != 0) {
		 *    calculate some useful min vals, eg. if getart_limit < 0
		 *    use lowest unread art + 2*getart_limit (to include holes)
		 *    snprintf(buf, sizeof(buf), "LISTGROUP %s %d-%d, group->name, min, max);
		 * }
		 */
		snprintf(buf, sizeof(buf), "LISTGROUP %s", group->name);
		if ((fp = nntp_command(buf, OK_GROUP, NULL, 0)) != NULL) {
			char *ptr;

#	ifdef DEBUG
			if (debug & DEBUG_NNTP)
				debug_print_file("NNTP", "setup_hard_base(%s)", buf);
#	endif /* DEBUG */

			while ((ptr = tin_fgets(fp, FALSE)) != NULL) {
				if (grpmenu.max >= max_art)
					expand_art();
				base[grpmenu.max++] = atoi(ptr);
			}

			if (tin_errno)
				return -1;

		} else {
			/*
			 * LISTGROUP failed, use GROUP command instead
			 */
			long start, last, count;
			char line[NNTP_STRLEN];

			/*
			 * Handle the obscure case that the user aborted before the LISTGROUP
			 * had a chance to respond
			 */
			if (tin_errno)
				return -1;

			snprintf(buf, sizeof(buf), "GROUP %s", group->name);
			if (nntp_command(buf, OK_GROUP, line, sizeof(line)) == NULL)
				return -1;

			if (sscanf(line, "%ld %ld %ld", &count, &start, &last) != 3)
				return -1;

#	ifdef DEBUG
			if (debug & DEBUG_NNTP)
				debug_print_file("NNTP", "setup_hard_base(%s)", buf);
#	endif /* DEBUG */
			/*
			 * TODO: AFAICS "total" and "count" arn't used in the code below
			 *       anymore, why do we bother to set them?
			 */
			total = count;
			if (last - count > start)
				count = last - start;

			while (start <= last) {
				if (grpmenu.max >= max_art)
					expand_art();
				base[grpmenu.max++] = start++;
			}
		}
#endif /* NNTP_ABLE */
	/*
	 * Reading off local spool, read the directory files
	 */
	} else {
		DIR *d;
		DIR_BUF *e;
		char group_path[PATH_LEN];
		long art;

		make_base_group_path(group->spooldir, group->name, group_path, sizeof(group_path));

		if (access(group_path, R_OK) != 0) {
			error_message(2, _(txt_not_exist));
			return -1;
		}

		if ((d = opendir(group_path)) != NULL) {
			while ((e = readdir(d)) != NULL) {
				art = atol(e->d_name);
				if (art >= 1) {
					total++;
					if (grpmenu.max >= max_art)
						expand_art();
					base[grpmenu.max++] = art;
				}
			}
			CLOSEDIR(d);
			qsort((char *) base, (size_t) grpmenu.max, sizeof(long), base_comp);
		}
	}

	if (grpmenu.max) {
		if (base[grpmenu.max - 1] > group->xmax)
			group->xmax = base[grpmenu.max - 1];
		expand_bitmap(group, base[0]);
	}

	return total;
}


/*
 * Main group indexing routine.
 *
 * Will read any existing index, create or incrementally update
 * the index by looking at the articles in the spool directory,
 * and attempt to write a new index if necessary.
 *
 * Returns FALSE if the user aborted the indexing, otherwise TRUE
 */
t_bool
index_group(
	struct t_group *group)
{
	int i;
	int changed;				/* Count of articles whose overview has changed */
	int getart_limit;
	int respnum;
	int total;
	long last_read_article;
	long min, new_min, max;
	t_bool caching_xover;
	t_bool filtered;

	if (group == NULL)
		return TRUE;

	if (!batch_mode)
		show_art_msg(group->name);

	signal_context = cArt;			/* Set this only once curr_group is valid */

	hash_reclaim();
	free_art_array();
	free_msgids();

	BegStopWatch("setup_hard_base()");

	/*
	 * Get list of valid article numbers
	 */
	if (setup_hard_base(group) < 0)
		return FALSE;

	EndStopWatch();
	PrintStopWatch();

#ifdef DEBUG
	if (debug & DEBUG_NEWSRC) {
		debug_print_comment("Before read_overview");
		debug_print_bitmap(group, NULL);
	}
#endif /* DEBUG */

	min = grpmenu.max ? base[0] : group->xmin;
	max = grpmenu.max ? base[grpmenu.max - 1] : min - 1;

	getart_limit = cmdline.args & CMDLINE_GETART_LIMIT ? cmdline.getart_limit : tinrc.getart_limit;

	if (getart_limit > 0) {
		if (grpmenu.max && (grpmenu.max > getart_limit))
			min = base[grpmenu.max - getart_limit];
		else
			getart_limit = 0;
	} else if (getart_limit < 0) {
		long first_unread = find_first_unread(group);

		if (min - first_unread < getart_limit)
			min = first_unread + getart_limit;
		else
			getart_limit = 0;
	}

	/*
	 * Quit now if no articles
	 */
	if (max < 0)
		return FALSE;

	top_art = 0;
	last_read_article = 0L;

	/*
	 * Read in the existing overview data for min..max
	 * This read has local=TRUE set if locally caching XOVER records to ensure
	 * we pull in any private overview caches in preference to using using OVER
	 *
	 * When reading local spool, this will pull in the system wide overview
	 * cache (if found) otherwise the private overview cache will be read
	 */
	caching_xover = (tinrc.cache_overview_files && nntp_caps.over_cmd && group->type == GROUP_TYPE_NEWS);
	if ((changed = read_overview(group, min, max, &last_read_article, caching_xover)) == -1)
		return FALSE;	/* user aborted indexing */

	/*
	 * Fill in the range last_read_article...max using XOVER
	 * Only do this if the previous read_overview() was against private cache
	 */
	if ((last_read_article < max) && caching_xover) {
		new_min = (last_read_article >= min) ? last_read_article + 1 : min;

		if ((changed += read_overview(group, new_min, max, &last_read_article, FALSE)) == -1)
			return FALSE;	/* user aborted indexing */
	} else
		caching_xover = FALSE;

	/*
	 * Mark as UNTHREADED all articles that have been verified as valid
	 * Get num of new arts to index so the user will have an idea of index time
	 */
	for (i = 0, total = 0; i < grpmenu.max; i++) {
		if ((respnum = valid_artnum(base[i])) >= 0) {
			arts[respnum].thread = ART_UNTHREADED;
			continue;
		}
		if (base[i] <= last_read_article)		/* It is vital this test be done last */
			continue;
		total++;
	}

	/*
	 * Add any articles to arts[] that are new or were killed
	 */
	if (total > 0) {
		new_min = (getart_limit != 0 && last_read_article < min) ? min - 1 : last_read_article;

		if ((changed += read_art_headers(group, total, new_min)) == -1)
			return FALSE;		/* user aborted indexing */
	}

#ifdef DEBUG
	if (debug & DEBUG_NEWSRC) {
		debug_print_comment("Before parse_unread_arts()");
		debug_print_bitmap(group, NULL);
	}
#endif /* DEBUG */
	/*
	 * Do this before calling art_mark(,, ART_READ) if you want
	 * the unread count to be correct.
	 */
	min = getart_limit > 0 ? min : 0L;
	parse_unread_arts(group, min);
#ifdef DEBUG
	if (debug & DEBUG_NEWSRC) {
		debug_print_comment("After parse_unread_arts()");
		debug_print_bitmap(group, NULL);
	}
#endif /* DEBUG */

	/*
	 * Stat all articles to see if any have expired
	 */
	for_each_art(i) {
		if (arts[i].thread == ART_EXPIRED) {
			changed++;
#ifdef DEBUG
			if (debug & DEBUG_NEWSRC)
				debug_print_comment("art.c: index_group() purging...");
#endif /* DEBUG */
			art_mark(group, &arts[i], ART_READ);
			if (group->attribute->show_only_unread_arts)
				arts[i].keep_in_base = FALSE;
		}
	}

	/*
	 * Only rewrite the index if it has changed
	 * TODO review the exact logic behind "|| caching_xover"
	 */
	if (changed || caching_xover)
		write_overview(group);

	/*
	 * Create the reference tree. The msgid and ref ptrs will
	 * be free()d now that the NovFile has been written.
	 */
	build_references(group);

	/*
	 * Needs access to the reference tree
	 */
	filtered = filter_articles(group);

	BegStopWatch("make_threads()");

	/*
	 * Thread the group
	 */
	make_threads(group, FALSE);

	EndStopWatch();
	PrintStopWatch();

	if ((changed > 0 || filtered) && !batch_mode)
		clear_message();

	return TRUE;
}


/*
 * Returns number of first unread article
 */
static long
find_first_unread(
	struct t_group *group)
{
	unsigned char *p;
	unsigned char *end = group->newsrc.xbitmap;
	long first = group->newsrc.xmin; /* initial value */

	if ((p = group->newsrc.xbitmap)) {
		end += group->newsrc.xbitlen / NBITS;
		for (; *p == '\0' && p < end; p++, first += NBITS)
			;
	}
	return first;
}


/*
 * Open an article for reading just the header
 * 'next' is used/updated with the next article number
 * to optimise the number of 'HEAD' commands issued on
 * groups with holes.
 */
static FILE *
open_art_header(
	char *groupname,
	long art,
	long *next)
{
	char buf[NNTP_STRLEN];
#ifdef NNTP_ABLE
	FILE *fp;
	int i;

	if (read_news_via_nntp && CURR_GROUP.type == GROUP_TYPE_NEWS) {
		/*
		 * Don't bother requesting if we have not got there yet.
		 * This is a big win if the group has got holes in it (ie. if 000's
		 * of articles have expired between active files min & max values).
		 */
		if (art < *next)
			return NULL;

		snprintf(buf, sizeof(buf), "HEAD %ld", art);
		if ((fp = nntp_command(buf, OK_HEAD, NULL, 0)) != NULL)
			return fp;

		/*
		 * HEAD failed, try to find NEXT
		 * Should return "223 artno message-id more text...."
		 */
		i = new_nntp_command("NEXT", OK_NOTEXT, buf, sizeof(buf));
		switch (i) {
			case OK_NOTEXT:
				*next = atoi(buf);		/* Set next art number */
				break;

#	ifndef BROKEN_LISTGROUP
			/*
			 * might happen if LISTGROUP doesn't select group, but
			 * we are not -DBROKEN_LISTGROUP
			 */
			case ERR_NCING:
				nntp_caps.broken_listgroup = TRUE;
				snprintf(buf, sizeof(buf), "GROUP %s", groupname);
				if (nntp_command(buf, OK_GROUP, NULL, 0) == NULL)
					return NULL;
				snprintf(buf, sizeof(buf), "HEAD %ld", art);
				if ((fp = nntp_command(buf, OK_HEAD, NULL, 0)) != NULL)
					return fp;
				if (nntp_command("NEXT", OK_NOTEXT, buf, sizeof(buf)))
					*next = atoi(buf);
				break;
#	endif /* !BROKEN_LISTGROUP */

			default:
				/*
				 * TODO: abort loop over all arts on ERR_NONEXT
				 */
#	ifndef BROKEN_LISTGROUP
				/*
				 * to avoid out of sync responses
				 * (listgroup seems to work, but didn't select new group,
				 *  so xover seems to work but returns old data)
				 * we set listgroup_broken = TRUE; once we saw a
				 * ERR_NOARTIG / ERR_NONEXT or the like - even if
				 * ERR_NOARTIG may occur on servers where listgroup
				 * isn't broken...
				 */
				nntp_caps.broken_listgroup = TRUE;
#	endif /* !BROKEN_LISTGROUP */
				break;
		}

		return NULL;
	}
#endif /* NNTP_ABLE */

	snprintf(buf, sizeof(buf), "%ld", art);
	return (fopen(buf, "r"));
}


/*
 * Called after XOVER/local/private overview databases have been loaded
 * Read and parse in headers for any arts not already found (usually
 * new articles that have not been indexed yet)
 * Any new articles that are added have ->thread set to ART_UNTHREADED
 * 'top' is the current highest artnum read
 *
 * Return values are:
 *   >0   Number of additional articles read in
 *    0   No additional (new) articles were found
 *   -1   user aborted during read
 */
static int
read_art_headers(
	struct t_group *group,
	int total,
	long top)
{
	FILE *fp;
	char dir[PATH_LEN];
	char *group_msg;
	int i;
	int modified = 0;
	long art;
	long head_next = -1; /* Reset the next article number index (for when HEAD fails) */
	t_bool res;

	/*
	 * Change to groups spooldir to optimize fopen()'s on local articles
	 * NB open_art_header() requires this
	 */
	if (!read_news_via_nntp || group->type != GROUP_TYPE_NEWS) {
		char buf[PATH_LEN];

		get_cwd(dir);
		make_base_group_path(group->spooldir, group->name, buf, sizeof(buf));
		my_chdir(buf);
	}

	group_msg = fmt_string(_(txt_group), cCOLS - strlen(_(txt_group)) + 2 - 3, group->name);
	for (i = 0; i < grpmenu.max; i++) {	/* for each article number */
		art = base[i];

		/*
		 * Skip articles that are below the low water mark or are
		 * already present
		 */
		if (valid_artnum(art) >= 0)
			continue;
		if (art <= top)
			continue;

		/*
		 * Try and open the article
		 */
		if ((fp = open_art_header(group->name, art, &head_next)) == NULL)
			continue;

		/*
		 * Add article to arts[]
		 */
		if (top_art >= max_art)
			expand_art();

		set_article(&arts[top_art]);
		arts[top_art].artnum = art;
		arts[top_art].thread = ART_UNTHREADED;

		res = parse_headers(fp, &arts[top_art]);

		TIN_FCLOSE(fp);
		if (tin_errno) {
			modified = -1;
			break;
		}

		if (!res) {
#ifdef DEBUG
			if (debug & DEBUG_NNTP) {
				char buf[PATH_LEN];

				snprintf(buf, sizeof(buf), "FAILED parse_headers(%ld)", art);
				debug_print_file("NNTP", "read_art_headers() %s", buf);
			}
#endif /* DEBUG */
			continue;
		}

		top = arts[top_art].artnum;	/* used if arts are killed */
		top_art++;

		if (++modified % MODULO_COUNT_NUM == 0)
			show_progress(group_msg, modified, total);
	}
	free(group_msg);

	/*
	 * Change back to previous dir before indexing started
	 */
	if (!read_news_via_nntp || group->type != GROUP_TYPE_NEWS)
		my_chdir(dir);

	return modified;
}


/*
 * The algorithm is elegant, using the fact that identical Subject lines
 * are hashed to the same node in table[] (see hashstr.c)
 *
 * Mark i as being in j's thread list if
 * . The article is _not_ being ignored
 * . The article is not already threaded
 * . One of the following is true:
 *    1) The subject lines are the same
 *    2) Both are part of the same archive (name's match and arch bit set)
 * IMHO the tests for archive name are redundant and have been for years
 */
static void
thread_by_subject(
	void)
{
	int i, j;
	struct t_hashnode *h;

	for_each_art(i) {
		if (IGNORE_ART_THREAD(i))
			continue;

		/*
		 * Get the contents of the magic marker in the hashnode
		 */
		h = (struct t_hashnode *) (arts[i].subject - sizeof(int) - sizeof(void *)); /* FIXME: cast increases required alignment of target type */

		j = h->aptr;

		if (j != -1 && j < i) {
#if 1
			if (arts[i].prev == ART_NORMAL && (arts[i].subject == arts[j].subject))
#else
			/* see also refs.c:collate_subjects() */
			if (arts[i].prev == ART_NORMAL && ((arts[i].subject == arts[j].subject) || (arts[i].archive && arts[j].archive && (arts[i].archive->name == arts[j].archive->name))))
#endif /* 1 */
			{
				arts[j].thread = i;
				arts[i].prev = j;
			}
		}

		/*
		 * Update the magic marker with the highest numbered mesg in
		 * arts[] that has been used in this thread so far
		 */
		h->aptr = i;
	}

#if 0
	fprintf(stderr, "Subj dump\n");
	fprintf(stderr, "%3s %3s %3s %3s : %3s %3s\n", "#", "Par", "Sib", "Chd", "In", "Thd");
	for_each_art(i) {
		fprintf(stderr, "%3d %3d %3d %3d : %3d %3d : %.50s %s\n", i,
			(arts[i].refptr->parent) ? arts[i].refptr->parent->article : -2,
			(arts[i].refptr->sibling) ? arts[i].refptr->sibling->article : -2,
			(arts[i].refptr->child) ? arts[i].refptr->child->article : -2,
			arts[i].prev, arts[i].thread, arts[i].refptr->txt, arts[i].subject);
	}
#endif /* 0 */
}


/*
 * This Threading algorithm threads articles into 'buckets' where each bucket
 * contains all the articles which match the root article's subject line to
 * the configured percentage. Eg, if the root article had the subject "asdf"
 * and the match percentage was configured to be 75% then any article would
 * match if its subject was no different in more than a single character.
 */
static void
thread_by_percentage(
	struct t_group *group)
{
	int i, j, k;
	int root_num = 0; /* The index number of the root we are currently working on. */
	int unmatched; /* This is the number of characters that don't match between the two strings */
	unsigned int percentage = 100 - group->attribute->thread_perc;
	size_t slen;

	/* First we need to sort art[] to simplify and speed up the matching. */
	SortBy(subj_comp_asc);

	/*
	 * Now we put all the articles which match enough into the thread. If
	 * an article doesn't match enough we create a new thread and then add
	 * to that and so on.
	 */
	base[0] = 0;
	arts[0].prev = ART_NORMAL;
	for_each_art(i) {
		if (i == 0)
			continue;

		/* Check each character to see if it matched enough */
		k = 0;
		unmatched = 0;
		for (j = 0; arts[base[root_num]].subject[j] != '\0' && arts[i].subject[k] != '\0'; j++, k++) {
			if (arts[base[root_num]].subject[j] != arts[i].subject[k])
				unmatched++;
		}

		/*
		 * By getting here we have a number of unmatched characters
		 * between the two strings. We also have the length of the
		 * strings available to us easily.
		 * All we need to do is see if the match is good enough, but
		 * we count differences in the length of the strings against
		 * them matching.
		 */
		if (!(slen = strlen(arts[base[root_num]].subject)))
			slen++;
		unmatched += abs(slen - strlen(arts[i].subject));
		if ((unmatched * 100) / slen > percentage) {
			/*
			 * If there is less greater than percentage% different start a
			 * new thread.
			 */
			base[++root_num] = i;
			arts[i].prev = ART_NORMAL;
			continue;
		} else {
			/*
			 * The subject lines match enough to consider them part of a single
			 * thread, so add the current article to the thread.
			 */
			if (arts[base[root_num]].thread < 0)
				arts[base[root_num]].thread = i;
			arts[i].prev = i - 1;
			arts[i - 1].thread = i;
			continue;
		}
	}
}


/*
 * This was brought over from tags.c, however this version doesn't not
 * opperate on base_index
 *
 * Parses a subject header of the type "multipart message subject (01/42)"
 * into a MultiPartInfo struct, or fails if the message subject isn't in the
 * right form.
 *
 * @return nonzero on success
 */
int
global_get_multipart_info(
	int aindex,
	MultiPartInfo *setme)
{
	int i, j, offi, offj;
	MultiPartInfo setmei, setmej;

	i = global_look_for_multipart_info(aindex, &setmei, '[', ']', &offi);
	j = global_look_for_multipart_info(aindex, &setmej, '(', ')', &offj);

	/* Ok i hits first */
	if (offi > offj) {
		*setme = setmei;
		return i;
	}

	/* Its j or they are both the same (which must be zero!) so we don't care */
	*setme = setmej;
	return j;
}


/*
 *	Again this was taken from tags.c, but works on global indicies into arts
 *	rather then on base_index.
 */
static int
global_look_for_multipart_info(
	int aindex,
	MultiPartInfo* setme,
	char start,
	char stop,
	int *offset)
{
	char *subj;
	char *pch;
	MultiPartInfo tmp;

	*offset = 0;

	/* entry assertions */
	assert(0 <= aindex && aindex < top_art && "invalid index");
	assert(setme != NULL && "setme must not be NULL");

	/* parse the message */
	subj = arts[aindex].subject;
	pch = strrchr(subj, start);
	if (!pch || !isdigit((int) pch[1]))
		return 0;

	tmp.base_index = aindex; /* This could be confusing because we are actually storing the index into arts, not the base_index. */
	tmp.subject_compare_len = pch - subj;
	tmp.part_number = (int) strtol(pch + 1, &pch, 10);
	if (*pch != '/' && *pch != '|')
		return 0;

	if (!isdigit((int) pch[1]))
		return 0;

	tmp.total = (int) strtol(pch + 1, &pch, 10);
	if (*pch != stop)
		return 0;

	tmp.subject = subj;
	*setme = tmp;
	*offset = pch - subj;
	return 1;
}


/*
 * Taken from tags.c but changed to use indicies into arts[] instead of
 * base_index. Changed so that even when we don't have all the parts we
 * return a valid array.
 *
 * Tries to find all the parts to the multipart message pointed to by
 * base_index.
 *
 * @return on success, the number of parts found. On failure, zero if not a
 * multipart or the negative value of the first missing part.
 * @param base_index index pointing to one of the messages in a multipart
 * message.
 * @param malloc_and_setme_info on success, set to a malloced array the
 * parts found.
 */
static int
global_get_multiparts(
	int aindex,
	MultiPartInfo **malloc_and_setme_info)
{
	int i;
	int part_index;
	MultiPartInfo tmp, tmp2;
	MultiPartInfo *info = NULL;

	/* entry assertions */
	assert(0 <= aindex && aindex < top_art && "Invalid index");
	assert(malloc_and_setme_info != NULL && "malloc_and_setme_info must not be NULL");

	/* make sure this is a multipart message... */
	if (!global_get_multipart_info(aindex, &tmp) || tmp.total < 1)
		return 0;

	/* make a temporary buffer to hold the multipart info... */
	info = my_malloc(sizeof(MultiPartInfo) * tmp.total);

	/* zero out part-number for the repost check below */
	for (i = 0; i < tmp.total; ++i) {
		info[i].total = tmp.total; /* Added this for thread_by_multipart */
		info[i].part_number = -1;
	}

	/* try to find all the multiparts... */
	for_each_art(i) {
		if (strncmp(arts[i].subject, tmp.subject, tmp.subject_compare_len))
			continue;

		if (!global_get_multipart_info(i, &tmp2))
			continue;

		/* 'test (1/5)' is not the same as 'test (1/15)' */
		if (tmp.total != tmp2.total)
			continue;

		part_index = tmp2.part_number - 1;

		/*
		 * skip "blah (00/102)" or "blah (103/102)" subjects
		 */
		if (part_index < 0 || part_index >= tmp.total)
			continue;

		/* repost check: do we already have this part? */
		if (info[part_index].part_number != -1) {
			assert(info[part_index].part_number == tmp2.part_number && "bookkeeping error");
			continue;
		}

		/* we have a match, hooray! */
		info[part_index] = tmp2;
	}

	/* see if we got them all. */
	for (i = 0; i < tmp.total; ++i) {
		if (info[i].part_number != i + 1) {
			*malloc_and_setme_info = info;
			return -(i + 1); /* missing part #(i+1) */
		}
	}

	/* looks like a success .. */
	*malloc_and_setme_info = info;
	return tmp.total;
}


/*
 *	The algorithm uses the tag multipart searches to thread articles together.
 */
static void
thread_by_multipart(
	void)
{
	int i, j, threadNum;
	MultiPartInfo *minfo = NULL;

	for_each_art(i) {
		if (IGNORE_ART_THREAD(i) || arts[i].prev >= 0 || !global_get_multiparts(i, &minfo))
			continue;

		threadNum = -1;
		for (j = minfo[0].total - 1; j >= 0; j--) {
			if (minfo[j].part_number != -1) {
				if (threadNum != -1) {
					arts[minfo[j].base_index].thread = threadNum;
					arts[threadNum].prev = minfo[j].base_index;
				}

				threadNum = minfo[j].base_index;
			}
		}
	}
	free(minfo);
}


/*
 * Go through the articles in arts[] and create threads. There are
 * 5 strategies currently defined :
 *
 *	THREAD_NONE		No threading
 *	THREAD_SUBJ		Threads are created using like Subject lines
 *	THREAD_REFS		Threads are created using the References headers
 *	THREAD_BOTH		Threads created using References and then Subject
 *	THREAD_MULTI	Threads created using Subject to search for Multiparts
 *	THREAD_PERC		Threads based upon a char for char match of greater than x%
 *
 * .thread and .prev are used to hold the threading information, see tin.h for
 * more information
 * Only process valid (unexpired) articles we haven't visited yet
 * (ie arts[].thread == ART_UNTHREADED)
 *
 * The rethread parameter is used to force the deletion of existing threading
 * information before threading which happens anyway expect when using
 * THREAD_NONE (I don't immediately see how this is useful)
 */
/* TODO: rewrite that user can easly combine different 'threading'
 *       methods, i.e:
 *       - thread_by_multipart() + collate_subjects()
 */
void
make_threads(
	struct t_group *group,
	t_bool rethread)
{
	if (!cmd_line && !batch_mode)
		info_message((group->attribute->thread_articles == THREAD_NONE ? _(txt_unthreading_arts) : _(txt_threading_arts)));

#ifdef DEBUG
	if (debug & DEBUG_MISC)
		error_message(2, "rethread=[%d]  thread_articles=[%d]  attr_thread_articles=[%d]",
				rethread, tinrc.thread_articles, group->attribute->thread_articles);
#endif /* DEBUG */

	/*
	 * Sort all the articles using the preferred method
	 * When find_base() is called, the bases are created ordered
	 * on arts[] and so the base messages under all threading systems
	 * will be sorted in this way.
	 */
	sort_arts(group->attribute->sort_article_type);

	/*
	 * Reset all the ptrs to articles following the above sort
	 */
	clear_art_ptrs();

	/*
	 * The threading pointers need to be reset if re-threading
	 * If using ref threading, revector the links back to the articles
	 */
	if (rethread || group->attribute->thread_articles) {
		int i;

		for_each_art(i) {
			if (arts[i].thread >= 0)
				arts[i].thread = ART_UNTHREADED;

			arts[i].prev = ART_NORMAL;

			/* Should never happen if tree is built properly */
			if (arts[i].refptr == NULL) {
#ifdef DEBUG
				if (debug & DEBUG_REFS) {
					my_fprintf(stderr, "\nError  : art->refptr is NULL\n");
					my_fprintf(stderr, "Artnum : %ld\n", arts[i].artnum);
					my_fprintf(stderr, "Subject: %s\n", arts[i].subject);
					my_fprintf(stderr, "From   : %s\n", arts[i].from);
					assert(arts[i].refptr != NULL);
				} else
#endif /* DEBUG */
					continue;
			}
			arts[i].refptr->article = i;
		}
	}

	/*
	 * Do the right thing according to the threading strategy
	 */
	switch (group->attribute->thread_articles) {
		case THREAD_NONE:
			break;

		case THREAD_SUBJ:
			thread_by_subject();
			break;

		case THREAD_REFS:
			thread_by_reference();
			break;

		case THREAD_BOTH:
			thread_by_reference();
			collate_subjects();
			break;

		case THREAD_MULTI:
			thread_by_multipart();
			break;

		case THREAD_PERC:
			thread_by_percentage(group);
			break;

		default: /* not reached */
			break;
	}

	/*
	 * Rebuild base[]
	 */
	find_base(group);
}


static t_compfunc
eval_sort_arts_func(
	unsigned int sort_art_type)
{
	switch (sort_art_type) {
		case SORT_ARTICLES_BY_NOTHING:		/* don't sort at all */
			return artnum_comp;

		case SORT_ARTICLES_BY_SUBJ_DESCEND:
			return subj_comp_desc;

		case SORT_ARTICLES_BY_SUBJ_ASCEND:
			return subj_comp_asc;

		case SORT_ARTICLES_BY_FROM_DESCEND:
			return from_comp_desc;

		case SORT_ARTICLES_BY_FROM_ASCEND:
			return from_comp_asc;

		case SORT_ARTICLES_BY_DATE_DESCEND:
			return date_comp_desc;

		case SORT_ARTICLES_BY_DATE_ASCEND:
			return date_comp_asc;

		case SORT_ARTICLES_BY_SCORE_DESCEND:
			return score_comp_desc;

		case SORT_ARTICLES_BY_SCORE_ASCEND:
			return score_comp_asc;

		case SORT_ARTICLES_BY_LINES_DESCEND:
			return lines_comp_desc;

		case SORT_ARTICLES_BY_LINES_ASCEND:
			return lines_comp_asc;

		default:
			break;
	}
	return NULL;
}


void
sort_arts(
	unsigned int sort_art_type)
{
	t_compfunc comp_func = eval_sort_arts_func(sort_art_type);

	if (comp_func)
		SortBy(comp_func);
}


static void
sort_base(
	unsigned int sort_threads_type)
{
	switch (sort_threads_type) {
		case SORT_THREADS_BY_SCORE_DESCEND:
		case SORT_THREADS_BY_SCORE_ASCEND:
			qsort(base, (size_t) grpmenu.max, sizeof(long), score_comp_base);
			break;

		case SORT_THREADS_BY_LAST_POSTING_DATE_DESCEND:
			qsort(base, (size_t) grpmenu.max, sizeof(long), last_date_comp_base_desc);
			break;

		case SORT_THREADS_BY_LAST_POSTING_DATE_ASCEND:
			qsort(base, (size_t) grpmenu.max, sizeof(long), last_date_comp_base_asc);
			break;
	}
}


/*
 * This is called to get header info for articles not already found in the
 * overview files.
 * Code reads (max_lineno) lines of article to catch headers like Archive-name:
 * which are not normally included in XOVER or even the normal block of headers.
 * How this is supposed to be useful when 99% of the time we'll have overview
 * data I don't know...
 * TODO: move Archive-name: parsing to article body parsing, remove the
 * TODO: max_lineno nonsense and parse just the hdrs. Only parse if
 * TODO: currgrp->auto_save is set, otherwise it is redundant info
 */
static t_bool
parse_headers(
	FILE *fp,
	struct t_article *h)
{
	char art_from_addr[HEADER_LEN];
	char art_full_name[HEADER_LEN];
	char *hdr, *ptr;
	unsigned int lineno = 0;
	unsigned int max_lineno = 25;
	t_bool got_from, got_lines, got_received;

	got_from = got_lines = got_received = FALSE;

	while ((ptr = tin_fgets(fp, TRUE)) != NULL) {
		/*
		 * Look for the end of information which tin wants to get.
		 * Applies when reading local spool and via NNTP.
		 */

		/*
		 * as Archive-name: is placed in the body it's safe to exit
		 * the loop if it was found.
		 */
		if (lineno++ > max_lineno || h->archive)
			break;

		unfold_header(ptr);
		switch (toupper((unsigned char) *ptr)) {
			case 'A':	/* Archive-name:  optional */
				/*
				 * Archive-name: {name}/{part|patch}{number}
				 * eg, acorn/faq/part01
				 */
				if ((hdr = parse_header(ptr + 1, "rchive-name", FALSE, FALSE, FALSE))) {
					char *s;

					if ((s = strrchr(hdr, '/')) != NULL) {
						struct t_archive *archptr = my_malloc(sizeof(struct t_archive));

						if (STRNCASECMPEQ(s + 1, "part", 4)) {
							archptr->partnum = my_strdup(s + 5);
							archptr->ispart = TRUE;
						} else if (STRNCASECMPEQ(s + 1, "patch", 5)) {
							archptr->partnum = my_strdup(s + 6);
							archptr->ispart = FALSE;
						} else {		/* part or patch must be present */
							free(archptr);
							continue;
						}
						strtok(archptr->partnum, "\n");
						*s = '\0';
						archptr->name = hash_str(hdr);
						h->archive = archptr;
					}
				}
				break;

			case 'D':	/* Date:  mandatory */
				if (!h->date) {
					if ((hdr = parse_header(ptr + 1, "ate", FALSE, FALSE, FALSE)))
						h->date = parsedate(hdr, (struct _TIMEINFO *) 0);
				}
				break;

			case 'F':	/* From:  mandatory */
				if (!got_from) {
					if ((hdr = parse_header(ptr + 1, "rom", FALSE, FALSE, FALSE))) {
						h->gnksa_code = parse_from(hdr, art_from_addr, art_full_name);
						h->from = hash_str(buffer_to_ascii(art_from_addr));
						if (*art_full_name)
							h->name = hash_str(eat_tab(convert_to_printable(rfc1522_decode(art_full_name), FALSE)));
						got_from = TRUE;
					}
				}
				break;

			case 'L':	/* Lines:  optional */
				if (!got_lines) {
					if ((hdr = parse_header(ptr + 1, "ines", FALSE, FALSE, FALSE))) {
						h->line_count = atoi(hdr);
						got_lines = TRUE;
					}
				}
				break;

			case 'M':	/* Message-ID:  mandatory */
				if (!h->msgid) {
					if ((hdr = parse_header(ptr + 1, "essage-ID", FALSE, FALSE, FALSE)))
						h->msgid = my_strdup(hdr);
				}
				break;

			case 'R':	/* References:  optional */
				if (!h->refs) {
					if ((hdr = parse_header(ptr + 1, "eferences", FALSE, FALSE, FALSE)))
						h->refs = my_strdup(hdr);
				}

				/* Received:  If found it's probably a mail article */
				if (!got_received) {
					if (parse_header(ptr + 1, "eceived", FALSE, FALSE, FALSE)) {
						max_lineno <<= 1;		/* double the max number of line to read for mails */
						got_received = TRUE;
					}
				}
				break;

			case 'S':	/* Subject:  mandatory */
				if (!h->subject) {
					if ((hdr = parse_header(ptr + 1, "ubject", FALSE, FALSE, FALSE)))
						h->subject = hash_str(eat_re(eat_tab(convert_to_printable(rfc1522_decode(hdr), FALSE)), FALSE));
				}
				break;

			case 'X':	/* Xref:  optional */
				if (!h->xref) {
					if ((hdr = parse_header(ptr + 1, "ref", FALSE, FALSE, FALSE)))
						h->xref = my_strdup(hdr);
				}
				break;

			default:
				break;
		} /* switch */

	} /* while */

#ifdef NNTP_ABLE
	if (ptr)
		drain_buffer(fp);
#endif /* NNTP_ABLE */

	if (tin_errno)
		return FALSE;

	/*
	 * The son of RFC 1036 states that the following hdrs are mandatory. It
	 * also states that Subject, Newsgroups and Path are too. Ho hum.
	 *
	 * What about readinng mail from local spool via ~/.tin/active.mail,
	 * they might not have a Message-ID but got_received is very likely to
	 * be true.
	 */
	if (got_from && h->date && h->msgid) {
		if (!h->subject)
			h->subject = hash_str("<No subject>");

#ifdef DEBUG
		debug_print_header(h);
#endif /* DEBUG */
		return TRUE;
	}

	return FALSE;
}


/*
 * Read in an overview index file. Fields are separated by TAB.
 * return the number of expired articles encountered or -1 if the user aborted
 * the read
 * 'top' is set to the highest artnum read
 * If 'local' is set then always open local overview cache in preference to
 * using NNTP XOVER
 *
 * Format (mandatory as far as line count [RFC2980]):
 *	1. article number (ie. 183)                [mandatory]
 *	2. Subject: line  (ie. Which newsreader?)  [mandatory]
 *	3. From: line     (ie. iain@ecrc.de)       [mandatory]
 *	4. Date: line     (rfc822 format)          [mandatory]
 *	5. MessageID:     (ie. <123@ether.net>)    [mandatory]
 *	6. References:    (ie. <message-id> ....)  [optional]
 *	7. Byte count     (Skipped - not used)     [mandatory]
 *	8. Line count     (ie. 23)                 [mandatory]
 *	9. Xref: line     (ie. alt.test:389)       [optional]
 */
static int
read_overview(
	struct t_group *group,
	long min,
	long max,
	long *top,
	t_bool local)
{
	FILE *fp;
	char *ptr;
	char *q;
	char *buf;
	char *group_msg;
	char art_full_name[HEADER_LEN];
	char art_from_addr[HEADER_LEN];
	unsigned int count;
	int expired = 0;
	long artnum;
	struct t_article *art;
	size_t over_fields = 1;

	/*
	 * open the overview file (whether it be local or via nntp)
	 */
	if ((fp = open_xover_fp(group, "r", min, max, local)) == NULL)
		return expired;

	if (group->xmax > max)
		group->xmax = max;

	group_msg = fmt_string(_(txt_group), cCOLS - strlen(_(txt_group)) + 2 - 3, group->name);

	/* get the number of fields per over-record as announced by LIST OVERVIEW.FMT */
	if (ofmt) {
		for (; ofmt[over_fields].name; over_fields++)
			;
	}
	if (!--over_fields) { /* e.g. nntp_caps.type == CAPABILITIES && !nntp_caps.list_overview_fmt -> assume defaults */
		ofmt = my_realloc(ofmt, sizeof(struct t_overview_fmt) * (8 + 1));
		ofmt[0].type = OVER_T_INT;
		ofmt[0].name = my_strdup("Artnum:");
		ofmt[1].type = OVER_T_STRING;
		ofmt[1].name = my_strdup("Subject:");
		ofmt[2].type = OVER_T_STRING;
		ofmt[2].name = my_strdup("From:");
		ofmt[3].type = OVER_T_STRING;
		ofmt[3].name = my_strdup("Date:");
		ofmt[4].type = OVER_T_STRING;
		ofmt[4].name = my_strdup("Message-ID:");
		ofmt[5].type = OVER_T_STRING;
		ofmt[5].name = my_strdup("References:");
		ofmt[6].type = OVER_T_INT;
		ofmt[6].name = my_strdup("Bytes:");
		ofmt[7].type = OVER_T_INT;
		ofmt[7].name = my_strdup("Lines:");
		ofmt[8].type = OVER_T_ERROR;
		ofmt[8].name = NULL;
		over_fields = 7;
	}

	while ((buf = tin_fgets(fp, FALSE)) != NULL) {
		if (need_resize) {
			handle_resize((need_resize == cRedraw) ? TRUE : FALSE);
			need_resize = cNo;
		}

		/*
		 * Read artnum
		 */
		if ((ptr = tin_strtok(buf, "\t")) == NULL)
			continue;

		/*
		 * read the article number, guaranteed to be the first field
		 */
		artnum = atol(ptr);

		/*
		 * artnum field invalid/corrupt or is 1st line of local cached overview
		 * (group name)
		 */
		if (artnum <= 0)
			continue;

		/*
		 * skip artnums below the given minimum (getart_limit)
		 */
		if (artnum < min)
			continue;

		/*
		 * Check to make sure article in nov file has not expired in group
		 */
		if (artnum < group->xmin) {
			expired++;
			continue;
		}

		/*
		 * artnum in overview data higher than groups high mark
		 *
		 * TODO: - warn user about broken overviews?
		 *       - try to parse the Xref:-line to get the correct artnum
		 *       - see also parse_unread_arts()
		 */
		if (artnum > group->xmax)
			continue;

		if (top_art >= max_art)
			expand_art();

		art = &arts[top_art];
		set_article(art);
		art->artnum = *top = artnum;

		/*
		 * Note: Fields after line count are not mandatory, use "LIST OVERVIEW.FMT"
		 *       to check for additions like we do with xref_supported
		 */
		for (count = 1; (ptr = tin_strtok(NULL, "\t")) != NULL; count++) {
			/* skip unexpected tailing fields */
			if (count > over_fields) {
#ifdef DEBUG
				if (debug & DEBUG_NNTP)
					debug_print_file("NNTP", "%s(%d) Unexpected overview-field %d of %d: %s", nntp_caps.over_cmd, artnum, count, over_fields, ptr);
#endif /* DEBUG */

				/* "common error" Xref:full in overview-data but not in OVERVIEW.FTM */
				if (count == over_fields + 1) {
					if (!strncasecmp(ptr, "Xref: ", 6)) {
#ifdef DEBUG
						if (debug & DEBUG_NNTP)
							debug_print_file("NNTP", "%s: found unexpected Xref: on semi std. position", nntp_caps.over_cmd);
#endif /* DEBUG */
						over_fields++;
						ofmt = my_realloc(ofmt, sizeof(struct t_overview_fmt) * (over_fields + 2)); /* + 2 = artnum and end-marker */
						ofmt[over_fields].type = OVER_T_FSTRING;
						ofmt[over_fields].name = my_strdup("Xref:");
						ofmt[over_fields + 1].type = OVER_T_ERROR;
						ofmt[over_fields + 1].name = NULL;
						xref_supported = TRUE;
					} else
						continue;
				} else
					continue;
			}

			/* for duplicated headers this is last match counts, INN >= 2.5.3 does first match counts */
			if (expensive_over_parse) { /* strange order */
				/* madatory fields */
				if (ofmt[count].type == OVER_T_STRING) {
					if (!strcasecmp(ofmt[count].name, "Subject:")) {
						if (*ptr)
							art->subject = hash_str(eat_re(eat_tab(convert_to_printable(rfc1522_decode(ptr), FALSE)), FALSE));
						else {
							art->subject = hash_str("");
#ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) empty overview-field %s", nntp_caps.over_cmd, artnum, ofmt[count].name);
#endif /* DEBUG */
						}
						continue;
					}

					if (!strcasecmp(ofmt[count].name, "From:")) {
						if (*ptr) {
							art->gnksa_code = parse_from(ptr, art_from_addr, art_full_name);
							art->from = hash_str(buffer_to_ascii(art_from_addr));
							if (*art_full_name)
								art->name = hash_str(eat_tab(convert_to_printable(rfc1522_decode(art_full_name), FALSE)));
						} else {
							art->from = hash_str("");
#ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) empty overview-field %s", nntp_caps.over_cmd, artnum, ofmt[count].name);
#endif /* DEBUG */
						}
						continue;
					}

					if (!strcasecmp(ofmt[count].name, "Date:")) {
						art->date = parsedate(ptr, (TIMEINFO *) 0);
#ifdef DEBUG
						if ((debug & DEBUG_NNTP) && art->date == (time_t) -1)
							debug_print_file("NNTP", "%s(%d) bogus overview-field %s %s", nntp_caps.over_cmd, artnum, ofmt[count].name, ptr);
#endif /* DEBUG */
						continue;
					}

					if (!strcasecmp(ofmt[count].name, "Message-ID:")) {
						if (*ptr) {
							FreeIfNeeded(art->msgid); /* if field is listed more than once in overview.fmt */
							art->msgid = my_strdup(ptr);
						} else {
							art->msgid = NULL;
#ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) empty overview-field %s", nntp_caps.over_cmd, artnum, ofmt[count].name);
#endif /* DEBUG */
						}
						continue;
					}

					if (!strcasecmp(ofmt[count].name, "References:")) {
						if (*ptr) {
							FreeIfNeeded(art->refs); /* if field is listed more than once in overview.fmt */
							art->refs = my_strdup(ptr);
						} else
							art->refs = NULL;
						continue;
					}
				}
				/* metadata fields */
				if (ofmt[count].type == OVER_T_INT) {
					if (!strcasecmp(ofmt[count].name, "Bytes:")) {
						if (*ptr) {
#ifdef DEBUG
							if ((debug & DEBUG_NNTP) && !isdigit((unsigned char) *ptr))
									debug_print_file("NNTP", "%s(%d) overview field %d (%s) mismatch: %s", nntp_caps.over_cmd, artnum, count, ofmt[count].name, ptr);
#endif /* DEBUG */
						}
						continue;
					}

					if (!strcasecmp(ofmt[count].name, "Lines:")) {
						if (*ptr) {
							if (isdigit((unsigned char) *ptr))
								art->line_count = atoi(ptr);
							else {
								art->line_count = 0;
#ifdef DEBUG
								if (debug & DEBUG_NNTP)
									debug_print_file("NNTP", "%s(%d) overview field %d (%s) mismatch: %s", nntp_caps.over_cmd, artnum, count, ofmt[count].name, ptr);
#endif /* DEBUG */
							}
						} else
							art->line_count = 0;
						continue;
					}
				}
			} else { /* first 7 fields are in RFC 3977 order */
				switch (count) {
					case 1: /* Subject: */
						if (*ptr)
							art->subject = hash_str(eat_re(eat_tab(convert_to_printable(rfc1522_decode(ptr), FALSE)), FALSE));
						else {
							art->subject = hash_str("");
#ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) empty overview-field %s", nntp_caps.over_cmd, artnum, ofmt[count].name);
#endif /* DEBUG */
						}
						break;

					case 2:	/* From: */
						if (*ptr) {
							art->gnksa_code = parse_from(ptr, art_from_addr, art_full_name);
							art->from = hash_str(buffer_to_ascii(art_from_addr));
							if (*art_full_name)
								art->name = hash_str(eat_tab(convert_to_printable(rfc1522_decode(art_full_name), FALSE)));
						} else {
							art->from = hash_str("");
#ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) empty overview-field %s", nntp_caps.over_cmd, artnum, ofmt[count].name);
#endif /* DEBUG */
						}
						break;

					case 3:	/* Date: */
						art->date = parsedate(ptr, (TIMEINFO *) 0);
#ifdef DEBUG
						if ((debug & DEBUG_NNTP) && art->date == (time_t) -1)
							debug_print_file("NNTP", "%s(%d) bogus overview-field %s %s", nntp_caps.over_cmd, artnum, ofmt[count].name, ptr);
#endif /* DEBUG */
						break;

					case 4:	/* Message-ID: */
						if (*ptr)
							art->msgid = my_strdup(ptr);
						else {
							art->msgid = NULL;
#ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) empty overview-field %s", nntp_caps.over_cmd, artnum, ofmt[count].name);
#endif /* DEBUG */
						}
						break;

					case 5:	/* References: */
						if (*ptr)
							art->refs = my_strdup(ptr);
						else
							art->refs = NULL;
						break;

					case 6:	/* :bytes || Bytes: */
						if (*ptr) {
#ifdef DEBUG
							if ((debug & DEBUG_NNTP) && !isdigit((unsigned char) *ptr))
								debug_print_file("NNTP", "%s(%d) overview field %d (%s) mismatch: %s", nntp_caps.over_cmd, artnum, count, ofmt[count].name, ptr);
#endif /* DEBUG */
						}
						break;

					case 7:	/* :lines || Lines: */
						if (*ptr) {
							if (isdigit((unsigned char) *ptr))
								art->line_count = atoi(ptr);
							else {
								art->line_count = 0;
#ifdef DEBUG
								if (debug & DEBUG_NNTP)
									debug_print_file("NNTP", "%s(%d) overview field %d (%s) mismatch: %s", nntp_caps.over_cmd, artnum, count, ofmt[count].name, ptr);
#endif /* DEBUG */
							}
						} else
							art->line_count = 0;
						break;

					default:
						break;
				}
			}

			/* optional fields; for duplicated headers: last match counts, INN >= 2.5.3 does first match counts */
			if (ofmt[count].type == OVER_T_FSTRING) {
				if (*ptr) {
					if (!strcasecmp(ofmt[count].name, "Xref:")) {
						if ((q = parse_header(ptr, "Xref", FALSE, FALSE, FALSE)) != NULL) {
							FreeIfNeeded(art->xref); /* if field is listed more than once in overview.fmt */
							art->xref = my_strdup(q);
						}
#ifdef DEBUG
						else {
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "%s(%d) bogus overview-field %s %s", nntp_caps.over_cmd, artnum, ofmt[count].name, ptr);
						}
#endif /* DEBUG */
					}
				}
				continue;
			}
		}

		/*
		 * RFC says Message-ID is mandatory in newsgroups (but not in
		 * mailgroups etc..) NB. a NULL Message-ID would abort if we ever do
		 * threading in mailgroups
		 */
		if (!art->msgid && group->type == GROUP_TYPE_NEWS)
			continue;

		/* we might loose accuracy here, but that shouldn't hurt */
		if (artnum % MODULO_COUNT_NUM == 0)
			show_progress(group_msg, artnum - min, max - min);

		top_art++;				/* Basically this statement commits the article */
	}

	free(group_msg);
	TIN_FCLOSE(fp);

	if (tin_errno)
		return -1;

#if defined(NNTP_ABLE) && defined(XHDR_XREF)
	if (read_news_via_nntp && !read_saved_news && !xref_supported && nntp_caps.hdr_cmd) {
		char cbuf[HEADER_LEN];
		static t_bool found;
		static t_bool first = TRUE;

		if (first) {
			found = TRUE;
			/*
			 * TODO: do once a start and cache full result
			 *       if "LIST HEADERS RANGE" failed try "LIST HEADERS"?
			 */
			if (nntp_caps.type == CAPABILITIES && nntp_caps.list_headers) {
				int i = new_nntp_command("LIST HEADERS RANGE", 215, cbuf, sizeof(cbuf));

				found = FALSE;
				switch (i) {
					case 215:
						while ((ptr = tin_fgets(FAKE_NNTP_FP, FALSE)) != NULL) {
#	ifdef DEBUG
							if (debug & DEBUG_NNTP)
								debug_print_file("NNTP", "<<< %s", ptr);
#	endif /* DEBUG */
							if (!found && ((*ptr == ':' && *(ptr + 1) == '\0') || !strncasecmp(ptr, "Xref", 4)))
								found = TRUE;
						}
						break;

					default:
						break;
				}
				first = FALSE;
			}
		}

		if (found) {
			snprintf(cbuf, sizeof(cbuf), "%s XREF %ld-%ld", nntp_caps.hdr_cmd, min, MAX(min, max));
			group_msg = fmt_string("%s XREF loop", nntp_caps.hdr_cmd); /* TODO: find a better message, move to lang.c */
			if ((fp = nntp_command(cbuf, nntp_caps.hdr ? OK_HDR : OK_HEAD, NULL, 0)) != NULL) { /* RFC 2980 (XHDR) uses 221; RFC 3977 (HDR) uses 225 */
				while ((ptr = tin_fgets(fp, FALSE)) != NULL) {
					artnum = atol(ptr);
					if (artnum <= 0 || artnum < group->xmin || artnum > group->xmax)
						continue;
					art = &arts[top_art];
					set_article(art);
					if (!art->xref && !strstr(ptr, "(none)")) {
						if ((q = strchr(ptr, ' ')) == NULL) /* skip article number */
							continue;
						ptr = q;
						while (*ptr && isspace((int) *ptr))
							ptr++;
						q = strchr(ptr, '\n');
						if (q)
							*q = '\0';
						art->xref = my_strdup(ptr);
					}
					/* we might loose accuracy here, but that shouldn't hurt */
					if (artnum % MODULO_COUNT_NUM == 0)
						show_progress(group_msg, artnum - min, max - min);
				}
			}
			free(group_msg);
		}
	}
#endif /* NNTP_ABLE && XHDR_XREF */

	return expired;
}


/*
 * Write an Nov/Xover index file. Fields are separated by '\t'.
 *
 * Format:
 *	1. article number (ie. 183)                [mandatory]
 *	2. Subject: line  (ie. Which newsreader?)  [mandatory]
 *	3. From: line     (ie. iain@ecrc.de)       [mandatory]
 *	4. Date: line     (rfc822 format)          [mandatory]
 *	5. MessageID:     (ie. <123@ether.net>)    [mandatory]
 *	6. References:    (ie. <message-id> ....)  [optional]
 *	7. Byte count     (Skipped - not used)     [mandatory]
 *	8. Line count     (ie. 23)                 [mandatory]
 *	9. Xref: line     (ie. alt.test:389)       [optional]
 *
 * TODO: as we don't use the original data, we currently can't store
 *       the data (from/subject) in the original charset (we don't store
 *       that info). this has the advantage that we can avoid raw 8bit data
 *       in our overviews, but the disadvantage that we might store the data
 *       with a wrong charset and thus lose information. a simmiliar problem
 *       exists with the data for the from:-line, we don't store it in the
 *       original format, whenever our from-parser (partially) fails we'll
 *       lose information in our overviews (but those couldn't be handeled
 *       by tin anyway, so this is not a real problem).
 *       long-term solution: store the original data in the overview
 *       (tin has to handle raw 8bit data and other ugly stuff in the
 *       overviews anyway and thus we preserver as much info as possible)
 *       this would require some changes in read_overview() and
 *       parse_headers(): don't do the decoding/unfolding there, but in a
 *       second pass right after write_overview(), or two additional fields
 *       which hold the raw data for from/subject. the latter has the
 *       disadvantage that it costs (much) more memory.
 */
static void
write_overview(
	struct t_group *group)
{
	FILE *fp;
	int i;
	struct t_article *article;

	/*
	 * Can't write or caching is off or getart_limit is set
	 */
	if (no_write || !tinrc.cache_overview_files || (cmdline.args & CMDLINE_GETART_LIMIT ? cmdline.getart_limit : tinrc.getart_limit) != 0)
		return;

	if ((fp = open_xover_fp(group, "w", 0L, 0L, FALSE)) == NULL)
		return;

	if (group->attribute->sort_article_type != SORT_ARTICLES_BY_NOTHING)
		SortBy(artnum_comp);

	/*
	 * Needed to preserve uniqueness in hashed private overview files
	 */
	fprintf(fp, "%s\n", group->name);

	for_each_art(i) {
		char *p;
		char *q, *ref;

		article = &arts[i];

		if (article->thread != ART_EXPIRED && article->artnum >= group->xmin) {
			ref = NULL;
			/*
			 * TODO: instead of tinrc.mm_local_charset we'd better use UTF-8
			 *       here and in print_from() in the CHARSET_CONVERSION case.
			 *       note that this requires something like
			 *          buffer_to_network(article->subject, "UTF-8");
			 *       right bfore the rfc1522_encode() call.
			 *
			 *       if we would cache the original undecoded data, we could
			 *       ignore stuff like this.
			 */
			p = rfc1522_encode(article->subject, tinrc.mm_local_charset, FALSE);
			/* as the subject might now be folded we have to unfold it */
			unfold_header(p);

			/*
			 * replace any '\t's with ' ' in the references-data
			 *
			 * TODO: nntpext-draft might come up with a new scheme:
			 *       For all fields, the value is processed by first
			 *       removing all US-ASCII CRLF pairs and then replacing
			 *       each remaining US-ASCII NUL, TAB, CR, or LF character
			 *       with a single US-ASCII space (for example, CR LF LF TAB
			 *       will become two spaces).
			 */
			if (article->refs) {
				ref = q = my_strdup(article->refs);
				while (*q) {
					if (*q == '\t')
						*q = ' ';
					q++;
				}
			}

			fprintf(fp, "%ld\t%s\t%s\t%s\t%s\t%s\t%d\t%d",
				article->artnum,
				group->attribute->post_8bit_header ? article->subject : p,
				print_from(group, article),
				print_date(article->date),
				BlankIfNull(article->msgid),
				BlankIfNull(ref),
				0,	/* bytes */
				article->line_count);

			if (article->xref)
				fprintf(fp, "\tXref: %s", article->xref);

			fprintf(fp, "\n");
			free(p);
			if (article->refs) {
				FreeIfNeeded(ref);
				q = NULL;
			}
		}
	}
	fchmod(fileno(fp), (mode_t) (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
	fclose(fp);
}


/*
 * A complex little function to determine the correct overview index file
 * according to 'mode' (read or write)
 * NULL is returned if the current setup dictates otherwise
 *
 * GROUP_TYPE_MAIL index files are read/written in ~/.tin/.mail
 * GROUP_TYPE_SAVE index files are read/written in ~/.tin/.save
 *
 * Both of these are hashed
 *
 * GROUP_TYPE_NEWS index files are a little bit more complex
 *
 * When hashing the index filename will be in format number.number.
 * Hashing the groupname gets a number. See if that #.1 file exists;
 * if so, read first line. Is this the group we want? If no, try #.2.
 * Repeat until no such file or we find an existing file that matches
 * our group. Return pointer to path or NULL if not found.
 */
static char *
find_nov_file(
	struct t_group *group,
	int mode)
{
	FILE *fp;
	const char *dir;
	char buf[PATH_LEN];
	int i;
	struct stat sb;
	unsigned long hash;
	static char nov_file[PATH_LEN];
	static t_bool once_only = FALSE;	/* Trap things that are done only 1 time */

	if (group == NULL || (mode != R_OK && mode != W_OK))
		return NULL;

	switch (group->type) {
		case GROUP_TYPE_MAIL:
			dir = index_maildir;
			break;

		case GROUP_TYPE_SAVE:
			dir = index_savedir;
			break;

		case GROUP_TYPE_NEWS:
			/*
			 * nntp.caps.over_cmd is not an issue here, any gripes and warnings
			 * about [X]OVER are handled in nntp_open()
			 */

			/*
			 * When reading via NNTP, system wide overviews are irrelevant, of
			 * course, and the private overview filename will be the same for
			 * both reading and writing.
			 *
			 * When working locally, we only use a private cache for reading
			 * if requested and when system wide overviews don't already exist.
			 * When writing then only private overviews can be used since
			 * updating system wide overviews is not safe wrt locking etc.
			 *
			 * See if local overview file $SPOOLDIR/<groupname>/.overview exists
			 */
#ifndef NNTP_ONLY
			if (!read_news_via_nntp) {
				make_base_group_path(novrootdir, group->name, buf, sizeof(buf));
				joinpath(nov_file, sizeof(nov_file), buf, novfilename);
				if (access(nov_file, R_OK) == 0) {
					if (mode == R_OK)
						return nov_file;		/* Use system wide overviews */
					else
						return NULL;			/* Don't write cache in this case */
				}
			}
#endif /* !NNTP_ONLY */

			/*
			 * We only get here when private overviews are going to be used
			 * Go no further if they are explicitly turned off
			 */
			if (!tinrc.cache_overview_files)
				return NULL;

			/*
			 * Append -<nntpserver> to private cache dir
			 */
			if (!once_only && nntp_server) {
				const char *from;
				char *to;
				int c;

				to = index_newsdir + strlen(index_newsdir);
				*(to++) = '-';
				for (from = nntp_server; (c = *from) != 0; ++from)
					*(to++) = tolower(c);
				*to = '\0';
				once_only = TRUE;
			}

			/*
			 * Only try to set up the private cache when writing. If it
			 * doesn't exist yet, then ergo we can't read from it.
			 * The cache will be checked/created on every write; a previous
			 * bug report complained that this was not the case
			 */
			if (stat(index_newsdir, &sb) == -1) {			/* Private cache doesn't exist */
				if (mode == R_OK)
					return NULL;
				if (my_mkdir(index_newsdir, (mode_t) S_IRWXU) != 0)
					return NULL;
			} else {
				if (!S_ISDIR(sb.st_mode))
					return NULL;
			}

			/*
			 * Update the newsgroups cache to point to the new location
			 * now that we know it is valid
			 */
			if (!once_only)
				joinpath(local_newsgroups_file, sizeof(local_newsgroups_file), index_newsdir, NEWSGROUPS_FILE);

			dir = index_newsdir;
			break;

		default: /* not reached */
			return NULL;
	}

	/*
	 * We only get here if writing to a private overview.
	 * These always have hashed filenames.
	 * Try <hash>.<seqno> and check the group name tagline until
	 * matching index file is found. If not found return next unused
	 * filename
	 */
	hash = hash_groupname(group->name);

	for (i = 1; ; i++) {
		char *ptr;

		snprintf(buf, sizeof(buf), "%lu.%d", hash, i);
		joinpath(nov_file, sizeof(nov_file), dir, buf);

		if ((fp = fopen(nov_file, "r")) == NULL)
			break;

		/*
		 * No group name header, so not a valid index file => overwrite it
		 */
		if (fgets(buf, (int) sizeof(buf), fp) == NULL) {
			fclose(fp);
			break;
		}
		fclose(fp);

		if ((ptr = strrchr(buf, '\n')) != NULL)
			*ptr = '\0';

		if (strcmp(buf, group->name) == 0)
			break;

	}

	return nov_file;
}


/*
 * Run the index file updater only for the groups we've loaded.
 */
void
do_update(
	t_bool catchup)
{
	int i, j, k = 0;
	time_t beg_epoch;
	struct t_group *group;

	if (verbose)
		(void) time(&beg_epoch);

	/*
	 * loop through groups and update any required index files
	 */
	for (i = 0; i < selmenu.max; i++) {
		group = &active[my_group[i]];
		/*
		 * FIXME: workaround to get a valid CURR_GROUP
		 * it also points to the currently processed group so that
		 * the correct attributes are used
		 * The correct fix is to get rid of CURR_GROUP
		 */
		selmenu.curr = i;

		if (group->bogus || !group->subscribed)
			continue;

		if (!index_group(group))
			continue;

		k++;

		if (verbose) {
			my_printf("%s %s\n", (catchup ? _(txt_catchup) : _(txt_updating)), group->name);
			my_flush();
		}

		if (catchup) {
			for_each_art(j)
				art_mark(group, &arts[j], ART_READ);
		}
	}

	if (verbose) {
		wait_message(0, _(txt_catchup_update_info),
			(catchup ? _(txt_caughtup) : _(txt_updated)), k,
			PLURAL(selmenu.max, txt_group), (unsigned long int) (time(NULL) - beg_epoch));
	}
}


static int
artnum_comp(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	/*
	 * s1->artnum less than s2->artnum
	 */
	if (s1->artnum < s2->artnum)
		return -1;

	/*
	 * s1->artnum greater than s2->artnum
	 */
	if (s1->artnum > s2->artnum)
		return 1;

	return 0;
}


/*
 * return result of strcmp (reversed for descending)
 */
static int
subj_comp_asc(
	t_comptype p1,
	t_comptype p2)
{
	int retval;
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if ((retval = strcasecmp(s1->subject, s2->subject))) /* != 0 */
		return retval;

	return s1->date - s2->date > 0 ? 1 : -1;
}


static int
subj_comp_desc(
	t_comptype p1,
	t_comptype p2)
{
	int retval;
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if ((retval = strcasecmp(s2->subject, s1->subject))) /* != 0 */
		return retval;

	return s1->date - s2->date > 0 ? 1 : -1;
}


/*
 * return result of strcmp (reversed for descending)
 */
static int
from_comp_asc(
	t_comptype p1,
	t_comptype p2)
{
	int retval;
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if ((retval = strcasecmp(s1->from, s2->from))) /* != 0 */
		return retval;

	return s1->date - s2->date > 0 ? 1 : -1;
}


static int
from_comp_desc(
	t_comptype p1,
	t_comptype p2)
{
	int retval;
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if ((retval = strcasecmp(s2->from, s1->from))) /* != 0 */
		return retval;

	return s1->date - s2->date > 0 ? 1 : -1;
}


/*
 * Works like strcmp() for comparing time_t type values
 * Return codes:
 *  -1:		If p1 is before p2
 *   0:		If they are the same time
 *   1:		If p1 is after p2
 * If the sort order is _not_ DATE_ASCEND then the sense of the above
 * is reversed.
 */
static int
date_comp_asc(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	/*
	 * s1->date less than s2->date
	 */
	if (s1->date < s2->date)
		return -1;

	/*
	 * s1->date greater than s2->date
	 */
	if (s1->date > s2->date)
		return 1;

	return 0;
}


static int
date_comp_desc(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	/*
	 * s2->date less than s1->date
	 */
	if (s2->date < s1->date)
		return -1;

	/*
	 * s2->date greater than s1->date
	 */
	if (s2->date > s1->date)
		return 1;

	return 0;
}


/*
 * Same again, but for art[].score
 */
static int
score_comp_asc(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if (s1->score < s2->score)
		return -1;

	if (s1->score > s2->score)
		return 1;

	return s1->date - s2->date > 0 ? 1 : -1;
}


static int
score_comp_desc(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if (s2->score < s1->score)
		return -1;

	if (s2->score > s1->score)
		return 1;

	return s1->date - s2->date > 0 ? 1 : -1;
}


/*
 * Same again, but for art[].line_count
 */
static int
lines_comp_asc(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if (s1->line_count < s2->line_count)
		return -1;

	if (s1->line_count > s2->line_count)
		return 1;

	return s1->date - s2->date > 0 ? 1 : -1;
}


static int
lines_comp_desc(
	t_comptype p1,
	t_comptype p2)
{
	const struct t_article *s1 = (const struct t_article *) p1;
	const struct t_article *s2 = (const struct t_article *) p2;

	if (s2->line_count < s1->line_count)
		return -1;

	if (s2->line_count > s1->line_count)
		return 1;

	return s1->date - s2->date > 0 ? 1 : -1;
}


/*
 * Compares the total score of two threads. Used for sorting base[].
 */
static int
score_comp_base(
	t_comptype p1,
	t_comptype p2)
{
	int a = get_score_of_thread(*(const long *) p1);
	int b = get_score_of_thread(*(const long *) p2);

	/* If scores are equal, compare using the article sort order.
	 * This determines the order in a group of equally scored threads.
	 */
	if (a == b) {
		const struct t_article *s1 = &arts[*(const long *) p1];
		const struct t_article *s2 = &arts[*(const long *) p2];
		t_compfunc comp_func = eval_sort_arts_func(CURR_GROUP.attribute->sort_article_type);

		if (comp_func)
			return (*comp_func)(s1, s2);
		return 0;
	}

	if (CURR_GROUP.attribute->sort_threads_type == SORT_THREADS_BY_SCORE_ASCEND)
		return a > b ? 1 : -1;
	return a < b ? 1 : -1;
}


/*
 * Compare the date of the last posted article of two threads.
 * Used for sorting base[].
 */
static int
last_date_comp_base_desc(
	t_comptype p1,
	t_comptype p2)
{
	time_t s1_last = get_last_posting_date(*(const long *) p1);
	time_t s2_last = get_last_posting_date(*(const long *) p2);

	if (s2_last < s1_last)
		return -1;

	if (s2_last > s1_last)
		return 1;

	return 0;
}


static int
last_date_comp_base_asc(
	t_comptype p1,
	t_comptype p2)
{
	time_t s1_last = get_last_posting_date(*(const long *) p1);
	time_t s2_last = get_last_posting_date(*(const long *) p2);

	if (s2_last > s1_last)
		return -1;

	if (s2_last < s1_last)
		return 1;

	return 0;
}


static time_t get_last_posting_date(
	long n)
{
	long i;
	time_t last = (time_t) 0;

	for (i = n; i >= 0; i = arts[i].thread) {
		if (arts[i].date > last)
			last = arts[i].date;
	}

	return last;
}


void
set_article(
	struct t_article *art)
{
	art->subject = NULL;
	art->from = NULL;
	art->name = NULL;
	art->date = (time_t) 0;
	art->xref = NULL;
	art->msgid = NULL;
	art->refs = NULL;
	art->refptr = NULL;
	art->line_count = -1;
	art->archive = NULL;
	art->tagged = 0;
	art->thread = ART_EXPIRED;
	art->prev = ART_NORMAL;
	art->score = 0;
	art->status = ART_UNREAD;
	art->killed = ART_NOTKILLED;
	art->zombie = FALSE;
	art->delete_it = FALSE;
	art->selected = FALSE;
	art->inrange = FALSE;
	art->matched = FALSE;
	art->keep_in_base = FALSE;
}


/*
 * Do a binary chop to see if 'art' (an article number) exists in arts[]
 * Naturally arts[] must be sorted on artnum
 * Return index into arts[] or -1
 */
static int
valid_artnum(
	long art)
{
	int prev, range;
	int dctop = top_art;
	int cur = 1;

	while ((dctop >>= 1))
		cur <<= 1;

	range = cur >> 1;
	cur--;

	forever {
		if (arts[cur].artnum == art)
			return cur;

		prev = cur;
		cur += ((arts[cur].artnum < art) ? range : -range);
		if (prev == cur)
			break;

		if (cur >= top_art)
			cur = top_art - 1;

		range >>= 1;
	}
	return -1;
}


/*
 * Loop over arts[] to see if 'art' (an article number) exists in arts[]
 * Needed if arts[] is not sorted on artnum
 * Return index into arts[] or -1
 */
int
find_artnum(
	long art)
{
	int i;

	for_each_art(i) {
		if (arts[i].artnum == art)
			return i;
	}
	return -1;
}


/*----------------------------- Overview handling -----------------------*/

static char *
print_date(
	time_t secs)
{
	static char date[25];
	struct tm *tm;
	static const char *const months_a[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	};

	tm = gmtime(&secs);
	snprintf(date, sizeof(date), "%02d %s %04d %02d:%02d:%02d GMT",
			tm->tm_mday,
			months_a[tm->tm_mon],
			tm->tm_year + 1900,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	return date;
}


static char *
print_from(
	struct t_group *group,
	struct t_article *article)
{
	char *p;
	static char from[PATH_LEN];

	*from = '\0';

	if (article->name != NULL) {
		p = rfc1522_encode(article->name, tinrc.mm_local_charset, FALSE);
		unfold_header(p);
		if (strpbrk(article->name, "\".:;<>@[]()\\") != NULL && article->name[0] != '"' && article->name[strlen(article->name)] != '"')
			snprintf(from, sizeof(from), "\"%s\" <%s>", group->attribute->post_8bit_header ? article->name : p, article->from);
		else
			snprintf(from, sizeof(from), "%s <%s>", group->attribute->post_8bit_header ? article->name : p, article->from);

		free(p);
	} else
		STRCPY(from, article->from);

	return from;
}


/*
 * Open a group news overview file
 * Use NNTP XOVER where possible unless 'local' is set
 */
static FILE *
open_xover_fp(
	struct t_group *group,
	const char *mode,
	long min,
	long max,
	t_bool local)
{
#ifdef NNTP_ABLE
	if (!local && nntp_caps.over_cmd && *mode == 'r' && group->type == GROUP_TYPE_NEWS) {
		char line[NNTP_STRLEN];

		if (!max)
			return NULL;
		if (min == max)
			snprintf(line, sizeof(line), "%s %ld", nntp_caps.over_cmd, min);
		else
			snprintf(line, sizeof(line), "%s %ld-%ld", nntp_caps.over_cmd, min, MAX(min, max));
		return (nntp_command(line, OK_XOVER, NULL, 0));
	}
#endif /* NNTP_ABLE */
	{
		FILE *fp;
		char *nov_file = find_nov_file(group, (*mode == 'r') ? R_OK : W_OK);

		if (nov_file != NULL) {
			if ((fp = fopen(nov_file, mode)) != NULL)
				return fp;

			if (*mode != 'r')
				error_message(2, _(txt_cannot_open), nov_file);
		}
	}
	return NULL;
}
