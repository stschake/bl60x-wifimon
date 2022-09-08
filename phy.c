#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "utility.h"
#include "rf_data.h"

extern void phy_config_rxgain(int offset);

void mdm_reset() {
    // mdm_swreset_set
    write_volatile_4(0x44c00888,0x1111);
    write_volatile_4(0x44c00888,0);
}

extern void rfc_config_channel(uint16_t freq);

void phy_hw_set_channel(uint16_t freq) {
    // riu_ofdmonly_setf = 0
    uint32_t v = read_volatile_4(0x44c0b390);
    write_volatile_4(0x44c0b390, v & 0xfffffeff);
    // mdm_rxdsssen_setf = 1
    v = read_volatile_4(0x44c00820);
    write_volatile_4(0x44c00820, v | 1);
    // mdm_mdmconf_setf
    write_volatile_4(0x44c00800,0);
    mdm_reset();
    // mdm_txstartdelay_setf
    write_volatile_4(0x44c00838,0xb4);
    // mdm_txctrl1
    write_volatile_4(0x44c0088c,0x1c13);
    // mdm_txctrl3
    write_volatile_4(0x44c00898,0x2d00438);
    // mdm_tbe_count_adjust_20_setf
    v = read_volatile_4(0x44c00858);
    write_volatile_4(0x44c00858,v & 0xffffff00);
    // mdm_dcestimctrl_pack
    write_volatile_4(0x44c0081c,0xf0f);
    // mdm_waithtstf_setf
    v = read_volatile_4(0x44c0081c);
    write_volatile_4(0x44c0081c,(v & 0xffffff80) | 7);
    // mdm_tddchtstfmargin_setf
    v = read_volatile_4(0x44c00834);
    write_volatile_4(0x44c00834,(v & 0xffffff) | 0x6000000);
    // mdm_smoothctrl_set
    write_volatile_4(0x44c00818,0x1880c06);
    // mdm_tbectrl2_set
    write_volatile_4(0x44c00860,0x7f03);
    // riu_rwnxagcaci20marg0_set
    write_volatile_4(0x44c0b340,0);
    // riu_rwnxagcaci20marg1_set
    write_volatile_4(0x44c0b344,0);
    // riu_rwnxagcaci20marg2_set
    write_volatile_4(0x44c0b348,0);
    // mdm_txcbwmax_setf = 0
    v = read_volatile_4(0x44c00824);
    write_volatile_4(0x44c00824,v & 0xfcffffff);
    rfc_config_channel(freq);

    // purely TX stuff
    /*
    uint8_t channel = phy_freq_to_channel(band, freq);
    rfc_apply_tx_power_offset(channel, poweroffset);
    trpc_update_vs_channel((int8_t)freq1);
    */
}

extern void rf_pri_xtalfreq(uint32_t freq);
extern void rf_pri_init(uint8_t reset, uint8_t chipv);

