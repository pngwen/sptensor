
#include <stdio.h>
#include <sptensor/sptensor.h>
#include <sptensor/hash.h>
#include <sptensor/coo.h>
#include <time.h>

int main(int argc, char **argv) 
{
    FILE *file;
    sptensor_t *t;
	
	clock_t start, end;
    double cpu_time_used;

    /* evaluate the command line */
    if(argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return -1;
    }

    /* attempt to open the file */
    file = fopen(argv[1], "r");
    if(!file){
        fprintf(stderr, "Could not open file: %s\n", argv[1]);
        return -2;
    }

	start = clock();
	
    /* read the tensor, and then write it to stdout */
    t = sptensor_coo_read(file);
	end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("time used = %f\n", cpu_time_used);
	
    /*sptensor_hash_write(stdout, t);*/
    sptensor_index_t idx[][3] = { {1,1,1},
	    			  {2,3,1},
				  {2,4,4},
				  {3,2,2}};
    double v[4] = {99.0,44.0, 100.0, 1.0};
    mpf_t mpfv;
    mpf_init(mpfv);
    int i;
    int j;
    start = clock();
    for(i = 0;i<1000000;i++) {
	for(j=0;j<3;j++) {
   		/*sptensor_hash_search(t->hashtable, idx[j], t->nbuckets,t->modes); */
    		mpf_set_d(mpfv, v[i]);