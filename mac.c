#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

static inline void blmac_enable_imp_pri_tbtt_setf(uint8_t imppritbtt) {
    if (imppritbtt)
        *(volatile uint32_t*)0x44b08074 |= 1;
    else
        *(volatile uint32_t*)0x44b08074 &= ~1;
}

static inline void blmac_enable_imp_sec_tbtt_setf(uint8_t impsectbtt) {
    if (impsectbtt)
        *(volatile uint32_t*)0x44b08074 |= 2;
    else
        *(volatile uint32_t*)0x44b08074 &= ~2;
}

static inline uint32_t blmac_mac_cntrl_1_get() {
    return *(volatile uint32_t*)0x44b0004c;
}

static inline void blmac_mac_cntrl_1_set(uint32_t v) {
    *(volatile uint32_t*)0x44b0004c = v;
}

static inline void blmac_abgn_mode_setf(uint32_t mode) {
    *(volatile uint32_t*)0x44b0004c = (*(volatile uint32_t*)0x44b0004c & 0xfffe3fff) | ((mode & 0x7) << 14);
}

static inline void blmac_key_sto_ram_reset() {
    *(volatile uint32_t*)0x44b0004c = (*(volatile uint32_t*)0x44b0004c & 0xffffdfff) | (1 << 13);
}

void mac_monitor_mode(void) {
    blmac_enable_imp_pri_tbtt_setf(0);
    blmac_enable_imp_sec_tbtt_setf(0);
    // disable ACK / CTA / BAR resp
    blmac_mac_cntrl_1_set(blmac_mac_cntrl_1_get() | 0x700);

    // rx cntrl: accept (essentially) all
    *(volatile uint32_t*)0x44b00060 = 0x7fffffde;
    // uncomment to filter out beacons
    //*(volatile uint32_t*)0x44b00060 &= ~((1 << 10) | (1 << 13));
    
    blmac_abgn_mode_setf(3);
    blmac_key_sto_ram_reset();
}

void blmac_active_clk_gating_setf(uint32_t value) {
    *(volatile uint32_t*)0x44b0004c = ((*(volatile uint32_t*)0x44b0004c) & 0xffffff7f) | (value << 7);
}

void blmac_next_state_setf(uint32_t state) {
    *(volatile uint32_t*)0x44b00038 = state;
}

uint32_t blmac_current_state_getf() {
    return (*(volatile uint32_t*)0x44b00038) & 0xf;
}

void blmac_mac_err_rec_cntrl_set(uint32_t value) {
    *(volatile uint32_t*)0x44b00054 = value;
}

extern void blmac_rx_flow_cntrl_en_setf(uint32_t value);

uint32_t blmac_timers_int_un_mask_get() {
    return *(volatile uint32_t*)0x44b0808c;
}

void blmac_timers_int_un_mask_set(uint32_t value) {
    *(volatile uint32_t*)0x44b0808c = value;
}

void blmac_tx_rx_int_ack_clear() {
    *(volatile uint32_t*)0x44b0807c = 0xffffffff;
}

void blmac_enable_master_gen_int_en_setf(uint32_t value) {
    *(volatile uint32_t*)0x44b08074 = ((*(volatile uint32_t*)0x44b08074) & 0x7fffffff) | (value << 31);
}

void blmac_enable_master_tx_rx_int_en_setf(uint32_t value) {
    *(volatile uint32_t*)0x44b08080 = ((*(volatile uint32_t*)0x44b08080) & 0x7fffffff) | (value << 31);
}

uint32_t hal_machw_time() {
    return *(volatile uint32_t*)0x44b00120;
}

void blmac_abs_timer_set(uint32_t v) {
    *(volatile uint32_t*)0x44b0013c = v;
}

void blmac_timers_int_event_clear(uint32_t ev) {
    *(uint32_t*)0x44b08088 = ev;
}

void mac_idle_req() {
    if (blmac_current_state_getf() == 0)
        return;

    __asm volatile( "csrc mstatus, 8" );
    blmac_abs_timer_set(hal_machw_time() + 50000);
    blmac_timers_int_event_clear(0x20);
    blmac_timers_int_un_mask_set(blmac_timers_int_un_mask_get() | 0x20);
    blmac_next_state_setf(0);
    __asm volatile( "csrs mstatus, 8" );
}