void _set_rfc() {
    // all assuming 40 MHz xtal
    // rf_lo_center_freq_mhz_setf
    uint32_t v = read_volatile_4(0x400011c0);
    write_volatile_4(0x400011c0, (v & 0xfffff000) | 2430);
    // rf_lo_sdmin_center_setf
    write_volatile_4(0x400011c4, 81 << 22);
    // rf_lo_sdmin_1m_setf
    write_volatile_4(0x400011c8, 139810);
    // rf_lo_sdmin_if_setf
    write_volatile_4(0x400011cc, 174762);

    union {
        uint8_t seq[2][2];
        uint32_t val;
    } lo_cw = {.val = 0};
    volatile uint32_t* cal_reg = (volatile uint32_t*)0x4000113c;
    for (int i = 0; i < 21; i++) {
        int freq_cw = rf_calib_data->lo[i].fcal;
        int idac_cw = rf_calib_data->lo[i].acal;
        if (i == 0)
            freq_cw += 1;
        lo_cw.seq[i&1][0] = idac_cw;
        lo_cw.seq[i&1][1] = freq_cw;
        if (i == 20 || (i&1)) {
            *cal_reg = lo_cw.val;
            lo_cw.val = 0;
            cal_reg++;
        }
    }
    
    // ignore TX stuff..
    // _sync_tx_power_offset

    // rf_rc_state_dbg_en_setf(0)
    v = read_volatile_4(0x40001004);
    write_volatile_4(0x40001004, v & 0xfffff7ff);
    // rf_fsm_st_dbg_en_setf(0)
    v = read_volatile_4(0x4000126c);
    write_volatile_4(0x4000126c, v & 0xfffffff7);
    // rf_fsm_lo_time_setf
    v = read_volatile_4(0x40001268);
    write_volatile_4(0x40001268, (v & 0xffff0000) | 0x1040);
    // rf_fsm_ctrl_en_setf(1)
    v = read_volatile_4(0x40001004);
    write_volatile_4(0x40001004, (v & 0xfffffffd) | 2);
    // rf_tx_gain_ctrl_hw_setf(1)
    v = read_volatile_4(0x4000100c);
    write_volatile_4(0x4000100c, v | 4);
    // rf_tx_dvga_gain_ctrl_hw_setf(1)
    v = read_volatile_4(0x40001600);
    write_volatile_4(0x40001600, v | 0x80000000);
    // rf_tx_dvga_gain_qdb_setf(0)
    v = read_volatile_4(0x40001600);
    write_volatile_4(0x40001600, v & 0x80ffffff);
    // rf_trxcal_ctrl_hw_setf(1)
    v = read_volatile_4(0x4000100c);
    write_volatile_4(0x4000100c, v | 0x20);
    // rf_rx_gain_ctrl_hw_setf(1)
    v = read_volatile_4(0x4000100c);
    write_volatile_4(0x4000100c, v | 2);
}

void _set_mdm() {
    // rc2_txhbf20coeffsel_setf(0)
    uint32_t v = read_volatile_4(0x44c0c218);
    write_volatile_4(0x44c0c218, v & 0xffff0000);
    // unknown MDM things
    write_volatile_4(0x44c03030, 0);
    write_volatile_4(0x44c03034, 0xa027f7f);
    write_volatile_4(0x44c03038, 0x23282317);
    write_volatile_4(0x44c0303c, 0x7f020a17);
    write_volatile_4(0x44c03040, 0x7f);
}

void rfc_init() {
    // rf_bbmode_4s_setf(0)
    uint32_t v = read_volatile_4(0x40001220);
    write_volatile_4(0x40001220, v & 0xfbffffff);
    // rf_bbmode_4s_en_setf(1)
    v = read_volatile_4(0x40001220);
    write_volatile_4(0x40001220, v | 0x8000000);

    rf_pri_init(1, 1);

    _set_rfc();
    _set_mdm();

    // rf_bbmode_4s_en_setf(0)
    v = read_volatile_4(0x40001220);
    write_volatile_4(0x40001220, v & 0xf7ffffff);
    // rf_fsm_t2r_cal_mode_setf(0)
    v = read_volatile_4(0x40001004);
    write_volatile_4(0x40001004, v & 0xfffffff3);
    // PDS->clkpll_output_en.clkpll_en_80m = 0
    v = read_volatile_4(0x4000e41c);
    write_volatile_4(0x4000e41c, v & 0xffffffbf);
    wait_us(1);
    // rf_fsm_t2r_cal_mode_setf(1)
    v = read_volatile_4(0x40001004);
    write_volatile_4(0x40001004, (v & 0xfffffff3) | 4);
    wait_us(1);
    // // PDS->clkpll_output_en.clkpll_en_80m = 1
    v = read_volatile_4(0x4000e41c);
    write_volatile_4(0x4000e41c, v | 0x40);
}

