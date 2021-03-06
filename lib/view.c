/*
    This is a collection of functions for dealing with tensor views.  
    Copyright (C) 2018 Robert Lowe <pngwen@acm.org>

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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sptensor/view.h>

/********************************
 * generic tensor view functions 
 ********************************/

/* write coordinate list to file */
void
tensor_write(FILE *file, tensor_view *v)
{
    sp_index_t *idx;
    int i,j;
    int nnz;
    
    /* get the preliminary info */
    nnz = TVNNZ(v);
    idx=TVIDX_ALLOC(v);

    /* print the header */
    fprintf(file, "%u", v->nmodes);
    for(j=0; j<v->nmodes; j++) {
	fprintf(file, "\t%u", v->dim[j]);
    }
    fprintf(file, "\n");

    /* go through the list, printing the tensor */
    for(i=0; i<nnz; i++) {
	TVIDX(v, i, idx);
	
	/* print the index */
	for(j=0; j<v->nmodes; j++) {
	    fprintf(file, "%d\t", idx[j]);
	}

	/* print the value */
	fprintf(file, "%g\n", TVGET(v,idx));
    }

    free(idx);
}

/* coordinate list print */
void
tensor_clprint(tensor_view *v)
{
    tensor_write(stdout, v);
}

/* 
 * Find the maximum number of digits to the left 
 * of the decimal place in the view (including -)
 */
static unsigned int
tensor_view_max_digits(tensor_view* v)
{
    double maxVal=0.0;
    double val, absval;
    int i;
    unsigned add = 0;
    unsigned int nnz;

    /* find the biggest absolute value */
    nnz = TVNNZ(v);
    for(i=0; i<nnz; i++) {
	val = TVGETI(v, i);
	absval = fabs(val);
	if( absval > maxVal) {
	    if(val < 0) {
		add = 1;
	    } else {
		add = 0;
	    }
	    maxVal = val;
	}
    }

    return (unsigned int) ceil(log10(maxVal))+add;
}


/* pretty print */
void
tensor_print(tensor_view *v, unsigned int precision)
{
    tensor_fprint(stdout, v, precision);
}


void
tensor_fprint(FILE *file, tensor_view *v, unsigned int precision)
{
    unsigned int width;
    int ui;  /* index being updated */
    sp_index_t *idx = TVIDX_ALLOC(v);
    short unsigned int done = 0;

    /* get the field width */
    width = tensor_view_max_digits(v) + 1;
    if(precision) {
	width += 1 + precision; /* allow for . and fractional part */
    }

    /* start at index 1...1 */
    for(ui=0; ui<v->nmodes; ui++) {
	idx[ui]=1;
    }

    /* iterate over all of the indexes and print in a row/slice 
     * major way
     */
    while(!done) {
	/* print the value */
	fprintf(file, "  %*.*lf", width, precision,TVGET(v, idx));

	/* detect if we are done */
	if(sptensor_indexcmp(v->nmodes, idx, v->dim) == 0) {
	    done = 1;
	}

	/* update the index */
	if(v->nmodes >= 2) {
	    ui = 1; /* columns first! */
	} else {
	    ui = 0;
	}

	/* look for the next index to increment */
	while(ui < v->nmodes && idx[ui] == v->dim[ui]) {
	    /* rollover! */
	    idx[ui] = 1;

	    if(ui == 0) {
		ui = 2; 
	    } else if(ui == 1) {
		ui = 0;
	    } else {
		ui++;
	    }
	}

	if(ui >= v->nmodes) {
	    ui = 0;
	}
	
	/* increment */
	idx[ui]++;

	/* terminate rows */
	if(ui == 0) {
	    fprintf(file, "\n");
	} else if(ui != 1) {
	    fprintf(file, "\n\n"); /* blank line separator */
	}
    }

    free(idx);
}


/* create an sptensor copy of a tensor view */
sptensor *
tensor_view_sptensor(tensor_view *v)
{
    sptensor *t;
    sp_index_t *idx;
    int i;
    unsigned int nnz;

    /* allocate things */
    idx = TVIDX_ALLOC(v);
    t = sptensor_alloc(v->nmodes, v->dim);
    nnz = TVNNZ(v);

    /* copy elements */
    for(i=0; i<nnz; i++) {
	TVIDX(v, i, idx);
	sptensor_set(t, idx, TVGET(v, idx));
    }

    /* cleanup and return */
    free(idx);
    return t;
}


