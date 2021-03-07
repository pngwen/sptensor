/*
    This is an implementation of a hash search which operates on vectors.
    It will either find the location of the item or it will find where it
    should be.
    Copyright (C) 2018  Robert Lowe <pngwen@acm.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */
#ifndef HASH_H
#define HASH_H
#include <sptensor/inzt.h>
#include <sptensor/sptensor.h>
#include <sptensor/vector.h>
#include <gmp.h>

#define NBUCKETS 128

/* Each hash table item has a key and value) */
typedef struct hash_item {
    mpz_t key;
    mpf_t value;
	
} hash_item;

/* hash storage type */
typedef struct sptensor_hash
{
    /* sptensor fields */
    int modes;
    sptensor_index_t *dim;
    sptensor_get_f get;
    sptensor_set_f set;
    sptensor_iterator_f iterator;
    sptensor_iterator_f nz_iterator;
    sptensor_free_f free;

    /* hash specific fields */
    sptensor_vector* hashtable;
	unsigned int nbuckets;
	unsigned int curr_size; /*this will grow as items are added to the hash table */
} sptensor_hash_t;


/* sptensor hash allocation functions */
sptensor_hash_t* sptensor_hash_alloc(sptensor_index_t *modes, int nmodes);
void sptensor_hash_free(sptensor_hash_t* t);

/* Hash element access functions */

/* Search the tensor for an index. Return the element number, -1 on failure 
	r - to hold index value upon return 
*/
void sptensor_hash_search(sptensor_hash_t *t, sptensor_index_t *idx, mpz_t v);

/* Function to insert an element in the hash table.
	r - to hold index value upon return (-1 if not found or table is full)
	v - value to insert 
*/
void sptensor_hash_set(sptensor_hash_t *t, sptensor_index_t *i, mpz_t r, mpf_t v);

/* If the hashtable reaches capicity, double the size and rehash all the existing items */
void sptensor_hash_rehash(sptensor_hash_t *t);


/* Iterator Functions */
sptensor_iterator_t* sptensor_hash_iterator(sptensor_t *t);
sptensor_iterator_t* sptensor_hash_nz_iterator(sptensor_t *t);

#endif