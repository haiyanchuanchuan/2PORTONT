#include <common.h>
#include <command.h>
#include <soc.h>

/*
 * Perform a memory persistance test.
 */
int do_mem_mtest_dtr (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	vu_long	*addr, *start, *end;
	ulong	val;
	ulong	readback;

	ulong	incr;
	ulong	pattern;
	int     rcode = 0;
	int	delay_count;

	if (argc > 1) {
		start = (ulong *)simple_strtoul(argv[1], NULL, 16);
	} else {
		start = (ulong *)(0xA0000000);
	}

	if (argc > 2) {
		end = (ulong *)simple_strtoul(argv[2], NULL, 16);
	} else {
		end = (ulong *)(0xA0000000 + (initdram(0) - 0x200000));
	}

	if (argc > 3) {
		pattern = (ulong)simple_strtoul(argv[3], NULL, 16);
	} else {
		pattern = 0xAAAAAAAA;
	}


	incr = 1;
	for (;;) {
		if (ctrlc()) {
			putc ('\n');
			return 1;
		}

		printf ("\rPattern %08lX  Writing..."
			"%12s"
			"\b\b\b\b\b\b\b\b\b\b",
			pattern, "");

		for (addr=start,val=pattern; addr<end; addr++) {
			*addr = val;
			val  += incr;
		}

		puts ("Delay...(Hit any key to read or waiting for 8 hours~) ");
		for(delay_count=0; delay_count<2880000;delay_count++){
			if((delay_count%288000) == 0)
				printf("=");

			udelay(10000);
			if( tstc() ){
				(void) getc();
				break;
			}
		}
		printf(">");
		puts (" Reading...");

		for (addr=start,val=pattern; addr<end; addr++) {
			readback = *addr;
			if (readback != val) {
				printf ("\nMem error @ 0x%08X: "
					"found %08lX, expected %08lX\n",
					(uint)addr, readback, val);
				rcode = 1;
			}
			val += incr;
		}

		/*
		 * Flip the pattern each time to make lots of zeros and
		 * then, the next time, lots of ones.  We decrement
		 * the "negative" patterns and increment the "positive"
		 * patterns to preserve this feature.
		 */
		if(pattern & 0x80000000) {
			pattern = -pattern;	/* complement & increment */
		}
		else {
			pattern = ~pattern;
		}
		incr = -incr;
	}
	return rcode;
}

#if (CONFIG_STANDALONE_UBOOT != 1)
#ifdef CONFIG_CMD_DRAM_DIG
extern int chk_dram_cfg(unsigned int  dram_freq_mhz);

/*
 * Do DRAM setting diagnosis.
 */
int do_dram_dig (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int dram_freq_mhz;

        if (argc < 2) {
		dram_freq_mhz = 0;
        }else{
		dram_freq_mhz = simple_strtoul(argv[1], NULL, 16);
	}

	return chk_dram_cfg(dram_freq_mhz);
}
#endif
#endif

#ifdef CONFIG_CMD_DRAM_TEST
extern int dram_test (int flag, int argc, char * const argv[]);
/*
 * Do DRAM test.
 */
int do_dram_test (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return dram_test(flag, argc, argv);
}
#endif
#ifdef CONFIG_CMD_FLASH_TEST
extern int flash_test (int flag, int argc, char * const argv[]);
/*
 * Do flash test.
 */
int do_flash_test (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return flash_test(flag, argc, argv);
}
#endif

#ifdef CONFIG_CMD_FOREVER_TEST
int  do_forever_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	while(1){
#ifdef CONFIG_CMD_CONCURENT_TEST
		do_concur_test(cmdtp, flag, argc, argv);
#endif
#ifdef CONFIG_CMD_DRAM_TEST
		do_dram_test(cmdtp, flag, argc, argv);
#endif
#ifdef CONFIG_CMD_IDMEM_TEST
		do_idmem_test(cmdtp, flag, argc, argv);
#endif
#ifdef CONFIG_CMD_GDMA_TEST
		do_gdma_test(cmdtp, flag, argc, argv);
#endif
	}
	return 1;
}
#endif

#ifdef CONFIG_CMD_GDMA_TEST
int do_gdma_test (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return gdma_test(flag, argc, argv);
}
#endif

#ifdef CONFIG_CMD_CONCURENT_TEST
extern int concur_test (int flag, int argc, char *argv[]);

int do_concur_test (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return concur_test(flag, argc, argv);
}
#endif

