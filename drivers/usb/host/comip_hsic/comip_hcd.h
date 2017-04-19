/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/comip_hcd.h $
 * $Revision: #57 $
 * $Date: 2010/11/29 $
 * $Change: 1636033 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */
#ifndef __COMIP_HCD_H__
#define __COMIP_HCD_H__

#include "usb.h"
#include "comip_hcd_if.h"
#include "comip_core_if.h"
#include "comip_list.h"
#include "comip_cil.h"

/**
 * @file
 *
 * This file contains the structures, constants, and interfaces for
 * the Host Contoller Driver (HCD).
 *
 * The Host Controller Driver (HCD) is responsible for translating requests
 * from the USB Driver into the appropriate actions on the comip controller.
 * It isolates the USBD from the specifics of the controller by providing an
 * API to the USBD.
 */

/*
 * Circular queue definitions.
 */
#define COMIP_CIRCLEQ_HEAD(name, type)                 \
	struct name {								\
		struct type *cqh_first; 	/* first element */ 	\
		struct type *cqh_last;		/* last element */		\
	}

struct comip_hcd_pipe_info {
    uint8_t dev_addr;
    uint8_t ep_num;
    uint8_t pipe_type;
    uint8_t pipe_dir;
    uint16_t mps;
};

struct comip_hcd_iso_packet_desc {
    uint32_t offset;
    uint32_t length;
    uint32_t actual_length;
    uint32_t status;
};

struct comip_qtd;

struct comip_hcd_urb {
    void *priv;
    struct comip_qtd *qtd;
    void *buf;
    dma_addr_t dma;
    void *setup_packet;
    dma_addr_t setup_dma;
    uint32_t length;
    uint32_t actual_length;
    uint32_t status;
    uint32_t error_count;
    uint32_t packet_count;
    uint32_t flags;
    uint16_t interval;
    struct comip_hcd_pipe_info pipe_info;
    struct comip_hcd_iso_packet_desc iso_descs[0];
};

static inline uint8_t comip_hcd_get_ep_num(struct comip_hcd_pipe_info *pipe)
{
    return pipe->ep_num;
}

static inline uint8_t comip_hcd_get_pipe_type(struct comip_hcd_pipe_info
                        *pipe)
{
    return pipe->pipe_type;
}

static inline uint16_t comip_hcd_get_mps(struct comip_hcd_pipe_info *pipe)
{
    return pipe->mps;
}

static inline uint8_t comip_hcd_get_dev_addr(struct comip_hcd_pipe_info *pipe)
{
    return pipe->dev_addr;
}

static inline uint8_t comip_hcd_is_pipe_isoc(struct comip_hcd_pipe_info *pipe)
{
    return (pipe->pipe_type == UE_ISOCHRONOUS);
}

static inline uint8_t comip_hcd_is_pipe_int(struct comip_hcd_pipe_info *pipe)
{
    return (pipe->pipe_type == UE_INTERRUPT);
}

static inline uint8_t comip_hcd_is_pipe_bulk(struct comip_hcd_pipe_info *pipe)
{
    return (pipe->pipe_type == UE_BULK);
}

static inline uint8_t comip_hcd_is_pipe_control(struct comip_hcd_pipe_info *pipe)
{
    return (pipe->pipe_type == UE_CONTROL);
}

static inline uint8_t comip_hcd_is_pipe_in(struct comip_hcd_pipe_info *pipe)
{
    return (pipe->pipe_dir == UE_DIR_IN);
}

static inline uint8_t comip_hcd_is_pipe_out(struct comip_hcd_pipe_info *pipe)
{
    return (!comip_hcd_is_pipe_in(pipe));
}

static inline void comip_hcd_fill_pipe(struct comip_hcd_pipe_info *pipe,
    uint8_t devaddr, uint8_t ep_num, uint8_t pipe_type, uint8_t pipe_dir, uint16_t mps)
{
    pipe->dev_addr = devaddr;
    pipe->ep_num = ep_num;
    pipe->pipe_type = pipe_type;
    pipe->pipe_dir = pipe_dir;
    pipe->mps = mps;
}

