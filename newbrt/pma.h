#ifndef PMA_H
#define PMA_H

#ident "Copyright (c) 2007 Tokutek Inc.  All rights reserved."

#include "brttypes.h"
#include "ybt.h"
#include "yerror.h"
#include "../include/db.h"
#include "log.h"

/* An in-memory Packed Memory Array dictionary. */
/* There is a built-in-cursor. */

/* All functions return 0 on success. */

typedef struct pma *PMA;
typedef struct pma_cursor *PMA_CURSOR;

/* compare 2 DBT's. return a value < 0, = 0, > 0 if a < b, a == b, a > b respectively */
typedef int (*pma_compare_fun_t)(DB *, const DBT *a, const DBT *b);

int toku_pma_create(PMA *, pma_compare_fun_t compare_fun, DB *, FILENUM filenum, int maxsize);

int toku_pma_set_compare(PMA pma, pma_compare_fun_t compare_fun);

/* set the duplicate mode 
   0 -> no duplications, TOKU_DB_DUP, TOKU_DB_DUPSORT */
int toku_pma_set_dup_mode(PMA pma, int mode);

/* set the duplicate compare function */
int toku_pma_set_dup_compare(PMA pma, pma_compare_fun_t dup_compare_fun);

/* verify the integrity of a pma */
void toku_pma_verify(PMA pma);

/* returns 0 if OK.
 * You must have freed all the cursors, otherwise returns nonzero and does nothing. */
int toku_pma_free (PMA *);

int  toku_pma_n_entries (PMA);

/* Returns an error if the key is already present. */
/* The values returned should not be modified.by the caller. */
/* Any cursors should be updated. */
/* Duplicates the key and keylen. */
//enum pma_errors toku_pma_insert (PMA, bytevec key, ITEMLEN keylen, bytevec data, ITEMLEN datalen);

// The DB pointer is there so that the comparison function can be called.
enum pma_errors toku_pma_insert (PMA, DBT*, DBT*, TOKUTXN, FILENUM, DISKOFF, u_int32_t /*random for fingerprint */, u_int32_t */*fingerprint*/);

/* This returns an error if the key is NOT present. */
int pma_replace (PMA, bytevec key, ITEMLEN keylen, bytevec data, ITEMLEN datalen);

/* delete pairs from the pma
   if val is 0 then delete all pairs from the pma that match the key
   if val is not 0 then only delete the pair that matches both the key and the val */
 
int toku_pma_delete (PMA, DBT */*key*/, DBT */*val*/, u_int32_t /*random for fingerprint*/, u_int32_t */*fingerprint*/, u_int32_t *deleted_size);

int toku_pma_insert_or_replace (PMA /*pma*/, DBT */*k*/, DBT */*v*/,
				int */*replaced_v_size*/, /* If it is a replacement, set to the size of the old value, otherwise set to -1. */
				TOKUTXN /*txn*/, FILENUM, DISKOFF,
				u_int32_t /*random for fingerprint*/, u_int32_t */*fingerprint*/);


/* Exposes internals of the PMA by returning a pointer to the guts.
 * Don't modify the returned data.  Don't free it. */
enum pma_errors toku_pma_lookup (PMA, DBT*, DBT*);

/*
 * The kv pairs in PMA are split into two (nearly) equal sized sets.
 * THe ones in the left half are left in PMA, the ones in the right half are put into NEWPMA.
 * The size is determined by the sum of the sizes of the keys and values. 
 * The NEWPMA must be empty.
 *
 * DISKOFF  - the disk offset of the node containing the PMA to be split.  (Needed for logging)
 * PMA      - the pma to be split.
 * PMA_SIZE - a variable containing the size of the disk image of the PMA.
 * RAND4SUM - the random number for fingerprinting
 * FINGERPRINT - the current fingerprint of the PMA.
 *
 * NEWDISKOFF, NEWPMA, NEWPMASIZE, NEWRAND4SUM, NEWFINGERPRINT -  The same information fo the pma to hold the stuff to be moved out of PMA.
 *
 * SPLITK  filled in with the resulting pivot key.
 *   The original PMA gets keys <= pivot key
 *   The NEWPMA gets keys > pivot key
 */
int toku_pma_split(TOKUTXN, FILENUM,
		   DISKOFF /*diskoff*/,    PMA /*pma*/,     unsigned int */*pma_size*/,     u_int32_t /*rand4sum*/,  u_int32_t */*fingerprint*/,
		   DBT */*splitk*/,
		   DISKOFF /*newdiskoff*/, PMA /*newpma*/,  unsigned int */*newpma_size*/,  u_int32_t /*newrand4sum*/,  u_int32_t */*newfingerprint*/);

/*
 * Insert several key value pairs into an empty pma.  
 * Doesn't delete any existing keys (even if they are duplicates)
 * Requires: The keys are sorted
 *
 * pma - the pma that the key value pairs will be inserted into.
 *      must be empty with no cursors.
 * keys - an array of keys
 * vals - an array of values
 * n_newpairs - the number of key value pairs
 */