#ifdef CONFIG_CMD_NEXT_FREQ
int do_next_freq (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *s;
	char buf[32];
	unsigned int clk_step, cpu_clk, mem_clk;

	if(_is_CKSEL_25MHz()){
		clk_step = 25;
	}else{
		clk_step = 20;
	}

        s = getenv("cpu_clk");
        if (s) {
		cpu_clk = (unsigned int)simple_strtoull(s, NULL, 10);
		if(cpu_clk < 500)
			cpu_clk = 500;

		if(cpu_clk > 700)
			cpu_clk = 700;
        }else{
		cpu_clk = 500;
	}

        s = getenv("mem_clk");
        if (s) {
		mem_clk = (unsigned int)simple_strtoull(s, NULL, 10);
		if(mem_clk < 100)
			mem_clk = 100;

		if(mem_clk > 400)
			mem_clk = 400;
        }else{
		mem_clk = 500;
	}

	cpu_clk += clk_step;
	if(cpu_clk > 700){
		cpu_clk = 500;
		mem_clk = mem_clk + (2*clk_step);
		if(mem_clk > 400)
			mem_clk = 100;
	}

	sprintf(buf, "%d", cpu_clk);
	setenv("cpu_clk", buf);
	sprintf(buf, "%d", mem_clk);
	setenv("mem_clk", buf);

}
#endif


#ifdef CONFIG_CMD_MEASURE_CPU_CLK
void get_cpu_cycle_per_ms(unsigned int *cnt_addr);

int do_measure_cpu_clk(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int cpu_cycles;
	unsigned int cnt_arry[8];

	get_cpu_cycle_per_ms(cnt_arry);

	cpu_cycles = cnt_arry[1];

	printf("\t- Total cpu cycles per ms: 0x%08x%08x\n", cnt_arry[0],cnt_arry[1]);
	printf("\t- CPU MHz: %d.%02d\n", (cpu_cycles/1000), ((cpu_cycles%1000)/10));

	return 0;

}
#endif



#ifdef CONFIG_CMD_DRAM_AC_TEST
int do_write_read_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	unsigned int tar_addr, tar_pat, times, read_tmp;

	volatile unsigned int *read_location, *write_location;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}else if(argc < 4){
		tar_addr = simple_strtoul(argv[1], NULL, 16);
		tar_pat = simple_strtoul(argv[2], NULL, 16);
		read_location = (volatile unsigned int *)tar_addr;
		write_location = (volatile unsigned int *)tar_addr;

		while(1){
			*write_location = tar_pat;
			read_tmp = *read_location;
		}
	}else{
		tar_addr = simple_strtoul(argv[1], NULL, 16);
		tar_pat = simple_strtoul(argv[2], NULL, 16);
		times = simple_strtoul(argv[3], NULL, 16);
		read_location = (volatile unsigned int *)tar_addr;
		write_location = (volatile unsigned int *)tar_addr;

		while(times--){
			*write_location = tar_pat;
			read_tmp = *read_location;
		}
	}

	return 0;
}

int do_read_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	unsigned int tar_addr, tar_pat, times;

	volatile unsigned int *read_location;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}else if(argc < 4){
		tar_addr = simple_strtoul(argv[1], NULL, 16);
		tar_pat = simple_strtoul(argv[2], NULL, 16);
		read_location = (volatile unsigned int *)tar_addr;
		*read_location = tar_pat;

		while(1){
			tar_pat = *read_location;
		}
	}else{
		tar_addr = simple_strtoul(argv[1], NULL, 16);
		tar_pat = simple_strtoul(argv[2], NULL, 16);
		times = simple_strtoul(argv[3], NULL, 16);
		read_location = (volatile unsigned int *)tar_addr;
		*read_location = tar_pat;
		printf("tar_addr(0x%08x), tar_pat(0x%08x), time(0x%08x)\n", tar_addr, tar_pat, times);
		while(times--){
			if(tar_pat != *read_location){
				printf("read data(0x%08x) != 0x%08x\n", *read_location, tar_pat);
				break;
			}
		}
	}

	return 0;
}

int do_write_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	unsigned int tar_addr, tar_pat, times;

	volatile unsigned int *write_location;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}else if(argc < 4){
		tar_addr = simple_strtoul(argv[1], NULL, 16);
		tar_pat = simple_strtoul(argv[2], NULL, 16);
		write_location = (volatile unsigned int *)tar_addr;

		printf("tar_addr(0x%08x), tar_pat(0x%08x), time(forever)\n", tar_addr, tar_pat);
		while(1){
			*write_location = tar_pat;
		}
	}else{
		tar_addr = simple_strtoul(argv[1], NULL, 16);
		tar_pat = simple_strtoul(argv[2], NULL, 16);
		times = simple_strtoul(argv[3], NULL, 16);
		write_location = (volatile unsigned int *)tar_addr;

		printf("tar_addr(0x%08x), tar_pat(0x%08x), time(0x%08x)\n", tar_addr, tar_pat, times);
		while(times--){
			*write_location = tar_pat;
		}
	}

	return 0;
}
#endif