/**
 * Phases for control transfers.
 */
typedef enum comip_control_phase {
    COMIP_CONTROL_SETUP,
    COMIP_CONTROL_DATA,
    COMIP_CONTROL_STATUS
} comip_control_phase_e;

/** Transaction types. */
typedef enum comip_transaction_type {
    COMIP_TRANSACTION_NONE,
    COMIP_TRANSACTION_PERIODIC,
    COMIP_TRANSACTION_NON_PERIODIC,
    COMIP_TRANSACTION_ALL
} comip_transaction_type_e;

struct comip_qh;

/**
 * A Queue Transfer Descriptor (QTD) holds the state of a bulk, control,
 * interrupt, or isochronous transfer. A single QTD is created for each URB
 * (of one of these types) submitted to the HCD. The transfer associated with
 * a QTD may require one or multiple transactions.
 *
 * A QTD is linked to a Queue Head, which is entered in either the
 * non-periodic or periodic schedule for execution. When a QTD is chosen for
 * execution, some or all of its transactions may be executed. After
 * execution, the state of the QTD is updated. The QTD may be retired if all
 * its transactions are complete or if an error occurred. Otherwise, it
 * remains in the schedule so more transactions can be executed later.
 */
typedef struct comip_qtd {
    /**
     * Determines the PID of the next data packet for the data phase of
     * control transfers. Ignored for other transfer types.<br>
     * One of the following values:
     *  - COMIP_HC_PID_DATA0
     *  - COMIP_HC_PID_DATA1
     */
    uint8_t data_toggle;

    /** Current phase for control transfers (Setup, Data, or Status). */
    comip_control_phase_e control_phase;

    /** Keep track of the current split type
     * for FS/LS endpoints on a HS Hub */
    uint8_t complete_split;

    /** How many bytes transferred during SSPLIT OUT */
    uint32_t ssplit_out_xfer_count;

    /**
     * Holds the number of bus errors that have occurred for a transaction
     * within this transfer.
     */
    uint8_t error_count;

    /**
     * Index of the next frame descriptor for an isochronous transfer. A
     * frame descriptor describes the buffer position and length of the
     * data to be transferred in the next scheduled (micro)frame of an
     * isochronous transfer. It also holds status for that transaction.
     * The frame index starts at 0.
     */
    uint16_t isoc_frame_index;

    /** Position of the ISOC split on full/low speed */
    uint8_t isoc_split_pos;

    /** Position of the ISOC split in the buffer for the current frame */
    uint16_t isoc_split_offset;

    /** URB for this transfer */
    struct comip_hcd_urb *urb;

    struct comip_qh *qh;

    /** This list of QTDs */
     COMIP_CIRCLEQ_ENTRY(comip_qtd) qtd_list_entry;

    /** Indicates if this QTD is currently processed by HW. */
    uint8_t in_process;

    /** Number of DMA descriptors for this QTD */
    uint8_t n_desc;

    /** 
     * Last activated frame(packet) index. 
     * Used in Descriptor DMA mode only.
     */
    uint16_t isoc_frame_index_last;

} comip_qtd_t;

COMIP_CIRCLEQ_HEAD(comip_qtd_list, comip_qtd);

/**
 * A Queue Head (QH) holds the static characteristics of an endpoint and
 * maintains a list of transfers (QTDs) for that endpoint. A QH structure may
 * be entered in either the non-periodic or periodic schedule.
 */