/* 
 * For most views, these generic functions will do.
 */
static unsigned int
base_view_nnz(tensor_view *v)
{
    /* The assumption is that our NNZ matches that of what we contain */
    return TVNNZ((tensor_view*)v->tns);
}


static void
base_view_get_idx(tensor_view *v, unsigned int i, sp_index_t *idx)
{
    sp_index_t *fidx;  /* idx of our contained view */

    /* get the internal view and the ith index thereof */
    fidx = TVIDX_ALLOC(v->tns);
    TVIDX(v->tns, i, fidx);

    /* convert and copy */
    TVFROM(v, fidx, idx);

    /* cleanup! */
    free(fidx);
}


static double
base_view_geti(tensor_view *v, unsigned int i)
{
    /* just pass it along! */
    return TVGETI(v->tns, i);
}


static double
base_view_get(tensor_view *v, sp_index_t *idx)
{
    sp_index_t *tidx;  /* idx of our contained view */
    double result;

    /* convert the index */
    tidx = TVIDX_ALLOC(v->tns);
    TVTO(v, idx, tidx);

    /* retrieve the result */
    result = TVGET(v->tns, tidx);

    /* cleanup and return */
    free(tidx);
    return result;
}


static void
base_view_set(tensor_view *v, sp_index_t *idx, double value)
{
    sp_index_t *tidx;  /* idx of our contained view */
    double result;

    /* convert the index */
    tidx = TVIDX_ALLOC(v->tns);
    TVTO(v, idx, tidx);

    /* set the value */
    TVSET(v->tns, tidx, value);

    /* cleanup */
    free(tidx);
}


static void
base_view_free(tensor_view *v)
{
    /* generally, just deallocating our internal object 
       and then ourselves will do nicely. */
    if(v->tns) {
	free(v->tns);
    }
    if(v->data) {
	free(v->data);
    }
    free(v);
}


static tensor_view *
base_view_alloc()
{
    tensor_view *v;

    /* basic allocation with base pointers to functions */
    v = malloc(sizeof(tensor_view));
    v->tns = NULL;
    v->data = NULL;
    v->dim = NULL;
    v->nmodes = 0;
    v->nnz = base_view_nnz;
    v->get_idx = base_view_get_idx;
    v->geti = base_view_geti;
    v->get = base_view_get;
    v->set = base_view_set;
    v->to = NULL;
    v->from = NULL;
    v->tvfree = base_view_free;
    return v;
}


/********************************************************
 * sptensor view - A simple wrapper for sptensor objects
 ********************************************************/
/* Static functions for sptensor*/
static unsigned int
sptensor_view_nnz(tensor_view *v)
{
    /* just return the number of entries in the tensor */
    return ((sptensor*)v->data)->ar->size;
}


static void
sptensor_view_get_idx(tensor_view *v, unsigned int i, sp_index_t *idx)
{
    sptensor *tns = (sptensor*) v->data;
    
    /* no translation, just copy */
    memcpy(idx, VPTR(tns->idx, i), tns->idx->element_size);
}


static double
sptensor_view_geti(tensor_view *v, unsigned int i)
{
    sptensor *tns = (sptensor*) v->data;

    return VVAL(double, tns->ar, i);
}


static double
sptensor_view_get(tensor_view *v, sp_index_t *idx)
{
    /* passthru */
    return sptensor_get((sptensor*)v->data, idx);
}


static void
sptensor_view_set(tensor_view *v, sp_index_t *idx, double value)
{
    /* passthru */
    sptensor_set((sptensor*)v->data, idx, value);
}


static void
sptensor_view_idxcpy(tensor_view *v, sp_index_t *in, sp_index_t *out)
{
    /* just copy */
    memcpy(out, in, ((sptensor*)v->data)->idx->element_size);
}


static void
sptensor_view_free(tensor_view *v)
{
    /* simply free the tensor wrapper */
    free(v);
}


/* allocate an sptensor view */
tensor_view *
sptensor_view(sptensor *tns)
{
    tensor_view *v;  /* the allocated view */

    /* allocate and populate the view */
    v = base_view_alloc();
    v->data = tns;
    v->dim = tns->dim;
    v->nmodes = tns->nmodes;
    v->nnz = sptensor_view_nnz;
    v->get_idx = sptensor_view_get_idx;
    v->geti = sptensor_view_geti;
    v->get = sptensor_view_get;
    v->set = sptensor_view_set;
    v->to = sptensor_view_idxcpy;
    v->from = sptensor_view_idxcpy;
    v->tvfree = sptensor_view_free;
    return v;
}


