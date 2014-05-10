/*
 *  Project   : tin - a Usenet reader
 *  Module    : refs.c
 *  Author    : Jason Faultless <jason@altarstone.com>
 *  Created   : 1996-05-09
 *  Updated   : 2008-12-04
 *  Notes     : Cacheing of message ids / References based threading
 *  Credits   : Richard Hodson <richard@macgyver.tele2.co.uk>
 *              hash_msgid, free_msgid
 *
 * Copyright (c) 1996-2009 Jason Faultless <jason@altarstone.com>
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

#define MAX_REFS	100			/* Limit recursion depth */
#define REF_SEP	" \t"			/* Separator chars in ref headers */

#ifdef DEBUG
#	define DEBUG_PRINT(x)	fprintf x
static FILE *dbgfd;
#endif /* DEBUG */

/*
 * local prototypes
 */
static char *_get_references(struct t_msgid *refptr, int depth);
static struct t_msgid *add_msgid(int key, const char *msgid, struct t_msgid *newparent);
static struct t_msgid *find_next(struct t_msgid *ptr);
static struct t_msgid *parse_references(char *r);
static t_bool valid_msgid(const char *msgid);
static unsigned int hash_msgid(const char *key);
static void add_to_parent(struct t_msgid *ptr);
static void build_thread(struct t_msgid *ptr);
#ifdef DEBUG
	static void dump_msgid_thread(struct t_msgid *ptr, int level);
	static void dump_msgid_threads(void);
#endif /* DEBUG */
#if 0
	static void dump_msgids(void);
	static void dump_thread(FILE *fp, struct t_msgid *msgid, int level);
#endif /* 0 */

/*
 * Set if the sorting algorithm goes 'upwards'
 */
static t_bool sort_ascend;

/*
 * The msgids are all hashed into a big array, with overspill
 */
static struct t_msgid *msgids[MSGID_HASH_SIZE] = {0};

/*
 * This part of the code deals with the caching and retrieval
 * of Message-ID and References headers
 *
 * Rationale:
 *    Even though the Message-ID is unique to an article, the References
 *    field contains msgids from elsewhere in the group. As the expiry
 *    period increases, so does the redundancy of data.
 *    At the time of writing, comp.os.ms-windows.advocacy held ~850
 *    articles. The references fields contained 192k of text, of which
 *    169k was saved using the new caching.
 *
 *    When threading on Refs, a much better view of the original thread
 *    can be built up using this data, and threading is much faster
 *    because all the article relationships are automatically available
 *    to us.
 *
 * NB: We don't cache msgids from the filter file.
 */

/*
 * Hash a message id. A msgid is of the form <unique@sitename>
 * (But badly broken message id's do occur)
 * We hash on the unique portion which should have good randomness in
 * the lower 5 bits. Propagate the random bits up with a shift, and
 * mix in the new bits with an exclusive or.
 *
 * This should generate about 5+strlen(string_start) bits of randomness.
 * MSGID_HASH_SIZE is a prime of order 2^11
 */
static unsigned int
hash_msgid(
	const char *key)
{
	unsigned int hash = 0;

	while (*key && *key != '@') {
		hash = (hash << 1) ^ *key;
		++key;
	}

	hash %= MSGID_HASH_SIZE;

	return hash;
}


/*
 * Thread us into our parents' list of children.
 */
static void
add_to_parent(
	struct t_msgid *ptr)
{
	struct t_msgid *p;

	if (!ptr->parent)
		return;

	/*
	 * Trivial case - if we are the first child (followup)
	 */
	if (ptr->parent->child == NULL) {
		ptr->parent->child = ptr;
		return;
	}

	/*
	 * Add this followup to the sibling chain of our parent.
	 * arts[] has been sorted by build_references and we add at the start or end
	 * of the chain depending on whether the sort method is ASCEND or DESCEND
	 * Unavailable articles go at the start of the chain if ASCEND (because
	 * we presume unavailable arts (ie REF_REF links) have expired), otherwise at the end.
	 * ie: if ASCEND && REF
	 *        add_to_start
	 *     else
	 *        add_to_end
	 */
	if (sort_ascend && (ptr->article == ART_UNAVAILABLE)) {
		/* Add to start */
		ptr->sibling = ptr->parent->child;
		ptr->parent->child = ptr;
	} else {
		/* Add to end */
		for (p = ptr->parent->child; p->sibling != NULL; p = p->sibling)
			;

/*		ptr->sibling is already NULL */
		p->sibling = ptr;
	}
}


