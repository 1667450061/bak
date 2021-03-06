/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/comip_hcd.c $
 * $Revision: #102 $
 * $Date: 2011/05/17 $
 * $Change: 1774110 $
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
/** @file
 * This file implements HCD Core. All code in this file is portable and doesn't
 * use any OS specific functions.
 * Interface provided by HCD Core is defined in <code><hcd_if.h></code>
 * header file.
 */

#include "comip_hcd.h"
#include "comip_otg_regs.h"
#include <linux/version.h>

comip_hcd_t *comip_hcd_alloc_hcd(void)
{
    return kzalloc(sizeof(comip_hcd_t), GFP_KERNEL);
}

/**
 * Connection timeout function.  An OTG host is required to display a
 * message if the device does not connect within 10 seconds.
 */
void comip_hcd_connect_timeout(void *ptr)
{
    COMIP_DEBUGPL(DBG_HCDV, "%s(%p)\n", __func__, ptr);
    COMIP_PRINTF("Connect Timeout\n");
    ERR("Device Not Connected/Responding\n");
}

#ifdef DEBUG
static void dump_channel_info(comip_hcd_t * hcd, comip_qh_t * qh)
{
    if (qh->channel != NULL) {
        comip_hc_t *hc = qh->channel;
        comip_list_link_t *item;
        comip_qh_t *qh_item;
        int num_channels = hcd->core_if->core_params->host_channels;
        int i;

        comip_hc_regs_t *hc_regs;
        hcchar_data_t hcchar;
        hcsplt_data_t hcsplt;
        hctsiz_data_t hctsiz;
        uint32_t hcdma;

        hc_regs = hcd->core_if->host_if->hc_regs[hc->hc_num];
        hcchar.d32 = readl(&hc_regs->hcchar);
        hcsplt.d32 = readl(&hc_regs->hcsplt);
        hctsiz.d32 = readl(&hc_regs->hctsiz);
        hcdma = readl(&hc_regs->hcdma);

        COMIP_PRINTF("  Assigned to channel %p:\n", hc);
        COMIP_PRINTF("    hcchar 0x%08x, hcsplt 0x%08x\n", hcchar.d32,
               hcsplt.d32);
        COMIP_PRINTF("    hctsiz 0x%08x, hcdma 0x%08x\n", hctsiz.d32,
               hcdma);
        COMIP_PRINTF("    dev_addr: %d, ep_num: %d, ep_is_in: %d\n",
               hc->dev_addr, hc->ep_num, hc->ep_is_in);
        COMIP_PRINTF("    ep_type: %d\n", hc->ep_type);
        COMIP_PRINTF("    max_packet: %d\n", hc->max_packet);
        COMIP_PRINTF("    data_pid_start: %d\n", hc->data_pid_start);
        COMIP_PRINTF("    xfer_started: %d\n", hc->xfer_started);
        COMIP_PRINTF("    halt_status: %d\n", hc->halt_status);
        COMIP_PRINTF("    xfer_buff: %p\n", hc->xfer_buff);
        COMIP_PRINTF("    xfer_len: %d\n", hc->xfer_len);
        COMIP_PRINTF("    qh: %p\n", hc->qh);
        COMIP_PRINTF("  NP inactive sched:\n");
        COMIP_LIST_FOREACH(item, &hcd->non_periodic_sched_inactive) {
            qh_item =
                COMIP_LIST_ENTRY(item, comip_qh_t, qh_list_entry);
            COMIP_PRINTF("    %p\n", qh_item);
        }
        COMIP_PRINTF("  NP active sched:\n");
        COMIP_LIST_FOREACH(item, &hcd->non_periodic_sched_active) {
            qh_item =
                COMIP_LIST_ENTRY(item, comip_qh_t, qh_list_entry);
            COMIP_PRINTF("    %p\n", qh_item);
        }
        COMIP_PRINTF("  Channels: \n");
        for (i = 0; i < num_channels; i++) {
            comip_hc_t *hc = hcd->hc_ptr_array[i];
            COMIP_PRINTF("    %2d: %p\n", i, hc);
        }
    }
}
#endif /* DEBUG */

/**
 * Work queue function for starting the HCD when A-Cable is connected.
 * The hcd_start() must be called in a process context.
 */
#if 0
static void hcd_start_func(void *_vp)
{
    comip_hcd_t *hcd = (comip_hcd_t *) _vp;

    COMIP_DEBUGPL(DBG_HCDV, "%s() %p\n", __func__, hcd);
    if (hcd) {
        hcd->fops->start(hcd);
    }
}
#endif
static void del_xfer_timers(comip_hcd_t * hcd)
{
#ifdef DEBUG
    int i;
    int num_channels = hcd->core_if->core_params->host_channels;
    for (i = 0; i < num_channels; i++) {
        COMIP_TIMER_CANCEL(hcd->core_if->hc_xfer_timer[i]);
    }
#endif
}

static void del_timers(comip_hcd_t * hcd)
{
    del_xfer_timers(hcd);
    COMIP_TIMER_CANCEL(hcd->conn_timer);
}

/**
 * Processes all the URBs in a single list of QHs. Completes them with
 * -ETIMEDOUT and frees the QTD.
 */
static void kill_urbs_in_qh_list(comip_hcd_t * hcd, comip_list_link_t * qh_list)
{
    comip_list_link_t *qh_item;
    comip_qh_t *qh;
    comip_qtd_t *qtd, *qtd_tmp;

    COMIP_LIST_FOREACH(qh_item, qh_list) {
        qh = COMIP_LIST_ENTRY(qh_item, comip_qh_t, qh_list_entry);
        COMIP_CIRCLEQ_FOREACH_SAFE(qtd, qtd_tmp,
                     &qh->qtd_list, qtd_list_entry) {
            qtd = COMIP_CIRCLEQ_FIRST(&qh->qtd_list);
            if (qtd->urb != NULL) {
                hcd->fops->complete(hcd, qtd->urb->priv,
                            qtd->urb, -ETIMEDOUT);
                comip_hcd_qtd_remove_and_free(hcd, qtd, qh);
            }

        }
    }
}

/**
 * Responds with an error status of ETIMEDOUT to all URBs in the non-periodic
 * and periodic schedules. The QTD associated with each URB is removed from
 * the schedule and freed. This function may be called when a disconnect is
 * detected or when the HCD is being stopped.
 */
static void kill_all_urbs(comip_hcd_t * hcd)
{
    kill_urbs_in_qh_list(hcd, &hcd->non_periodic_sched_inactive);
    kill_urbs_in_qh_list(hcd, &hcd->non_periodic_sched_active);
    kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_inactive);
    kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_ready);
    kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_assigned);
    kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_queued);
}

/**
 * Start the connection timer.  An OTG host is required to display a
 * message if the device does not connect within 10 seconds.  The
 * timer is deleted if a port connect interrupt occurs before the
 * timer expires.
 */
#if 0
static void comip_hcd_start_connect_timer(comip_hcd_t * hcd)
{
    COMIP_TIMER_SCHEDULE(hcd->conn_timer, 10000 /* 10 secs */ );
}
#endif
/**
 * HCD Callback function for disconnect of the HCD.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
int32_t comip_hcd_disconnect_cb(void *p)
{
    gintsts_data_t intr;
    comip_hcd_t *comip_hcd = p;

    /*
     * Set status flags for the hub driver.
     */
    comip_hcd->flags.b.port_connect_status_change = 1;
    comip_hcd->flags.b.port_connect_status = 0;

    /*
     * Shutdown any transfers in process by clearing the Tx FIFO Empty
     * interrupt mask and status bits and disabling subsequent host
     * channel interrupts.
     */
    intr.d32 = 0;
    intr.b.nptxfempty = 1;
    intr.b.ptxfempty = 1;
    intr.b.hcintr = 1;
    COMIP_MODIFY_REG32(&comip_hcd->core_if->core_global_regs->gintmsk,
             intr.d32, 0);
    COMIP_MODIFY_REG32(&comip_hcd->core_if->core_global_regs->gintsts,
             intr.d32, 0);

    del_timers(comip_hcd);

    /*
     * Turn off the vbus power only if the core has transitioned to device
     * mode. If still in host mode, need to keep power on to detect a
     * reconnection.
     */
    if (comip_is_device_mode(comip_hcd->core_if)) {
        if (comip_hcd->core_if->op_state != A_SUSPEND) {
            hprt0_data_t hprt0 = {.d32 = 0 };
            COMIP_PRINTF("Disconnect: PortPower off\n");
            hprt0.b.prtpwr = 0;
            COMIP_WRITE_REG32(comip_hcd->core_if->host_if->hprt0,
                    hprt0.d32);
        }

        comip_disable_host_interrupts(comip_hcd->core_if);
    }

    /* Respond with an error status to all URBs in the schedule. */
    kill_all_urbs(comip_hcd);

    if (comip_is_host_mode(comip_hcd->core_if)) {
        /* Clean up any host channels that were in use. */
        int num_channels;
        int i;
        comip_hc_t *channel;
        comip_hc_regs_t *hc_regs;
        hcchar_data_t hcchar;

        num_channels = comip_hcd->core_if->core_params->host_channels;

        if (!comip_hcd->core_if->dma_enable) {
            /* Flush out any channel requests in slave mode. */
            for (i = 0; i < num_channels; i++) {
                channel = comip_hcd->hc_ptr_array[i];
                if (COMIP_CIRCLEQ_EMPTY_ENTRY
                    (channel, hc_list_entry)) {
                    hc_regs =
                        comip_hcd->core_if->
                        host_if->hc_regs[i];
                    hcchar.d32 =
                        readl(&hc_regs->hcchar);
                    if (hcchar.b.chen) {
                        hcchar.b.chen = 0;
                        hcchar.b.chdis = 1;
                        hcchar.b.epdir = 0;
                        COMIP_WRITE_REG32
                            (&hc_regs->hcchar,
                             hcchar.d32);
                    }
                }
            }
        }

        for (i = 0; i < num_channels; i++) {
            channel = comip_hcd->hc_ptr_array[i];
            if (COMIP_CIRCLEQ_EMPTY_ENTRY(channel, hc_list_entry)) {
                hc_regs =
                    comip_hcd->core_if->host_if->hc_regs[i];
                hcchar.d32 = readl(&hc_regs->hcchar);
                if (hcchar.b.chen) {
                    /* Halt the channel. */
                    hcchar.b.chdis = 1;
                    COMIP_WRITE_REG32(&hc_regs->hcchar,
                            hcchar.d32);
                }

                comip_hc_cleanup(comip_hcd->core_if,
                           channel);
                COMIP_CIRCLEQ_INSERT_TAIL
                    (&comip_hcd->free_hc_list, channel,
                     hc_list_entry);
                /*
                 * Added for Descriptor DMA to prevent channel double cleanup
                 * in release_channel_ddma(). Which called from ep_disable
                 * when device disconnect.
                 */
                channel->qh = NULL;
            }
        }
    }

    if (comip_hcd->fops->disconnect) {
        comip_hcd->fops->disconnect(comip_hcd);
    }

    return 1;
}
/**
 * Halts the comip host mode operations in a clean manner. USB transfers are
 * stopped.
 */
void comip_hcd_stop(comip_hcd_t * hcd)
{
    hprt0_data_t hprt0 = {.d32 = 0 };

    COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD STOP\n");

    /*
     * The root hub should be disconnected before this function is called.
     * The disconnect will clear the QTD lists (via ..._hcd_urb_dequeue)
     * and the QH lists (via ..._hcd_endpoint_disable).
     */

    /* Turn off all host-specific interrupts. */
    comip_disable_host_interrupts(hcd->core_if);

    /* Turn off the vbus power */
    COMIP_PRINTF("PortPower off\n");
    hprt0.b.prtpwr = 0;
    COMIP_WRITE_REG32(hcd->core_if->host_if->hprt0, hprt0.d32);
    mdelay(1);
}

int comip_hcd_urb_enqueue(comip_hcd_t * hcd,
    comip_hcd_urb_t * comip_urb, void **ep_handle, int atomic_alloc)
{
    comip_irqflags_t flags;
    int retval = 0;
    comip_qtd_t *qtd;
    int return_flag = 0;

    if (!hcd->flags.b.port_connect_status) {
        /* No longer connected. */
        ERR("Not connected\n");
        return -ENODEV;
    }

    qtd = comip_hcd_qtd_create(comip_urb, atomic_alloc);
    if (qtd == NULL) {
        ERR("COMIP OTG HCD URB Enqueue failed creating QTD\n");
        return -ENOMEM;
    }

    retval =
        comip_hcd_qtd_add(qtd, hcd, (comip_qh_t **) ep_handle, atomic_alloc, &return_flag);
    if (retval < 0) {
        ERR("COMIP OTG HCD URB Enqueue failed adding QTD. "
              "Error status %d\n", retval);
        comip_hcd_qtd_free(qtd);
    } else {
        qtd->qh = *ep_handle;
    }

    if (hcd->core_if->dma_desc_enable && retval == 0) {
        comip_transaction_type_e tr_type;
#if 0
        if ((qtd->qh->ep_type == UE_BULK)
            && !(qtd->urb->flags & URB_GIVEBACK_ASAP)) {
            /* Do not schedule SG transcations until qtd has URB_GIVEBACK_ASAP set */
            return 0;
        }
#endif
	if (return_flag == 1) {

		return 0;
	}
        COMIP_SPINLOCK_IRQSAVE(hcd->lock, &flags);
        tr_type = comip_hcd_select_transactions(hcd);
        if (tr_type != COMIP_TRANSACTION_NONE) {
            comip_hcd_queue_transactions(hcd, tr_type);
        }
        COMIP_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
    }

    return retval;
}

