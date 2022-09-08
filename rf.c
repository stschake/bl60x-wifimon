#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "utility.h"
#include "rf_data.h"

static uint16_t channel_cw_table[21];
static uint16_t channel_cnt_opt_table[40];

// for 40 MHz xtal
static const uint16_t channel_cnt_table[21] = {
    0xa6eb, 0xa732, 0xa779, 0xa7c0, 0xa808, 0xa84f, 
    0xa896, 0xa8dd, 0xa924, 0xa96b, 0xa9b2, 0xa9f9, 
    0xaa40, 0xaa87, 0xaacf, 0xab16, 0xab5d, 0xaba4, 
    0xabeb, 0xac32, 0xac79
};

void rf_fsm_ctrl_en_setf(int val) {
    *(volatile uint32_t*)0x40001004 = (val << 1) | (*(volatile uint32_t*)0x40001004 & 0xfffffffd);
}

void rf_fsm_lo_rdy_rst_setf(int val) {
    *(volatile uint32_t*)0x40001268 = (val << 17) | (*(volatile uint32_t*)0x40001268 & 0xfffdffff);
}

void rf_fsm_st_dbg_en_setf(int val) {
    *(volatile uint32_t*)0x4000126c = (val << 3) | (*(volatile uint32_t*)0x4000126c & 0xfffffff7);
}

void rf_fsm_st_dbg_setf(int val) {
    *(volatile uint32_t*)0x4000126c = val | (*(volatile uint32_t*)0x4000126c & 0xfffffff8);
}

void rfc_config_channel(uint32_t freq) {
    // rfif_int_lo_unlocked_mask_setf
    *(volatile uint32_t*)0x40001228 = *(volatile uint32_t*)0x40001228 | 8;
    // rf_lo_ctrl_hw_setf
    *(volatile uint32_t*)0x4000100c = *(volatile uint32_t*)0x4000100c | 0x241;
    // rf_ch_ind_wifi_setf
    *(volatile uint32_t*)0x40001264 = (freq & 0xfff) | (*(volatile uint32_t*)0x40001264 & ~0xfff);

    rf_fsm_lo_rdy_rst_setf(1);
    wait_us(10);
    rf_fsm_lo_rdy_rst_setf(0);
    wait_us(10);
    rf_fsm_ctrl_en_setf(0);
    wait_us(10);
    rf_fsm_ctrl_en_setf(1);
    wait_us(10);
    rf_fsm_st_dbg_setf(1);
    wait_us(10);
    rf_fsm_st_dbg_en_setf(1);
    wait_us(10);
    rf_fsm_st_dbg_setf(2);
    wait_us(10);
    rf_fsm_st_dbg_en_setf(0);
    wait_us(10);

    // TODO: notch
}

void rf_pri_start_rxdfe() {
    *(volatile uint32_t*)0x40001220 &= ~0x60;
    *(volatile uint32_t*)0x40001220 |= 0x21;
    *(volatile uint32_t*)0x40001220 |= 0x40;
}

void rf_pri_auto_gain() {
    *(volatile uint32_t*)0x4000100c |= 6;   
}

void rf_pri_stop_rxdfe() {
    *(volatile uint32_t*)0x40001220 &= ~0x61;
}

void rf_pri_stop_txdfe() {
    *(volatile uint32_t*)0x40001220 &= ~0x1982;
}

void rf_pri_start_txdfe() {
    *(volatile uint32_t*)0x40001220 &= 0xfffffe7f;
    *(volatile uint32_t*)0x40001220 = ((*(volatile uint32_t*)0x40001220) & 0xffffe7ff) | 0x1082;
    *(volatile uint32_t*)0x40001220 |= 0x100;
}

extern void rf_pri_fcal();
extern void rf_pri_lo_acal();
extern void rf_pri_roscal();
extern void rf_pri_rccal();

void rf_pri_full_cal() {
    rf_pri_start_rxdfe();
    rf_pri_start_txdfe();
    rf_pri_fcal();

    // don't seem to be necessary for RX
    //rf_pri_lo_acal();
    //rf_pri_roscal();
    //rf_pri_rccal();
    //rf_pri_txcal();

    rf_pri_auto_gain();
    rf_pri_stop_rxdfe();
    rf_pri_stop_txdfe();
}