void mac_reset() {
    blmac_active_clk_gating_setf(0);
    blmac_next_state_setf(0);
    blmac_mac_err_rec_cntrl_set(0x7c);
    while (blmac_current_state_getf() != 0) { };
    blmac_rx_flow_cntrl_en_setf(1);
    blmac_timers_int_un_mask_set(blmac_timers_int_un_mask_get() & 0xffffffc0);
    blmac_tx_rx_int_ack_clear();
    blmac_enable_master_gen_int_en_setf(1);
    blmac_enable_master_tx_rx_int_en_setf(1);
    blmac_active_clk_gating_setf(1);
}

void blmac_gen_int_ack_clear(uint32_t mask) {
    *(volatile uint32_t*)0x44b08070 = mask;
}

uint32_t blmac_gen_int_status_get() {
    return *(uint32_t*)0x44b0806c;
}

uint32_t blmac_gen_int_enable_get() {
    return *(uint32_t*)0x44b08074;
}

extern void ke_evt_set(uint32_t ev);

uint32_t blmac_timers_int_event_get() {
    return *(uint32_t*)0x44b08084;
}

void hal_machw_abs_timer_handler() {
    uint32_t ev = blmac_timers_int_event_get();
    blmac_timers_int_event_clear(ev);
}

void mac_gen_handler() {
    uint32_t genint = blmac_gen_int_status_get() & blmac_gen_int_enable_get();
    blmac_gen_int_ack_clear(genint);

    if (genint & 0x00008)
        hal_machw_abs_timer_handler();
}

void blmac_rx_cntrl_set(uint32_t v) {
    *(volatile uint32_t*)0x44b00060 = v;
}

void blmac_soft_reset_setf(uint32_t v) {
    *(volatile uint32_t*)0x44b08050 = v;
}

uint32_t blmac_soft_reset_getf() {
    return (*(volatile uint32_t*)0x44b08050) & 1;
}

void blmac_mac_core_clk_freq_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000e4;
    *(volatile uint32_t*)0x44b000e4 = (temp & ~0xff) | (v & 0xff);
}

void blmac_tx_rf_delay_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000e4;
    *(volatile uint32_t*)0x44b000e4 = (temp & 0xfffc00ff) | ((v & 0x3ff) << 8);
}

void blmac_tx_chain_delay_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000e4;
    *(volatile uint32_t*)0x44b000e4 = (temp & 0xf003ffff) | ((v & 0x3ff) << 18);
}

void blmac_rx_rf_delay_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000ec;
    *(volatile uint32_t*)0x44b000ec = (temp & 0xc00fffff) | ((v & 0x3ff) << 20);
}

void blmac_tx_delay_rf_on_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000ec;
    *(volatile uint32_t*)0x44b000ec = (temp & 0xfff003ff) | ((v & 0x3ff) << 10);
}

void blmac_mac_proc_delay_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000ec;
    *(volatile uint32_t*)0x44b000ec = (temp & 0xfffffc00) | (v & 0x3ff);
}

void blmac_wt_2_crypt_clk_ratio_setf(uint8_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000f0;
    *(volatile uint32_t*)0x44b000f0 = (temp & ~0x3) | (v & 0x3);
}

void blmac_sifs_b_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000f4;
    *(volatile uint32_t*)0x44b000f4 = (temp & 0xff0000ff) | ((v & 0xffff) << 8);
}

void blmac_sifs_a_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b000f8;
    *(volatile uint32_t*)0x44b000f8 = (temp & 0xff0000ff) | ((v & 0xffff) << 8);
}

void blmac_rifs_to_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b00104;
    *(volatile uint32_t*)0x44b00104 = (temp & 0xc00fffff) | ((v & 0x3ff) << 20);
}

void blmac_rifs_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b00104;
    *(volatile uint32_t*)0x44b00104 = (temp & 0xfff003ff) | ((v & 0x3ff) << 10);
}

void blmac_tx_dma_proc_dly_in_mac_clk_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b00104;
    *(volatile uint32_t*)0x44b00104 = (temp & 0xfffffc00) | (v & 0x3ff);
}

void mac_setfreq() {
    blmac_mac_core_clk_freq_setf(40 /*MHz*/);
    // no idea what they end up setting here..
    blmac_tx_rf_delay_in_mac_clk_setf(0);
    blmac_tx_chain_delay_in_mac_clk_setf(136);
    // no idea what they end up setting here..
    // blmac_slot_time_in_mac_clk_setf(..)
    blmac_rx_rf_delay_in_mac_clk_setf(39);
    // no idea what they end up setting here..
    // blmac_tx_delay_rf_on_in_mac_clk_setf();
    blmac_mac_proc_delay_in_mac_clk_setf(180);
    blmac_wt_2_crypt_clk_ratio_setf(3);
    // no idea what they end up setting here..
    // blmac_sifs_b_in_mac_clk_setf();
    // blmac_sifs_a_in_mac_clk_setf();
    // blmac_rifs_to_in_mac_clk_setf();
    // blmac_rifs_in_mac_clk_setf();
    // blmac_tx_dma_proc_dly_in_mac_clk_setf();
}