/*
 * Do "whole system reset" that depens on different project. 
 */
int do_reset_all(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	SYSTEM_RESET();
	fprintf(stderr, "*** system reset failed ***\n");
	return 0;
}

#ifdef CONFIG_CMD_DRAM_AC_TEST
U_BOOT_CMD(
        mwrite_test,    4,    1,     do_write_test,
        "mwrite_test   - Keep writing the pattern to the input address location.",
        "[target address] [target pattern] [number of time]\n"
        "    - Keep writing the pattern to the address with the input times.\n"
	"      if there is no input time, write forever."
);
U_BOOT_CMD(
        mread_test,    4,    1,     do_read_test,
        "mwrite_test   - Keep reading the pattern to the input address location.",
        "[target address] [target pattern] [number of time]\n"
        "    - Keep reading the pattern to the address with the input times.\n"
	"      if there is no input time, read forever."
);
U_BOOT_CMD(
        mwrite_read_test,    4,    1,     do_write_read_test,
        "mwrite_test   - Keep writing and reading the pattern at the input address location.",
        "[target address] [target pattern] [number of time]\n"
        "    - Keep writing and reading the pattern to the address with the input times.\n"
	"      if there is no input time, write/read forever."
);
#endif
#if (CONFIG_STANDALONE_UBOOT != 1)
#ifdef CONFIG_CMD_DRAM_DIG
U_BOOT_CMD(
        mdram_dig,    4,    1,     do_dram_dig,
        "mdram_dig   - Check RTK DRAM configuration.",
        "[no parameters]\n"
        "    - Check RTK DRAM configuration."
);
#endif
#endif

#ifdef CONFIG_CMD_DRAM_TEST
U_BOOT_CMD(
        mdram_test,    10,    1,     do_dram_test,
        "mdram_test   - do DRAM test.",
        "[-l/-loops <test loops>] [-r/-range <star address> <test size>] [-b/-block_e] [-reset <all>] [-pstr <pass msg>] [-fstr <fail msg>]\n"
        "    - do DRAM test."
);
#endif
#ifdef CONFIG_CMD_FLASH_TEST
U_BOOT_CMD(
        mflash_test,    6,    1,     do_flash_test,
        "mflash_test   - do flash test.",
        "[-l/-loops <test loops>] [-b/-block_e] [-reset <all>]\n"
        "    - do flash test."
);
#endif
#ifdef CONFIG_CMD_FOREVER_TEST
U_BOOT_CMD(
        forever_test,    5,    1,     do_forever_test,
        "forever_test     - Keep looping test mdram_test, midmem_test, mgdma_test.",
        "[no parameters]\n"
        "    -  forever test DRAM ."
);
#endif
#ifdef CONFIG_CMD_GDMA_TEST
U_BOOT_CMD(
        mgdma_test,    4,    1,     do_gdma_test,
        "mgdma_test   - do gdma test.",
        "[no parameters]\n"
        "    - do gdma test."
);
#endif

#ifdef CONFIG_CMD_IDMEM_TEST
U_BOOT_CMD(
        midmem_test,    4,    1,     do_idmem_test,
        "midmem_test   - do IDMEM test.",
        "[no parameters]\n"
        "    - do IDMEM test."
);
#endif
#ifdef CONFIG_CMD_CONCURENT_TEST
U_BOOT_CMD(
        mconcur_test,    4,    1,     do_concur_test,
        "mconcur_test   - do concurrent test.",
        "[no parameters]\n"
        "    - do concurrent test."
);
#endif
#ifdef CONFIG_CMD_NEXT_FREQ
U_BOOT_CMD(
        next_freq,    4,    1,     do_next_freq,
        "next_freq   - next cpu clock, memory clock",
        "[no parameters]\n"
        "    - setting the next cpu_clk, mem_clk value."
);
#endif

int btg_sub_sys(int argc, char * const argv[]);
int do_btg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	return btg_sub_sys(argc, argv);
}

extern s8_t btg_prompt_string[];

U_BOOT_CMD(
        btg,          6,    0,     do_btg,
        "bus traffic generator sub-system",
        btg_prompt_string
);

U_BOOT_CMD(
        reset_all, 1, 0,    do_reset_all,
        "Perform whole chip RESET of the CPU",
        ""
);