/*
 * Checks if Message-ID has valid format
 * Returns TRUE if it does, FALSE if it does not
 *
 * TODO: combine with post.c:damaged_id()?
 */
static t_bool
valid_msgid(
	const char *msgid)
{
	size_t mlen = 0;
	t_bool at_present = 0;

	if (!msgid || *msgid != '<')
		return FALSE;

	while (isascii((unsigned char) *msgid) && isgraph((unsigned char) *msgid) && !iscntrl((unsigned char) *msgid) && *msgid != '>') {
		if (*msgid == '@')
			at_present = TRUE;
		mlen++;
		msgid++;
	}

	if (!at_present || (*msgid != '>') || mlen <= 2 || *(msgid + 1))
		return FALSE;

	return TRUE;
}


/*
 * Adds or updates a message id in the cache.
 * We return a ptr to the msgid, whether located or newly created.
 *
 * . If the message id is new, add it to the cache, creating parent, child
 *   & sibling ptrs if a parent is given.
 *
 * . If the message id is a duplicate, then:
 *     a) If no parent or the same parent, is given, no action is needed.
 *
 *     b) If a parent is specified and the current parent is NULL, then
 *        add in the new parent and create child/sibling ptrs.
 *        Because we add Message-ID headers first, we don't have to worry
 *        about bogus Reference headers messing things up.
 *
 *     c) If a conflicting parent is given:
 *
 *        If (key == REF_REF) ignore the error - probably the refs
 *        headers are broken or have been truncated.
 *
 *        Otherwise we have a genuine problem, two articles in one group
 *        with identical Message-IDs. This is indicative of a broken
 *        overview database.
 */
static struct t_msgid *
add_msgid(
	int key,
	const char *msgid,
	struct t_msgid *newparent)
{
	struct t_msgid *ptr;
	struct t_msgid *i;
	unsigned int h;

	if (!msgid) {
		error_message(2, "add_msgid: NULL msgid\n");
		giveup();
	}

	h = hash_msgid(msgid + 1);				/* Don't hash the initial '<' */

#ifdef DEBUG
	if (debug & DEBUG_REFS)
		DEBUG_PRINT((dbgfd, "---------------- Add %s %s with parent %s\n", (key == MSGID_REF) ? "MSG" : "REF", msgid, (newparent == NULL) ? _("unchanged") : newparent->txt));
#endif /* DEBUG */

	/*
	 * Look for this message id in the cache.
	 * Broken software will sometimes damage the case of a message-id.
	 */
	for (i = msgids[h]; i != NULL; i = i->next) {
		if (strcasecmp(i->txt, msgid) != 0)				/* No match yet */
			continue;

		/*
		 * CASE 1a - No parent specified, do nothing
		 */
		if (newparent == NULL) {
#ifdef DEBUG
			if (debug & DEBUG_REFS)
				DEBUG_PRINT((dbgfd, "nop: %s No parent specified\n", i->txt));
#endif /* DEBUG */
			return i;
		}

		/*
		 * CASE 1b - Parent not changed, do nothing
		 */
		if (newparent == i->parent) {
#ifdef DEBUG
			if (debug & DEBUG_REFS)
				DEBUG_PRINT((dbgfd, "dup: %s -> %s (no change)\n", i->txt, i->parent ? i->parent->txt : "NULL"));
#endif /* DEBUG */
			return i;
		}

		/*
		 * CASE2 - A parent has been given where there was none before.
		 *         Change parent from null -> not-null & update ptrs
		 */
		if (i->parent == NULL) {
			/*
			 * Detect & ignore circular reference paths by looking for the
			 * new parent in this thread
			 */
			for (ptr = newparent; ptr != NULL; ptr = ptr->parent) {
				if (ptr == i) {
#ifdef DEBUG
					if (debug & DEBUG_REFS)
						DEBUG_PRINT((dbgfd, "Avoiding circular reference! (%s)\n", (key == MSGID_REF) ? "MSG" : "REF"));
#endif /* DEBUG */
					return i;
				}
			}

			i->parent = newparent;
			add_to_parent(i);
#ifdef DEBUG
			if (debug & DEBUG_REFS)
				DEBUG_PRINT((dbgfd, "set: %s -> %s\n", i->txt, newparent ? newparent->txt : _("None")));
#endif /* DEBUG */
			return i;
		}

		/*
		 * CASE 3 - A new parent has been given that conflicts with the
		 *			current one. This is caused by
		 * 1) A duplicate Message-ID in the spool (very bad !)
		 * 2) corrupt References header
		 * All we can do is ignore the error
		 */
		if (i->parent != newparent) {
#ifdef DEBUG
			if (debug & DEBUG_REFS)
				DEBUG_PRINT((dbgfd, "Warning: (%s) Ignoring %s -> %s (already %s)\n",
					(key == MSGID_REF) ? "MSG" : "REF", i->txt,
					newparent ? newparent->txt : "None", i->parent->txt));
#endif /* DEBUG */

			return i;
		}

		error_message(2, "Error: Impossible combination of conditions !\n");
		return i;
	}

#ifdef DEBUG
	if (debug & DEBUG_REFS)
		DEBUG_PRINT((dbgfd, "new: %s -> %s\n", msgid, (newparent)?newparent->txt:"None"));
#endif /* DEBUG */