void agc_config() {
    // riu_htstfgainen_setf
    uint32_t v = read_volatile_4(0x44c0b390);
    write_volatile_4(0x44c0b390,v & 0xfffeffff);
    // riu_rifsdeten_setf
    v = read_volatile_4(0x44c0b390);
    write_volatile_4(0x44c0b390,v & 0xfffffbff);
    // riu_fe20gain_setf
    v = read_volatile_4(0x44c0b3a4);
    write_volatile_4(0x44c0b3a4,v & 0xffffff00);
    // riu_fe40gain_setf
    v = read_volatile_4(0x44c0b3a4);
    write_volatile_4(0x44c0b3a4,v & 0xffff00ff);
    // riu_vpeakadcqdbv_setf
    v = read_volatile_4(0x44c0b394);
    write_volatile_4(0x44c0b394,(v & 0xff00ffff) | 0xf80000);
    // riu_adcpowmindbm_setf
    v = read_volatile_4(0x44c0b398);
    write_volatile_4(0x44c0b398,(v & 0xffff00ff) | 0x9e00);
    // riu_adcpowsupthrdbm_setf
    v = read_volatile_4(0x44c0b3c4);
    write_volatile_4(0x44c0b3c4,(v & 0xffffff00) | 0xce);
    // riu_satdelay50ns_setf
    v = read_volatile_4(0x44c0b364);
    write_volatile_4(0x44c0b364,(v & 0xe0ffffff) | 0x8000000);
    // riu_sathighthrdbv_setf
    v = read_volatile_4(0x44c0b364);
    write_volatile_4(0x44c0b364,(v & 0xffc0ffff )| 0x3c0000);
    // riu_satlowthrdbv_setf
    v = read_volatile_4(0x44c0b364);
    write_volatile_4(0x44c0b364,(v & 0xffffc0ff) | 0x3800);
    // riu_satthrdbv_setf
    v = read_volatile_4(0x44c0b364);
    write_volatile_4(0x44c0b364,(v & 0xffffffc0 )| 0x39);
    // riu_crossdnthrqdbm_setf
    v = read_volatile_4(0x44c0b368);
    write_volatile_4(0x44c0b368,(v & 0xffc00fff) | 0x70000);
    // riu_crossupthrqdbm_setf
    v = read_volatile_4(0x44c0b368);
    write_volatile_4(0x44c0b368,(v & 0xfffffc00) | 0x70);
    // riu_rampupgapqdb_setf
    v = read_volatile_4(0x44c0b36c);
    write_volatile_4(0x44c0b36c,(v & 0xffffff00) | 0x12);
    // riu_rampupndlindex_setf
    v = read_volatile_4(0x44c0b36c);
    write_volatile_4(0x44c0b36c,(v & 0xfffff8ff) | 0x500);
    // riu_rampdngapqdb_setf
    v = read_volatile_4(0x44c0b36c);
    write_volatile_4(0x44c0b36c,(v & 0xff00ffff) | 0x280000);
    // riu_rampdnndlindex_setf
    v = read_volatile_4(0x44c0b36c);
    write_volatile_4(0x44c0b36c,(v & 0xf8ffffff) | 0x7000000);
    // riu_adcpowdisthrdbv_setf
    v = read_volatile_4(0x44c0b370);
    write_volatile_4(0x44c0b370,(v & 0xff80ffff) | 0x580000);
    // riu_idinbdpowgapdnqdbm_setf
    v = read_volatile_4(0x44c0b3c0);
    write_volatile_4(0x44c0b3c0,(v & 0xffffff) | 0x18000000);
    // riu_evt0op3_setf
    v = read_volatile_4(0x44c0b380);
    write_volatile_4(0x44c0b380,(v & 0xfff03fff) | 0xf8000);
    // riu_evt0op2_setf
    v = read_volatile_4(0x44c0b380);
    write_volatile_4(0x44c0b380,(v & 0xfc0fffff) | 0x3700000);
    // riu_evt0op1_setf
    v = read_volatile_4(0x44c0b380);
    write_volatile_4(0x44c0b380,(v & 0x3ffffff) | 0x4000000);
    // riu_evt0pathcomb_setf
    v = read_volatile_4(0x44c0b380);
    write_volatile_4(0x44c0b380,v & 0xffffdfff);
    // riu_evt0opcomb_setf
    v = read_volatile_4(0x44c0b380);
    write_volatile_4(0x44c0b380,(v & 0xffffe3ff) | 0x400);
    // riu_evt1op1_setf
    v = read_volatile_4(0x44c0b384);
    write_volatile_4(0x44c0b384,(v & 0x3ffffff) | 0xe4000000);
    // riu_evt1op2_setf
    v = read_volatile_4(0x44c0b384);
    write_volatile_4(0x44c0b384,(v & 0xfc0fffff) | 0x3700000);
    // riu_evt1op3_setf
    v = read_volatile_4(0x44c0b384);
    write_volatile_4(0x44c0b384,(v & 0xfff03fff) | 0x50000);
    // riu_evt1pathcomb_setf
    v = read_volatile_4(0x44c0b384);
    write_volatile_4(0x44c0b384,v & 0xffffdfff);
    // riu_evt1opcomb_setf
    v = read_volatile_4(0x44c0b384);
    write_volatile_4(0x44c0b384,(v & 0xffffe3ff) | 0x800);
    // riu_evt2op1_setf
    v = read_volatile_4(0x44c0b388);
    write_volatile_4(0x44c0b388,(v & 0x3ffffff) | 0x3c000000);
    // riu_evt2op2_setf
    v = read_volatile_4(0x44c0b388);
    write_volatile_4(0x44c0b388,(v & 0xfc0fffff) | 0x1700000);
    // riu_evt2op3_setf
    v = read_volatile_4(0x44c0b388);
    write_volatile_4(0x44c0b388,(v & 0xfff03fff) | 0xa8000);
    // riu_evt2pathcomb_setf
    v = read_volatile_4(0x44c0b388);
    write_volatile_4(0x44c0b388,v & 0xffffdfff);
    // riu_evt2opcomb_setf
    v = read_volatile_4(0x44c0b388);
    write_volatile_4(0x44c0b388,(v & 0xffffe3ff) | 0x1400);
    // riu_evt3op1_setf
    v = read_volatile_4(0x44c0b38c);
    write_volatile_4(0x44c0b38c,(v & 0x3ffffff) | 0x64000000);
    // riu_evt3op2_setf
    v = read_volatile_4(0x44c0b38c);
    write_volatile_4(0x44c0b38c,v & 0xfc0fffff);
    // riu_evt3op3_setf
    v = read_volatile_4(0x44c0b38c);
    write_volatile_4(0x44c0b38c,(v & 0xfff03fff) | 0x38000);
    // riu_evt3opcomb_setf
    v = read_volatile_4(0x44c0b38c);
    write_volatile_4(0x44c0b38c,(v & 0xffffe3ff) | 0x800);
    // rc2_evt4op1_setf
    v = read_volatile_4(0x44c0c830);
    write_volatile_4(0x44c0c830,(v & 0x3ffffff) | 0xfc000000);
    // rc2_evt4op2_setf
    v = read_volatile_4(0x44c0c830);
    write_volatile_4(0x44c0c830,(v & 0xfc0fffff) | 0x100000);
    // rc2_evt4op3_setf
    v = read_volatile_4(0x44c0c830);
    write_volatile_4(0x44c0c830,(v & 0xfff03fff) | 0xd8000);
    // rc2_evt4opcomb_setf
    v = read_volatile_4(0x44c0c830);
    write_volatile_4(0x44c0c830,(v & 0xffffe3ff) | 0x1400);
    // rc2_pkdet_mode_setf
    v = read_volatile_4(0x44c0c814);
    write_volatile_4(0x44c0c814,v & 0xfffffffc);
    // rc2_pkdet_cnt_thr_setf
    v = read_volatile_4(0x44c0c814);
    write_volatile_4(0x44c0c814,(v & 0xffffffc3) | 8);
    v = read_volatile_4(0x44c0c814);
    write_volatile_4(0x44c0c814,(v & 0xffffffc3) | 8);
    // rc2_rx0_vga_idx_max_setf
    v = read_volatile_4(0x44c0c040);
    write_volatile_4(0x44c0c040,(v & 0xfe0fffff) | 0xc00000);
    // rc2_rx0_vga_idx_min_setf
    v = read_volatile_4(0x44c0c040);
    write_volatile_4(0x44c0c040,(v & 0xfff07fff) | 0x18000);
    // rc2_rx0_lna_idx_max_setf
    v = read_volatile_4(0x44c0c044);
    write_volatile_4(0x44c0c044,(v & 0xffff00ff) | 0x800);
    // rc2_rx0_lna_idx_min_setf
    v = read_volatile_4(0x44c0c044);
    write_volatile_4(0x44c0c044,v & 0xffffff00);
    //phy_config_rxgain(0);
    // riu_inbdpowmindbm_setf
    v = read_volatile_4(0x44c0b3a0);
    write_volatile_4(0x44c0b3a0,(v & 0xffffff00) | 0x9e);
    // riu_inbdpowsupthrdbm_setf
    v = read_volatile_4(0x44c0b3c0);
    write_volatile_4(0x44c0b3c0,(v & 0xffffff00) | 0xa4);
    // riu_inbdpowinfthrdbm_setf
    v = read_volatile_4(0x44c0b3c0);
    write_volatile_4(0x44c0b3c0,(v & 0xffff00ff) | 0xa300);
    // rc2_inbdpow_adj_thr_dbm_setf
    v = read_volatile_4(0x44c0c82c);
    write_volatile_4(0x44c0c82c,(v & 0xffffff00) | 0xb5);
    // rc2_inbdpowsupthr_adj_en_setf
    v = read_volatile_4(0x44c0c82c);
    write_volatile_4(0x44c0c82c,v | 0x100);
    // rc2_inbdpowinfthr_adj_en_setf
    v = read_volatile_4(0x44c0c82c);
    write_volatile_4(0x44c0c82c,(v & 0xfffff7ff) | 0x800);
    // rc2_reflevofdmthd_en_setf
    v = read_volatile_4(0x44c0c838);
    write_volatile_4(0x44c0c838,(v & 0x7fffffff) | 0x80000000);
    // rc2_reflevofdmthd_setf
    v = read_volatile_4(0x44c0c838);
    write_volatile_4(0x44c0c838,(v & 0xfff80000) | 0x100);
    // rc2_reflevdsssthd_en_setf
    v = read_volatile_4(0x44c0c83c);
    write_volatile_4(0x44c0c83c,(v & 0x7fffffff) | 0x80000000);
    // rc2_reflevdsssthd_setf
    v = read_volatile_4(0x44c0c83c);
    write_volatile_4(0x44c0c83c,(v & 0xfff00000) | 0x17c);
    // rc2_reflevdssscontthd_en_setf
    v = read_volatile_4(0x44c0c840);
    write_volatile_4(0x44c0c840,(v & 0x7fffffff) | 0x80000000);
    // rc2_reflevdssscontthd_setf
    v = read_volatile_4(0x44c0c840);
    write_volatile_4(0x44c0c840,(v & 0xffc00000) | 0x100);
    // rc2_inbdpowfastvalid_cnt_setf
    v = read_volatile_4(0x44c0c82c);
    write_volatile_4(0x44c0c82c,(v & 0xff007fff) | 0x200000);
}