typedef struct comip_qh {
    /**
     * Endpoint type.
     * One of the following values:
     *  - USB_ENDPOINT_XFER_CONTROL
     *  - USB_ENDPOINT_XFER_BULK
     *  - USB_ENDPOINT_XFER_INT
     *  - USB_ENDPOINT_XFER_ISOC
     */
    uint8_t ep_type;
    uint8_t ep_is_in;

    /** wMaxPacketSize Field of Endpoint Descriptor. */
    uint16_t maxp;

    /**
     * Device speed.
     * One of the following values:
     *  - COMIP_EP_SPEED_LOW
     *  - COMIP_EP_SPEED_FULL
     *  - COMIP_EP_SPEED_HIGH
     */
    uint8_t dev_speed;

    /**
     * Determines the PID of the next data packet for non-control
     * transfers. Ignored for control transfers.<br>
     * One of the following values:
     *  - COMIP_HC_PID_DATA0
     *  - COMIP_HC_PID_DATA1
     */
    uint8_t data_toggle;

    /** Ping state if 1. */
    uint8_t ping_state;

    /**
     * List of QTDs for this QH.
     */
    struct comip_qtd_list qtd_list;

    /** Host channel currently processing transfers for this QH. */
    struct comip_hc *channel;

    /** Full/low speed endpoint on high-speed hub requires split. */
    uint8_t do_split;

    /** @name Periodic schedule information */
    /** @{ */

    /** Bandwidth in microseconds per (micro)frame. */
    uint16_t usecs;

    /** Interval between transfers in (micro)frames. */
    uint16_t interval;

    /**
     * (micro)frame to initialize a periodic transfer. The transfer
     * executes in the following (micro)frame.
     */
    uint16_t sched_frame;

    /** (micro)frame at which last start split was initialized. */
    uint16_t start_split_frame;

    /** @} */

    /** 
     * Used instead of original buffer if 
     * it(physical address) is not dword-aligned.
     */
    uint8_t *dw_align_buf;
    dma_addr_t dw_align_buf_dma;

    /** Entry for QH in either the periodic or non-periodic schedule. */
    comip_list_link_t qh_list_entry;

    /** @name Descriptor DMA support */
    /** @{ */

    /** Descriptor List. */
    comip_host_dma_desc_t *desc_list;

    /** Descriptor List physical address. */
    dma_addr_t desc_list_dma;

    /** 
     * Xfer Bytes array.
     * Each element corresponds to a descriptor and indicates 
     * original XferSize size value for the descriptor.
     */
    uint32_t *n_bytes;

    /** Actual number of transfer descriptors in a list. */
    uint16_t ntd;

    /** First activated isochronous transfer descriptor index. */
    uint8_t td_first;
    /** Last activated isochronous transfer descriptor index. */
    uint8_t td_last;

    /** @} */

} comip_qh_t;


COMIP_CIRCLEQ_HEAD(hc_list, comip_hc);

/**
 * This structure holds the state of the HCD, including the non-periodic and
 * periodic schedules.
 */
struct comip_hcd {
    /** The COMIP otg device pointer */
    struct comip_hcd_device *hcd_dev;
    /** COMIP OTG Core Interface Layer */
    comip_core_if_t *core_if;

    /** Function HCD driver callbacks */
    struct comip_hcd_function_ops *fops;

    /** Internal COMIP HCD Flags */
    volatile union comip_hcd_internal_flags {
        uint32_t d32;
        struct {
            uint32_t port_connect_status_change:1;
            uint32_t port_connect_status:1;
            uint32_t port_reset_change:1;
            uint32_t port_enable_change:1;
            uint32_t port_suspend_change:1;
            uint32_t port_over_current_change:1;
            uint32_t port_l1_change:1;
            uint32_t reserved:25;
        } b;
    } flags;

    /**
     * Inactive items in the non-periodic schedule. This is a list of
     * Queue Heads. Transfers associated with these Queue Heads are not
     * currently assigned to a host channel.
     */
    comip_list_link_t non_periodic_sched_inactive;

    /**
     * Active items in the non-periodic schedule. This is a list of
     * Queue Heads. Transfers associated with these Queue Heads are
     * currently assigned to a host channel.
     */
    comip_list_link_t non_periodic_sched_active;