	/*
	 * This is a new node, so build a structure for it
	 */
	ptr = my_malloc(sizeof(struct t_msgid) + strlen(msgid));

	strcpy(ptr->txt, msgid);
	ptr->parent = newparent;
	ptr->child = ptr->sibling = NULL;
	ptr->article = (key == MSGID_REF ? top_art : ART_UNAVAILABLE);

	add_to_parent(ptr);

	/*
	 * Insert at head of list for speed.
	 */
	ptr->next = msgids[h];
	msgids[h] = ptr;

	return ptr;
}


/*
 * Find a Message-ID in the cache. Return ptr to this node, or NULL if
 * not found.
 */
struct t_msgid *
find_msgid(
	const char *msgid)
{
	unsigned int h;
	struct t_msgid *i;

	h = hash_msgid(msgid + 1);				/* Don't hash the initial '<' */

	/*
	 * Look for this message id in the cache.
	 * Broken software will sometimes damage the case of a message-id.
	 */
	for (i = msgids[h]; i != NULL; i = i->next) {
		if (strcasecmp(i->txt, msgid) == 0)				/* Found it */
			return i;
	}

	return NULL;
}


/*
 * Take a raw line of references data and return a ptr to a linked list of
 * msgids, starting with the most recent entry. (Thus the list is reversed)
 * Following the parent ptrs leads us back to the start of the thread.
 *
 * We iterate through the refs, adding each to the msgid cache, with
 * the previous ref as the parent.
 * The space saving vs. storing the refs as a single string is significant.
 */
static struct t_msgid *
parse_references(
	char *r)
{
	char *ptr;
	struct t_msgid *parent, *current;

	if (!r)
		return NULL;

#ifdef DEBUG
	if (debug & DEBUG_REFS)
		DEBUG_PRINT((dbgfd, "parse_references: %s\n", r));
#endif /* DEBUG */

	/*
	 * Break the refs down, using REF_SEP as delimiters
	 */
	if ((ptr = strtok(r, REF_SEP)) == NULL)
		return NULL;

	/*
	 * By definition, the head of the thread has no parent
	 */
	parent = NULL;

	if (!valid_msgid(ptr))
		return NULL;

	current = add_msgid(REF_REF, ptr, parent);

	while ((ptr = strtok(NULL, REF_SEP)) != NULL) {
		if (valid_msgid(ptr)) {
			parent = current;
			current = add_msgid(REF_REF, ptr, parent);
		}
	}

	return current;
}


/*
 * Reconstruct the References: field from the parent pointers
 * NB: In deep threads this can lead to a very long line. If you want to use
 *     this function to build a Reference: line for posting be aware that the
 *     server might refuse long lines -- short it accordingly!
 */