static const uint32_t agcdata[] = {
    0x20000000, 0x0400000f, 0x3000106f, 0x60000000, 0x04000031, 0x3000200f, 0x5b000000, 0x04000034,
    0x300030ef, 0x32000000, 0x04000057, 0x20000000, 0x0400000f, 0x2819008f, 0x08000106, 0x38008000,
    0x01000000, 0x08000012, 0x38018000, 0x10000821, 0x34200015, 0x38018000, 0x0c004203, 0x34200018,
    0x28018000, 0x3420001a, 0x38020005, 0x40000001, 0x0800001d, 0x30000005, 0x44002c01, 0x04000020,
    0x30000005, 0x3d000303, 0x04000023, 0x30000005, 0x3e000200, 0x04000026, 0x30000005, 0x9f000001,
    0x04000029, 0x30000005, 0x84020200, 0x0400002c, 0x380a208f, 0x1404053e, 0x0800002f, 0x2000339f,
    0x041dd40f, 0x38010000, 0x10000231, 0x08d0443f, 0x50002000, 0xa0000001, 0xe4000038, 0xdc00003c,
    0x58012000, 0x0d001401, 0x146dc83c, 0x0800003f, 0x38008000, 0x60000000, 0x08000031, 0x38012000,
    0x3e000101, 0x08000042, 0x3801a007, 0x3d000101, 0x08000045, 0x380280ef, 0x1f191010, 0x08000048,
    0x480a09ef, 0xa400004b, 0x0800002f, 0x20000c2f, 0x0400004d, 0x60000c2f, 0x30b00055, 0x28400053,
    0x04000051, 0x28100caf, 0x09b00069, 0x28080caf, 0x09b00069, 0x28050caf, 0x09b00069, 0x400004ef,
    0xac00005a, 0x04000060, 0x300004ef, 0x84020201, 0x0400005d, 0x300004ef, 0xa0000001, 0x04000060,
    0x380424ef, 0x1f1f1f11, 0x08000063, 0x30000cef, 0x20202031, 0x04000066, 0x48050eef, 0x6da04469,
    0x08000069, 0x60000eaf, 0x71b0446d, 0x71b00070, 0x04104473, 0x30000eaf, 0x29000300, 0x04000076,
    0x30000eaf, 0x29000100, 0x04000076, 0x30000eaf, 0x29000000, 0x04000076, 0x3801800f, 0x0ef1ef07,
    0x36008079, 0x20000000, 0x0400007b, 0x38024005, 0x3e000201, 0x0800007e, 0x30004007, 0x32000000,
    0x04000081, 0x30004007, 0x32000001, 0x04000084, 0x30004007, 0x3d000303, 0x04000087, 0x3000400f,
    0x3f000102, 0x0400008a, 0x3000400f, 0x20202071, 0x0400008d, 0x380442ef, 0x1f102021, 0x08000090,
    0x28104a6f, 0x08000092, 0x30004a6f, 0x4100000f, 0x04000095, 0x30004b6f, 0x21000003, 0x04000098,
    0x80004b6f, 0x71b0449d, 0x701044a0, 0x6c1044a0, 0x041044a3, 0x30004f6f, 0x29000301, 0x040000a6,
    0x30004f6f, 0x29000101, 0x040000a6, 0x30004f6f, 0x29000001, 0x040000a6, 0x38324bef, 0x34000000,
    0x080000a9, 0x30004b6f, 0x21000101, 0x040000ac, 0xe0004b6f, 0x79d044b4, 0x71b044b7, 0x741044ba,
    0x6c1044bd, 0x781044ba, 0x701044bd, 0x041044c0, 0x3000430f, 0x29030002, 0x040000f1, 0x3000430f,
    0x29000302, 0x040000c6, 0x3000430f, 0x29010002, 0x040000f1, 0x3000430f, 0x29000102, 0x040000c6,
    0x3000430f, 0x29000002, 0x040000c3, 0x3000030f, 0x32000000, 0x0400000f, 0x3000438f, 0x34000000,
    0x040000c9, 0x7896438f, 0x4100000f, 0x8c0000ce, 0x080000c3, 0x901044ec, 0x8864420f, 0xc00000d7,
    0x901044ec, 0x081044d3, 0x941044d5, 0x2000420f, 0x940450ee, 0x2000420f, 0x041044ee, 0x20004005,
    0x041044d9, 0x9832400f, 0x3d010101, 0x901044ec, 0x080000d3, 0x941044d5, 0xc40000df, 0x5802800f,
    0x0ef1ef07, 0x800000ec, 0x042044e3, 0x30004000, 0x33000001, 0x040000e6, 0x38034005, 0x3d030303,
    0x080000e9, 0x30004007, 0x33000000, 0x040000ec, 0x2000400f, 0x940450ee, 0x3801400f, 0x64000101,
    0x0810440f, 0x3000030f, 0x32000000, 0x040000f4, 0x3802030f, 0x0ef4ef07, 0x360080f7, 0x3000130f,
    0x32000100, 0x040000fa, 0x5aee130f, 0x4100000f, 0x7c000101, 0x080000fe, 0x3000030f, 0x32000000,
    0x0400000f, 0x492c110f, 0x8c000104, 0x080000fe, 0x2000100f, 0x9404500f, 0x3000008f, 0x0eeaed0b,
    0x36000109, 0x38038000, 0x34000000, 0x0800010c, 0x38010000, 0x0a000000, 0x0800010f, 0x28028005,
    0x08000111, 0x2000028f, 0x04000113, 0x2000028f, 0x04000115, 0x3000128f, 0x32000100, 0x04000118,
    0x5aee138f, 0x4100000f, 0x7c00011c, 0x080000fe, 0x592c138f, 0x29000000, 0x8c000120, 0x080000fe,
    0x2000128f, 0x9404500f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xc0088d03,
};