int comip_hcd_urb_dequeue(comip_hcd_t * hcd,
    comip_hcd_urb_t * comip_urb)
{
    comip_qh_t *qh;
    comip_qtd_t *urb_qtd;

    urb_qtd = comip_urb->qtd;
    qh = urb_qtd->qh;
#ifdef DEBUG
    if (CHK_DEBUG_LEVEL(DBG_HCDV | DBG_HCD_URB)) {
        if (urb_qtd->in_process) {
            dump_channel_info(hcd, qh);
        }
    }
#endif
    if (urb_qtd->in_process && qh->channel) {
        /* The QTD is in process (it has been assigned to a channel). */
        if (hcd->flags.b.port_connect_status) {
            /*
             * If still connected (i.e. in host mode), halt the
             * channel so it can be used for other transfers. If
             * no longer connected, the host registers can't be
             * written to halt the channel since the core is in
             * device mode.
             */
            comip_hc_halt(hcd->core_if, qh->channel,
                    COMIP_HC_XFER_URB_DEQUEUE);
        }
    }

    /*
     * Free the QTD and clean up the associated QH. Leave the QH in the
     * schedule if it has any remaining QTDs.
     */

    if (!hcd->core_if->dma_desc_enable) {
        uint8_t b = urb_qtd->in_process;
        comip_hcd_qtd_remove_and_free(hcd, urb_qtd, qh);
        if (b) {
            comip_hcd_qh_deactivate(hcd, qh, 0);
            qh->channel = NULL;
        } else if (COMIP_CIRCLEQ_EMPTY(&qh->qtd_list)) {
            comip_hcd_qh_remove(hcd, qh);
        }
    } else {
        comip_hcd_qtd_remove_and_free(hcd, urb_qtd, qh);
    }
    return 0;
}

int comip_hcd_endpoint_disable(comip_hcd_t * hcd, void *ep_handle,
    int retry)
{
    comip_qh_t *qh = (comip_qh_t *) ep_handle;
    int retval = 0;
    comip_irqflags_t flags;

    if (retry < 0) {
        retval = -EINVAL;
        goto done;
    }

    if (!qh) {
        retval = -EINVAL;
        goto done;
    }

    COMIP_SPINLOCK_IRQSAVE(hcd->lock, &flags);

    while (!COMIP_CIRCLEQ_EMPTY(&qh->qtd_list) && retry) {
        COMIP_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
        retry--;
        mdelay(5);
        COMIP_SPINLOCK_IRQSAVE(hcd->lock, &flags);
    }

    comip_hcd_qh_remove(hcd, qh);

    COMIP_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
    /*
     * Split comip_hcd_qh_remove_and_free() into qh_remove
     * and qh_free to prevent stack dump on COMIP_DMA_FREE() with
     * irq_disabled (spinlock_irqsave) in comip_hcd_desc_list_free()
     * and comip_hcd_frame_list_alloc().
     */
    comip_hcd_qh_free(hcd, qh);

done:
    return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
int comip_hcd_endpoint_reset(comip_hcd_t * hcd, void *ep_handle)
{
    int retval = 0;
    comip_qh_t *qh = (comip_qh_t *) ep_handle;
    if (!qh)
        return -EINVAL;

    qh->data_toggle = COMIP_HC_PID_DATA0;
    return retval;
}
#endif

/**
 * Reset tasklet function
 */
static void reset_tasklet_func(void *data)
{
    comip_hcd_t *comip_hcd = (comip_hcd_t *) data;
    comip_core_if_t *core_if = comip_hcd->core_if;
    hprt0_data_t hprt0;

    COMIP_DEBUGPL(DBG_HCDV, "USB RESET tasklet called\n");

    hprt0.d32 = comip_read_hprt0(core_if);
    hprt0.b.prtrst = 1;
    COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
    mdelay(100);

    hprt0.b.prtrst = 0;
    COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
    comip_hcd->flags.b.port_reset_change = 1;
}

static void qh_list_free(comip_hcd_t * hcd, comip_list_link_t * qh_list)
{
    comip_list_link_t *item;
    comip_qh_t *qh;
    comip_irqflags_t flags;

    if (!qh_list->next) {
        /* The list hasn't been initialized yet. */
        return;
    }
    /*
     * Hold spinlock here. Not needed in that case if bellow 
     * function is being called from ISR 
     */
    COMIP_SPINLOCK_IRQSAVE(hcd->lock, &flags);
    /* Ensure there are no QTDs or URBs left. */
    kill_urbs_in_qh_list(hcd, qh_list);
    COMIP_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);

    COMIP_LIST_FOREACH(item, qh_list) {
        qh = COMIP_LIST_ENTRY(item, comip_qh_t, qh_list_entry);
        comip_hcd_qh_remove_and_free(hcd, qh);
    }
}

/**
 * Exit from Hibernation if Host did not detect SRP from connected SRP capable
 * Device during SRP time by host power up.
 */
void comip_hcd_power_up(void *ptr)
{
    gpwrdn_data_t gpwrdn = {.d32 = 0 };
    comip_core_if_t *core_if = (comip_core_if_t *) ptr;

    COMIP_PRINTF("%s called\n", __FUNCTION__);

    if (!core_if->hibernation_suspend) {
        COMIP_PRINTF("Already exited from Hibernation\n");
        return;
    }

    /* Switch on the voltage to the core */
    gpwrdn.b.pwrdnswtch = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
    udelay(10);

    /* Reset the core */
    gpwrdn.d32 = 0;
    gpwrdn.b.pwrdnrstn = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
    udelay(10);

    /* Disable power clamps */
    gpwrdn.d32 = 0;
    gpwrdn.b.pwrdnclmp = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

    /* Remove reset the core signal */
    gpwrdn.d32 = 0;
    gpwrdn.b.pwrdnrstn = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
    udelay(10);

    /* Disable PMU interrupt */
    gpwrdn.d32 = 0;
    gpwrdn.b.pmuintsel = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

    core_if->hibernation_suspend = 0;

    /* Disable PMU */
    gpwrdn.d32 = 0;
    gpwrdn.b.pmuactv = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
    udelay(10);

    /* Enable VBUS */
    gpwrdn.d32 = 0;
    gpwrdn.b.dis_vbus = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

    comip_core_init(core_if);
    comip_enable_global_interrupts(core_if);
    cil_hcd_start(core_if);
}

/**
 * Frees secondary storage associated with the comip_hcd structure contained
 * in the struct usb_hcd field.
 */
static void comip_hcd_free(comip_hcd_t * comip_hcd)
{
    int i;

    COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD FREE\n");

    del_timers(comip_hcd);

    /* Free memory for QH/QTD lists */
    qh_list_free(comip_hcd, &comip_hcd->non_periodic_sched_inactive);
    qh_list_free(comip_hcd, &comip_hcd->non_periodic_sched_active);
    qh_list_free(comip_hcd, &comip_hcd->periodic_sched_inactive);
    qh_list_free(comip_hcd, &comip_hcd->periodic_sched_ready);
    qh_list_free(comip_hcd, &comip_hcd->periodic_sched_assigned);
    qh_list_free(comip_hcd, &comip_hcd->periodic_sched_queued);

    /* Free memory for the host channels. */
    for (i = 0; i < MAX_EPS_CHANNELS; i++) {
        comip_hc_t *hc = comip_hcd->hc_ptr_array[i];

#ifdef DEBUG
        if (comip_hcd->core_if->hc_xfer_timer[i]) {
            COMIP_TIMER_FREE(comip_hcd->core_if->hc_xfer_timer[i]);
        }
#endif
        if (hc != NULL) {
            COMIP_DEBUGPL(DBG_HCDV, "HCD Free channel #%i, hc=%p\n",
                    i, hc);
            kfree(hc);
        }
    }

    if (comip_hcd->core_if->dma_enable) {
        if (comip_hcd->status_buf_dma) {
            COMIP_DMA_FREE(COMIP_HCD_STATUS_BUF_SIZE,
                     comip_hcd->status_buf,
                     comip_hcd->status_buf_dma);
        }
    } else if (comip_hcd->status_buf != NULL) {
        kfree(comip_hcd->status_buf);
    }
    kfree(comip_hcd->lock);
    /* Set core_if's lock pointer to NULL */
    comip_hcd->core_if->lock = NULL;

    COMIP_TIMER_FREE(comip_hcd->conn_timer);
    kfree(comip_hcd->reset_tasklet);

#ifdef COMIP_DEV_SRPCAP
    if (comip_hcd->core_if->power_down == 2 &&
        comip_hcd->core_if->pwron_timer) {
        COMIP_TIMER_FREE(comip_hcd->core_if->pwron_timer);
    }
#endif
    kfree(comip_hcd);
}

int comip_hcd_init(comip_hcd_t * hcd, comip_core_if_t * core_if)
{
    int retval = 0;
    int num_channels;
    int i;
    comip_hc_t *channel;

    hcd->lock = COMIP_SPINLOCK_ALLOC();
    if (!hcd->lock) {
        ERR("Could not allocate lock");
        kfree(hcd);
        retval = -ENOMEM;
        goto out;
    }
    hcd->core_if = core_if;

    /* Initialize the non-periodic schedule. */
    COMIP_LIST_INIT(&hcd->non_periodic_sched_inactive);
    COMIP_LIST_INIT(&hcd->non_periodic_sched_active);

    /* Initialize the periodic schedule. */
    COMIP_LIST_INIT(&hcd->periodic_sched_inactive);
    COMIP_LIST_INIT(&hcd->periodic_sched_ready);
    COMIP_LIST_INIT(&hcd->periodic_sched_assigned);
    COMIP_LIST_INIT(&hcd->periodic_sched_queued);

    /*
     * Create a host channel descriptor for each host channel implemented
     * in the controller. Initialize the channel descriptor array.
     */
    COMIP_CIRCLEQ_INIT(&hcd->free_hc_list);
    num_channels = hcd->core_if->core_params->host_channels;
    memset(hcd->hc_ptr_array, 0, sizeof(hcd->hc_ptr_array));
    for (i = 0; i < num_channels; i++) {
        channel = kzalloc(sizeof(comip_hc_t), GFP_KERNEL);
        if (channel == NULL) {
            retval = -ENOMEM;
            ERR("%s: host channel allocation failed\n",
                  __func__);
            comip_hcd_free(hcd);
            goto out;
        }
        channel->hc_num = i;
        hcd->hc_ptr_array[i] = channel;
#ifdef DEBUG
        hcd->core_if->hc_xfer_timer[i] =
            COMIP_TIMER_ALLOC("hc timer", hc_xfer_timeout,
                    &hcd->core_if->hc_xfer_info[i]);
#endif
        COMIP_DEBUGPL(DBG_HCDV, "HCD Added channel #%d, hc=%p\n", i,
                channel);
    }

    /* Initialize the Connection timeout timer. */
    hcd->conn_timer = COMIP_TIMER_ALLOC("Connection timer",
                      comip_hcd_connect_timeout, 0);

    /* Initialize reset tasklet. */
    hcd->reset_tasklet = COMIP_TASK_ALLOC("reset_tasklet", reset_tasklet_func, hcd);
#ifdef COMIP_DEV_SRPCAP
    if (hcd->core_if->power_down == 2) {
        /* Initialize Power on timer for Host power up in case hibernation */
        hcd->core_if->pwron_timer = COMIP_TIMER_ALLOC("PWRON TIMER",
                                    comip_hcd_power_up, core_if);
    }
#endif  

    /*
     * Allocate space for storing data on status transactions. Normally no
     * data is sent, but this space acts as a bit bucket. This must be
     * done after usb_add_hcd since that function allocates the DMA buffer
     * pool.
     */
    if (hcd->core_if->dma_enable) {
        hcd->status_buf =
            COMIP_DMA_ALLOC(COMIP_HCD_STATUS_BUF_SIZE,
                  &hcd->status_buf_dma);
    } else {
        hcd->status_buf = COMIP_ALLOC(COMIP_HCD_STATUS_BUF_SIZE);
    }
    if (!hcd->status_buf) {
        retval = -ENOMEM;
        ERR("%s: status_buf allocation failed\n", __func__);
        comip_hcd_free(hcd);
        goto out;
    }

    hcd->otg_port = 1;
    hcd->frame_list = NULL;
    hcd->frame_list_dma = 0;
out:
    return retval;
}

void comip_hcd_remove(comip_hcd_t * hcd)
{
    /* Turn off all host-specific interrupts. */
    comip_disable_host_interrupts(hcd->core_if);

    comip_hcd_free(hcd);
}

/**
 * Initializes dynamic portions of the comip HCD state.
 */
static void comip_hcd_reinit(comip_hcd_t * hcd)
{
    int num_channels;
    int i;
    comip_hc_t *channel;
    comip_hc_t *channel_tmp;

    hcd->flags.d32 = 0;

    hcd->non_periodic_qh_ptr = &hcd->non_periodic_sched_active;
    hcd->non_periodic_channels = 0;
    hcd->periodic_channels = 0;

    /*
     * Put all channels in the free channel list and clean up channel
     * states.
     */
    COMIP_CIRCLEQ_FOREACH_SAFE(channel, channel_tmp,
                 &hcd->free_hc_list, hc_list_entry) {
        COMIP_CIRCLEQ_REMOVE(&hcd->free_hc_list, channel, hc_list_entry);
    }

    num_channels = hcd->core_if->core_params->host_channels;
    for (i = 0; i < num_channels; i++) {
        channel = hcd->hc_ptr_array[i];
        COMIP_CIRCLEQ_INSERT_TAIL(&hcd->free_hc_list, channel,
                    hc_list_entry);
        comip_hc_cleanup(hcd->core_if, channel);
    }

    /* Initialize the COMIP core for host mode operation. */
    comip_core_host_init(hcd->core_if);

    /* Set core_if's lock pointer to the hcd->lock */
    hcd->core_if->lock = hcd->lock;
}