    /**
     * Pointer to the next Queue Head to process in the active
     * non-periodic schedule.
     */
    comip_list_link_t *non_periodic_qh_ptr;

    /**
     * Inactive items in the periodic schedule. This is a list of QHs for
     * periodic transfers that are _not_ scheduled for the next frame.
     * Each QH in the list has an interval counter that determines when it
     * needs to be scheduled for execution. This scheduling mechanism
     * allows only a simple calculation for periodic bandwidth used (i.e.
     * must assume that all periodic transfers may need to execute in the
     * same frame). However, it greatly simplifies scheduling and should
     * be sufficient for the vast majority of OTG hosts, which need to
     * connect to a small number of peripherals at one time.
     *
     * Items move from this list to periodic_sched_ready when the QH
     * interval counter is 0 at SOF.
     */
    comip_list_link_t periodic_sched_inactive;

    /**
     * List of periodic QHs that are ready for execution in the next
     * frame, but have not yet been assigned to host channels.
     *
     * Items move from this list to periodic_sched_assigned as host
     * channels become available during the current frame.
     */
    comip_list_link_t periodic_sched_ready;

    /**
     * List of periodic QHs to be executed in the next frame that are
     * assigned to host channels.
     *
     * Items move from this list to periodic_sched_queued as the
     * transactions for the QH are queued to the COMIP controller.
     */
    comip_list_link_t periodic_sched_assigned;

    /**
     * List of periodic QHs that have been queued for execution.
     *
     * Items move from this list to either periodic_sched_inactive or
     * periodic_sched_ready when the channel associated with the transfer
     * is released. If the interval for the QH is 1, the item moves to
     * periodic_sched_ready because it must be rescheduled for the next
     * frame. Otherwise, the item moves to periodic_sched_inactive.
     */
    comip_list_link_t periodic_sched_queued;

    /**
     * Total bandwidth claimed so far for periodic transfers. This value
     * is in microseconds per (micro)frame. The assumption is that all
     * periodic transfers may occur in the same (micro)frame.
     */
    uint16_t periodic_usecs;

    /**
     * Frame number read from the core at SOF. The value ranges from 0 to
     * COMIP_HFNUM_MAX_FRNUM.
     */
    uint16_t frame_number;

    /**
     * Free host channels in the controller. This is a list of
     *comip_hc_t items.
     */
    struct hc_list free_hc_list;
    /**
     * Number of host channels assigned to periodic transfers. Currently
     * assuming that there is a dedicated host channel for each periodic
     * transaction and at least one host channel available for
     * non-periodic transactions.
     */
    int periodic_channels;

    /**
     * Number of host channels assigned to non-periodic transfers.
     */
    int non_periodic_channels;

    /**
     * Array of pointers to the host channel descriptors. Allows accessing
     * a host channel descriptor given the host channel number. This is
     * useful in interrupt handlers.
     */
    struct comip_hc *hc_ptr_array[MAX_EPS_CHANNELS];

    /**
     * Buffer to use for any data received during the status phase of a
     * control transfer. Normally no data is transferred during the status
     * phase. This buffer is used as a bit bucket.
     */
    uint8_t *status_buf;

    /**
     * DMA address for status_buf.
     */
    dma_addr_t status_buf_dma;
#define COMIP_HCD_STATUS_BUF_SIZE 64

    /**
     * Connection timer. An OTG host must display a message if the device
     * does not connect. Started when the VBus power is turned on via
     * sysfs attribute "buspower".
     */
    comip_timer_t *conn_timer;

    /* Tasket to do a reset */
    comip_tasklet_t *reset_tasklet;

    /*  */
    comip_spinlock_t *lock;

    /**
     * Private data that could be used by OS wrapper.
     */
    void *priv;

    uint8_t otg_port;

    /** Frame List */
    uint32_t *frame_list;

    /** Frame List DMA address */
    dma_addr_t frame_list_dma;