static void agc_download() {
    // FSM reset = 1
    uint32_t v = *(volatile uint32_t*)0x44c0b390;
    *(volatile uint32_t*)0x44c0b390 = v | (1ul << 12);
    // AGC mem clk force = 1
    v = *(volatile uint32_t*)0x44c00874;
    *(volatile uint32_t*)0x44c00874 = v | (1ul << 29);

    // upload AGC memory
    volatile uint32_t* agcram = (volatile uint32_t*)0x54c0a000;
    for (int i = 0; i < 512; i++)
        agcram[i] = agcdata[i];

    // AGC mem clk force = 0
    v = *(volatile uint32_t*)0x44c00874;
    *(volatile uint32_t*)0x44c00874 = v & ~(1 << 29);
    // FSM reset = 0
    v = *(volatile uint32_t*)0x44c0b390;
    *(volatile uint32_t*)0x44c0b390 = v & ~(1 << 12);
    // RC PA off delay
    v = *(volatile uint32_t*)0x44c0c020;
    *(volatile uint32_t*)0x44c0c020 = (v & 0xfc00ffff) | 0x140000;
} 

uint32_t phy_get_nss() {
    uint32_t v = *(volatile uint32_t*)0x44c00000;
    return ((v >> 8) & 0xf) - 1;
}

