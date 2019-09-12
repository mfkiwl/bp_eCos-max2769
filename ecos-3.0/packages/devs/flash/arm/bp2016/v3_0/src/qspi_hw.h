#ifndef QSPI_HW_H__
#define QSPI_HW_H__

enum qspi_sr_status {
    QSPI_SR_RX_FULL,
    QSPI_SR_TX_EMPTY,
    QSPI_SR_TX_OVERFLOW,
    QSPI_SR_RX_UNDERFLOW,
    QSPI_SR_RX_OVERFLOW,
    QSPI_SR_NONE
};

inline void qspi_hw_init(void);
inline void qspi_ahb_enable(void);
inline void qspi_reg_enable(void);
inline void qspi_push_data_8bit(cyg_uint8 d);
inline void qspi_push_data_16bit(cyg_uint16 d);
inline void qspi_push_data_32bit(cyg_uint32 d);
inline cyg_uint8 qspi_get_data_8bit(void);
inline cyg_uint16 qspi_get_data_16bit(void);
inline cyg_uint32 qspi_get_data_32bit(void);
inline void qspi_set_rx_fifo_level(int rft);
inline void qspi_set_tx_fifo_level(int tft);
inline int qspi_get_rx_fifo_d_num(void);
inline int qspi_get_tx_fifo_d_num(void);
inline void qspi_set_baudr(cyg_uint32 clk_div);
inline cyg_uint32 mask_all_interrupts(void);
inline void set_interrupts_mask(cyg_uint32 imr);
inline cyg_uint32 qspi_clr_all_interrupts(void);
inline cyg_uint32 qspi_get_interrupt_status(void);
inline void qspi_rx_full_inten(int en);
inline void qspi_tx_empty_inten(int en);
inline void qspi_set_rx_ndf(cyg_uint32 ndf);
inline bool qspi_is_busy(void);
inline void qspi_tx_init(void);
inline bool qspi_rx_init(cyg_uint8 cmd, cyg_uint8 dfs);
int qspi_polling_raw_status(void);
int qspi_irq_read_status(void);

inline cyg_uint32 qspi_rx_fifo_flush(void);
inline cyg_uint32 qspi_tx_fifo_flush(void);
inline cyg_uint32 qspi_cache_flush(void);
inline void qspi_set_dma_ctrl(cyg_uint32 en);
inline void qspi_set_dma_tdlr(cyg_uint32 tdlr);
inline void qspi_set_dma_rdlr(cyg_uint32 rdlr);

#endif