	/* COMIP memory pools */
	struct dma_pool		*qh_pool;	/* qh per active urb */
	struct dma_pool		*frame_pool;	/* one or more per qh */
#ifdef DEBUG
    uint32_t frrem_samples;
    uint64_t frrem_accum;

    uint32_t hfnum_7_samples_a;
    uint64_t hfnum_7_frrem_accum_a;
    uint32_t hfnum_0_samples_a;
    uint64_t hfnum_0_frrem_accum_a;
    uint32_t hfnum_other_samples_a;
    uint64_t hfnum_other_frrem_accum_a;

    uint32_t hfnum_7_samples_b;
    uint64_t hfnum_7_frrem_accum_b;
    uint32_t hfnum_0_samples_b;
    uint64_t hfnum_0_frrem_accum_b;
    uint32_t hfnum_other_samples_b;
    uint64_t hfnum_other_frrem_accum_b;
#endif
};

/** @name Transaction Execution Functions */
/** @{ */
extern comip_transaction_type_e comip_hcd_select_transactions(comip_hcd_t
    * hcd);
extern void comip_hcd_queue_transactions(comip_hcd_t * hcd,
    comip_transaction_type_e tr_type);

/** @} */

/** @name Interrupt Handler Functions */
/** @{ */
extern int32_t comip_hcd_handle_intr(comip_hcd_t * comip_hcd);
extern int32_t comip_hcd_handle_sof_intr(comip_hcd_t * comip_hcd);
extern int32_t comip_hcd_handle_rx_status_q_level_intr(comip_hcd_t *
                             comip_hcd);
extern int32_t comip_hcd_handle_np_tx_fifo_empty_intr(comip_hcd_t *
                            comip_hcd);
extern int32_t comip_hcd_handle_perio_tx_fifo_empty_intr(comip_hcd_t *
                               comip_hcd);
extern int32_t comip_hcd_handle_incomplete_periodic_intr(comip_hcd_t *
                               comip_hcd);
extern int32_t comip_hcd_handle_disconnect_intr(comip_hcd_t * comip_hcd);

extern int32_t comip_hcd_handle_port_intr(comip_hcd_t * comip_hcd);
extern int32_t comip_hcd_handle_conn_id_status_change_intr(comip_hcd_t *
                                 comip_hcd);
extern int32_t comip_hcd_handle_disconnect_intr(comip_hcd_t * comip_hcd);
extern int32_t comip_hcd_handle_hc_intr(comip_hcd_t * comip_hcd);
extern int32_t comip_hcd_handle_hc_n_intr(comip_hcd_t * comip_hcd,
                        uint32_t num);
extern int32_t comip_hcd_handle_session_req_intr(comip_hcd_t * comip_hcd);
extern int32_t comip_hcd_handle_wakeup_detected_intr(comip_hcd_t *
                               comip_hcd);
/** @} */

/** @name Schedule Queue Functions */
/** @{ */

/* Implemented in comip_hcd_queue.c */
extern comip_qh_t *comip_hcd_qh_create(comip_hcd_t * hcd,
    comip_hcd_urb_t * urb, int atomic_alloc);
extern void comip_hcd_qh_free(comip_hcd_t * hcd, comip_qh_t * qh);
extern int comip_hcd_qh_add(comip_hcd_t * hcd, comip_qh_t * qh);
extern void comip_hcd_qh_remove(comip_hcd_t * hcd, comip_qh_t * qh);
extern void comip_hcd_qh_deactivate(comip_hcd_t * hcd, comip_qh_t * qh,
    int sched_csplit);

/** Remove and free a QH */
static inline void comip_hcd_qh_remove_and_free(comip_hcd_t * hcd,
    comip_qh_t * qh)
{
    comip_irqflags_t flags;
    COMIP_SPINLOCK_IRQSAVE(hcd->lock, &flags);
    comip_hcd_qh_remove(hcd, qh);
    COMIP_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
    comip_hcd_qh_free(hcd, qh);
}