uint32_t phy_get_ntx() {
    uint32_t v = *(volatile uint32_t*)0x44c00000;
    return ((v >> 4) & 0xf) - 1;
}

uint32_t phy_get_chbw() {
    uint32_t v = *(volatile uint32_t*)0x44c00000;
    return (v >> 24) & 0x3;
}

uint32_t phy_get_nsts() {
    uint32_t v = *(volatile uint32_t*)0x44c00000;
    return ((v >> 12) & 0xf) - 1;
}

uint32_t phy_get_ldpcdec() {
    uint32_t v = *(volatile uint32_t*)0x44c00000;
    return (v >> 19) & 0x1;
}

uint32_t phy_get_ldpcenc() {
    uint32_t v = *(volatile uint32_t*)0x44c00000;
    return (v >> 18) & 0x1;
}

void phy_init() {
    // MDM conf 20Mhz
    *(volatile uint32_t*)0x44c00800 = 0;
    mdm_reset();
    // MDM rxchan
    *(volatile uint32_t*)0x44c00820 = 0x20d;
    // rxnssmax
    uint32_t v = *(volatile uint32_t*)0x44c00820;
    *(volatile uint32_t*)0x44c00820 = (v & 0xffffff8f) | (phy_get_nss() << 4);
    // rxndpnstsmax
    v = *(volatile uint32_t*)0x44c00820;
    *(volatile uint32_t*)0x44c00820 = (v & 0xffff8fff) | (phy_get_nsts() << 12);
    // rxldpcen
    v = *(volatile uint32_t*)0x44c00820;
    *(volatile uint32_t*)0x44c00820 = (v & 0xfffffeff) | (phy_get_ldpcdec() << 8);
    // rxvhten - 0, this chip has no VHT support
    v = *(volatile uint32_t*)0x44c00820;
    *(volatile uint32_t*)0x44c00820 = v & 0xfffffffd;
    // rxmumimoen - 0, this chip has no MU MIMO
    v = *(volatile uint32_t*)0x44c00820;
    *(volatile uint32_t*)0x44c00820 = v & 0xfffeffff;
    // r3024 precomp?
    v = *(volatile uint32_t*)0x44c03024;
    *(volatile uint32_t*)0x44c03024 = (v & 0xffc0ffff) | 0x2d0000;
    // MDM rxframe violation mask
    *(volatile uint32_t*)0x44c0089c = 0xffffffff;

    // MDM txchan
    *(volatile uint32_t*)0x44c00824 = 0x20d;
    // txnssmax
    v = *(volatile uint32_t*)0x44c00824;
    *(volatile uint32_t*)0x44c00824 = (v & 0xffffff8f) | (phy_get_nss() << 4);
    // ntxmax
    v = *(volatile uint32_t*)0x44c00824;
    *(volatile uint32_t*)0x44c00824 = (v & 0xff8fffff) | (phy_get_ntx() << 20);
    // txcbwmax
    v = *(volatile uint32_t*)0x44c00824;
    *(volatile uint32_t*)0x44c00824 = (v & 0xfcffffff) | (phy_get_chbw() << 24);
    // txldpcen
    v = *(volatile uint32_t*)0x44c00824;
    *(volatile uint32_t*)0x44c00824 = (v & 0xfffffeff) | (phy_get_ldpcenc() << 8);
    // vht - 0, this chip has no VHT support
    v = *(volatile uint32_t*)0x44c00824;
    *(volatile uint32_t*)0x44c00824 = v & 0xfffffffd;
    // txmumimoen - 0, this chip has no MU MIMO
    v = *(volatile uint32_t*)0x44c00824;
    *(volatile uint32_t*)0x44c00824 = v & 0xfffeffff;

    // AGC reset mode, rxtdctrl1
    v = *(volatile uint32_t*)0x44c00834;
    *(volatile uint32_t*)0x44c00834 = v | 1;

    // MDM smoothctrl disable cfgsmoothforce
    v = *(volatile uint32_t*)0x44c00818;
    *(volatile uint32_t*)0x44c00818 = v & 0xfffbffff;
    // set smooth mid high
    v = *(volatile uint32_t*)0x44c00830;
    *(volatile uint32_t*)0x44c00830 = (v & 0xffff0000) | 0x1b0f;

    // NDBPSMAX
    *(volatile uint32_t*)0x44c0083c = 0x4920492;
    // rcclkforce = 1
    v = *(volatile uint32_t*)0x44c00874;
    *(volatile uint32_t*)0x44c00874 = (v & 0xf7ffffff) | 0x8000000;
    // RIU txshift4044
    v = *(volatile uint32_t*)0x44c0b500;
    *(volatile uint32_t*)0x44c0b500 = (v & 0xffffcfff) | 0x2000;

    uint32_t iqcomp = (*(volatile uint32_t*)0x44c0b000) >> 21;
    if (iqcomp & 1) {
        v = *(volatile uint32_t*)0x44c0b110;
        *(volatile uint32_t*)0x44c0b110 = v & 0xfffffff0;
        *(volatile uint32_t*)0x44c0b118 = 0;
    }

    // limit RIU to 1 antenna
    *(volatile uint32_t*)0x44c0b004 = 1;
    // limit AGC with single antenna
    v = *(volatile uint32_t*)0x44c0b390;
    *(volatile uint32_t*)0x44c0b390 = (v & 0xfffffffc) | 1;
    // AGC CCA timeout
    *(volatile uint32_t*)0x44c0b3bc = 4000000;
    // IRQ MAC CCA timeout en
    v = *(volatile uint32_t*)0x44c0b414;
    *(volatile uint32_t*)0x44c0b414 = v | 0x100;

    agc_config();
    agc_download();
}