static char *
_get_references(
	struct t_msgid *refptr,
	int depth)
{
	char *refs;
	static size_t len;		/* Accumulated size */
	static size_t pos;		/* current insertion position */

	if (depth == 1)
		len = 0;

	len += strlen(refptr->txt) + 1;	/* msgid + space */
	if (refptr->parent == NULL || depth > MAX_REFS) {

#ifdef DEBUG
		if (debug & DEBUG_REFS) {
			if (depth > MAX_REFS)
				error_message(2, "Warning: Too many refs near to %s. Truncated\n", refptr->txt);
		}
#endif /* DEBUG */
		refs = my_malloc(len + 1);	/* total length + nullbyte */
		pos = 0;
	} else
		refs = _get_references(refptr->parent, depth + 1);

	sprintf(refs + pos, "%s ", refptr->txt);
	pos = strlen(refs);

	return refs;
}


/*
 * A wrapper to the above, null terminate the string
 */
char *
get_references(
	struct t_msgid *refptr)
{
	char *refs;
	size_t len;

	if (refptr == NULL)
		return NULL;

	refs = _get_references(refptr, 1);
	len = strlen(refs);
	refs[len - 1] = '\0';

	return refs;
}


/*
 * Clear the entire msgid cache, freeing up all chains. This is
 * normally only needed when entering a new group
 */
void
free_msgids(
	void)
{
	int i;
	struct t_msgid *ptr, *next, **msgptr;

	msgptr = msgids;				/* first list */

	for (i = MSGID_HASH_SIZE - 1; i >= 0; i--) {	/* count down is faster */
		ptr = *msgptr;
		*msgptr++ = NULL;			/* declare list empty */

		while (ptr != NULL) {	/* for each node in the list */

			next = ptr->next;		/* grab ptr before we free node */
			free(ptr);			/* release node */

			ptr = next;			/* hop down the chain */
		}
	}
}


#if 0	/* But please don't remove it */
/*
 * Function to dump an ASCII tree map of a thread rooted at msgid.
 * Output goes to fp, level is the current depth of the tree.
 */
static void
dump_thread(
	FILE *fp,
	struct t_msgid *msgid,
	int level)
{
	char buff[120];		/* This is _probably_ enough */
	char *ptr = buff;
	int i, len;

	/*
	 * Dump the current article
	 */
	sprintf(ptr, "%3d %*s", msgid->article, 2*level, "  ");

	len = strlen(ptr);
	i = cCOLS - len - 20;

	if (msgid->article >= 0)
		sprintf(ptr + len, "%-*.*s   %-17.17s", i, i, arts[msgid->article].subject, (arts[msgid->article].name) ? arts[msgid->article].name : arts[msgid->article].from);
	else
		sprintf(ptr + len, "%-*.*s", i, i, _("[- Unavailable -]"));

	fprintf(fp, "%s\n", ptr);

	if (msgid->child != NULL)
		dump_thread(fp, msgid->child, level + 1);

	if (msgid->sibling != NULL)
		dump_thread(fp, msgid->sibling, level);

	return;
}


static void
dump_msgids(
	void)
{
	int i;
	struct t_msgid *ptr;

	my_fprintf(stderr, "Dumping...\n");

	for (i = 0; i < MSGID_HASH_SIZE; i++) {
		if (msgids[i] != NULL) {
			my_fprintf(stderr, "node %d", i);
			for (ptr = msgids[i]; ptr != NULL; ptr = ptr->next)
				my_fprintf(stderr, " -> %s", ptr->txt);

			my_fprintf(stderr, "\n");

		}
	}
}
#endif /* 0 */


/*
 * The rest of this code deals with reference threading
 *
 * Legend:
 *
 * . When a new thread is started, the root message will have no
 *   References: field
 *
 * . When a followup is posted, the message-id that was referred to
 *   will be appended to the References: field. If no References:
 *   field exists, a new one will be created, containing the single
 *   message-id
 *
 * . The References: field should not be truncated, though in practice
 *   this will happen, often in badly broken ways.
 *
 * This is simplistic, so check out RFC 1036 & son of RFC 1036 for full
 * details from the posting point of view.
 *
 * We attempt to maintain 3 pointers in each message-id to handle threading
 * on References:
 *
 * 1) parent  - the article that the current one was in reply to
 *              An article with no References: has no parent, therefore
 *              it is the root of a thread.
 *
 * 2) sibling - the next reply in sequence to parent.
 *
 * 3) child   - the first reply to the current article.
 *
 * These pointers are automatically set up when we read in the
 * headers for a group.
 *
 * It remains for us to fill in the .thread and .prev ptrs in
 * each article that exists in the spool, using the intelligence of
 * the reference tree to locate the 'next' article in the thread.
 *
 */