int toku_pma_bulk_insert(TOKUTXN, FILENUM, DISKOFF, PMA pma, DBT *keys, DBT *vals, int n_newpairs, u_int32_t rand4sem, u_int32_t *fingerprint);

/* Move the cursor to the beginning or the end or to a key */
int toku_pma_cursor (PMA, PMA_CURSOR *, void** /*sskey*/, void ** /*ssval*/); // the sskey and ssval point to variables that hold blocks that can be used to return values for zero'd DBTS.
int toku_pma_cursor_free (PMA_CURSOR*);

/* get the pma that a pma cursor is bound to */
int toku_pma_cursor_get_pma(PMA_CURSOR c, PMA *pma);
int toku_pma_cursor_set_position_last (PMA_CURSOR c);
int toku_pma_cursor_set_position_first (PMA_CURSOR c);
int toku_pma_cursor_set_position_next (PMA_CURSOR c); /* Requires the cursor is init'd.  Returns DB_NOTFOUND if we fall off the end. */
int toku_pma_cursor_set_position_prev (PMA_CURSOR c);

/* get the key and data under the cursor 
   if even_deleted is 1 then the key and val under the cursor are returned even if 
   it has been deleted previously */
int toku_pma_cursor_get_current(PMA_CURSOR c, DBT *key, DBT *val, int even_deleted);

/* move the cursor to the kv pair matching the key and the val if nonzero*/
int toku_pma_cursor_set_both(PMA_CURSOR c, DBT *key, DBT *val);

/* set the cursor to the smallest key in the pma >= key and the val if nonzero*/
int toku_pma_cursor_set_range_both(PMA_CURSOR c, DBT *key, DBT *val);

/* delete the key value pair under the cursor, return the size of the pair */
int toku_pma_cursor_delete_under(PMA_CURSOR /*c*/, int */*kvsize*/, u_int32_t /*rand4sem*/, u_int32_t */*fingerprint*/);

int toku_pma_random_pick(PMA, bytevec *key, ITEMLEN *keylen, bytevec *data, ITEMLEN *datalen);

unsigned int toku_pma_index_limit(PMA); // How many slots are in the PMA right now?
int toku_pmanode_valid(PMA, unsigned int);
bytevec toku_pmanode_key(PMA, unsigned int);
ITEMLEN toku_pmanode_keylen(PMA, unsigned int);
bytevec toku_pmanode_val(PMA, unsigned int);
ITEMLEN toku_pmanode_vallen(PMA, unsigned int);

void toku_pma_iterate (PMA, void(*)(bytevec,ITEMLEN,bytevec,ITEMLEN, void*), void*);

#define PMA_ITERATE_IDX(table,idx,keyvar,keylenvar,datavar,datalenvar,body) ({ \
  unsigned int idx;							\
  for (idx=0; idx<toku_pma_index_limit(table); idx++) {		       \
    if (toku_pmanode_valid(table,idx)) {                                    \
      bytevec keyvar = toku_pmanode_key(table,idx);                         \
      ITEMLEN keylenvar = toku_pmanode_keylen(table,idx);                   \
      bytevec datavar = toku_pmanode_val(table, idx);                       \
      ITEMLEN datalenvar = toku_pmanode_vallen(table, idx);                 \
      body;                                                            \
    } } })

#define PMA_ITERATE(table,keyvar,keylenvar,datavar,datalenvar,body) PMA_ITERATE_IDX(table, __i, keyvar, keylenvar, datavar, datalenvar, body)

void toku_pma_verify_fingerprint (PMA pma, u_int32_t rand4fingerprint, u_int32_t fingerprint);

// Set the PMA's size, without moving anything.
int toku_resize_pma_exactly (PMA pma, int oldsize, int newsize);

int toku_pma_set_at_index (PMA, unsigned int /*index*/, DBT */*key*/, DBT */*value*/); // If the index is wrong or there is a value already, return nonzero
int toku_pma_clear_at_index (PMA, unsigned int /*index*/); // If the index is wrong or there is a value already, return nonzero


// Requires:  No open cursors on the pma.
// Return nonzero if the indices are somehow wrong.
int toku_pma_move_indices (PMA pma_from, PMA pma_to, INTPAIRARRAY fromto,
			   u_int32_t rand_from, u_int32_t *fingerprint_from,
			   u_int32_t rand_to,   u_int32_t *fingerprint_to,
			   u_int32_t *n_in_buf_from, u_int32_t *n_in_buf_to);
// Move things backwards according to fromto.
int toku_pma_move_indices_back (PMA pma_backto, PMA pma_backfrom, INTPAIRARRAY fromto,
				u_int32_t rand_backto,   u_int32_t *fingerprint_backto,
				u_int32_t rand_backfrom, u_int32_t *fingerprint_backfrom,
				u_int32_t *n_in_buf_backto, u_int32_t *n_in_buf_backfrom
				);

void toku_pma_show_stats (void);


#endif