/* allocate an sptensor view and create a new sptensor to fill it.
   This uses the base free, which will deallocate the underlying sptensor
   when the view is freed.

   nmodes - The number of modes
   dim    - The dimensions of the tensor
*/
tensor_view *
tensor_alloc(int nmodes, sp_index_t *dim)
{
    tensor_view *result;
    
    /* allocate the tensor and view */
    result = sptensor_view(sptensor_alloc(nmodes, dim));

    /* set up our free method */
    result->tvfree = base_view_free;

    return result;
}


/* Copy a tensor view as a newly allocated tensor with underlying sptensor */
tensor_view *tensor_view_deep_copy(tensor_view *t)
{
    tensor_view *result;
    int i;
    unsigned int nnz;
    sp_index_t *idx;

    /* allocate the new tensor and index */
    result = tensor_alloc(t->nmodes, t->dim);
    idx = malloc(sizeof(sp_index_t) * t->nmodes);

    /* copy the tensor's non-zero elements */
    nnz = TVNNZ(t);
    for(i=0; i<nnz; i++) {
	TVIDX(t, i, idx);
	TVSET(result, idx, TVGETI(t, i));
    }

    /* cleanup an return */
    free(idx);
    return result;
}


/***************************************
 * Dense Tensor Representation/View
 ***************************************/
struct dense_tensor {
    sp_index_t *mul;
    unsigned int totalCount;
    double *elem;
};


static unsigned int
dense_tensor_nnz(tensor_view *v)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;
    unsigned int nnz=0;
    int i;

    for(i=0; i<dtns->totalCount; i++) {
	if(dtns->elem[i] != 0.0) {
	    nnz++;
	}
    }

    return nnz;
}


static void
dense_tensor_idx(tensor_view *v, unsigned int i, sp_index_t *idx)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;
    int ui;
    int j;
    int first = 1;

    /* find the ith non-zero element */
    for(j=0; i>=0 && j<dtns->totalCount; j++) {
	if(dtns->elem[j] != 0) {
	    if(first) {
		first = 0;
		if(i == 0) break;
	    } else {
		i--;
	    }
	}
    }

    /* translate the index */
    for(ui=0; ui<v->nmodes; ui++){
	idx[ui] = j/dtns->mul[ui]+1;
	j %= dtns->mul[ui];
    }
}


static double
dense_tensor_geti(tensor_view *v, unsigned int i)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;
    int j;
    int first = 1;

    /* find the ith non-zero element */
    for(j=0; i>=0 && j<dtns->totalCount; j++) {
	if(dtns->elem[j] != 0) {
	    if(first) {
		first = 0;
		if(i == 0) break;
	    } else {
		i--;
	    }
	}
    }

    return dtns->elem[j];
}


static unsigned int
dense_tensor_compute_index(tensor_view *v, sp_index_t *idx)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;
    int j=0;
    int ui;

    for(ui=0; ui<v->nmodes; ui++) {
	j += (idx[ui]-1) * dtns->mul[ui];
    }

    return j;
}


static double
dense_tensor_get(tensor_view *v, sp_index_t *idx)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;

    return dtns->elem[dense_tensor_compute_index(v, idx)];
}


static void
dense_tensor_set(tensor_view *v, sp_index_t *idx, double val)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;

    dtns->elem[dense_tensor_compute_index(v, idx)] = val;
}


static void
dense_tensor_idxcpy(tensor_view *v, sp_index_t *in, sp_index_t *out)
{
    memcpy(out, in, v->nmodes * sizeof(sp_index_t));
}


static void
dense_tensor_free(tensor_view *v)
{
    struct dense_tensor *dtns = (struct dense_tensor *) v->data;

    free(dtns->mul);
    free(dtns->elem);
    free(v->data);
    free(v->dim);
    free(v);
}