/**
 * Assigns transactions from a QTD to a free host channel and initializes the
 * host channel to perform the transactions. The host channel is removed from
 * the free list.
 *
 * @param hcd The HCD state structure.
 * @param qh Transactions from the first QTD for this QH are selected and
 * assigned to a free host channel.
 */
static void assign_and_init_hc(comip_hcd_t * hcd, comip_qh_t * qh)
{
    comip_hc_t *hc;
    comip_qtd_t *qtd;
    comip_hcd_urb_t *urb;
    void* ptr = NULL;

    COMIP_DEBUGPL(DBG_HCDV, "%s(%p,%p)\n", __func__, hcd, qh);

    hc = COMIP_CIRCLEQ_FIRST(&hcd->free_hc_list);

    /* Remove the host channel from the free list. */
    COMIP_CIRCLEQ_REMOVE_INIT(&hcd->free_hc_list, hc, hc_list_entry);

    qtd = COMIP_CIRCLEQ_FIRST(&qh->qtd_list);

    urb = qtd->urb;
    qh->channel = hc;

    qtd->in_process = 1;

    /*
     * Use usb_pipedevice to determine device address. This address is
     * 0 before the SET_ADDRESS command and the correct address afterward.
     */
    hc->dev_addr = comip_hcd_get_dev_addr(&urb->pipe_info);
    hc->ep_num = comip_hcd_get_ep_num(&urb->pipe_info);
    hc->speed = qh->dev_speed;
    hc->max_packet = comip_max_packet(qh->maxp);

    hc->xfer_started = 0;
    hc->halt_status = COMIP_HC_XFER_NO_HALT_STATUS;
    hc->error_state = (qtd->error_count > 0);
    hc->halt_on_queue = 0;
    hc->halt_pending = 0;
    hc->requests = 0;

    /*
     * The following values may be modified in the transfer type section
     * below. The xfer_len value may be reduced when the transfer is
     * started to accommodate the max widths of the XferSize and PktCnt
     * fields in the HCTSIZn register.
     */

    hc->ep_is_in = (comip_hcd_is_pipe_in(&urb->pipe_info) != 0);
    if (hc->ep_is_in) {
        hc->do_ping = 0;
    } else {
        hc->do_ping = qh->ping_state;
    }

    hc->data_pid_start = qh->data_toggle;
    hc->multi_count = 1;

    if (hcd->core_if->dma_enable) {
        hc->xfer_buff = (uint8_t *) urb->dma + urb->actual_length;

        /* For non-dword aligned case */
        if (((uint32_t)hc->xfer_buff & 0x3)
            && !hcd->core_if->dma_desc_enable) {
            ptr = (uint8_t *) urb->buf + urb->actual_length;
        }
    } else {
        hc->xfer_buff = (uint8_t *) urb->buf + urb->actual_length;
    }
    hc->xfer_len = urb->length - urb->actual_length;
    hc->xfer_count = 0;

    /*
     * Set the split attributes
     */
    hc->do_split = 0;
    if (qh->do_split) {
        uint32_t hub_addr, port_addr;
        hc->do_split = 1;
        hc->xact_pos = qtd->isoc_split_pos;
        hc->complete_split = qtd->complete_split;
        hcd->fops->hub_info(hcd, urb->priv, &hub_addr, &port_addr);
        hc->hub_addr = (uint8_t) hub_addr;
        hc->port_addr = (uint8_t) port_addr;
    }

    switch (comip_hcd_get_pipe_type(&urb->pipe_info)) {
    case UE_CONTROL:
        hc->ep_type = COMIP_EP_TYPE_CONTROL;
        switch (qtd->control_phase) {
        case COMIP_CONTROL_SETUP:
            COMIP_DEBUGPL(DBG_HCDV, "  Control setup transaction\n");
            hc->do_ping = 0;
            hc->ep_is_in = 0;
            hc->data_pid_start = COMIP_HC_PID_SETUP;
            if (hcd->core_if->dma_enable) {
                hc->xfer_buff = (uint8_t *) urb->setup_dma;
            } else {
                hc->xfer_buff = (uint8_t *) urb->setup_packet;
            }
            hc->xfer_len = 8;
            ptr = NULL;
            break;
        case COMIP_CONTROL_DATA:
            COMIP_DEBUGPL(DBG_HCDV, "  Control data transaction\n");
            hc->data_pid_start = qtd->data_toggle;
            break;
        case COMIP_CONTROL_STATUS:
            /*
             * Direction is opposite of data direction or IN if no
             * data.
             */
            COMIP_DEBUGPL(DBG_HCDV, "  Control status transaction\n");
            if (urb->length == 0) {
                hc->ep_is_in = 1;
            } else {
                hc->ep_is_in =
                    comip_hcd_is_pipe_out(&urb->pipe_info);
            }
            if (hc->ep_is_in) {
                hc->do_ping = 0;
            }

            hc->data_pid_start = COMIP_HC_PID_DATA1;

            hc->xfer_len = 0;
            if (hcd->core_if->dma_enable) {
                hc->xfer_buff = (uint8_t *) hcd->status_buf_dma;
            } else {
                hc->xfer_buff = (uint8_t *) hcd->status_buf;
            }
            ptr = NULL;
            break;
        }
        break;
    case UE_BULK:
        hc->ep_type = COMIP_EP_TYPE_BULK;
        break;
    case UE_INTERRUPT:
        hc->ep_type = COMIP_EP_TYPE_INTR;
        break;
    case UE_ISOCHRONOUS:
        {
            struct comip_hcd_iso_packet_desc *frame_desc;

            hc->ep_type = COMIP_EP_TYPE_ISOC;

            if (hcd->core_if->dma_desc_enable)
                break;

            frame_desc = &urb->iso_descs[qtd->isoc_frame_index];

            frame_desc->status = 0;

            if (hcd->core_if->dma_enable) {
                hc->xfer_buff = (uint8_t *) urb->dma;
            } else {
                hc->xfer_buff = (uint8_t *) urb->buf;
            }
            hc->xfer_buff +=
                frame_desc->offset + qtd->isoc_split_offset;
            hc->xfer_len =
                frame_desc->length - qtd->isoc_split_offset;

            /* For non-dword aligned buffers */
            if (((uint32_t)hc->xfer_buff & 0x3)
                && hcd->core_if->dma_enable) {
                ptr =
                    (uint8_t *) urb->buf + frame_desc->offset +
                    qtd->isoc_split_offset;
            } else
                ptr = NULL;

            if (hc->xact_pos == COMIP_HCSPLIT_XACTPOS_ALL) {
                if (hc->xfer_len <= 188) {
                    hc->xact_pos = COMIP_HCSPLIT_XACTPOS_ALL;
                } else {
                    hc->xact_pos =
                        COMIP_HCSPLIT_XACTPOS_BEGIN;
                }
            }
        }
        break;
    }
    /* non DWORD-aligned buffer case */ 
    if (ptr) {
        uint32_t buf_size;
        if (hc->ep_type != COMIP_EP_TYPE_ISOC) {
            buf_size = hcd->core_if->core_params->max_transfer_size;
        } else {                
            buf_size = 4096;
        }
        if (!qh->dw_align_buf) {
            qh->dw_align_buf = COMIP_DMA_ALLOC_ATOMIC(buf_size,
                             &qh->dw_align_buf_dma);
            if (!qh->dw_align_buf) {
                ERR
                    ("%s: Failed to allocate memory to handle "
                     "non-dword aligned buffer case\n",
                     __func__);
                return;
            }
        }
        if (!hc->ep_is_in) {
            memcpy(qh->dw_align_buf, ptr, hc->xfer_len);
        }
        hc->align_buff = qh->dw_align_buf_dma;
    } else {
        hc->align_buff = 0;
    }

    if (hc->ep_type == COMIP_EP_TYPE_INTR ||
        hc->ep_type == COMIP_EP_TYPE_ISOC) {
        /*
         * This value may be modified when the transfer is started to
         * reflect the actual transfer length.
         */
        hc->multi_count = comip_hb_mult(qh->maxp);
    }

    if (hcd->core_if->dma_desc_enable)
        hc->desc_list_addr = qh->desc_list_dma;

    comip_hc_init(hcd->core_if, hc);
    hc->qh = qh;
}

/**
 * This function selects transactions from the HCD transfer schedule and
 * assigns them to available host channels. It is called from HCD interrupt
 * handler functions.
 *
 * @param hcd The HCD state structure.
 *
 * @return The types of new transactions that were assigned to host channels.
 */
comip_transaction_type_e comip_hcd_select_transactions(comip_hcd_t * hcd)
{
    comip_list_link_t *qh_ptr;
    comip_qh_t *qh;
    int num_channels;
    comip_transaction_type_e ret_val = COMIP_TRANSACTION_NONE;

#ifdef DEBUG_SOF
    COMIP_DEBUGPL(DBG_HCD, "  Select Transactions\n");
#endif

    /* Process entries in the periodic ready list. */
    qh_ptr = COMIP_LIST_FIRST(&hcd->periodic_sched_ready);

    while (qh_ptr != &hcd->periodic_sched_ready &&
           !COMIP_CIRCLEQ_EMPTY(&hcd->free_hc_list)) {

        qh = COMIP_LIST_ENTRY(qh_ptr, comip_qh_t, qh_list_entry);
        assign_and_init_hc(hcd, qh);

        /*
         * Move the QH from the periodic ready schedule to the
         * periodic assigned schedule.
         */
        qh_ptr = COMIP_LIST_NEXT(qh_ptr);
        COMIP_LIST_MOVE_HEAD(&hcd->periodic_sched_assigned,
                   &qh->qh_list_entry);

        ret_val = COMIP_TRANSACTION_PERIODIC;
    }

    /*
     * Process entries in the inactive portion of the non-periodic
     * schedule. Some free host channels may not be used if they are
     * reserved for periodic transfers.
     */
    qh_ptr = hcd->non_periodic_sched_inactive.next;
    num_channels = hcd->core_if->core_params->host_channels;
    while (qh_ptr != &hcd->non_periodic_sched_inactive &&
           (hcd->non_periodic_channels <
        num_channels - hcd->periodic_channels) &&
           !COMIP_CIRCLEQ_EMPTY(&hcd->free_hc_list)) {

        qh = COMIP_LIST_ENTRY(qh_ptr, comip_qh_t, qh_list_entry);

        assign_and_init_hc(hcd, qh);

        /*
         * Move the QH from the non-periodic inactive schedule to the
         * non-periodic active schedule.
         */
        qh_ptr = COMIP_LIST_NEXT(qh_ptr);
        COMIP_LIST_MOVE_HEAD(&hcd->non_periodic_sched_active,
                   &qh->qh_list_entry);

        if (ret_val == COMIP_TRANSACTION_NONE) {
            ret_val = COMIP_TRANSACTION_NON_PERIODIC;
        } else {
            ret_val = COMIP_TRANSACTION_ALL;
        }

        hcd->non_periodic_channels++;
    }

    return ret_val;
}

/**
 * Attempts to queue a single transaction request for a host channel
 * associated with either a periodic or non-periodic transfer. This function
 * assumes that there is space available in the appropriate request queue. For
 * an OUT transfer or SETUP transaction in Slave mode, it checks whether space
 * is available in the appropriate Tx FIFO.
 *
 * @param hcd The HCD state structure.
 * @param hc Host channel descriptor associated with either a periodic or
 * non-periodic transfer.
 * @param fifo_dwords_avail Number of DWORDs available in the periodic Tx
 * FIFO for periodic transfers or the non-periodic Tx FIFO for non-periodic
 * transfers.
 *
 * @return 1 if a request is queued and more requests may be needed to
 * complete the transfer, 0 if no more requests are required for this
 * transfer, -1 if there is insufficient space in the Tx FIFO.
 */
static int queue_transaction(comip_hcd_t * hcd,
    comip_hc_t * hc, uint16_t fifo_dwords_avail)
{
    int retval;

    if (hcd->core_if->dma_enable) {
        if (hcd->core_if->dma_desc_enable) {
            if (!hc->xfer_started
                || (hc->ep_type == COMIP_EP_TYPE_ISOC)) {
                comip_hcd_start_xfer_ddma(hcd, hc->qh);
                hc->qh->ping_state = 0;
            }
        } else if (!hc->xfer_started) {
            comip_hc_start_transfer(hcd->core_if, hc);
            hc->qh->ping_state = 0;
        }
        retval = 0;
    } else if (hc->halt_pending) {
        /* Don't queue a request if the channel has been halted. */
        retval = 0;
    } else if (hc->halt_on_queue) {
        comip_hc_halt(hcd->core_if, hc, hc->halt_status);
        retval = 0;
    } else if (hc->do_ping) {
        if (!hc->xfer_started) {
            comip_hc_start_transfer(hcd->core_if, hc);
        }
        retval = 0;
    } else if (!hc->ep_is_in || hc->data_pid_start == COMIP_HC_PID_SETUP) {
        if ((fifo_dwords_avail * 4) >= hc->max_packet) {
            if (!hc->xfer_started) {
                comip_hc_start_transfer(hcd->core_if, hc);
                retval = 1;
            } else {
                retval =
                    comip_hc_continue_transfer(hcd->core_if,
                                 hc);
            }
        } else {
            retval = -1;
        }
    } else {
        if (!hc->xfer_started) {
            comip_hc_start_transfer(hcd->core_if, hc);
            retval = 1;
        } else {
            retval = comip_hc_continue_transfer(hcd->core_if, hc);
        }
    }

    return retval;
}