void rf_pri_fixed_val_regs() {
    // dcdc18_vpfm_aon
    uint32_t v = read_volatile_4(0x4000f814);
    write_volatile_4(0x4000f814, (v & 0xfffff0ff) | 0x300);
    // sw_ldo11_rt_vout_sel
    v = read_volatile_4(0x4000f030);
    write_volatile_4(0x4000f030, (v & 0xf0ffffff) | 0x8000000);
    // RF pucr1
    v = read_volatile_4(0x40001030);
    write_volatile_4(0x40001030, v | 0x1001);
    // xtal_capcode_extra_aon
    v = read_volatile_4(0x4000f884);
    write_volatile_4(0x4000f884, v | 4);
    wait_us(10000);

    // RF pa1
    v = read_volatile_4(0x40001064);
    write_volatile_4(0x40001064,(v & 0xffff8008) | 0xc << 8 | 5 << 4 | 0x4002);
    // RF pa_reg_ctrl_hw1
    v = read_volatile_4(0x40001128);
    write_volatile_4(0x40001128, (v & 0xff800fff) | 0xc << 0x10 | 5 << 0xc | 0x400000);
    // RF pa_reg_ctrl_hw2
    v = read_volatile_4(0x4000112c);
    write_volatile_4(0x4000112c,
                    (((v & 0xfffff800) | 0xc << 4 | 5) & 0xff800fff) |
                    0x400 | 0xc << 0x10 | 5 << 0xc | 0x400000);
    // RF adda2.adc_gt_rm
    v = read_volatile_4(0x40001090);
    write_volatile_4(0x40001090,v | 0x10000);
    // RF fbdv.lo_fbdv_halfstep_en
    v = read_volatile_4(0x400010b8);
    write_volatile_4(0x400010b8, 1 << 4 | (v & 0xffffffef));
    // RF lo_reg_ctrl_hw1
    v = read_volatile_4(0x40001138);
    write_volatile_4(0x40001138, (v & 0xfffffffc) | 1 << 1 | 1);
    v = read_volatile_4(0x40001138);
    write_volatile_4(0x40001138, (v & 0xfffcfff7) | 0x300);
    // RF pa_reg_wifi_ctrl_hw
    v = read_volatile_4(0x40001130);
    write_volatile_4(0x40001130,v & 0xfffefffe);
}

extern uint16_t tmxcss[3];

void rf_pri_init(uint8_t reset, uint8_t chipv) {
    // reset assumed 1, chipv assumed 1
    // we have inlined all chipv stuff except the TX things this sets
    // rf_pri_chipv(1);

    rf_pri_fixed_val_regs();

    // PDS pu_rst_clkpll.clkpll_reset_postdiv
    uint32_t v = read_volatile_4(0x4000e400);
    write_volatile_4(0x4000e400, 1 << 1 | (v & 0xfffffffd));
    // PDS clkpll_sdm.clkpll_dither_sel
    v = read_volatile_4(0x4000e418);
    write_volatile_4(0x4000e418, 2 << 0x18 | (v & 0xfcffffff));
    // RF adda1.dac_dvdd_sel
    v = read_volatile_4(0x4000108c);
    write_volatile_4(0x4000108c,(v & 0xfffffffc) | 2);
    // HBN sw_ldo11soc_vout_sel_aon
    v = read_volatile_4(0x4000f030);
    write_volatile_4(0x4000f030,(v & 0xfff0ffff) | 0xc0000);

    // TX only?
    //rf_pri_set_gain_table_regs();

    // PDS clkpll_top_ctrl.clkpll_refclk_sel
    v = read_volatile_4(0x4000e404);
    write_volatile_4(0x4000e404,v | 0x10000);
    // PDS clkpll_output_en
    v = read_volatile_4(0x4000e41c);
    write_volatile_4(0x4000e41c,v | 0xff);

    // there is a soft reset option here where we just call rf_pri_restore_cal_reg()
    // but we always do full cal
    rf_pri_full_cal();
}