/* Create a dense tensor view (useful for smaller tensors) */
tensor_view *
dense_tensor_alloc(int nmodes, sp_index_t *dim)
{
    tensor_view *v; /* the allocated view */
    struct dense_tensor *dtns;
    int i;
    

    /* allocate and poulate the view */
    v = base_view_alloc();
    v->data = malloc(sizeof(struct dense_tensor));
    v->dim = malloc(sizeof(sp_index_t) * nmodes);
    memcpy(v->dim, dim, sizeof(sp_index_t) * nmodes);
    v->nmodes = nmodes;
    v->nnz = dense_tensor_nnz;
    v->get_idx = dense_tensor_idx;
    v->geti = dense_tensor_geti;
    v->get = dense_tensor_get;
    v->set =dense_tensor_set;
    v->to = dense_tensor_idxcpy;
    v->from = dense_tensor_idxcpy;
    v->tvfree  = dense_tensor_free;

    /* allocate and set up the dense tensor elements */
    dtns = (struct dense_tensor *)v->data;
    dtns->mul = malloc(sizeof(sp_index_t) * nmodes);
    dtns->mul[nmodes-1] = 1;
    dtns->totalCount = v->dim[nmodes-1];
    for(i=nmodes-2; i>=0; i--) {
	dtns->totalCount *= v->dim[i];
	dtns->mul[i] = dtns->mul[i+1]*v->dim[i];
    }
    dtns->elem = calloc(dtns->totalCount, sizeof(double));
    return v;
}



/***************************************
 * Identity Tensor
 ***************************************/
static unsigned int
identity_nnz(tensor_view *v)
{
    int min=v->dim[0];
    int i;

    /* find the minimum mode */
    for(i=0; i<v->nmodes; i++) {
	if(v->dim[i] < min) {
	    min = v->dim[i];
	}
    }

    /* The smallest mode is the number of elements on diagonal */
    return min;
}



static void
identity_get_idx(tensor_view *v, unsigned int i, sp_index_t *idx)
{
    int j;

    /* just repeat i+1 nmodes times */
    for(j=0; j<v->nmodes; j++) {
	idx[j] = i+1;
    }
}


static double
identity_get(tensor_view *v, sp_index_t *idx)
{
    int i;

    /* if they don't match, return 0.0 */
    for(i=1; i<v->nmodes; i++) {
	if(idx[i] != idx[0]) {
	    return 0.0;
	}
    }

    /* looks like they all match! */
    return 1.0;
}


static void
identity_tensor_free(tensor_view *v)
{
    free(v->dim);
    free(v);
}


static double
identity_tensor_geti(tensor_view *v, unsigned int i)
{
    return 1.0;
}


tensor_view *
identity_tensor(int nmodes, sp_index_t *dim)
{
    tensor_view *v;

    v = base_view_alloc();
    v->nmodes = nmodes;
    v->dim = TVIDX_ALLOC(v);
    memcpy(v->dim, dim, sizeof(sp_index_t) * nmodes);
    v->nnz = identity_nnz;
    v->get_idx = identity_get_idx;
    v->geti = identity_tensor_geti;
    v->get = identity_get;
    v->set = 0x00;  /* identity tensors are immutable */
    v->to = sptensor_view_idxcpy; /* just copy */
    v->from = sptensor_view_idxcpy;
    v->tvfree = identity_tensor_free;
    
    return v;
}



/***************************************
 * Unfoleded Tensor View
 ***************************************/
struct unfold_view
{
    int n;             /* Dimension we unfold along */
    sp_index_t *jk;    /* jk coeffecients for unfolding */
    sp_index_t dim[2]; /* This is always 2 mode! */
};


static void
unfold_to(tensor_view *v, sp_index_t *in, sp_index_t *out)
{
    struct unfold_view *uv = (struct unfold_view*)v->data;
    int i;
    int j;
    int k=v->tns->nmodes-2;

    j=in[1]-1;
    for(i=v->tns->nmodes-1; i>=0; i--) {
        if(i == uv->n) {
            /* handle the folded mode */
            out[i] = in[0];
        } else {
            /* handle the rest */
            out[i] = j/uv->jk[k]+1;
            j = j% uv->jk[k];
            k--;
        }
    }
}


static void
unfold_from(tensor_view *v, sp_index_t *in, sp_index_t *out)
{
    struct unfold_view *uv = (struct unfold_view*)v->data;
    int i;
    int k=0;

    /* get the i part */
    out[0] = in[uv->n];

    /* get the j part */
    out[1] = 1;
    for(i=0; i<v->tns->nmodes; i++) {
	/* skip the folded dimension */
	if(i == uv->n) {
	    continue;
	}
	out[1] += (in[i] - 1) * uv->jk[k++];
    }
}