/**
 * Processes periodic channels for the next frame and queues transactions for
 * these channels to the comip controller. After queueing transactions, the
 * Periodic Tx FIFO Empty interrupt is enabled if there are more transactions
 * to queue as Periodic Tx FIFO or request queue space becomes available.
 * Otherwise, the Periodic Tx FIFO Empty interrupt is disabled.
 */
static void process_periodic_channels(comip_hcd_t * hcd)
{
    hptxsts_data_t tx_status;
    comip_list_link_t *qh_ptr;
    comip_qh_t *qh;
    int status;
    int no_queue_space = 0;
    int no_fifo_space = 0;

    comip_host_global_regs_t *host_regs;
    host_regs = hcd->core_if->host_if->host_global_regs;

    COMIP_DEBUGPL(DBG_HCDV, "Queue periodic transactions\n");
#ifdef DEBUG
    tx_status.d32 = readl(&host_regs->hptxsts);
    COMIP_DEBUGPL(DBG_HCDV,
            "  P Tx Req Queue Space Avail (before queue): %d\n",
            tx_status.b.ptxqspcavail);
    COMIP_DEBUGPL(DBG_HCDV, "  P Tx FIFO Space Avail (before queue): %d\n",
            tx_status.b.ptxfspcavail);
#endif

    qh_ptr = hcd->periodic_sched_assigned.next;
    while (qh_ptr != &hcd->periodic_sched_assigned) {
        tx_status.d32 = readl(&host_regs->hptxsts);
        if (tx_status.b.ptxqspcavail == 0) {
            no_queue_space = 1;
            break;
        }

        qh = COMIP_LIST_ENTRY(qh_ptr, comip_qh_t, qh_list_entry);

        /*
         * Set a flag if we're queuing high-bandwidth in slave mode.
         * The flag prevents any halts to get into the request queue in
         * the middle of multiple high-bandwidth packets getting queued.
         */
        if (!hcd->core_if->dma_enable && qh->channel->multi_count > 1) {
            hcd->core_if->queuing_high_bandwidth = 1;
        }
        status =
            queue_transaction(hcd, qh->channel,
                      tx_status.b.ptxfspcavail);
        if (status < 0) {
            no_fifo_space = 1;
            break;
        }

        /*
         * In Slave mode, stay on the current transfer until there is
         * nothing more to do or the high-bandwidth request count is
         * reached. In DMA mode, only need to queue one request. The
         * controller automatically handles multiple packets for
         * high-bandwidth transfers.
         */
        if (hcd->core_if->dma_enable || status == 0 ||
            qh->channel->requests == qh->channel->multi_count) {
            qh_ptr = qh_ptr->next;
            /*
             * Move the QH from the periodic assigned schedule to
             * the periodic queued schedule.
             */
            COMIP_LIST_MOVE_HEAD(&hcd->periodic_sched_queued,
                       &qh->qh_list_entry);

            /* done queuing high bandwidth */
            hcd->core_if->queuing_high_bandwidth = 0;
        }
    }

    if (!hcd->core_if->dma_enable) {
        comip_core_global_regs_t *global_regs;
        gintmsk_data_t intr_mask = {.d32 = 0 };

        global_regs = hcd->core_if->core_global_regs;
        intr_mask.b.ptxfempty = 1;
#ifdef DEBUG
        tx_status.d32 = readl(&host_regs->hptxsts);
        COMIP_DEBUGPL(DBG_HCDV,
                "  P Tx Req Queue Space Avail (after queue): %d\n",
                tx_status.b.ptxqspcavail);
        COMIP_DEBUGPL(DBG_HCDV,
                "  P Tx FIFO Space Avail (after queue): %d\n",
                tx_status.b.ptxfspcavail);
#endif
        if (!COMIP_LIST_EMPTY(&hcd->periodic_sched_assigned) ||
            no_queue_space || no_fifo_space) {
            /*
             * May need to queue more transactions as the request
             * queue or Tx FIFO empties. Enable the periodic Tx
             * FIFO empty interrupt. (Always use the half-empty
             * level to ensure that new requests are loaded as
             * soon as possible.)
             */
            COMIP_MODIFY_REG32(&global_regs->gintmsk, 0,
                     intr_mask.d32);
        } else {
            /*
             * Disable the Tx FIFO empty interrupt since there are
             * no more transactions that need to be queued right
             * now. This function is called from interrupt
             * handlers to queue more transactions as transfer
             * states change.
             */
            COMIP_MODIFY_REG32(&global_regs->gintmsk, intr_mask.d32,
                     0);
        }
    }
}

/**
 * Processes active non-periodic channels and queues transactions for these
 * channels to the comip controller. After queueing transactions, the NP Tx
 * FIFO Empty interrupt is enabled if there are more transactions to queue as
 * NP Tx FIFO or request queue space becomes available. Otherwise, the NP Tx
 * FIFO Empty interrupt is disabled.
 */
static void process_non_periodic_channels(comip_hcd_t * hcd)
{
    gnptxsts_data_t tx_status;
    comip_list_link_t *orig_qh_ptr;
    comip_qh_t *qh;
    int status;
    int no_queue_space = 0;
    int no_fifo_space = 0;
    int more_to_do = 0;

    comip_core_global_regs_t *global_regs =
        hcd->core_if->core_global_regs;

    COMIP_DEBUGPL(DBG_HCDV, "Queue non-periodic transactions\n");
#ifdef DEBUG
    tx_status.d32 = readl(&global_regs->gnptxsts);
    COMIP_DEBUGPL(DBG_HCDV,
            "  NP Tx Req Queue Space Avail (before queue): %d\n",
            tx_status.b.nptxqspcavail);
    COMIP_DEBUGPL(DBG_HCDV, "  NP Tx FIFO Space Avail (before queue): %d\n",
            tx_status.b.nptxfspcavail);
#endif
    /*
     * Keep track of the starting point. Skip over the start-of-list
     * entry.
     */
    if (hcd->non_periodic_qh_ptr == &hcd->non_periodic_sched_active) {
        hcd->non_periodic_qh_ptr = hcd->non_periodic_qh_ptr->next;
    }
    orig_qh_ptr = hcd->non_periodic_qh_ptr;

    /*
     * Process once through the active list or until no more space is
     * available in the request queue or the Tx FIFO.
     */
    do {
        tx_status.d32 = readl(&global_regs->gnptxsts);
        if (!hcd->core_if->dma_enable && tx_status.b.nptxqspcavail == 0) {
            no_queue_space = 1;
            break;
        }

        qh = COMIP_LIST_ENTRY(hcd->non_periodic_qh_ptr, comip_qh_t,
                    qh_list_entry);
        status =
            queue_transaction(hcd, qh->channel,
                      tx_status.b.nptxfspcavail);

        if (status > 0) {
            more_to_do = 1;
        } else if (status < 0) {
            no_fifo_space = 1;
            break;
        }

        /* Advance to next QH, skipping start-of-list entry. */
        hcd->non_periodic_qh_ptr = hcd->non_periodic_qh_ptr->next;
        if (hcd->non_periodic_qh_ptr == &hcd->non_periodic_sched_active) {
            hcd->non_periodic_qh_ptr =
                hcd->non_periodic_qh_ptr->next;
        }

    } while (hcd->non_periodic_qh_ptr != orig_qh_ptr);

    if (!hcd->core_if->dma_enable) {
        gintmsk_data_t intr_mask = {.d32 = 0 };
        intr_mask.b.nptxfempty = 1;

#ifdef DEBUG
        tx_status.d32 = readl(&global_regs->gnptxsts);
        COMIP_DEBUGPL(DBG_HCDV,
                "  NP Tx Req Queue Space Avail (after queue): %d\n",
                tx_status.b.nptxqspcavail);
        COMIP_DEBUGPL(DBG_HCDV,
                "  NP Tx FIFO Space Avail (after queue): %d\n",
                tx_status.b.nptxfspcavail);
#endif
        if (more_to_do || no_queue_space || no_fifo_space) {
            /*
             * May need to queue more transactions as the request
             * queue or Tx FIFO empties. Enable the non-periodic
             * Tx FIFO empty interrupt. (Always use the half-empty
             * level to ensure that new requests are loaded as
             * soon as possible.)
             */
            COMIP_MODIFY_REG32(&global_regs->gintmsk, 0,
                     intr_mask.d32);
        } else {
            /*
             * Disable the Tx FIFO empty interrupt since there are
             * no more transactions that need to be queued right
             * now. This function is called from interrupt
             * handlers to queue more transactions as transfer
             * states change.
             */
            COMIP_MODIFY_REG32(&global_regs->gintmsk, intr_mask.d32,
                     0);
        }
    }
}

/**
 * This function processes the currently active host channels and queues
 * transactions for these channels to the comip controller. It is called
 * from HCD interrupt handler functions.
 *
 * @param hcd The HCD state structure.
 * @param tr_type The type(s) of transactions to queue (non-periodic,
 * periodic, or both).
 */
void comip_hcd_queue_transactions(comip_hcd_t * hcd,
    comip_transaction_type_e tr_type)
{
#ifdef DEBUG_SOF
    COMIP_DEBUGPL(DBG_HCD, "Queue Transactions\n");
#endif
    /* Process host channels associated with periodic transfers. */
    if ((tr_type == COMIP_TRANSACTION_PERIODIC ||
         tr_type == COMIP_TRANSACTION_ALL) &&
        !COMIP_LIST_EMPTY(&hcd->periodic_sched_assigned)) {

        process_periodic_channels(hcd);
    }

    /* Process host channels associated with non-periodic transfers. */
    if (tr_type == COMIP_TRANSACTION_NON_PERIODIC ||
        tr_type == COMIP_TRANSACTION_ALL) {
        if (!COMIP_LIST_EMPTY(&hcd->non_periodic_sched_active)) {
            process_non_periodic_channels(hcd);
        } else {
            /*
             * Ensure NP Tx FIFO empty interrupt is disabled when
             * there are no non-periodic transfers to process.
             */
            gintmsk_data_t gintmsk = {.d32 = 0 };
            gintmsk.b.nptxfempty = 1;
            COMIP_MODIFY_REG32(&hcd->core_if->
                     core_global_regs->gintmsk, gintmsk.d32,
                     0);
        }
    }
}

#ifdef COMIP_HS_ELECT_TST
/*
 * Quick and dirty hack to implement the HS Electrical Test
 * SINGLE_STEP_GET_DEVICE_DESCRIPTOR feature.
 *
 * This code was copied from our userspace app "hset". It sends a
 * Get Device Descriptor control sequence in two parts, first the
 * Setup packet by itself, followed some time later by the In and
 * Ack packets. Rather than trying to figure out how to add this
 * functionality to the normal driver code, we just hijack the
 * hardware, using these two function to drive the hardware
 * directly.
 */

static comip_core_global_regs_t *global_regs;
static comip_host_global_regs_t *hc_global_regs;
static comip_hc_regs_t *hc_regs;
static uint32_t *data_fifo;

static void do_setup(void)
{
    gintsts_data_t gintsts;
    hctsiz_data_t hctsiz;
    hcchar_data_t hcchar;
    haint_data_t haint;
    hcint_data_t hcint;

    /* Enable HAINTs */
    COMIP_WRITE_REG32(&hc_global_regs->haintmsk, 0x0001);

    /* Enable HCINTs */
    COMIP_WRITE_REG32(&hc_regs->hcintmsk, 0x04a3);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);

    /* Read HAINT */
    haint.d32 = readl(&hc_global_regs->haint);

    /* Read HCINT */
    hcint.d32 = readl(&hc_regs->hcint);

    /* Read HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);

    /* Clear HCINT */
    COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

    /* Clear HAINT */
    COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

    /* Clear GINTSTS */
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);

    /*
     * Send Setup packet (Get Device Descriptor)
     */

    /* Make sure channel is disabled */
    hcchar.d32 = readl(&hc_regs->hcchar);
    if (hcchar.b.chen) {
        hcchar.b.chdis = 1;
//              hcchar.b.chen = 1;
        COMIP_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);
        //sleep(1);
        mdelay(1000);

        /* Read GINTSTS */
        gintsts.d32 = readl(&global_regs->gintsts);

        /* Read HAINT */
        haint.d32 = readl(&hc_global_regs->haint);

        /* Read HCINT */
        hcint.d32 = readl(&hc_regs->hcint);

        /* Read HCCHAR */
        hcchar.d32 = readl(&hc_regs->hcchar);

        /* Clear HCINT */
        COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

        /* Clear HAINT */
        COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

        /* Clear GINTSTS */
        COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

        hcchar.d32 = readl(&hc_regs->hcchar);
    }

    /* Set HCTSIZ */
    hctsiz.d32 = 0;
    hctsiz.b.xfersize = 8;
    hctsiz.b.pktcnt = 1;
    hctsiz.b.pid = COMIP_HC_PID_SETUP;
    COMIP_WRITE_REG32(&hc_regs->hctsiz, hctsiz.d32);

    /* Set HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);
    hcchar.b.eptype = COMIP_EP_TYPE_CONTROL;
    hcchar.b.epdir = 0;
    hcchar.b.epnum = 0;
    hcchar.b.mps = 8;
    hcchar.b.chen = 1;
    COMIP_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);

    /* Fill FIFO with Setup data for Get Device Descriptor */
    data_fifo = (uint32_t *) ((char *)global_regs + 0x1000);
    COMIP_WRITE_REG32(data_fifo++, 0x01000680);
    COMIP_WRITE_REG32(data_fifo++, 0x00080000);

    gintsts.d32 = readl(&global_regs->gintsts);

    /* Wait for host channel interrupt */
    do {
        gintsts.d32 = readl(&global_regs->gintsts);
    } while (gintsts.b.hcintr == 0);

    /* Disable HCINTs */
    COMIP_WRITE_REG32(&hc_regs->hcintmsk, 0x0000);

    /* Disable HAINTs */
    COMIP_WRITE_REG32(&hc_global_regs->haintmsk, 0x0000);

    /* Read HAINT */
    haint.d32 = readl(&hc_global_regs->haint);

    /* Read HCINT */
    hcint.d32 = readl(&hc_regs->hcint);

    /* Read HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);

    /* Clear HCINT */
    COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

    /* Clear HAINT */
    COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

    /* Clear GINTSTS */
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);
}