/** Allocates memory for a QH structure.
 * @return Returns the memory allocate or NULL on error. */
static inline comip_qh_t *comip_hcd_qh_alloc(int atomic_alloc)
{
    if (atomic_alloc)
        return (comip_qh_t *) kzalloc(sizeof(comip_qh_t), GFP_ATOMIC);
    else
        return (comip_qh_t *) kzalloc(sizeof(comip_qh_t), GFP_KERNEL);
}

extern comip_qtd_t *comip_hcd_qtd_create(comip_hcd_urb_t * urb,
    int atomic_alloc);
extern void comip_hcd_qtd_init(comip_qtd_t * qtd, comip_hcd_urb_t * urb);
extern int comip_hcd_qtd_add(comip_qtd_t * qtd, comip_hcd_t * comip_hcd,
    comip_qh_t ** qh, int atomic_alloc);

/** Allocates memory for a QTD structure.
 * @return Returns the memory allocate or NULL on error. */
static inline comip_qtd_t *comip_hcd_qtd_alloc(int atomic_alloc)
{
    if (atomic_alloc)
        return (comip_qtd_t *) kzalloc(sizeof(comip_qtd_t), GFP_ATOMIC);
    else
        return (comip_qtd_t *) kzalloc(sizeof(comip_qtd_t), GFP_KERNEL);
}

/** Frees the memory for a QTD structure.  QTD should already be removed from
 * list.
 * @param qtd QTD to free.*/
static inline void comip_hcd_qtd_free(comip_qtd_t * qtd)
{
    kfree(qtd);
}

/** Removes a QTD from list.
 * @param hcd HCD instance.
 * @param qtd QTD to remove from list.
 * @param qh QTD belongs to.
 */
static inline void comip_hcd_qtd_remove(comip_hcd_t * hcd,
    comip_qtd_t * qtd, comip_qh_t * qh)
{
    COMIP_CIRCLEQ_REMOVE(&qh->qtd_list, qtd, qtd_list_entry);
}

/** Remove and free a QTD 
  * Need to disable IRQ and hold hcd lock while calling this function out of 
  * interrupt servicing chain */
static inline void comip_hcd_qtd_remove_and_free(comip_hcd_t * hcd,
    comip_qtd_t * qtd, comip_qh_t * qh)
{
    comip_hcd_qtd_remove(hcd, qtd, qh);
    comip_hcd_qtd_free(qtd);
}

/** @} */

/** @name Descriptor DMA Supporting Functions */
/** @{ */

extern void comip_hcd_start_xfer_ddma(comip_hcd_t * hcd, comip_qh_t * qh);
extern void comip_hcd_complete_xfer_ddma(comip_hcd_t * hcd,
    comip_hc_t * hc, comip_hc_regs_t * hc_regs, comip_halt_status_e halt_status);

extern int comip_hcd_qh_init_ddma(comip_hcd_t * hcd, comip_qh_t * qh);
extern void comip_hcd_qh_free_ddma(comip_hcd_t * hcd, comip_qh_t * qh);

/** @} */

/** @name Internal Functions */
/** @{ */
comip_qh_t *comip_urb_to_qh(comip_hcd_urb_t * urb);
/** @} */

#ifdef CONFIG_USB_COMIP_LPM
extern int comip_hcd_get_hc_for_lpm_tran(comip_hcd_t * hcd,
    uint8_t devaddr);
extern void comip_hcd_free_hc_from_lpm(comip_hcd_t * hcd);
#endif

/** Gets the QH that contains the list_head */
#define comip_list_to_qh(_list_head_ptr_) container_of(_list_head_ptr_, comip_qh_t, qh_list_entry)

/** Gets the QTD that contains the list_head */
#define comip_list_to_qtd(_list_head_ptr_) container_of(_list_head_ptr_, comip_qtd_t, qtd_list_entry)