static void
unfold_free(tensor_view *v)
{
    struct unfold_view *uv;
    
    uv = (struct unfold_view *) v->data;
    free(uv->jk);
    free(uv);
    free(v);
}


/* Unfold a tensor along dimension n */
tensor_view *
unfold_tensor(tensor_view *v, sp_index_t n)
{
    tensor_view *tv;
    struct unfold_view *uv;
    int i,k;

    /* allocate and populate the tensor view */
    uv = malloc(sizeof(struct unfold_view));
    uv->n = n;
    tv = base_view_alloc();
    tv->tns = v;
    tv->data = uv;
    tv->nmodes = 2;
    tv->dim = uv->dim;
    tv->to = unfold_to;
    tv->from = unfold_from;
    tv->tvfree = unfold_free;

    /* compute the dimensions and jk coeffecients */
    tv->dim[0] = v->dim[n];
    tv->dim[1] = 1;
    uv->jk = malloc(sizeof(sp_index_t) * (v->nmodes - 1));
    uv->jk[0]=1;
    k=1;
    for(i=0; i<v->nmodes; i++) {
        /* skip the unfold dimension */
        if(i == n) continue;

        /* multiply the dimension */
        tv->dim[1] *= v->dim[i];

        /* compute jk */
        uv->jk[k] = uv->jk[k-1] * v->dim[i];
        k++;

    }

    return tv;
}



/***************************************
 * Tensor Slice View
 ***************************************/

/*
 * Allocate a tensor slice spec for the given view.
 */
tensor_slice_spec *
tensor_slice_spec_alloc(tensor_view *v)
{
    tensor_slice_spec *spec;
    int i;
    
    /* allocate the main part of the spec */
    spec = malloc(sizeof(tensor_slice_spec));

    /* allocate all the details in the spec */
    spec->fixed = malloc(sizeof(sp_index_t)*v->nmodes);
    spec->begin = malloc(sizeof(sp_index_t)*v->nmodes);
    spec->end = malloc(sizeof(sp_index_t)*v->nmodes);

    /* The default spec is for the entire tensor */
    for(i=0; i<v->nmodes; i++) {
        spec->fixed[i] = 0;
        spec->begin[i] = 1;
        spec->end[i] = v->dim[i];
    }
    
    return spec;
}


/*
 * Free a tensor slice spec
 */
void
tensor_slice_spec_free(tensor_slice_spec *spec)
{
    /* free all the details */
    free(spec->fixed);
    free(spec->begin);
    free(spec->end);

    /* free the spec itself */
    free(spec);
}


static unsigned short int
tensor_slice_index_within(tensor_view *v, sp_index_t *idx)
{
    tensor_slice_spec *spec = (tensor_slice_spec*) v->data;
    int i;

    for(i=0; i<TVNMODES(v->tns); i++) {
        if(spec->fixed[i] && idx[i] != spec->fixed[i]) {
            return 0;
        } else if(idx[i] < spec->begin[i] || idx[i] > spec->end[i]) {
            return 0;
        }
    }

    return 1;
}


static unsigned int
tensor_slice_nnz(tensor_view *v)
{
    tensor_slice_spec *spec = (tensor_slice_spec*) v->data;
    unsigned int count = 0;
    unsigned int n, i;
    sp_index_t *idx;

    /* initailize some things */
    n = TVNNZ(v->tns);
    idx = malloc(sizeof(sp_index_t) * TVNMODES(v->tns));

    /* count all the nz indexes that are within the bounds of this slice. */
    for(i=0; i<n; i++) {
        TVIDX(v->tns, i, idx);
        if(tensor_slice_index_within(v, idx)) {
            count++;
        }
    }

    /* return the number of nnz items */
    free(idx);
    return count;
}


static void
tensor_slice_idx(tensor_view *v, unsigned int i, sp_index_t *idx)
{
    sp_index_t *fidx;
    unsigned int n, j;

    /* some initializers */
    fidx = malloc(sizeof(sp_index_t) * TVNMODES(v->tns));
    n = TVNNZ(v->tns);

    /* find the ith nnz index within the slice */
    for(j=0; j<n; j++) {
        TVIDX(v->tns, j, fidx);
        if(tensor_slice_index_within(v, fidx)) {
            if(i==0) {
                TVFROM(v, fidx, idx);
                break;
            } else {
                i--;
            }
        }
    }

    /* cleanup */
    free(fidx);
}