void blmac_gen_int_enable_set(uint32_t v) {
    *(volatile uint32_t*)0x44b08074 = v;
}

void blmac_rate_controller_mpif_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b0004c;
    *(volatile uint32_t*)0x44b0004c = (temp & 0xfffff7ff) | ((v & 0x1) << 11);
}

void blmac_rx_flow_cntrl_en_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b00054;
    *(volatile uint32_t*)0x44b00054 = (temp & 0xfffeffff) | ((v & 0x1) << 16);
}

void blmac_max_rx_length_set(uint32_t v) {
    *(volatile uint32_t*)0x44b00150 = v & 0xfffff;
}

void blmac_mib_table_reset_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b0004c;
    *(volatile uint32_t*)0x44b0004c = (temp & 0xffffefff) | ((v & 0x1) << 12);
}

void blmac_key_sto_ram_reset_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b0004c;
    *(volatile uint32_t*)0x44b0004c = (temp & 0xffffdfff) | ((v & 0x1) << 13);
}

void blmac_dyn_bw_en_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b00310;
    *(volatile uint32_t*)0x44b00310 = (temp & 0xffffff7f) | ((v & 0x1) << 7);
}

void blmac_max_phy_ntx_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b0009c;
    *(volatile uint32_t*)0x44b0009c = (temp & 0xe3ffffff) | ((v & 0x7) << 26);
}

void blmac_tsf_mgt_disable_setf(uint32_t v) {
    uint32_t temp = *(volatile uint32_t*)0x44b0004c;
    *(volatile uint32_t*)0x44b0004c = (temp & 0xfdffffff) | ((v & 0x1) << 25);
}

void mac_active() {
    // blmac next state
    *(volatile uint32_t*)0x44b00038 = 0x30;
}

void mac_stop() {
    blmac_soft_reset_setf(1);
    while (blmac_soft_reset_getf() != 0);
}

void mac_init() {
    // this is more of a reset
    mac_stop();

    // coex stuff
    *(volatile uint32_t*)0x44b00404 = 0x24f637;
    // toggling coex enable to reset?
    *(volatile uint32_t*)0x44b00400 = 1;
    *(volatile uint32_t*)0x44b00400 = 0;
    *(volatile uint32_t*)0x44b00400 = 0x69;
    // disable coexAutoPTIAdj
    *(volatile uint32_t*)0x44b00400 &= ~0x20;
    // sysctrl92 ptr config
    *(volatile uint32_t*)0x44920004 = 0x5010001f;

    mac_setfreq();
    blmac_gen_int_enable_set(0x8373f14c);
    blmac_rate_controller_mpif_setf(0);
    
    // encryption ram config
    *(volatile uint32_t*)0x44b000d8 = 0x21108;
    // tx rx int enable
    *(volatile uint32_t*)0x44b08080 = 0x800a07c0;
    // mac cntrl settings: activeClkGating, disable(ACK|CTS|BAR)Resp, rxRIFSEn
    *(volatile uint32_t*)0x44b0004c |= 0x4000780;
    blmac_rx_flow_cntrl_en_setf(1);
    blmac_rx_cntrl_set(0x7fffffde);
    // rx trigger timer
    *(volatile uint32_t*)0x44b00114 = 0x3010a;
    // beacon control 1
    *(volatile uint32_t*)0x44b00064 = 0xff900064;
    blmac_max_rx_length_set(0x1000);
    // edca control
    *(volatile uint32_t*)0x44b00224 = 0;
    // max power level (ofdm/dsss = 32)
    *(volatile uint32_t*)0x44b000a0 = 0x2020;
    blmac_mib_table_reset_setf(1);
    blmac_key_sto_ram_reset_setf(1);
    // debug port sel
    *(volatile uint32_t*)0x44b00510 = 0x1c25;
    blmac_dyn_bw_en_setf(1);
    // they set phy_get_ntx() + 1, no idea what that value is
    // blmac_max_phy_ntx_setf();
    blmac_tsf_mgt_disable_setf(1);
}