uint16_t rf_pri_fcal_meas(uint32_t cw) {
    // vco1 lo_vco_freq_cw
    uint32_t v = *(volatile uint32_t*)0x400010a0;
    *(volatile uint32_t*)0x400010a0 = (v & ~0xff) | cw;

    wait_us(100);

    // vco4 fcal_cnt_start = 1
    v = *(volatile uint32_t*)0x400010ac;
    *(volatile uint32_t*)0x400010ac = v | 0x10;

    // wait for count ready
    do {
        v = *(volatile uint32_t*)0x400010ac;
        // vco4 fcal_cnt_ready
        if ((v >> 20) & 1)
            break;
    } while (1);

    // vco3 fcal_cnt_op
    uint16_t meas = (*(volatile uint32_t*)0x400010a8) >> 16;

    // vco4 fcal_cnt_start = 0
    v = *(volatile uint32_t*)0x400010ac;
    *(volatile uint32_t*)0x400010ac = v & ~0x10;
    return meas;
}

static struct {
    uintptr_t ptr;
    uint32_t value;
} rf_pri_state[] = {
    { 0x40001004, 0 }, // rf_fsm_ctrl_hw
    { 0x4000100c, 0 }, // rfctrl_hw_en
    { 0x4000101c, 0 }, // rfcal_ctrlen
    { 0x40001030, 0 }, // pucr1
    { 0x400010b8, 0 }, // fbdv
    { 0x400010c0, 0 }, // sdm1
    { 0x400010c4, 0 }, // sdm2
    { 0x40001084, 0 }, // rbb3
    { 0x4000108c, 0 }, // adda1
    { 0x40001600, 0 }, // dfe_ctrl_0
    { 0x4000160c, 0 }, // dfe_ctrl_3
    { 0x40001618, 0 }, // dfe_ctrl_6
    { 0x4000161c, 0 }, // dfe_ctrl_7
    { 0x40001048, 0 }, // trx_gain1
    { 0x4000120c, 0 }, // singen_ctrl0
    { 0x40001214, 0 }, // singen_ctrl2
    { 0x40001218, 0 }, // singen_ctrl3
    { 0x4000123c, 0 }, // sram_ctrl0
    { 0x40001240, 0 }, // sram_ctrl1
    { 0x40001244, 0 }, // sram_ctrl2
    { 0x400010f0, 0 }, // rf_resv_reg1
    { 0x40001064, 0 }, // pa1
    { 0x40001058, 0 }, // ten_ac
    { 0x40001220, 0 }, // rfif_dfe_ctrl0
    { 0x40001070, 0 }, // tbb
    { 0x400010a4, 0 }, // vco2
};

void rf_pri_save_state_for_cal() {
    const int n = sizeof(rf_pri_state)/sizeof(rf_pri_state[0]);
    for (int i = 0; i < n; i++) {
        rf_pri_state[i].value = *(volatile uint32_t*)rf_pri_state[i].ptr;
    }
}

void rf_pri_restore_state_for_cal() {
    const int n = sizeof(rf_pri_state)/sizeof(rf_pri_state[0]);
    for (int i = 0; i < n; i++) {
        *(volatile uint32_t*)rf_pri_state[i].ptr = rf_pri_state[i].value;
    }
}

// rf_pri_manu_pu(7)
void rf_pri_fcal_mode() {
    // rf_fsm_ctrl_en = 0
    uint32_t v = *(volatile uint32_t*)0x40001004;
    *(volatile uint32_t*)0x40001004 = v & ~2;
    // rfctrl_hw_en = 0
    *(volatile uint32_t*)0x4000100c = 0;

    // RF pucr1
    v = *(volatile uint32_t*)0x40001030;
    // lna, rmxgm, rmx, tosdac, rbb, adc
    v &= ~((1 << 8) | (1 << 9) | (1 << 10) | (1 << 31) | (1 << 11) | (1 << 14));
    // adc_clk_en, pkdet, rosdac, pwrmax, pa, tmx
    v &= ~((1 << 13) | (1 << 28) | (1 << 29) | (1 << 30) | (1 << 16) | (1 << 17));
    // tbb, dac, rxbuf, txbuf, trsw_en
    v &= ~((1 << 18) | (1 << 19) | (1 << 24) | (1 << 25) | (1 << 26));
    // osmx, pfd, fbdv, vco
    v |= (1 << 23) | (1 << 22) | (1 << 21) | (1 << 20);
    *(volatile uint32_t*)0x40001030 = v;
}