static void
tensor_slice_to(tensor_view *v, sp_index_t *in, sp_index_t *out)
{
    tensor_slice_spec *spec = (tensor_slice_spec*) v->data;
    int i;
    int n;

    /* initialize */
    n = TVNMODES(v->tns);

    /* convert each mode of the output */
    for(i=0; i<n; i++) {
        if(spec->fixed[i]) {
            out[i] = spec->fixed[i];
        } else {
            out[i] = *(in++) - 1 + spec->begin[i];
        }
    }
}


static void
tensor_slice_from(tensor_view *v, sp_index_t *in, sp_index_t *out)
{
    tensor_slice_spec *spec = (tensor_slice_spec*) v->data;
    int i;
    int n;

    /* initialize */
    n = TVNMODES(v->tns);

    /* go through each of the input modes */
    for(i=0; i<n; i++) {
        /* skip the fixed indexes */
        if(spec->fixed[i]) {
            continue;
        }

        /* convert the non-fixed indexes */
        *(out++) = in[i] - spec->begin[i] + 1;
    }
}


static void
tensor_slice_free(tensor_view *v)
{
    tensor_slice_spec_free((tensor_slice_spec*)v->data);
    free(v->dim);
}


/* A slice of a tensor 
 *   v    - The view to slice
 *   spec - The slice spec
 */
tensor_view *
tensor_slice(tensor_view *v, tensor_slice_spec *spec)
{
    tensor_view *sv;
    tensor_slice_spec *tvspec;
    int i, j;

    /* set up the basics */
    sv = base_view_alloc();
    sv->tns = v;

    /* copy the spec */
    tvspec = tensor_slice_spec_alloc(v);
    sv->data = tvspec;
    memcpy(tvspec->fixed, spec->fixed, sizeof(sp_index_t)*v->nmodes);
    memcpy(tvspec->begin, spec->begin, sizeof(sp_index_t)*v->nmodes);
    memcpy(tvspec->end, spec->end, sizeof(sp_index_t)*v->nmodes);

    /* count the number of modes */
    sv->nmodes = v->nmodes;
    for(i=0; i<v->nmodes; i++) {
        if(tvspec->fixed[i]) {
            sv->nmodes--;
	}
    }

    /* compute the dimensions */
    sv->dim = malloc(sizeof(sp_index_t)*sv->nmodes);
    j = 0;
    for(i=0; i<v->nmodes; i++) {
        /* skip fixed dimensions */
        if(tvspec->fixed[i]) {
            continue;
        }

        /* compute the others */
        sv->dim[j++] = tvspec->end[i] - tvspec->begin[i] + 1;
    }

    /* wire up the functions */
    sv->nnz = tensor_slice_nnz;
    sv->get_idx = tensor_slice_idx;
    sv->to = tensor_slice_to;
    sv->from = tensor_slice_from;
    sv->tvfree = tensor_slice_free;

    return sv;
}



/**********************************
 * tensor_transpose 
 **********************************/
static void
tensor_transpose_index(tensor_view *v, sp_index_t* in, sp_index_t *out)
{
    unsigned int *t = (unsigned int *)(v->data);
    /* copy the index */
    memmove(out, in, sizeof(sp_index_t) * v->nmodes);

    /* transpose */
    out[t[0]] = in[t[1]];
    out[t[1]] = in[t[0]];
}


static void
tensor_transpose_free(tensor_view *v)
{
    free(v->dim);
    free(v->data);
    free(v);
}


/* A view with two indices transposed
 *   v - The view to transpose
 *   i - The first mode to swap
 *   j - The second mode to swap
 */
tensor_view *tensor_transpose(tensor_view *v, unsigned int i, unsigned int j)
{
    tensor_view *tv;

    tv = base_view_alloc();
    tv->to = tensor_transpose_index;
    tv->from = tensor_transpose_index;
    tv->tvfree = tensor_transpose_free;

    /* set up the transposition data */
    tv->data = malloc(sizeof(unsigned int) * 2);
    ((unsigned int*)(tv->data))[0] = i;
    ((unsigned int*)(tv->data))[1] = j;

    /* set up the tensor and dimensions */
    tv->tns = v;
    tv->nmodes = v->nmodes;
    tv->dim = malloc(sizeof(sp_index_t) * v->nmodes);
    tensor_transpose_index(tv, v->dim, tv->dim);

    return tv;
}
