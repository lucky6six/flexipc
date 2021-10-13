/* arch mm */


/* 
 * DCZID_EL0: Data Cache Zero ID register
 * DZP, bit [4] used to indicate whether use of DC ZVA instructions is permitted or not
 * BS, bit [3:0] used to indicate log2 of the block size in words. 
 * The maximum size supported is 2KB (value == 9).
 */

/* A global varible to inidicate block size which will be cleaned by dc zva call */
int dczva_line_size = 0;

/*
 * Read Data Cache Zero ID register
 */
long read_dczid(void)
{
	long val;

	asm volatile("mrs %0, dczid_el0\n\t":"=r"(val));
	return val;
}

/*
 * Check whether support DC ZVA and get dczva_line_size
 */
void cache_setup(void)
{
	long dczid_val;
	int dczid_bs;

	dczid_val = read_dczid();
	if (dczid_val & (1 << 4)) {
		/* the 4th bit indicates the instruction is disabled */
		dczva_line_size = 0;
	}
	else {
		/* the zero size stores in the last four bits */
		dczid_bs = dczid_val & (0xf);
		dczva_line_size = sizeof(int) << dczid_bs;
	}
}