void rf_pri_fcal() {
    // fcal status = 1
    uint32_t v = *(volatile uint32_t*)0x40001014;
    *(volatile uint32_t*)0x40001014 = (v & 0xffffffcf) | 0x10;

    rf_pri_save_state_for_cal();
    rf_pri_fcal_mode();

    // rfcal_ctrlen fcal_en = 1
    v = *(volatile uint32_t*)0x4000101c;
    *(volatile uint32_t*)0x4000101c = v | 8;
    // vco1 lo_vco_freq_cw = 0x80
    v = *(volatile uint32_t*)0x400010a0;
    *(volatile uint32_t*)0x400010a0 = (v & ~0xff) | 0x80;
    // fbdv lo_fbdv_sel_fb_clk = 0
    v = *(volatile uint32_t*)0x400010b8;
    *(volatile uint32_t*)0x400010b8 = v & 0xffffcfff;
    // vco3 fcal_div = 2133 (for 40 MHz xtal)
    v = *(volatile uint32_t*)0x400010a8;
    *(volatile uint32_t*)0x400010a8 = (v & 0xffff0000) | 2133;

    // RF sdm2
    *(volatile uint32_t*)0x400010c4 = 0x1000000;
    // sdm1 lo_sdm_bypass = 1
    v = *(volatile uint32_t*)0x400010c0;
    *(volatile uint32_t*)0x400010c0 = v | 0x1000;

    // sdm1 lo_sdm_rstb = 0
    v = *(volatile uint32_t*)0x400010c0;
    *(volatile uint32_t*)0x400010c0 = v & 0xfffeffff;
    // fbdv lo_fbdv_rst = 1
    v = *(volatile uint32_t*)0x400010b8;
    *(volatile uint32_t*)0x400010b8 = v | 0x10000;
    wait_us(10);
    // sdm1 lo_sdm_rstb = 1
    v = *(volatile uint32_t*)0x400010c0;
    *(volatile uint32_t*)0x400010c0 = v | 0x10000;
    // fbdv lo_fbdv_rst = 0
    v = *(volatile uint32_t*)0x400010b8;
    *(volatile uint32_t*)0x400010b8 = v & 0xfffeffff;
    wait_us(50);

    // vco2 lo_vco_vbias_cw = 2
    v = *(volatile uint32_t*)0x400010a4;
    *(volatile uint32_t*)0x400010a4 = (v & 0xfffffffc) | 2;
    wait_us(50);

    // channel count ranges for 40MHz xtal
    const uint16_t c0 = 0xa6a0;
    const uint16_t c1 = 0xa6e0;
    const uint16_t c2 = 0xace0;
    uint16_t cw = 0x80;
    while (1) {
        for (int j = 6; j >= 0; j--) {
            int cnt = rf_pri_fcal_meas(cw);
            if (cnt < c0) {
                cw -= (1 << j);
            } else {
                if (cnt > c1) {
                    cw += (1 << j);
                } else {
                    break;
                }
            }
        }

        if (cw >= 15)
            break;

        cw = 0x80;
        // sdm1 lo_sdm_rstb = 0
        v = *(volatile uint32_t*)0x400010c0;
        *(volatile uint32_t*)0x400010c0 = v & 0xfffeffff;
        // fbdv lo_fbdv_rst = 1
        v = *(volatile uint32_t*)0x400010b8;
        *(volatile uint32_t*)0x400010b8 = v | 0x10000;
        wait_us(50);
        // sdm1 lo_sdm_rstb = 1
        v = *(volatile uint32_t*)0x400010c0;
        *(volatile uint32_t*)0x400010c0 = v | 0x10000;
        // fbdv lo_fbdv_rst = 0
        v = *(volatile uint32_t*)0x400010b8;
        *(volatile uint32_t*)0x400010b8 = v & 0xfffeffff;
        wait_us(50);
    }
    cw++;
    channel_cnt_opt_table[0] = rf_pri_fcal_meas(cw);
    for (int i = 1; i < 40; i++) {
        uint16_t cnt = rf_pri_fcal_meas(cw-i);
        channel_cnt_opt_table[i] = cnt;
        if (cnt > c2)
            break;
    }
    for (int i = 0, j = 0; i < 21; i++) {
        while (channel_cnt_opt_table[j] < channel_cnt_table[i])
            j++;
        channel_cw_table[i] = cw - (j - 1);
        if (j > 0)
            j++;
    }

    rf_pri_restore_state_for_cal();
    for (int i = 0; i < 21; i++) {
        rf_calib_data->lo[i].fcal = channel_cw_table[i];
    }

    // fcal status = 3
    v = *(volatile uint32_t*)0x40001014;
    *(volatile uint32_t*)0x40001014 = v | 0x30;
}