/*
 * Clear out all the article fields from the msgid hash prior to a
 * rethread.
 */
void
clear_art_ptrs(
	void)
{
	int i;
	struct t_msgid *ptr;

	for (i = MSGID_HASH_SIZE - 1; i >= 0; i--) {
		for (ptr = msgids[i]; ptr != NULL; ptr = ptr->next)
			ptr->article = ART_UNAVAILABLE;
	}
}


#ifdef DEBUG
/*
 * Dump out all the threads from the msgid point of view, show the
 * related article index in arts[] where possible
 * A thread is defined as a starting article with no parent
 */
static void
dump_msgid_thread(
	struct t_msgid *ptr,
	int level)
{
	fprintf(dbgfd, "%*s %s (%d)\n", level * 3, "   ", ptr->txt, ptr->article);

	if (ptr->child != NULL)
		dump_msgid_thread(ptr->child, level + 1);

	if (ptr->sibling != NULL)
		dump_msgid_thread(ptr->sibling, level);

	return;
}


static void
dump_msgid_threads(
	void)
{
	int i;
	struct t_msgid *ptr;

	fprintf(dbgfd, "Dump started.\n\n");

	for (i = 0; i < MSGID_HASH_SIZE; i++) {
		if (msgids[i] != NULL) {

			for (ptr = msgids[i]; ptr != NULL; ptr = ptr->next) {

				if (ptr->parent == NULL) {
					dump_msgid_thread(ptr, 1);
					fprintf(dbgfd, "\n");
				}
			}
		}
	}

	fprintf(dbgfd, "Dump complete.\n\n");
}
#endif /* DEBUG */


/*
 * Find the next message in the thread.
 * We descend children before siblings, and only return articles that
 * exist in arts[] or NULL if we are truly at the end of a thread.
 * If there are no more down pointers, backtrack to find a sibling
 * to continue the thread, we note this with the 'bottom' flag.
 *
 * A Message-ID will not be included in a thread if
 *  It doesn't point to an article OR
 *     (it's already threaded/expired OR it has been autokilled)
 */
#define SKIP_ART(ptr)	\
	(ptr && (ptr->article == ART_UNAVAILABLE || \
		(arts[ptr->article].thread != ART_UNTHREADED || \
			(tinrc.kill_level == KILL_NOTHREAD && arts[ptr->article].killed))))

static struct t_msgid *
find_next(
	struct t_msgid *ptr)
{
	static t_bool bottom = FALSE;

	/*
	 * Keep going while we haven't bottomed out and we haven't
	 * got something in arts[]
	 */
	while (ptr != NULL) {

		/*
		 * Children first, unless bottom is set
		 */
		if (!bottom && ptr->child != NULL) {
			ptr = ptr->child;

			/*
			 * If article not present, keep going
			 */
			if (SKIP_ART(ptr))
				continue;
			else
				break;
		}

		if (ptr->sibling != NULL) {
			bottom = FALSE;

			ptr = ptr->sibling;

			/*
			 * If article not present, keep going
			 */
			if (SKIP_ART(ptr))
				continue;
			else
				break;
		}

		/*
		 * No more child or sibling to follow, backtrack up to
		 * a sibling if we can find one
		 */
		if (ptr->child == NULL && ptr->sibling == NULL) {
			while (ptr != NULL && ptr->sibling == NULL)
				ptr = ptr->parent;

			/*
			 * We've backtracked up to the parent with a suitable sibling
			 * go round once again to move to this sibling
			 */
			if (ptr)
				bottom = TRUE;
			else
				break;		/* Nothing found, exit with NULL */
		}
	}

	return ptr;
}