static void do_in_ack(void)
{
    gintsts_data_t gintsts;
    hctsiz_data_t hctsiz;
    hcchar_data_t hcchar;
    haint_data_t haint;
    hcint_data_t hcint;
    host_grxsts_data_t grxsts;

    /* Enable HAINTs */
    COMIP_WRITE_REG32(&hc_global_regs->haintmsk, 0x0001);

    /* Enable HCINTs */
    COMIP_WRITE_REG32(&hc_regs->hcintmsk, 0x04a3);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);

    /* Read HAINT */
    haint.d32 = readl(&hc_global_regs->haint);

    /* Read HCINT */
    hcint.d32 = readl(&hc_regs->hcint);

    /* Read HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);

    /* Clear HCINT */
    COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

    /* Clear HAINT */
    COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

    /* Clear GINTSTS */
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);

    /*
     * Receive Control In packet
     */

    /* Make sure channel is disabled */
    hcchar.d32 = readl(&hc_regs->hcchar);
    if (hcchar.b.chen) {
        hcchar.b.chdis = 1;
        hcchar.b.chen = 1;
        COMIP_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);
        //sleep(1);
        mdelay(1000);

        /* Read GINTSTS */
        gintsts.d32 = readl(&global_regs->gintsts);

        /* Read HAINT */
        haint.d32 = readl(&hc_global_regs->haint);

        /* Read HCINT */
        hcint.d32 = readl(&hc_regs->hcint);

        /* Read HCCHAR */
        hcchar.d32 = readl(&hc_regs->hcchar);

        /* Clear HCINT */
        COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

        /* Clear HAINT */
        COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

        /* Clear GINTSTS */
        COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

        hcchar.d32 = readl(&hc_regs->hcchar);
    }

    /* Set HCTSIZ */
    hctsiz.d32 = 0;
    hctsiz.b.xfersize = 8;
    hctsiz.b.pktcnt = 1;
    hctsiz.b.pid = COMIP_HC_PID_DATA1;
    COMIP_WRITE_REG32(&hc_regs->hctsiz, hctsiz.d32);

    /* Set HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);
    hcchar.b.eptype = COMIP_EP_TYPE_CONTROL;
    hcchar.b.epdir = 1;
    hcchar.b.epnum = 0;
    hcchar.b.mps = 8;
    hcchar.b.chen = 1;
    COMIP_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);

    gintsts.d32 = readl(&global_regs->gintsts);

    /* Wait for receive status queue interrupt */
    do {
        gintsts.d32 = readl(&global_regs->gintsts);
    } while (gintsts.b.rxstsqlvl == 0);

    /* Read RXSTS */
    grxsts.d32 = readl(&global_regs->grxstsp);

    /* Clear RXSTSQLVL in GINTSTS */
    gintsts.d32 = 0;
    gintsts.b.rxstsqlvl = 1;
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    switch (grxsts.b.pktsts) {
    case COMIP_GRXSTS_PKTSTS_IN:
        /* Read the data into the host buffer */
        if (grxsts.b.bcnt > 0) {
            int i;
            int word_count = (grxsts.b.bcnt + 3) / 4;

            data_fifo = (uint32_t *) ((char *)global_regs + 0x1000);

            for (i = 0; i < word_count; i++) {
                (void)readl(data_fifo++);
            }
        }
        break;

    default:
        break;
    }

    gintsts.d32 = readl(&global_regs->gintsts);

    /* Wait for receive status queue interrupt */
    do {
        gintsts.d32 = readl(&global_regs->gintsts);
    } while (gintsts.b.rxstsqlvl == 0);

    /* Read RXSTS */
    grxsts.d32 = readl(&global_regs->grxstsp);

    /* Clear RXSTSQLVL in GINTSTS */
    gintsts.d32 = 0;
    gintsts.b.rxstsqlvl = 1;
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    switch (grxsts.b.pktsts) {
    case COMIP_GRXSTS_PKTSTS_IN_XFER_COMP:
        break;

    default:
        break;
    }

    gintsts.d32 = readl(&global_regs->gintsts);

    /* Wait for host channel interrupt */
    do {
        gintsts.d32 = readl(&global_regs->gintsts);
    } while (gintsts.b.hcintr == 0);

    /* Read HAINT */
    haint.d32 = readl(&hc_global_regs->haint);

    /* Read HCINT */
    hcint.d32 = readl(&hc_regs->hcint);

    /* Read HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);

    /* Clear HCINT */
    COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

    /* Clear HAINT */
    COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

    /* Clear GINTSTS */
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);

//      usleep(100000);
//      mdelay(100);
    mdelay(1);

    /*
     * Send handshake packet
     */

    /* Read HAINT */
    haint.d32 = readl(&hc_global_regs->haint);

    /* Read HCINT */
    hcint.d32 = readl(&hc_regs->hcint);

    /* Read HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);

    /* Clear HCINT */
    COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

    /* Clear HAINT */
    COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

    /* Clear GINTSTS */
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);

    /* Make sure channel is disabled */
    hcchar.d32 = readl(&hc_regs->hcchar);
    if (hcchar.b.chen) {
        hcchar.b.chdis = 1;
        hcchar.b.chen = 1;
        COMIP_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);
        //sleep(1);
        mdelay(1000);

        /* Read GINTSTS */
        gintsts.d32 = readl(&global_regs->gintsts);

        /* Read HAINT */
        haint.d32 = readl(&hc_global_regs->haint);

        /* Read HCINT */
        hcint.d32 = readl(&hc_regs->hcint);

        /* Read HCCHAR */
        hcchar.d32 = readl(&hc_regs->hcchar);

        /* Clear HCINT */
        COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

        /* Clear HAINT */
        COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

        /* Clear GINTSTS */
        COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

        hcchar.d32 = readl(&hc_regs->hcchar);
    }

    /* Set HCTSIZ */
    hctsiz.d32 = 0;
    hctsiz.b.xfersize = 0;
    hctsiz.b.pktcnt = 1;
    hctsiz.b.pid = COMIP_HC_PID_DATA1;
    COMIP_WRITE_REG32(&hc_regs->hctsiz, hctsiz.d32);

    /* Set HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);
    hcchar.b.eptype = COMIP_EP_TYPE_CONTROL;
    hcchar.b.epdir = 0;
    hcchar.b.epnum = 0;
    hcchar.b.mps = 8;
    hcchar.b.chen = 1;
    COMIP_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);

    gintsts.d32 = readl(&global_regs->gintsts);

    /* Wait for host channel interrupt */
    do {
        gintsts.d32 = readl(&global_regs->gintsts);
    } while (gintsts.b.hcintr == 0);

    /* Disable HCINTs */
    COMIP_WRITE_REG32(&hc_regs->hcintmsk, 0x0000);

    /* Disable HAINTs */
    COMIP_WRITE_REG32(&hc_global_regs->haintmsk, 0x0000);

    /* Read HAINT */
    haint.d32 = readl(&hc_global_regs->haint);

    /* Read HCINT */
    hcint.d32 = readl(&hc_regs->hcint);

    /* Read HCCHAR */
    hcchar.d32 = readl(&hc_regs->hcchar);

    /* Clear HCINT */
    COMIP_WRITE_REG32(&hc_regs->hcint, hcint.d32);

    /* Clear HAINT */
    COMIP_WRITE_REG32(&hc_global_regs->haint, haint.d32);

    /* Clear GINTSTS */
    COMIP_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

    /* Read GINTSTS */
    gintsts.d32 = readl(&global_regs->gintsts);
}
#endif

