#ifndef RE_RXU_CNTRL
#define RE_RXU_CNTRL

#include <stdint.h>
#include <stdbool.h>

#include "utility.h"

struct rx_pbd {
    uint32_t upattern; // +0
    struct rx_pbd* next; // +4
    uint8_t* datastartptr; // +8
    uint8_t* dataendptr; // +12
    uint16_t bufstatinfo; // +16
    uint16_t reserved; // +18
}; // :816:8

struct rx_hd {
    uint32_t upatternrx; // +0
    struct rx_dmadesc *next; // +4
    struct rx_pbd *first_pbd_ptr; // +8
    struct rx_swdesc *swdesc; // +12
    uint8_t* datastartptr; // +16
    uint8_t* dataendptr; // +20
    uint32_t headerctrlinfo; // +24
    uint16_t frmlen; // +28

    union {
        uint16_t value;
        struct {
            uint16_t reserved : 8;
            uint16_t mpdu_cnt : 6;
            uint16_t ampdu_cnt : 2;
        };
    } ampdu_stat_info; // +30

    uint32_t tsflo; // +32
    uint32_t tsfhi; // +36

    union {
        uint32_t value;
        struct {
            uint32_t leg_length : 12;
            uint32_t leg_rate : 4;
            uint32_t ht_length : 16;
        };
    } recvec1a; // +40

    union {
        uint32_t value;
        struct {
            uint32_t _ht_length : 4;
            uint32_t short_gi : 1;
            uint32_t stbc : 2;
            uint32_t smoothing : 1;
            uint32_t mcs : 7;
            uint32_t pre_type : 1;
            uint32_t format_mod : 3;
            uint32_t ch_bw : 2;
            uint32_t n_sts : 3;
            uint32_t lsig_valid : 1;
            uint32_t sounding : 1;
            uint32_t num_extn_ss : 2;
            uint32_t aggregation : 1;
            uint32_t fec_coding : 1;
            uint32_t dyn_bw : 1;
            uint32_t doze_not_allowed : 1;
        };
    } recvec1b; // +44

    union {
        uint32_t value;
        struct {
            uint32_t antenna_set : 8;
            uint32_t partial_aid : 9;
            uint32_t group_id : 6;
            uint32_t reserved_1c : 1;
            int32_t rssi1 : 8;
        };
    } recvec1c; // +48

    union {
        uint32_t value;
        struct {
            int32_t rssi2 : 8;
            int32_t rssi3 : 8;
            int32_t rssi4 : 8;
            uint32_t reserved_1d : 8;
        };
    } recvec1d; // +52

    union {
        uint32_t value;
        struct {
            uint32_t rcpi : 8;
            uint32_t evm1 : 8;
            uint32_t evm2 : 8;
            uint32_t evm3 : 8;
        };
    } recvec2a; // +56

    union {
        uint32_t value;
        struct {
            uint32_t evm4 : 8;
            uint32_t reserved2b_1 : 8;
            uint32_t reserved2b_2 : 8;
            uint32_t reserved2b_3 : 8;
        };
    } recvec2b; // +60

    union {
        uint32_t value;
        struct {
            /** Status **/
            uint32_t rx_vect2_valid     : 1;
            uint32_t resp_frame         : 1;
            /** Decryption Status */
            uint32_t decr_status        : 3;
            uint32_t rx_fifo_oflow      : 1;

            /** Frame Unsuccessful */
            uint32_t undef_err          : 1;
            uint32_t phy_err            : 1;
            uint32_t fcs_err            : 1;
            uint32_t addr_mismatch      : 1;
            uint32_t ga_frame           : 1;
            uint32_t current_ac         : 2;

            uint32_t frm_successful_rx  : 1;
            /** Descriptor Done  */
            uint32_t desc_done_rx       : 1;
            /** Key Storage RAM Index */
            uint32_t key_sram_index     : 10;
            /** Key Storage RAM Index Valid */
            uint32_t key_sram_v         : 1;
            uint32_t type               : 2;
            uint32_t subtype            : 4;
        };
    } statinfo; // +64
}; // :774:8
_Static_assert (sizeof(struct rx_hd) == 68, "wrong rx_hd size");

struct phy_channel_info {
    /** PHY channel information 1 */
    uint32_t phy_band           : 8;
    uint32_t phy_channel_type   : 8;
    uint32_t phy_prim20_freq    : 16;
    /** PHY channel information 2 */
    uint32_t phy_center1_freq   : 16;
    uint32_t phy_center2_freq   : 16;
};
_Static_assert (sizeof(struct phy_channel_info) == 8, "wrong phy_channel_info size");

struct rx_dmadesc {
    struct rx_hd hd; // +0
    struct phy_channel_info phy_info; // +68

    union {
        struct {
            uint32_t is_amsdu     : 1;
            uint32_t is_80211_mpdu: 1;
            uint32_t is_4addr     : 1;
            uint32_t new_peer     : 1;
            uint32_t user_prio    : 3;
            uint32_t rsvd0        : 1;
            uint32_t vif_idx      : 8;    // 0xFF if invalid VIF index
            uint32_t sta_idx      : 8;    // 0xFF if invalid STA index
            uint32_t dst_idx      : 8;    // 0xFF if unknown destination STA
        };
        uint32_t value;
    } flags; // +76

    uint32_t pattern; // +80
    uint32_t payl_offset; // +84
    uint32_t reserved_pad[2]; // +88
    uint32_t use_in_tcpip; // +96
}; // :834:8

struct rx_payloaddesc {
    struct rx_pbd pbd; // +0
    uint32_t pd_status; // +20
    uint8_t *buffer_rx; // +24
    void *pbuf_holder[6]; // +28
}; // :854:8

struct rx_swdesc {
    struct list_header list_hdr; // +0
    struct rx_dmadesc *dma_hdrdesc; // +4
    struct rx_payloaddesc *pd; // +8
    struct rx_pbd *last_pbd; // +12
    struct rx_pbd *spare_pbd; // +16
    uint32_t host_id; // +20
    uint32_t frame_len; // +24
    uint8_t status; // +28
    uint8_t pbd_count; // +29
    uint8_t use_in_tcpip; // +30
}; // :52:8

typedef void (*frame_handler)(struct rx_swdesc* swdesc);

void rx_timer_int_handler();
bool rx_any_ready();
void rx_process(frame_handler handler);
void rx_init();

#endif