/*
 * Run the .thread and .prev pointers through the members of this
 * thread.
 */
static void
build_thread(
	struct t_msgid *ptr)
{
	struct t_msgid *newptr;

	/*
	 * If the root article is gone/expired/killed, find the first valid one
	 */
	if (SKIP_ART(ptr))
		ptr = find_next(ptr);

	/*
	 * Keep working through the thread, updating the ptrs as we go
	 */
	while ((newptr = find_next(ptr)) != NULL) {
		arts[newptr->article].prev = ptr->article;
		arts[ptr->article].thread = newptr->article;
		ptr = newptr;
	}
}


/*
 * Run a new set of threads through the base articles, using the
 * parent / child / sibling  / article pointers in the msgid hash.
 */
void
thread_by_reference(
	void)
{
	int i;
	struct t_msgid *ptr;

#ifdef DEBUG
	if (debug & DEBUG_REFS) {
		char file[PATH_LEN];

		joinpath(file, sizeof(file), TMPDIR, "REFS.info");
		dbgfd = fopen(file, "w");
		dump_msgid_threads();
	}
#endif /* DEBUG */

	/*
	 * Build threads starting from root msgids (ie without parent)
	 */
	for (i = 0; i < MSGID_HASH_SIZE; i++) {
		if (msgids[i] != NULL) {
			for (ptr = msgids[i]; ptr != NULL; ptr = ptr->next) {
				if (ptr->parent == NULL)
					build_thread(ptr);
			}
		}
	}

#ifdef DEBUG
	if (debug & DEBUG_REFS) {
		fprintf(dbgfd, "Full dump of threading info...\n");
		fprintf(dbgfd, "%3s %3s %3s %3s : %3s %3s\n", "#", "Par", "Sib", "Chd", "In", "Thd");

		for_each_art(i) {
			fprintf(dbgfd, "%3d %3d %3d %3d : %3d %3d : %.50s %s\n", i,
				(arts[i].refptr->parent) ? arts[i].refptr->parent->article : -2,
				(arts[i].refptr->sibling) ? arts[i].refptr->sibling->article : -2,
				(arts[i].refptr->child) ? arts[i].refptr->child->article : -2,
				arts[i].prev, arts[i].thread, arts[i].refptr->txt, arts[i].subject);
		}

		fclose(dbgfd);
	}
#endif /* DEBUG */

	return;
}


/*
 * Do the equivalent of subject threading, but only on the thread base
 * messages.
 * This should help thread together mistakenly multiply posted articles,
 * articles which were posted to a group rather than as followups, those
 * with missing ref headers etc.
 * We add joined threads onto the end of the .thread chain of the previous
 * thread. arts[] is already sorted, so the sorting of these will be
 * correct.
 */
void
collate_subjects(
	void)
{
	int i, j, art;
	struct t_hashnode *h;

	/*
	 * Run through the root messages of each thread. We have to traverse
	 * using arts[] and not msgids[] to preserve the sorting.
	 */
	for_each_art(i) {
		/*
		 * Ignore already threaded and expired arts
		 */
		if (arts[i].prev >= 0 || IGNORE_ART(i))
			continue;

		/*
		 * Get the contents of the magic marker in the hashnode
		 */
		h = (struct t_hashnode *) (arts[i].subject - sizeof(int) - sizeof(void *)); /* FIXME: cast increases required alignment of target type */
		j = h->aptr;

		if (j != -1 && j < i) {
			/*
			 * Modified form of the subject threading - the only difference
			 * is that we have to add later threads onto the end of the
			 * previous thread
			 */
			if (
					(arts[i].subject == arts[j].subject) /* ||
					(arts[i].archive && arts[j].archive && (arts[i].archive->name == arts[j].archive->name)) */
			) {
/*DEBUG_PRINT((dbgfd, "RES: %d is now previous, at end of %d\n", i, j));*/

				for (art = j; arts[art].thread >= 0; art = arts[art].thread)
					;

				arts[art].thread = i;
				arts[i].prev = art;
			}
		}

		/*
		 * Update the magic marker with the highest numbered mesg in
		 * arts[] that has been used in this thread so far
		 */
		h->aptr = i;
	}

	return;
}