/** Handles hub class-specific requests. */
int comip_hcd_hub_control(comip_hcd_t * comip_hcd,
    uint16_t typeReq, uint16_t wValue, uint16_t wIndex, uint8_t * buf, uint16_t wLength)
{
    int retval = 0;

    comip_core_if_t *core_if = comip_hcd->core_if;
    usb_hub_descriptor_t *hub_desc;
    hprt0_data_t hprt0 = {.d32 = 0 };

    uint32_t port_status;

    switch (typeReq) {
    case UCR_CLEAR_HUB_FEATURE:
        COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                "ClearHubFeature 0x%x\n", wValue);
        switch (wValue) {
        case UHF_C_HUB_LOCAL_POWER:
        case UHF_C_HUB_OVER_CURRENT:
            /* Nothing required here */
            break;
        default:
            retval = -EINVAL;
            ERR("COMIP OTG HCD - "
                  "ClearHubFeature request %xh unknown\n",
                  wValue);
        }
        break;
    case UCR_CLEAR_PORT_FEATURE:
#ifdef CONFIG_USB_COMIP_LPM
        if (wValue != UHF_PORT_L1)
#endif
            if (!wIndex || wIndex > 1)
                goto error;

        switch (wValue) {
        case UHF_PORT_ENABLE:
            COMIP_DEBUGPL(DBG_ANY, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_ENABLE\n");
            hprt0.d32 = comip_read_hprt0(core_if);
            hprt0.b.prtena = 1;
            COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
            break;
        case UHF_PORT_SUSPEND:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_SUSPEND\n");

            if (core_if->power_down == 2) {
                comip_host_hibernation_restore(core_if, 0, 0);
            } else {
                COMIP_WRITE_REG32(core_if->pcgcctl, 0);
                mdelay(5);

                hprt0.d32 = comip_read_hprt0(core_if);
                hprt0.b.prtres = 1;
                COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
                hprt0.b.prtsusp = 0;
                /* Clear Resume bit */
                mdelay(100);
                hprt0.b.prtres = 0;
                COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
            }
            break;
#ifdef CONFIG_USB_comip_LPM
        case UHF_PORT_L1:
            {
                pcgcctl_data_t pcgcctl = {.d32 = 0 };
                glpmcfg_data_t lpmcfg = {.d32 = 0 };

                lpmcfg.d32 =
                    readl(&core_if->
                           core_global_regs->glpmcfg);
                lpmcfg.b.en_utmi_sleep = 0;
                lpmcfg.b.hird_thres &= (~(1 << 4));
                lpmcfg.b.prt_sleep_sts = 1;
                COMIP_WRITE_REG32(&core_if->
                        core_global_regs->glpmcfg,
                        lpmcfg.d32);

                /* Clear Enbl_L1Gating bit. */
                pcgcctl.b.enbl_sleep_gating = 1;
                COMIP_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32,
                         0);

                mdelay(5);

                hprt0.d32 = comip_read_hprt0(core_if);
                hprt0.b.prtres = 1;
                COMIP_WRITE_REG32(core_if->host_if->hprt0,
                        hprt0.d32);
                /* This bit will be cleared in wakeup interrupt handle */
                break;
            }
#endif
        case UHF_PORT_POWER:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_POWER\n");
            hprt0.d32 = comip_read_hprt0(core_if);
            hprt0.b.prtpwr = 0;
            COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
            break;
        case UHF_PORT_INDICATOR:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_INDICATOR\n");
            /* Port inidicator not supported */
            break;
        case UHF_C_PORT_CONNECTION:
            /* Clears drivers internal connect status change
             * flag */
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_C_CONNECTION\n");
            comip_hcd->flags.b.port_connect_status_change = 0;
            break;
        case UHF_C_PORT_RESET:
            /* Clears the driver's internal Port Reset Change
             * flag */
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_C_RESET\n");
            comip_hcd->flags.b.port_reset_change = 0;
            break;
        case UHF_C_PORT_ENABLE:
            /* Clears the driver's internal Port
             * Enable/Disable Change flag */
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_C_ENABLE\n");
            comip_hcd->flags.b.port_enable_change = 0;
            break;
        case UHF_C_PORT_SUSPEND:
            /* Clears the driver's internal Port Suspend
             * Change flag, which is set when resume signaling on
             * the host port is complete */
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_C_SUSPEND\n");
            comip_hcd->flags.b.port_suspend_change = 0;
            break;
#ifdef CONFIG_USB_COMIP_LPM
        case UHF_C_PORT_L1:
            comip_hcd->flags.b.port_l1_change = 0;
            break;
#endif
        case UHF_C_PORT_OVER_CURRENT:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "ClearPortFeature USB_PORT_FEAT_C_OVER_CURRENT\n");
            comip_hcd->flags.b.port_over_current_change = 0;
            break;
        default:
            retval = -EINVAL;
            ERR("COMIP OTG HCD - "
                  "ClearPortFeature request %xh "
                  "unknown or unsupported\n", wValue);
        }
        break;
    case UCR_GET_HUB_DESCRIPTOR:
        COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                "GetHubDescriptor\n");
        hub_desc = (usb_hub_descriptor_t *) buf;
        hub_desc->bDescLength = 9;
        hub_desc->bDescriptorType = 0x29;
        hub_desc->bNbrPorts = 1;
        USETW(hub_desc->wHubCharacteristics, 0x08);
        hub_desc->bPwrOn2PwrGood = 1;
        hub_desc->bHubContrCurrent = 0;
        hub_desc->DeviceRemovable[0] = 0;
        hub_desc->DeviceRemovable[1] = 0xff;
        break;
    case UCR_GET_HUB_STATUS:
        COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                "GetHubStatus\n");
        memset(buf, 0, 4);
        break;
    case UCR_GET_PORT_STATUS:
        COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                "GetPortStatus wIndex = 0x%04x FLAGS=0x%08x\n",
                wIndex, comip_hcd->flags.d32);
        if (!wIndex || wIndex > 1)
            goto error;

        port_status = 0;

        if (comip_hcd->flags.b.port_connect_status_change)
            port_status |= (1 << UHF_C_PORT_CONNECTION);

        if (comip_hcd->flags.b.port_enable_change)
            port_status |= (1 << UHF_C_PORT_ENABLE);

        if (comip_hcd->flags.b.port_suspend_change)
            port_status |= (1 << UHF_C_PORT_SUSPEND);

        if (comip_hcd->flags.b.port_l1_change)
            port_status |= (1 << UHF_C_PORT_L1);

        if (comip_hcd->flags.b.port_reset_change) {
            port_status |= (1 << UHF_C_PORT_RESET);
        }

        if (comip_hcd->flags.b.port_over_current_change) {
            COMIP_WARN("Overcurrent change detected\n");
            port_status |= (1 << UHF_C_PORT_OVER_CURRENT);
        }

        if (!comip_hcd->flags.b.port_connect_status) {
            /*
             * The port is disconnected, which means the core is
             * either in device mode or it soon will be. Just
             * return 0's for the remainder of the port status
             * since the port register can't be read if the core
             * is in device mode.
             */
            *((__le32 *) buf) = comip_cpu_to_le32(&port_status);
            break;
        }

        hprt0.d32 = readl(core_if->host_if->hprt0);
        COMIP_DEBUGPL(DBG_HCDV, "  HPRT0: 0x%08x\n", hprt0.d32);

        if (hprt0.b.prtconnsts)
            port_status |= (1 << UHF_PORT_CONNECTION);

        if (hprt0.b.prtena)
            port_status |= (1 << UHF_PORT_ENABLE);

        if (hprt0.b.prtsusp)
            port_status |= (1 << UHF_PORT_SUSPEND);

        if (hprt0.b.prtovrcurract)
            port_status |= (1 << UHF_PORT_OVER_CURRENT);

        if (hprt0.b.prtrst)
            port_status |= (1 << UHF_PORT_RESET);

        if (hprt0.b.prtpwr)
            port_status |= (1 << UHF_PORT_POWER);

        if (hprt0.b.prtspd == COMIP_HPRT0_PRTSPD_HIGH_SPEED)
            port_status |= (1 << UHF_PORT_HIGH_SPEED);
        else if (hprt0.b.prtspd == COMIP_HPRT0_PRTSPD_LOW_SPEED)
            port_status |= (1 << UHF_PORT_LOW_SPEED);

        if (hprt0.b.prttstctl)
            port_status |= (1 << UHF_PORT_TEST);
        if (comip_get_lpm_portsleepstatus(comip_hcd->core_if)) {
            port_status |= (1 << UHF_PORT_L1);
        }
        /*
           For Synopsys HW emulation of Power down wkup_control asserts the 
           hreset_n and prst_n on suspned. This causes the HPRT0 to be zero. 
           We intentionally tell the software that port is in L2Suspend state. 
           Only for STE.
        */
        if ((core_if->power_down == 2)
            && (core_if->hibernation_suspend == 1)) {
            port_status |= (1 << UHF_PORT_SUSPEND);
        }
        /* USB_PORT_FEAT_INDICATOR unsupported always 0 */

        *((__le32 *) buf) = comip_cpu_to_le32(&port_status);

        break;
    case UCR_SET_HUB_FEATURE:
        COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                "SetHubFeature\n");
        /* No HUB features supported */
        break;
    case UCR_SET_PORT_FEATURE:
        if (wValue != UHF_PORT_TEST && (!wIndex || wIndex > 1))
            goto error;

        if (!comip_hcd->flags.b.port_connect_status) {
            /*
             * The port is disconnected, which means the core is
             * either in device mode or it soon will be. Just
             * return without doing anything since the port
             * register can't be written if the core is in device
             * mode.
             */
            break;
        }

        switch (wValue) {
        case UHF_PORT_SUSPEND:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "SetPortFeature - USB_PORT_FEAT_SUSPEND\n");
            if (comip_hcd_otg_port(comip_hcd) != wIndex) {
                goto error;
            }
            if (core_if->power_down == 2) {
                int timeout = 300;
                comip_irqflags_t flags;
                pcgcctl_data_t pcgcctl = {.d32 = 0 };
                gpwrdn_data_t gpwrdn = {.d32 = 0 };
                gusbcfg_data_t gusbcfg = {.d32 = 0 };
#ifdef COMIP_DEV_SRPCAP
                int32_t otg_cap_param = core_if->core_params->otg_cap;
#endif
                COMIP_PRINTF("Preparing for complete power-off\n");

                /* Save registers before hibernation */
                comip_save_global_regs(core_if);
                comip_save_host_regs(core_if);

                hprt0.d32 = comip_read_hprt0(core_if);
                hprt0.b.prtsusp = 1;
                hprt0.b.prtena = 0;
                COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
                /* Spin hprt0.b.prtsusp to became 1 */
                do {
                    hprt0.d32 = comip_read_hprt0(core_if);
                    if (hprt0.b.prtsusp) {
                        break;
                    }
                    mdelay(1);
                } while (--timeout);
                if (!timeout) {
                    COMIP_WARN("Suspend wasn't genereted\n");
                }
                udelay(10);

                /*
                 * We need to disable interrupts to prevent servicing of any IRQ
                 * during going to hibernation
                 */
                COMIP_SPINLOCK_IRQSAVE(comip_hcd->lock, &flags);
                core_if->lx_state = COMIP_L2;
                gusbcfg.d32 =
                    readl(&core_if->core_global_regs->
                           gusbcfg);
                if (gusbcfg.b.ulpi_utmi_sel == 1) {
                    /* ULPI interface */
                    /* Suspend the Phy Clock */
                    pcgcctl.d32 = 0;
                    pcgcctl.b.stoppclk = 1;
                    COMIP_MODIFY_REG32(core_if->pcgcctl, 0,
                             pcgcctl.d32);
                    udelay(10);
                    gpwrdn.b.pmuactv = 1;
                    COMIP_MODIFY_REG32(&core_if->
                             core_global_regs->
                             gpwrdn, 0, gpwrdn.d32);
                } else {
                    /* UTMI+ Interface */
                    gpwrdn.b.pmuactv = 1;
                    COMIP_MODIFY_REG32(&core_if->
                             core_global_regs->
                             gpwrdn, 0, gpwrdn.d32);
                    udelay(10);
                    pcgcctl.b.stoppclk = 1;
                    COMIP_MODIFY_REG32(core_if->pcgcctl, 0, pcgcctl.d32);
                    udelay(10);
                }
#ifdef COMIP_DEV_SRPCAP                
                gpwrdn.d32 = 0;
                gpwrdn.b.dis_vbus = 1;
                COMIP_MODIFY_REG32(&core_if->core_global_regs->
                         gpwrdn, 0, gpwrdn.d32);
#endif
                gpwrdn.d32 = 0;
                gpwrdn.b.pmuintsel = 1;
                COMIP_MODIFY_REG32(&core_if->core_global_regs->
                         gpwrdn, 0, gpwrdn.d32);
                udelay(10);

                gpwrdn.d32 = 0;
#ifdef COMIP_DEV_SRPCAP
                gpwrdn.b.srp_det_msk = 1;
#endif
                gpwrdn.b.disconn_det_msk = 1;
                gpwrdn.b.lnstchng_msk = 1;
                gpwrdn.b.sts_chngint_msk = 1;
                COMIP_MODIFY_REG32(&core_if->core_global_regs->
                         gpwrdn, 0, gpwrdn.d32);
                udelay(10);

                /* Enable Power Down Clamp and all interrupts in GPWRDN */
                gpwrdn.d32 = 0;
                gpwrdn.b.pwrdnclmp = 1;
                COMIP_MODIFY_REG32(&core_if->core_global_regs->
                         gpwrdn, 0, gpwrdn.d32);
                udelay(10);

                /* Switch off VDD */
                gpwrdn.d32 = 0;
                gpwrdn.b.pwrdnswtch = 1;
                COMIP_MODIFY_REG32(&core_if->core_global_regs->
                         gpwrdn, 0, gpwrdn.d32);

#ifdef COMIP_DEV_SRPCAP
                if (otg_cap_param == COMIP_CAP_PARAM_HNP_SRP_CAPABLE)
                {
                    core_if->pwron_timer_started = 1;
                    COMIP_TIMER_SCHEDULE(core_if->pwron_timer, 6000 /* 6 secs */ );
                }
#endif
                /* Save gpwrdn register for further usage if stschng interrupt */
                core_if->gr_backup->gpwrdn_local =
                        readl(&core_if->core_global_regs->gpwrdn);

                /* Set flag to indicate that we are in hibernation */
                core_if->hibernation_suspend = 1;
                COMIP_SPINUNLOCK_IRQRESTORE(comip_hcd->lock,flags);

                COMIP_PRINTF("Host hibernation completed\n");
                // Exit from case statement
                break;

            }
            if (comip_hcd_otg_port(comip_hcd) == wIndex &&
                comip_hcd->fops->get_b_hnp_enable(comip_hcd)) {
                gotgctl_data_t gotgctl = {.d32 = 0 };
                gotgctl.b.hstsethnpen = 1;
                COMIP_MODIFY_REG32(&core_if->core_global_regs->
                         gotgctl, 0, gotgctl.d32);
            }
            hprt0.d32 = comip_read_hprt0(core_if);
            hprt0.b.prtsusp = 1;
            COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
            {
                comip_irqflags_t flags;
                /* Update lx_state */
                COMIP_SPINLOCK_IRQSAVE(comip_hcd->lock, &flags);
                core_if->lx_state = COMIP_L2;
                COMIP_SPINUNLOCK_IRQRESTORE(comip_hcd->lock, flags);
            }
            /* Suspend the Phy Clock */
            {
                pcgcctl_data_t pcgcctl = {.d32 = 0 };
                pcgcctl.b.stoppclk = 1;
                COMIP_MODIFY_REG32(core_if->pcgcctl, 0,
                         pcgcctl.d32);
                udelay(10);
            }

            /* For HNP the bus must be suspended for at least 200ms. */
            if (comip_hcd->fops->get_b_hnp_enable(comip_hcd)) {
                pcgcctl_data_t pcgcctl = {.d32 = 0 };
                pcgcctl.b.stoppclk = 1;
                COMIP_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32, 0);
                mdelay(200);
            }
            break;
        case UHF_PORT_POWER:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "SetPortFeature - USB_PORT_FEAT_POWER\n");
            hprt0.d32 = comip_read_hprt0(core_if);
            hprt0.b.prtpwr = 1;
            COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
            break;
        case UHF_PORT_RESET:
            if ((core_if->power_down == 2)
                && (core_if->hibernation_suspend == 1)) {
                /* If we are going to exit from Hibernated
                 * state via USB RESET.
                 */
                comip_host_hibernation_restore(core_if, 0, 1);
            } else {
                hprt0.d32 = comip_read_hprt0(core_if);

                COMIP_DEBUGPL(DBG_HCD,
                        "COMIP OTG HCD HUB CONTROL - "
                        "SetPortFeature - USB_PORT_FEAT_RESET\n");
                {
                    pcgcctl_data_t pcgcctl = {.d32 = 0 };
                    pcgcctl.b.enbl_sleep_gating = 1;
                    pcgcctl.b.stoppclk = 1;
                    COMIP_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32, 0);
                    COMIP_WRITE_REG32(core_if->pcgcctl, 0);
                }
#ifdef CONFIG_USB_COMIP_LPM
                {
                    glpmcfg_data_t lpmcfg;
                    lpmcfg.d32 =
                        readl(&core_if->core_global_regs->glpmcfg);
                    if (lpmcfg.b.prt_sleep_sts) {
                        lpmcfg.b.en_utmi_sleep = 0;
                        lpmcfg.b.hird_thres &= (~(1 << 4));
                        COMIP_WRITE_REG32
                            (&core_if->core_global_regs->glpmcfg,
                             lpmcfg.d32);
                        mdelay(1);
                    }
                }
#endif
                hprt0.d32 = comip_read_hprt0(core_if);
                /* Clear suspend bit if resetting from suspended state. */
                hprt0.b.prtsusp = 0;

                hprt0.b.prtpwr = 1;
                hprt0.b.prtrst = 1;
                COMIP_PRINTF("Indeed it is in host mode hprt0 = %08x\n",hprt0.d32);
                COMIP_WRITE_REG32(core_if->host_if->hprt0,
                        hprt0.d32);

                /* Clear reset bit in 10ms (FS/LS) or 50ms (HS) */
                mdelay(100);
                hprt0.b.prtrst = 0;
                COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
                core_if->lx_state = COMIP_L0;  /* Now back to the on state */
            }
            break;
