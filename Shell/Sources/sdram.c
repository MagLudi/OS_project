/**
 * sdram.c
 * SDRAM initialization routines
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#include <stdint.h>
#include "derivative.h"
#include "sdram.h"

void sdramInit(void) {
	/* Enable DDR controller clock using the System Clock Gating
	 Control Register 3 (SIM_SCGC3) (See 12.2.11 on page 338 of
	 the K70 Sub-Family * Reference Manual, Rev. 2, Dec 2011) */
	SIM_SCGC3 |= SIM_SCGC3_DDR_MASK;

	/* Enable all DDR I/O pads and set the DDR configuration to
	 "DDR2 Full Strength" using the Misc Control Register
	 (SIM_MCR) (See 12.2.26 on page 357 of the K70 Sub-Family
	 Reference Manual, Rev. 2, Dec 2011) */
	SIM_MCR |= SIM_MCR_DDRPEN_MASK
			| SIM_MCR_DDRCFG(SIM_MCR_DDRCFG_DDR2_FULL_STRENGTH);

	/* Force RCR (Read Clock Recovery module) software reset in
	 addition to system reset using the RCR Control Register
	 (DDR_RCR) (See 34.4.65 on page 992 of the K70 Sub-Family
	 Reference Manual, Rev. 2, Dec 2011) */
	DDR_RCR |= DDR_RCR_RST_MASK;

	/* Chip Select 0's On Die Termination (ODT) resistance is 75 ohms,
	 delay chain #0 has 10 buffers using the I/O Pad Control Register
	 (DDR_PAD_CTRL) (See 34.4.66 on page 992 of the K70 Sub-Family
	 Reference Manual, Rev. 2, Dec 2011) */
	DDR_PAD_CTRL =
	DDR_PAD_CTRL_PAD_ODT_CS0(DDR_PAD_CTRL_PAD_ODT_CS0_75_OHMS)
			| DDR_PAD_CTRL_SPARE_DLY_CTRL(
					DDR_PAD_CTRL_SPARE_DLY_CTRL_10_BUFFERS) | 0x00030200;
	/* Was the following line: */
	// *(vuint32 *)(0x400Ae1ac) = 0x01030203;
	/* Note: The replacement code above accurately replicates all
	 of the original bits, but many are listed in the documentation
	 as reserved and always are zero. */

	/* Set DRAM Class to DDR2 using the DDR Control Register 0 (DDR_CR00)
	 (See 34.4.1 on page 936 of the K70 Sub-Family Reference Manual,
	 Rev. 2, Dec 2011) */
	DDR_CR00 = DDR_CR00_DDRCLS(DDR_CR00_DDRCLS_DDR2);

	/*
	 * auto-refresh commands issued 2 times during DRAM initialization
	 * DRAM initialization time is 49 cycles
	 */
	DDR_CR02 = DDR_CR02_INITAREF(2) | DDR_CR02_TINIT(49);
	// Was: DDR_CR02 = 0x02000031;

	/*
	 * CAS-to-CAS delay is 2 cycles
	 * write latency is 2 cycles
	 * latency gate is 5 half-cycles
	 * CAS latency linear value is 6 half-cycles
	 */
	DDR_CR03 = DDR_CR03_TCCD(2) |
	DDR_CR03_WRLAT(2) |
	DDR_CR03_LATGATE(5) |
	DDR_CR03_LATLIN(6);
	// Was: DDR_CR03 = 0x02020506;

	/*
	 * minimum row active time is 6 cycles
	 * time between same row access is 9 cycles
	 * time between different row access is 2 cycles
	 * burst interrupt interval is 2 cycles
	 */
	DDR_CR04 = DDR_CR04_TRASMIN(6) |
	DDR_CR04_TRC(9) |
	DDR_CR04_TRRD(2) |
	DDR_CR04_TBINT(2);
	// Was: DDR_CR04 = 0x06090202;

	/*
	 * tMRD is 2 cycles
	 * read-to-precharge time is 2 cycles
	 * precharge time is 3 cycles
	 * write-to-read time is 2 cycles
	 */
	DDR_CR05 = DDR_CR05_TMRD(2) |
	DDR_CR05_TRTP(2) |
	DDR_CR05_TRP(3) |
	DDR_CR05_TWTR(2);
	// Was: DDR_CR05 = 0x02020302;

	/*
	 * maximum row access time is 36928 cycles
	 * wait time between mode commands is 2 cycles
	 */
	DDR_CR06 = DDR_CR06_TRASMAX(36928) | DDR_CR06_TMOD(2) | 0x02000000;
	// Was: DDR_CR06 = 0x02904002;
	/* Note: The replacement code above accurately replicates all
	 of the original bits, but many are listed in the documentation
	 as reserved and always are zero. */

	/*
	 * enable concurrent auto-precharge
	 * disable auto-precharge
	 * minimum clock low pulse width during self refresh is 3 cycles
	 * clock pulse width is 3 cycles
	 */
	DDR_CR07 = DDR_CR07_CCAPEN_MASK | DDR_CR07_TCKESR(3) | DDR_CR07_CLKPW(3);
	// Was: DDR_CR07 = 0x01000303;

	/*
	 * auto-precharge write recovery time is 5 cycles
	 * write-recovery time is 3 cycles
	 * RAS-to-CAS delay is 2 cycles
	 * memory controller supports tRAS lockout
	 */
	DDR_CR08 = DDR_CR08_TDAL(5) | DDR_CR08_TWR(3) | DDR_CR08_TRASDI(2) |
	DDR_CR08_TRAS_MASK;
	// Was: DDR_CR08 = 0x05030201;

	/*
	 * burst length is 4 words
	 * DLL lock time is 200 cycles
	 */
	DDR_CR09 = DDR_CR09_BSTLEN(DDR_CR09_BSTLEN_4_WORDS) | DDR_CR09_TDLL(200);
	// Was: DDR_CR09 = 0x020000c8;

	/*
	 * TRP all bank time is 3 cycles
	 * clock enable to precharge delay is 50 cycles
	 * tFAW is 7 cycles
	 */
	DDR_CR10 = DDR_CR10_TRPAB(3) | DDR_CR10_TCPD(50) | DDR_CR10_TFAW(7);
	// Was: DDR_CR10 = 0x03003207;

	/*
	 * enable refresh commands
	 */
	DDR_CR11 = DDR_CR11_TREFEN_MASK;
	// Was: DDR_CR11 = 0x01000000;

	/*
	 * refresh time is 1170 cycles
	 * refresh command time is 49 cycles
	 */
	DDR_CR12 = DDR_CR12_TREF(1170) | DDR_CR12_TRFC(49);
	// Was: DDR_CR12 = 0x04920031;

	/*
	 * tREFINT is 5 cycles
	 */
	DDR_CR13 = DDR_CR13_TREFINT(5);
	// Was: DDR_CR13 = 0x00000005;

	/*
	 * self-refresh exit time is 200 cycles
	 * power-down exit command period is 2 cycles
	 */
	DDR_CR14 = DDR_CR14_TXSR(200) | DDR_CR14_TPDEX(2);
	// Was: DDR_CR14 = 0x00c80002;

	/*
	 * tXSNR is 50 cycles
	 */
	DDR_CR15 = DDR_CR15_TXSNR(50);
	// Was: DDR_CR15 = 0x00000032;

	/*
	 * enable quick self-refresh
	 */
	DDR_CR16 = DDR_CR16_QKREF_MASK;
	// Was: DDR_CR16 = 0x00000001;

	/*
	 * hold clock stable for 3 cycles before exiting self-refresh
	 * hold clock stable for 3 cycles after entering self-refresh
	 */
	DDR_CR20 = DDR_CR20_CKSRX(3) | DDR_CR20_CKSRE(3);
	// Was: DDR_CR20 = 0x00030300;

	/*
	 * program 0x0004 into memory mode register 1 for chip select
	 * program 0x0232 into memory mode register 0 for chip select
	 */
	DDR_CR21 = DDR_CR21_MR1DAT0(0x0004) | DDR_CR21_MR0DAT0(0x0232);
	// Was: DDR_CR21 = 0x00040232;

	/*
	 * program 0x0000 into memory mode register 3 for chip select
	 * program 0x0000 into memory mode register 2 for chip select
	 */
	DDR_CR22 = DDR_CR22_MR3DAT0(0) | DDR_CR22_MR2DATA0(0);
	// Was: DDR_CR22 = 0x00000000;

	/*
	 * ???
	 */
	// Was: DDR_CR23 = 0x00040302;
	// In KDS 3.2.0, DDR_CR23 is declared as read-only
	/* Note: The replacement code above accurately replicates all
	 of the original bits, but many are listed in the documentation
	 as reserved and always are zero. */

	/*
	 * auto-precharge bit is address bit 10
	 * use 10 (= 11 - 1) column pins
	 * use 14 (= 16 - 2) address pins
	 * 8 bank mode
	 */
	DDR_CR25 = DDR_CR25_APREBIT(10) | DDR_CR25_COLSIZ(1) | DDR_CR25_ADDPINS(2) |
	DDR_CR25_BNK8_MASK;
	// Was: DDR_CR25 = 0x0a010201;

	/*
	 * enable bank split
	 * enable address collision detection
	 * initialize command age counter to 255
	 * initialize age counter to 255
	 */
	DDR_CR26 = DDR_CR26_BNKSPT_MASK |
	DDR_CR26_ADDCOL_MASK | DDR_CR26_CMDAGE(255) | DDR_CR26_AGECNT(255);
	// Was: DDR_CR26 = 0x0101FFFF;

	/*
	 * enable command swapping in command queue
	 * enable read-write grouping in command queue
	 * enable priority in command queue
	 * enable placement logic in command queue
	 */
	DDR_CR27 = DDR_CR27_SWPEN_MASK |
	DDR_CR27_RWEN_MASK |
	DDR_CR27_PRIEN_MASK |
	DDR_CR27_PLEN_MASK;
	// Was: DDR_CR27 = 0x01010101;

	/*
	 * chip select is enabled
	 */
	DDR_CR28 = DDR_CR28_CSMAP_MASK | 0x00000002;
	// Was: DDR_CR28 = 0x00000003;
	/* Note: The replacement code above accurately replicates all
	 of the original bits, but many are listed in the documentation
	 as reserved and always are zero. */

	/*
	 * ???
	 */
	DDR_CR29 = 0x00000000;
	// Was: DDR_CR29 = 0x00000000;

	/*
	 * enable automatic DLL resync after refresh
	 */
	DDR_CR30 = DDR_CR30_RSYNCRF_MASK;
	// Was: DDR_CR30 = 0x00000001;

	/*
	 * CS has active ODT termination when CS performs a write
	 * CS has active ODT termination when CS performs a read
	 */
	DDR_CR34 = DDR_CR34_ODTWRCS_MASK |
	DDR_CR34_ODTRDC_MASK | 0x02020000;
	// Was: DDR_CR34 = 0x02020101;
	/* Note: The replacement code above accurately replicates all
	 of the original bits, but many are listed in the documentation
	 as reserved and always are zero. */

	/*
	 * ???
	 */
	// Was: DDR_CR36 = 0x01010201;
	// In KDS 3.2.0, DDR_CR36 is declared as read-only
	/* Note: The replacement code above accurately replicates all
	 of the original bits, but many are listed in the documentation
	 as reserved and always are zero. */

	/*
	 * insert 2 cycles between reads and writes on same chip select
	 */
	DDR_CR37 = DDR_CR37_R2WSAME(2);
	// Was: DDR_CR37 = 0x00000200;

	/*
	 * subdivide port 0 INCR write request into controller commands of size 32 bytes
	 */
	DDR_CR38 = DDR_CR38_PWRCNT(32);
	// Was: DDR_CR38 = 0x00200000;

	/*
	 * port 0 write command priority is 1 (0 is highest, 3 is lowest)
	 * port 0 read command priority is 1
	 * subdivide port 0 INCR read request into controller commands of size 32 bytes
	 */
	DDR_CR39 = DDR_CR39_WP0(1) | DDR_CR39_RP0(1) | DDR_CR39_P0RDCNT(32);
	// Was: DDR_CR39 = 0x01010020;

	/*
	 * subdivide port 1 INCR write request into controller commands of size 32 bytes
	 */
	DDR_CR40 = DDR_CR40_P1WRCNT(32);
	// Was: DDR_CR40 = 0x00002000;

	/*
	 * port 1 write command priority is 1 (0 is highest, 3 is lowest)
	 * port 1 read command priority is 1
	 * subdivide port 1 INCR read request into controller commands of size 32 bytes
	 */
	DDR_CR41 = DDR_CR41_WP1(1) | DDR_CR41_RP1(1) | DDR_CR41_P1RDCNT(32);
	// Was: DDR_CR41 = 0x01010020;

	/*
	 * subdivide port 2 INCR write request into controller commands of size 32 bytes
	 */
	DDR_CR42 = DDR_CR42_P2WRCNT(32);
	// Was: DDR_CR42 = 0x00002000;

	/*
	 * port 2 write command priority is 1 (0 is highest, 3 is lowest)
	 * port 2 read command priority is 1
	 * subdivide port 2 INCR read request into controller commands of size 32 bytes
	 */
	DDR_CR43 = DDR_CR43_WP2(1) | DDR_CR43_RP2(1) | DDR_CR43_P2RDCNT(32);
	// Was: DDR_CR43 = 0x01010020;

	/*
	 * free-running latency control
	 */
	DDR_CR44 = 0x00000000;
	// Was: DDR_CR44 = 0x00000000;

	/*
	 * port 0 priority 3 commands have relative priority 3 (0 is lowest, 15 is highest)
	 * port 0 priority 2 commands have relative priority 3
	 * port 0 priority 1 commands have relative priority 3
	 * port 0 priority 0 commands have relative priority 3
	 */
	DDR_CR45 = DDR_CR45_P0PRI3(3) |
	DDR_CR45_P0PRI2(3) |
	DDR_CR45_P0PRI1(3) |
	DDR_CR45_P0PRI0(3);
	// Was: DDR_CR45 = 0x03030303;

	/*
	 * port 1 priority 0 commands have relative priority 2
	 * port 0 priority relax counter trigger at 100
	 * reassigned port 0 order is 1 (0 is highest, 3 is lowest)
	 */
	DDR_CR46 = DDR_CR46_P1PRI0(2) | DDR_CR46_P0PRIRLX(100) | DDR_CR46_P0ORD(1);
	// Was: DDR_CR46 = 0x02006401;

	/*
	 * reassigned port 1 order is 1 (0 is highest, 3 is lowest)
	 * port 1 priority 3 commands have relative priority 2 (0 is lowest, 15 is highest)
	 * port 1 priority 2 commands have relative priority 2
	 * port 1 priority 1 commands have relative priority 2
	 */
	DDR_CR47 = DDR_CR47_P1ORD(1) |
	DDR_CR47_P1PRI3(2) |
	DDR_CR47_P1PRI2(2) |
	DDR_CR47_P1PRI1(2);
	// Was: DDR_CR47 = 0x01020202;

	/*
	 * port 2 priority 1 commands have relative priority 1 (0 is lowest, 15 is highest)
	 * port 2 priority 0 commands have relative priority 1 (0 is lowest, 15 is highest)
	 * port 1 priority relax counter trigger at 100
	 */
	DDR_CR48 = DDR_CR48_P2PRI1(1) | DDR_CR48_P2PRI0(1) | DDR_CR48_P1PRIRLX(100);
	// Was: DDR_CR48 = 0x01010064;

	/*
	 * reassigned port 2 order is 2 (0 is highest, 3 is lowest)
	 * port 2 priority 3 commands have relative priority 1 (0 is lowest, 15 is highest)
	 * port 2 priority 2 commands have relative priority 1
	 */
	DDR_CR49 = DDR_CR49_P2ORD(2) | DDR_CR49_P2PRI3(1) | DDR_CR49_P2PRI2(1);
	// Was: DDR_CR49 = 0x00020101;

	/*
	 * port 2 priority relax counter trigger at 100
	 */
	DDR_CR50 = DDR_CR50_P2PRIRLX(100);
	// Was: DDR_CR50 = 0x00000064;

	/*
	 * read data enable base is 2
	 * PHY read latency base is 6
	 * PHY write latency base is 2
	 */
	DDR_CR52 = DDR_CR52_RDDTENBAS(2) |
	DDR_CR52_PHYRDLAT(6) |
	DDR_CR52_PYWRLTBS(2);
	// Was: DDR_CR52 = 0x02000602;

	/*
	 * DFI tCTRLUPD_MAX is 968
	 */
	DDR_CR53 = DDR_CR53_CTRLUPDMX(968);
	// Was: DDR_CR53 = 0x03c80000;

	/*
	 * DFI tPHYUPD_TYPE1 is 968
	 * DFI tPHYUPD_TYPE0 is 968
	 */
	DDR_CR54 = DDR_CR54_PHYUPDTY1(968) | DDR_CR54_PHYUPDTY0(968);
	// Was: DDR_CR54 = 0x03c803c8;

	/*
	 * DFI tPHYUPD_TYPE3 is 968
	 * DFI tPHYUPD_TYPE2 is 968
	 */
	DDR_CR55 = DDR_CR55_PHYUPDTY3(968) | DDR_CR55_PHYUPDTY2(968);
	// Was: DDR_CR55 = 0x03c803c8;

	/*
	 * write latency adjust is 2
	 * read latency adjust is 3
	 * tPHYUPD_RESP is 968
	 */
	DDR_CR56 = DDR_CR56_WRLATADJ(2) |
	DDR_CR56_RDLATADJ(3) |
	DDR_CR56_PHYUPDRESP(968);
	// Was: DDR_CR56 = 0x020303c8;

	/*
	 * enable use of non-DFI odt_alt_signal ODT alternate
	 * delay from DFI clock enable to memory clock enable is 1 cycle
	 * delay from DFI command to memory command is 2 cycles
	 */
	DDR_CR57 = DDR_CR57_ODTALTEN_MASK | DDR_CR57_CLKENDLY(1) |
	DDR_CR57_CMDDLY(2);
	// Was: DDR_CR57 = 0x01010002;

	__asm("nop");

	/* Initiate command processing in the memory controller */
	DDR_CR00 |= DDR_CR00_START_MASK;

	while (!(DDR_CR30 & DDR_CR30_INTSTAT_DRAM_INIT_COMPLETE_MASK)) {
		/* Spin, waiting for DRAM initialization to complete */
	}

	/* Set DDR address size translation to 128M Bytes using the Control
	 Register (MCM_CR) (See 17.2.3 on page 419 of the K70 Sub-Family
	 Reference Manual, Rev. 2, Dec 2011) */
	MCM_CR |= MCM_CR_DDRSIZE(MCM_CR_DDRSIZE_128_MBYTES);
}