/*
 * Builds the reference tree:
 *
 * 1) Sort the article base. This will ensure that articles and their
 *    siblings are inserted in the correct order.
 * 2) Add each Message-ID header and its direct reference ('reliable info')
 *    to the cache. Son of RFC 1036 mandates that if References headers must
 *    be trimmed, then at least the (1st three and) last reference should be
 *    maintained.
 * 3) Add rest of References header to the cache. This information is less
 *    reliable than the info added in 2) and is only used to fill in any
 *    gaps in the reference tree - no information is superceded.
 * 4) free() up the msgid and refs headers once cached
 */
void
build_references(
	struct t_group *group)
{
	char *s;
	int i;
	struct t_article *art;
	struct t_msgid *refs;

	/*
	 * The articles are currently unsorted, and are as they were put by setup_hard_base()
	 */
	if (group->attribute->sort_article_type != SORT_ARTICLES_BY_NOTHING)
		sort_arts(group->attribute->sort_article_type);

	sort_ascend = (group->attribute->sort_article_type == SORT_ARTICLES_BY_SUBJ_ASCEND ||
	               group->attribute->sort_article_type == SORT_ARTICLES_BY_FROM_ASCEND ||
	               group->attribute->sort_article_type == SORT_ARTICLES_BY_DATE_ASCEND ||
	               group->attribute->sort_article_type == SORT_ARTICLES_BY_SCORE_ASCEND ||
	               group->attribute->sort_article_type == SORT_ARTICLES_BY_LINES_ASCEND);

#ifdef DEBUG
	if (debug & DEBUG_REFS) {
		char file[PATH_LEN];

		joinpath(file, sizeof(file), TMPDIR, "REFS.dump");
		dbgfd = fopen(file, "w");
		SETVBUF(dbgfd, NULL, _IONBF, 0);
		fprintf(dbgfd, "MSGID phase\n");
	}
#endif /* DEBUG */

	/*
	 * Add the Message-ID headers to the cache, using the last Reference
	 * as the parent
	 */
	for_each_art(i) {
		art = &arts[i];

		if (art->refs) {
			strip_line(art->refs);

			/*
			 * Add the last ref, and then trim it to save wasting time adding
			 * it again later
			 * Check for circular references to current article
			 *
			 * TODO: do this in a single pass
			 */
			if ((s = strrchr(art->refs, '<')) != NULL) {
				if (!strcmp(art->msgid, s)) {
					/*
					 * Remove circular reference to current article
					 */
#ifdef DEBUG
					if (debug & DEBUG_REFS)
						DEBUG_PRINT((dbgfd, "removing circular reference to: %s\n", s));
#endif /* DEBUG */
					*s = '\0';
				}
			}
			if (s != NULL) {
				if (valid_msgid(art->msgid))
					art->refptr = add_msgid(MSGID_REF, art->msgid, add_msgid(REF_REF, s, NULL));
				*s = '\0';
			} else {
				if (valid_msgid(art->msgid))
					art->refptr = add_msgid(MSGID_REF, art->msgid, add_msgid(REF_REF, art->refs, NULL));
				FreeAndNull(art->refs);
			}
		} else
			if (valid_msgid(art->msgid))
				art->refptr = add_msgid(MSGID_REF, art->msgid, NULL);
		FreeAndNull(art->msgid);	/* Now cached - discard this */
	}

#ifdef DEBUG
	if (debug & DEBUG_REFS)
		DEBUG_PRINT((dbgfd, "REFS phase\n"));
#endif /* DEBUG */
	/*
	 * Add the References data to the cache
	 */
	for_each_art(i) {
		if (!arts[i].refs)						/* No refs - skip */
			continue;

		art = &arts[i];

		/*
		 * Add the remaining references as parent to the last ref we added
		 * earlier. skip call to add_msgid when NULL pointer
		 */

		refs = parse_references(art->refs);

		if (art->refptr && art->refptr->parent && valid_msgid(art->refptr->parent->txt))
			add_msgid(REF_REF, art->refptr->parent->txt, refs);

		FreeAndNull(art->refs);
	}

#ifdef DEBUG
	if (debug & DEBUG_REFS)
		fclose(dbgfd);
#endif /* DEBUG */
}