#ifdef COMIP_HS_ELECT_TST
        case UHF_PORT_TEST:
            {
                uint32_t t;
                gintmsk_data_t gintmsk;

                t = (wIndex >> 8);  /* MSB wIndex USB */
                COMIP_DEBUGPL(DBG_HCD,
                        "COMIP OTG HCD HUB CONTROL - "
                        "SetPortFeature - USB_PORT_FEAT_TEST %d\n",
                        t);
                COMIP_WARN("USB_PORT_FEAT_TEST %d\n", t);
                if (t < 6) {
                    hprt0.d32 = comip_read_hprt0(core_if);
                    hprt0.b.prttstctl = t;
                    COMIP_WRITE_REG32(core_if->host_if->hprt0,
                            hprt0.d32);
                } else {
                    /* Setup global vars with reg addresses (quick and
                     * dirty hack, should be cleaned up)
                     */
                    global_regs = core_if->core_global_regs;
                    hc_global_regs =
                        core_if->host_if->host_global_regs;
                    hc_regs =
                        (comip_hc_regs_t *) ((char *)
                                   global_regs +
                                   0x500);
                    data_fifo =
                        (uint32_t *) ((char *)global_regs +
                              0x1000);

                    if (t == 6) {   /* HS_HOST_PORT_SUSPEND_RESUME */
                        /* Save current interrupt mask */
                        gintmsk.d32 =
                            readl
                            (&global_regs->gintmsk);

                        /* Disable all interrupts while we muck with
                         * the hardware directly
                         */
                        COMIP_WRITE_REG32(&global_regs->gintmsk, 0);

                        /* 15 second delay per the test spec */
                        mdelay(15000);

                        /* Drive suspend on the root port */
                        hprt0.d32 =
                            comip_read_hprt0(core_if);
                        hprt0.b.prtsusp = 1;
                        hprt0.b.prtres = 0;
                        COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

                        /* 15 second delay per the test spec */
                        mdelay(15000);

                        /* Drive resume on the root port */
                        hprt0.d32 =
                            comip_read_hprt0(core_if);
                        hprt0.b.prtsusp = 0;
                        hprt0.b.prtres = 1;
                        COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
                        mdelay(100);

                        /* Clear the resume bit */
                        hprt0.b.prtres = 0;
                        COMIP_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

                        /* Restore interrupts */
                        COMIP_WRITE_REG32(&global_regs->gintmsk, gintmsk.d32);
                    } else if (t == 7) {    /* SINGLE_STEP_GET_DEVICE_DESCRIPTOR setup */
                        /* Save current interrupt mask */
                        gintmsk.d32 =
                            readl
                            (&global_regs->gintmsk);

                        /* Disable all interrupts while we muck with
                         * the hardware directly
                         */
                        COMIP_WRITE_REG32(&global_regs->gintmsk, 0);

                        /* 15 second delay per the test spec */
                        mdelay(15000);

                        /* Send the Setup packet */
                        do_setup();

                        /* 15 second delay so nothing else happens for awhile */
                        mdelay(15000);

                        /* Restore interrupts */
                        COMIP_WRITE_REG32(&global_regs->gintmsk, gintmsk.d32);
                    } else if (t == 8) {    /* SINGLE_STEP_GET_DEVICE_DESCRIPTOR execute */
                        /* Save current interrupt mask */
                        gintmsk.d32 =
                            readl
                            (&global_regs->gintmsk);

                        /* Disable all interrupts while we muck with
                         * the hardware directly
                         */
                        COMIP_WRITE_REG32(&global_regs->gintmsk, 0);

                        /* Send the Setup packet */
                        do_setup();

                        /* 15 second delay so nothing else happens for awhile */
                        mdelay(15000);

                        /* Send the In and Ack packets */
                        do_in_ack();

                        /* 15 second delay so nothing else happens for awhile */
                        mdelay(15000);

                        /* Restore interrupts */
                        COMIP_WRITE_REG32(&global_regs->gintmsk, gintmsk.d32);
                    }
                }
                break;
            }
#endif /* COMIP_HS_ELECT_TST */

        case UHF_PORT_INDICATOR:
            COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB CONTROL - "
                    "SetPortFeature - USB_PORT_FEAT_INDICATOR\n");
            /* Not supported */
            break;
        default:
            retval = -EINVAL;
            ERR("COMIP OTG HCD - "
                  "SetPortFeature request %xh "
                  "unknown or unsupported\n", wValue);
            break;
        }
        break;
#ifdef CONFIG_USB_COMIP_LPM
    case UCR_SET_AND_TEST_PORT_FEATURE:
        if (wValue != UHF_PORT_L1) {
            goto error;
        }
        {
            int portnum, hird, devaddr, remwake;
            glpmcfg_data_t lpmcfg;
            uint32_t time_usecs;
            gintsts_data_t gintsts;
            gintmsk_data_t gintmsk;

            if (!comip_get_param_lpm_enable(core_if)) {
                goto error;
            }
            if (wValue != UHF_PORT_L1 || wLength != 1) {
                goto error;
            }
            /* Check if the port currently is in SLEEP state */
            lpmcfg.d32 =
                readl(&core_if->core_global_regs->glpmcfg);
            if (lpmcfg.b.prt_sleep_sts) {
                INFO("Port is already in sleep mode\n");
                buf[0] = 0; /* Return success */
                break;
            }

            portnum = wIndex & 0xf;
            hird = (wIndex >> 4) & 0xf;
            devaddr = (wIndex >> 8) & 0x7f;
            remwake = (wIndex >> 15);

            if (portnum != 1) {
                retval = -EINVAL;
                INFO("Wrong port number(%d) in SetandTestPortFeature request\n",
                     portnum);
                break;
            }

            DBG("SetandTestPortFeature request: portnum = %d, hird = %d, devaddr = %d, rewake = %d\n",
                 portnum, hird, devaddr, remwake);
            /* Disable LPM interrupt */
            gintmsk.d32 = 0;
            gintmsk.b.lpmtranrcvd = 1;
            COMIP_MODIFY_REG32(&core_if->core_global_regs->gintmsk,
                     gintmsk.d32, 0);

            if (comip_hcd_send_lpm
                (comip_hcd, devaddr, hird, remwake)) {
                retval = -EINVAL;
                break;
            }

            time_usecs = 10 * (lpmcfg.b.retry_count + 1);
            /* We will consider timeout if time_usecs microseconds pass,
             * and we don't receive LPM transaction status.
             * After receiving non-error responce(ACK/NYET/STALL) from device,
             *  core will set lpmtranrcvd bit.
             */
            do {
                gintsts.d32 =
                    readl(&core_if->core_global_regs->gintsts);
                if (gintsts.b.lpmtranrcvd) {
                    break;
                }
                udelay(1);
            } while (--time_usecs);
            /* lpm_int bit will be cleared in LPM interrupt handler */

            /* Now fill status
             * 0x00 - Success
             * 0x10 - NYET
             * 0x11 - Timeout
             */
            if (!gintsts.b.lpmtranrcvd) {
                buf[0] = 0x3;   /* Completion code is Timeout */
                comip_hcd_free_hc_from_lpm(comip_hcd);
            } else {
                lpmcfg.d32 =
                    readl(&core_if->core_global_regs->glpmcfg);
                if (lpmcfg.b.lpm_resp == 0x3) {
                    /* ACK responce from the device */
                    buf[0] = 0x00;  /* Success */
                } else if (lpmcfg.b.lpm_resp == 0x2) {
                    /* NYET responce from the device */
                    buf[0] = 0x2;
                } else {
                    /* Otherwise responce with Timeout */
                    buf[0] = 0x3;
                }
            }
            COMIP_PRINTF("Device responce to LPM trans is %x\n",
                   lpmcfg.b.lpm_resp);
            COMIP_MODIFY_REG32(&core_if->core_global_regs->gintmsk, 0,
                     gintmsk.d32);

            break;
        }
#endif /* CONFIG_USB_comip_LPM */
    default:
error:
        retval = -EINVAL;
        COMIP_WARN("COMIP OTG HCD - "
             "Unknown hub control request type or invalid typeReq: %xh wIndex: %xh wValue: %xh\n",
             typeReq, wIndex, wValue);
        break;
    }

    return retval;
}

#ifdef CONFIG_USB_COMIP_LPM
/** Returns index of host channel to perform LPM transaction. */
int comip_hcd_get_hc_for_lpm_tran(comip_hcd_t * hcd, uint8_t devaddr)
{
    comip_core_if_t *core_if = hcd->core_if;
    comip_hc_t *hc;
    hcchar_data_t hcchar;
    gintmsk_data_t gintmsk = {.d32 = 0 };

    if (COMIP_CIRCLEQ_EMPTY(&hcd->free_hc_list)) {
        COMIP_PRINTF("No free channel to select for LPM transaction\n");
        return -1;
    }

    hc = COMIP_CIRCLEQ_FIRST(&hcd->free_hc_list);

    /* Mask host channel interrupts. */
    gintmsk.b.hcintr = 1;
    COMIP_MODIFY_REG32(&core_if->core_global_regs->gintmsk, gintmsk.d32, 0);

    /* Fill fields that core needs for LPM transaction */
    hcchar.b.devaddr = devaddr;
    hcchar.b.epnum = 0;
    hcchar.b.eptype = COMIP_EP_TYPE_CONTROL;
    hcchar.b.mps = 64;
    hcchar.b.lspddev = (hc->speed == COMIP_EP_SPEED_LOW);
    hcchar.b.epdir = 0; /* OUT */
    COMIP_WRITE_REG32(&core_if->host_if->hc_regs[hc->hc_num]->hcchar,
            hcchar.d32);

    /* Remove the host channel from the free list. */
    COMIP_CIRCLEQ_REMOVE_INIT(&hcd->free_hc_list, hc, hc_list_entry);

    COMIP_PRINTF("hcnum = %d devaddr = %d\n", hc->hc_num, devaddr);

    return hc->hc_num;
}

/** Release hc after performing LPM transaction */
void comip_hcd_free_hc_from_lpm(comip_hcd_t * hcd)
{
    comip_hc_t *hc;
    glpmcfg_data_t lpmcfg;
    uint8_t hc_num;

    lpmcfg.d32 = readl(&hcd->core_if->core_global_regs->glpmcfg);
    hc_num = lpmcfg.b.lpm_chan_index;

    hc = hcd->hc_ptr_array[hc_num];

    COMIP_PRINTF("Freeing channel %d after LPM\n", hc_num);
    /* Return host channel to free list */
    COMIP_CIRCLEQ_INSERT_TAIL(&hcd->free_hc_list, hc, hc_list_entry);
}

int comip_hcd_send_lpm(comip_hcd_t * hcd, uint8_t devaddr, uint8_t hird,
             uint8_t bRemoteWake)
{
    glpmcfg_data_t lpmcfg;
    pcgcctl_data_t pcgcctl = {.d32 = 0 };
    int channel;

    channel = comip_hcd_get_hc_for_lpm_tran(hcd, devaddr);
    if (channel < 0) {
        return channel;
    }

    pcgcctl.b.enbl_sleep_gating = 1;
    COMIP_MODIFY_REG32(hcd->core_if->pcgcctl, 0, pcgcctl.d32);

    /* Read LPM config register */
    lpmcfg.d32 = readl(&hcd->core_if->core_global_regs->glpmcfg);

    /* Program LPM transaction fields */
    lpmcfg.b.rem_wkup_en = bRemoteWake;
    lpmcfg.b.hird = hird;
    lpmcfg.b.hird_thres = 0x1c;
    lpmcfg.b.lpm_chan_index = channel;
    lpmcfg.b.en_utmi_sleep = 1;
    /* Program LPM config register */
    COMIP_WRITE_REG32(&hcd->core_if->core_global_regs->glpmcfg, lpmcfg.d32);

    /* Send LPM transaction */
    lpmcfg.b.send_lpm = 1;
    COMIP_WRITE_REG32(&hcd->core_if->core_global_regs->glpmcfg, lpmcfg.d32);

    return 0;
}

#endif /* CONFIG_USB_comip_LPM */

int comip_hcd_is_status_changed(comip_hcd_t * hcd, int port)
{
    int retval;

    if (port != 1) {
        return -EINVAL;
    }

    retval = (hcd->flags.b.port_connect_status_change ||
          hcd->flags.b.port_reset_change ||
          hcd->flags.b.port_enable_change ||
          hcd->flags.b.port_suspend_change ||
          hcd->flags.b.port_over_current_change);
#ifdef DEBUG
    if (retval) {
        COMIP_DEBUGPL(DBG_HCD, "COMIP OTG HCD HUB STATUS DATA:"
                " Root port status changed\n");
        COMIP_DEBUGPL(DBG_HCDV, "  port_connect_status_change: %d\n",
                hcd->flags.b.port_connect_status_change);
        COMIP_DEBUGPL(DBG_HCDV, "  port_reset_change: %d\n",
                hcd->flags.b.port_reset_change);
        COMIP_DEBUGPL(DBG_HCDV, "  port_enable_change: %d\n",
                hcd->flags.b.port_enable_change);
        COMIP_DEBUGPL(DBG_HCDV, "  port_suspend_change: %d\n",
                hcd->flags.b.port_suspend_change);
        COMIP_DEBUGPL(DBG_HCDV, "  port_over_current_change: %d\n",
                hcd->flags.b.port_over_current_change);
    }
#endif
    return retval;
}