/** Check if QH is non-periodic  */
#define comip_qh_is_non_per(_qh_ptr_) ((_qh_ptr_->ep_type == UE_BULK) || \
    (_qh_ptr_->ep_type == UE_CONTROL))

/** High bandwidth multiplier as encoded in highspeed endpoint descriptors */
#define comip_hb_mult(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))

/** Packet size for any kind of endpoint descriptor */
#define comip_max_packet(wMaxPacketSize) ((wMaxPacketSize) & 0x07ff)

/**
 * Returns true if _frame1 is less than or equal to _frame2. The comparison is
 * done modulo COMIP_HFNUM_MAX_FRNUM. This accounts for the rollover of the
 * frame number when the max frame number is reached.
 */
static inline int comip_frame_num_le(uint16_t frame1, uint16_t frame2)
{
    return ((frame2 - frame1) & COMIP_HFNUM_MAX_FRNUM) <=
        (COMIP_HFNUM_MAX_FRNUM >> 1);
}

/**
 * Returns true if _frame1 is greater than _frame2. The comparison is done
 * modulo COMIP_HFNUM_MAX_FRNUM. This accounts for the rollover of the frame
 * number when the max frame number is reached.
 */
static inline int comip_frame_num_gt(uint16_t frame1, uint16_t frame2)
{
    return (frame1 != frame2) &&
        (((frame1 - frame2) & COMIP_HFNUM_MAX_FRNUM) <
         (COMIP_HFNUM_MAX_FRNUM >> 1));
}

/**
 * Increments _frame by the amount specified by _inc. The addition is done
 * modulo COMIP_HFNUM_MAX_FRNUM. Returns the incremented value.
 */
static inline uint16_t comip_frame_num_inc(uint16_t frame, uint16_t inc)
{
    return (frame + inc) & COMIP_HFNUM_MAX_FRNUM;
}

static inline uint16_t comip_full_frame_num(uint16_t frame)
{
    return (frame & COMIP_HFNUM_MAX_FRNUM) >> 3;
}

static inline uint16_t comip_micro_frame_num(uint16_t frame)
{
    return frame & 0x7;
}

void comip_hcd_save_data_toggle(comip_hc_t * hc,
    comip_hc_regs_t * hc_regs, comip_qtd_t * qtd);

#ifdef DEBUG
/**
 * Macro to sample the remaining PHY clocks left in the current frame. This
 * may be used during debugging to determine the average time it takes to
 * execute sections of code. There are two possible sample points, "a" and
 * "b", so the _letter argument must be one of these values.
 *
 * To dump the average sample times, read the "hcd_frrem" sysfs attribute. For
 * example, "cat /sys/devices/lm0/hcd_frrem".
 */
#define comip_sample_frrem(_hcd, _qh, _letter) \
{ \
    hfnum_data_t hfnum; \
    comip_qtd_t *qtd; \
    qtd = list_entry(_qh->qtd_list.next, comip_qtd_t, qtd_list_entry); \
    if (usb_pipeint(qtd->urb->pipe) && _qh->start_split_frame != 0 && !qtd->complete_split) { \
        hfnum.d32 = readl(&_hcd->core_if->host_if->host_global_regs->hfnum); \
        switch (hfnum.b.frnum & 0x7) { \
        case 7: \
            _hcd->hfnum_7_samples_##_letter++; \
            _hcd->hfnum_7_frrem_accum_##_letter += hfnum.b.frrem; \
            break; \
        case 0: \
            _hcd->hfnum_0_samples_##_letter++; \
            _hcd->hfnum_0_frrem_accum_##_letter += hfnum.b.frrem; \
            break; \
        default: \
            _hcd->hfnum_other_samples_##_letter++; \
            _hcd->hfnum_other_frrem_accum_##_letter += hfnum.b.frrem; \
            break; \
        } \
    } \
}
#else
#define comip_sample_frrem(_hcd, _qh, _letter)
#endif
#endif