int comip_hcd_get_frame_number(comip_hcd_t * comip_hcd)
{
    hfnum_data_t hfnum;
    hfnum.d32 =
        readl(&comip_hcd->core_if->host_if->host_global_regs->
               hfnum);

#ifdef DEBUG_SOF
    COMIP_DEBUGPL(DBG_HCDV, "COMIP OTG HCD GET FRAME NUMBER %d\n",
            hfnum.b.frnum);
#endif
    return hfnum.b.frnum;
}

int comip_hcd_start(comip_hcd_t * hcd,
              struct comip_hcd_function_ops *fops)
{
    int retval = 0;

    hcd->fops = fops;
    if (!comip_is_device_mode(hcd->core_if) ) {
        comip_hcd_reinit(hcd);
    } else {
        retval = -ENODEV;
    }

    return retval;
}

void *comip_hcd_get_priv_data(comip_hcd_t * hcd)
{
    return hcd->priv;
}

void comip_hcd_set_priv_data(comip_hcd_t * hcd, void *priv_data)
{
    hcd->priv = priv_data;
}

uint32_t comip_hcd_otg_port(comip_hcd_t * hcd)
{
    return hcd->otg_port;
}

uint32_t comip_hcd_is_b_host(comip_hcd_t * hcd)
{
    uint32_t is_b_host;
    if (hcd->core_if->op_state == B_HOST) {
        is_b_host = 1;
    } else {
        is_b_host = 0;
    }

    return is_b_host;
}

comip_hcd_urb_t *comip_hcd_urb_alloc(comip_hcd_t * hcd,
    int iso_desc_count, int atomic_alloc)
{
    comip_hcd_urb_t *comip_urb;
    uint32_t size;

    size =
        sizeof(*comip_urb) +
        iso_desc_count * sizeof(struct comip_hcd_iso_packet_desc);
    if (atomic_alloc)
        comip_urb = kzalloc(size, GFP_ATOMIC);
    else
        comip_urb = kzalloc(size, GFP_KERNEL);

    comip_urb->packet_count = iso_desc_count;

    return comip_urb;
}

void comip_hcd_urb_set_pipeinfo(comip_hcd_urb_t * comip_urb,
    uint8_t dev_addr, uint8_t ep_num, uint8_t ep_type, uint8_t ep_dir, uint16_t mps)
{
    comip_hcd_fill_pipe(&comip_urb->pipe_info, dev_addr, ep_num,
                  ep_type, ep_dir, mps);
#ifdef DEBUG
    COMIP_DEBUGPL(DBG_HCDV,
        "addr = %d, ep_num = %d, ep_dir = 0x%x, ep_type = 0x%x, mps = %d\n",
         dev_addr, ep_num, ep_dir, ep_type, mps);
#endif
}

void comip_hcd_urb_set_params(comip_hcd_urb_t * comip_urb,
    void *urb_handle, void *buf, comip_dma_t dma, uint32_t buflen, void *setup_packet,
    comip_dma_t setup_dma, uint32_t flags, uint16_t interval)
{
    comip_urb->priv = urb_handle;
    comip_urb->buf = buf;
    comip_urb->dma = dma;
    comip_urb->length = buflen;
    comip_urb->setup_packet = setup_packet;
    comip_urb->setup_dma = setup_dma;
    comip_urb->flags = flags;
    comip_urb->interval = interval;
    comip_urb->status = -EINPROGRESS;
}

uint32_t comip_hcd_urb_get_status(comip_hcd_urb_t * comip_urb)
{
    return comip_urb->status;
}

uint32_t comip_hcd_urb_get_actual_length(comip_hcd_urb_t * comip_urb)
{
    return comip_urb->actual_length;
}

uint32_t comip_hcd_urb_get_error_count(comip_hcd_urb_t * comip_urb)
{
    return comip_urb->error_count;
}

void comip_hcd_urb_set_iso_desc_params(comip_hcd_urb_t * comip_urb,
    int desc_num, uint32_t offset, uint32_t length)
{
    comip_urb->iso_descs[desc_num].offset = offset;
    comip_urb->iso_descs[desc_num].length = length;
}

uint32_t comip_hcd_urb_get_iso_desc_status(comip_hcd_urb_t * comip_urb,
    int desc_num)
{
    return comip_urb->iso_descs[desc_num].status;
}

uint32_t comip_hcd_urb_get_iso_desc_actual_length(comip_hcd_urb_t *
    comip_urb, int desc_num)
{
    return comip_urb->iso_descs[desc_num].actual_length;
}

int comip_hcd_is_bandwidth_allocated(comip_hcd_t * hcd, void *ep_handle)
{
    int allocated = 0;
    comip_qh_t *qh = (comip_qh_t *) ep_handle;

    if (qh) {
        if (!COMIP_LIST_EMPTY(&qh->qh_list_entry)) {
            allocated = 1;
        }
    }
    return allocated;
}

int comip_hcd_is_bandwidth_freed(comip_hcd_t * hcd, void *ep_handle)
{
    comip_qh_t *qh = (comip_qh_t *) ep_handle;
    int freed = 0;
    COMIP_ASSERT(qh, "qh is not allocated\n");

    if (COMIP_LIST_EMPTY(&qh->qh_list_entry)) {
        freed = 1;
    }

    return freed;
}

uint8_t comip_hcd_get_ep_bandwidth(comip_hcd_t * hcd, void *ep_handle)
{
    comip_qh_t *qh = (comip_qh_t *) ep_handle;
    COMIP_ASSERT(qh, "qh is not allocated\n");
    return qh->usecs;
}

void comip_hcd_dump_state(comip_hcd_t * hcd)
{
#ifdef DEBUG
    int num_channels;
    int i;
    gnptxsts_data_t np_tx_status;
    hptxsts_data_t p_tx_status;

    num_channels = hcd->core_if->core_params->host_channels;
    COMIP_PRINTF("\n");
    COMIP_PRINTF
        ("************************************************************\n");
    COMIP_PRINTF("HCD State:\n");
    COMIP_PRINTF("  Num channels: %d\n", num_channels);
    for (i = 0; i < num_channels; i++) {
        comip_hc_t *hc = hcd->hc_ptr_array[i];
        COMIP_PRINTF("  Channel %d:\n", i);
        COMIP_PRINTF("    dev_addr: %d, ep_num: %d, ep_is_in: %d\n",
               hc->dev_addr, hc->ep_num, hc->ep_is_in);
        COMIP_PRINTF("    speed: %d\n", hc->speed);
        COMIP_PRINTF("    ep_type: %d\n", hc->ep_type);
        COMIP_PRINTF("    max_packet: %d\n", hc->max_packet);
        COMIP_PRINTF("    data_pid_start: %d\n", hc->data_pid_start);
        COMIP_PRINTF("    multi_count: %d\n", hc->multi_count);
        COMIP_PRINTF("    xfer_started: %d\n", hc->xfer_started);
        COMIP_PRINTF("    xfer_buff: %p\n", hc->xfer_buff);
        COMIP_PRINTF("    xfer_len: %d\n", hc->xfer_len);
        COMIP_PRINTF("    xfer_count: %d\n", hc->xfer_count);
        COMIP_PRINTF("    halt_on_queue: %d\n", hc->halt_on_queue);
        COMIP_PRINTF("    halt_pending: %d\n", hc->halt_pending);
        COMIP_PRINTF("    halt_status: %d\n", hc->halt_status);
        COMIP_PRINTF("    do_split: %d\n", hc->do_split);
        COMIP_PRINTF("    complete_split: %d\n", hc->complete_split);
        COMIP_PRINTF("    hub_addr: %d\n", hc->hub_addr);
        COMIP_PRINTF("    port_addr: %d\n", hc->port_addr);
        COMIP_PRINTF("    xact_pos: %d\n", hc->xact_pos);
        COMIP_PRINTF("    requests: %d\n", hc->requests);
        COMIP_PRINTF("    qh: %p\n", hc->qh);
        if (hc->xfer_started) {
            hfnum_data_t hfnum;
            hcchar_data_t hcchar;
            hctsiz_data_t hctsiz;
            hcint_data_t hcint;
            hcintmsk_data_t hcintmsk;
            hfnum.d32 =
                readl(&hcd->core_if->
                       host_if->host_global_regs->hfnum);
            hcchar.d32 =
                readl(&hcd->core_if->host_if->
                       hc_regs[i]->hcchar);
            hctsiz.d32 =
                readl(&hcd->core_if->host_if->
                       hc_regs[i]->hctsiz);
            hcint.d32 =
                readl(&hcd->core_if->host_if->
                       hc_regs[i]->hcint);
            hcintmsk.d32 =
                readl(&hcd->core_if->host_if->
                       hc_regs[i]->hcintmsk);
            COMIP_PRINTF("    hfnum: 0x%08x\n", hfnum.d32);
            COMIP_PRINTF("    hcchar: 0x%08x\n", hcchar.d32);
            COMIP_PRINTF("    hctsiz: 0x%08x\n", hctsiz.d32);
            COMIP_PRINTF("    hcint: 0x%08x\n", hcint.d32);
            COMIP_PRINTF("    hcintmsk: 0x%08x\n", hcintmsk.d32);
        }
        if (hc->xfer_started && hc->qh) {
            comip_qtd_t *qtd;
            comip_hcd_urb_t *urb;
            
            COMIP_CIRCLEQ_FOREACH(qtd, &hc->qh->qtd_list, qtd_list_entry) {
                if (!qtd->in_process)
                    break;
                
                urb = qtd->urb;
            COMIP_PRINTF("    URB Info:\n");
            COMIP_PRINTF("      qtd: %p, urb: %p\n", qtd, urb);
            if (urb) {
                COMIP_PRINTF("      Dev: %d, EP: %d %s\n",
                       comip_hcd_get_dev_addr(&urb->
                                    pipe_info),
                       comip_hcd_get_ep_num(&urb->
                                  pipe_info),
                       comip_hcd_is_pipe_in(&urb->
                                  pipe_info) ?
                       "IN" : "OUT");
                COMIP_PRINTF("      Max packet size: %d\n",
                       comip_hcd_get_mps(&urb->
                                   pipe_info));
                COMIP_PRINTF("      transfer_buffer: %p\n",
                       urb->buf);
                COMIP_PRINTF("      transfer_dma: %p\n",
                       (void *)urb->dma);
                COMIP_PRINTF("      transfer_buffer_length: %d\n",
                       urb->length);
                    COMIP_PRINTF("      actual_length: %d\n",
                           urb->actual_length);
                }
            }
        }
    }
    COMIP_PRINTF("  non_periodic_channels: %d\n", hcd->non_periodic_channels);
    COMIP_PRINTF("  periodic_channels: %d\n", hcd->periodic_channels);
    COMIP_PRINTF("  periodic_usecs: %d\n", hcd->periodic_usecs);
    np_tx_status.d32 =
        readl(&hcd->core_if->core_global_regs->gnptxsts);
    COMIP_PRINTF("  NP Tx Req Queue Space Avail: %d\n",
           np_tx_status.b.nptxqspcavail);
    COMIP_PRINTF("  NP Tx FIFO Space Avail: %d\n",
           np_tx_status.b.nptxfspcavail);
    p_tx_status.d32 =
        readl(&hcd->core_if->host_if->host_global_regs->hptxsts);
    COMIP_PRINTF("  P Tx Req Queue Space Avail: %d\n",
           p_tx_status.b.ptxqspcavail);
    COMIP_PRINTF("  P Tx FIFO Space Avail: %d\n", p_tx_status.b.ptxfspcavail);
    comip_hcd_dump_frrem(hcd);
    comip_dump_global_registers(hcd->core_if);
    comip_dump_host_registers(hcd->core_if);
    COMIP_PRINTF
        ("************************************************************\n");
    COMIP_PRINTF("\n");
#endif
}

#ifdef DEBUG
void comip_print_setup_data(uint8_t * setup)
{
    int i;
    if (CHK_DEBUG_LEVEL(DBG_HCD)) {
        COMIP_PRINTF("Setup Data = MSB ");
        for (i = 7; i >= 0; i--)
            COMIP_PRINTF("%02x ", setup[i]);
        COMIP_PRINTF("\n");
        COMIP_PRINTF("  bmRequestType Tranfer = %s\n",
               (setup[0] & 0x80) ? "Device-to-Host" :
               "Host-to-Device");
        COMIP_PRINTF("  bmRequestType Type = ");
        switch ((setup[0] & 0x60) >> 5) {
        case 0:
            COMIP_PRINTF("Standard\n");
            break;
        case 1:
            COMIP_PRINTF("Class\n");
            break;
        case 2:
            COMIP_PRINTF("Vendor\n");
            break;
        case 3:
            COMIP_PRINTF("Reserved\n");
            break;
        }
        COMIP_PRINTF("  bmRequestType Recipient = ");
        switch (setup[0] & 0x1f) {
        case 0:
            COMIP_PRINTF("Device\n");
            break;
        case 1:
            COMIP_PRINTF("Interface\n");
            break;
        case 2:
            COMIP_PRINTF("Endpoint\n");
            break;
        case 3:
            COMIP_PRINTF("Other\n");
            break;
        default:
            COMIP_PRINTF("Reserved\n");
            break;
        }
        COMIP_PRINTF("  bRequest = 0x%0x\n", setup[1]);
        COMIP_PRINTF("  wValue = 0x%0x\n", *((uint16_t *) & setup[2]));
        COMIP_PRINTF("  wIndex = 0x%0x\n", *((uint16_t *) & setup[4]));
        COMIP_PRINTF("  wLength = 0x%0x\n\n", *((uint16_t *) & setup[6]));
    }
}
#endif

void comip_hcd_dump_frrem(comip_hcd_t * hcd)
{

}
