/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTW_PWRCTRL_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>

#ifdef CONFIG_RTL8723A
#include <rtl8723a_hal.h>
#elif defined(CONFIG_RTL8723B)
#include <rtl8723b_hal.h>
#endif

#ifdef CONFIG_IPS
void ips_enter(_adapter * padapter)
{
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct xmit_priv *pxmit_priv = &padapter->xmitpriv;

#if (MP_DRIVER == 1)
	if (padapter->registrypriv.mp_mode == 1)
		return;
#endif

	if (pxmit_priv->free_xmitbuf_cnt != pxmit_priv->real_allocate_xmitbuf_cnt ||
		pxmit_priv->free_xmit_extbuf_cnt != NR_XMIT_EXTBUFF) {
		DBG_871X_LEVEL(_drv_err_, "ips_enter There are some pkts to transmit exit\n");
		DBG_871X("free_xmitbuf_cnt: %d, free_xmit_extbuf_cnt: %d\n",
			pxmit_priv->free_xmitbuf_cnt, pxmit_priv->free_xmit_extbuf_cnt);
		return;
	}

	_enter_pwrlock(&pwrpriv->lock);

	pwrpriv->bips_processing = _TRUE;

	// syn ips_mode with request
	pwrpriv->ips_mode = pwrpriv->ips_mode_req;

	pwrpriv->ips_enter_cnts++;
	DBG_871X("==>ips_enter cnts:%d\n",pwrpriv->ips_enter_cnts);
#ifdef CONFIG_BT_COEXIST
	BT_IpsNotify(padapter, pwrpriv->change_rfpwrstate);
#endif
	if(rf_off == pwrpriv->change_rfpwrstate )
	{
		pwrpriv->bpower_saving = _TRUE;
		DBG_871X("nolinked power save enter\n");

		if(pwrpriv->ips_mode == IPS_LEVEL_2)
			pwrpriv->bkeepfwalive = _TRUE;

		rtw_ips_pwr_down(padapter);
		pwrpriv->rf_pwrstate = rf_off;
	}
	pwrpriv->bips_processing = _FALSE;

	_exit_pwrlock(&pwrpriv->lock);

}

int ips_leave(_adapter * padapter)
{
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct security_priv* psecuritypriv=&(padapter->securitypriv);
	struct mlme_priv		*pmlmepriv = &(padapter->mlmepriv);
	int result = _SUCCESS;
	sint keyid;

	if(!is_primary_adapter(padapter))
		return _SUCCESS;

	_enter_pwrlock(&pwrpriv->lock);

	if((pwrpriv->rf_pwrstate == rf_off) &&(!pwrpriv->bips_processing))
	{
		pwrpriv->bips_processing = _TRUE;
		pwrpriv->change_rfpwrstate = rf_on;
		pwrpriv->ips_leave_cnts++;
		DBG_871X("==>ips_leave cnts:%d\n",pwrpriv->ips_leave_cnts);

		if ((result = rtw_ips_pwr_up(padapter)) == _SUCCESS) {
			pwrpriv->rf_pwrstate = rf_on;
		}
		DBG_871X("nolinked power save leave\n");

		if((_WEP40_ == psecuritypriv->dot11PrivacyAlgrthm) ||(_WEP104_ == psecuritypriv->dot11PrivacyAlgrthm))
		{
			DBG_871X("==>%s,channel(%d),processing(%x)\n",__FUNCTION__,padapter->mlmeextpriv.cur_channel,pwrpriv->bips_processing);
			set_channel_bwmode(padapter, padapter->mlmeextpriv.cur_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
			for(keyid=0;keyid<4;keyid++){
				if(pmlmepriv->key_mask & BIT(keyid)){
					if(keyid == psecuritypriv->dot11PrivacyKeyIndex)
						result=rtw_set_key(padapter,psecuritypriv, keyid, 1);
					else
						result=rtw_set_key(padapter,psecuritypriv, keyid, 0);
				}
			}
		}
#ifdef CONFIG_BT_COEXIST
		BT_IpsNotify(padapter, pwrpriv->change_rfpwrstate);
#endif
		DBG_871X("==> ips_leave.....LED(0x%08x)...\n",rtw_read32(padapter,0x4c));
		pwrpriv->bips_processing = _FALSE;

		pwrpriv->bkeepfwalive = _FALSE;
		pwrpriv->bpower_saving = _FALSE;
	}

	_exit_pwrlock(&pwrpriv->lock);

	return result;
}


#endif

#ifdef CONFIG_AUTOSUSPEND
extern void autosuspend_enter(_adapter* padapter);
extern int autoresume_enter(_adapter* padapter);
#endif

#ifdef SUPPORT_HW_RFOFF_DETECTED
int rtw_hw_suspend(_adapter *padapter );
int rtw_hw_resume(_adapter *padapter);
#endif

bool rtw_pwr_unassociated_idle(_adapter *adapter)
{
	_adapter *buddy = adapter->pbuddy_adapter;
	struct mlme_priv *pmlmepriv = &(adapter->mlmepriv);
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &(adapter->wdinfo);
#ifdef CONFIG_IOCTL_CFG80211
	struct cfg80211_wifidirect_info *pcfg80211_wdinfo = &adapter->cfg80211_wdinfo;
#endif
#endif

	bool ret = _FALSE;

	if (adapter_to_pwrctl(adapter)->ips_deny_time >= rtw_get_current_time()) {
		//DBG_871X("%s ips_deny_time\n", __func__);
		goto exit;
	}

	if (check_fwstate(pmlmepriv, _FW_LINKED|_FW_UNDER_SURVEY|_FW_UNDER_LINKING) == _TRUE
		|| check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE
		|| check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE | WIFI_ADHOC_STATE) == _TRUE
		#if defined(CONFIG_P2P) && defined(CONFIG_IOCTL_CFG80211) && defined(CONFIG_P2P_IPS)
		|| pcfg80211_wdinfo->is_ro_ch
		#elif defined(CONFIG_P2P)
		|| !rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE)
		#endif
		#if defined(CONFIG_P2P) && defined(CONFIG_IOCTL_CFG80211)
		|| rtw_get_passing_time_ms(pcfg80211_wdinfo->last_ro_ch_time) < 3000
		#endif
	) {
		goto exit;
	}

	/* consider buddy, if exist */
	if (buddy) {
		struct mlme_priv *b_pmlmepriv = &(buddy->mlmepriv);
		#ifdef CONFIG_P2P
		struct wifidirect_info *b_pwdinfo = &(buddy->wdinfo);
		#ifdef CONFIG_IOCTL_CFG80211
		struct cfg80211_wifidirect_info *b_pcfg80211_wdinfo = &buddy->cfg80211_wdinfo;
		#endif
		#endif

		if (check_fwstate(b_pmlmepriv, _FW_LINKED|_FW_UNDER_SURVEY|_FW_UNDER_LINKING) == _TRUE
			|| check_fwstate(b_pmlmepriv, WIFI_AP_STATE) == _TRUE
			|| check_fwstate(b_pmlmepriv, WIFI_ADHOC_MASTER_STATE | WIFI_ADHOC_STATE) == _TRUE
			#if defined(CONFIG_P2P) && defined(CONFIG_IOCTL_CFG80211) && defined(CONFIG_P2P_IPS)
			|| b_pcfg80211_wdinfo->is_ro_ch
			#elif defined(CONFIG_P2P)
			|| !rtw_p2p_chk_state(b_pwdinfo, P2P_STATE_NONE)
			#endif
		) {
			goto exit;
		}
	}
	ret = _TRUE;

exit:
	return ret;
}

#if defined (PLATFORM_LINUX)||defined (PLATFORM_FREEBSD)
void rtw_ps_processor(_adapter*padapter)
{
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo );
#endif //CONFIG_P2P
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
#ifdef SUPPORT_HW_RFOFF_DETECTED
	rt_rf_power_state rfpwrstate;
#endif //SUPPORT_HW_RFOFF_DETECTED

	pwrpriv->ps_processing = _TRUE;

#ifdef CONFIG_WOWLAN
	if(pwrpriv->wowlan_mode == _TRUE)
	{
		DBG_871X("%s(): wowlan_mode is TRUE, puspond ps processor!\n", __func__);
		goto exit;
	}
#endif

#ifdef SUPPORT_HW_RFOFF_DETECTED
	if(pwrpriv->bips_processing == _TRUE)
		goto exit;

	//DBG_871X("==> fw report state(0x%x)\n",rtw_read8(padapter,0x1ca));
	if(pwrpriv->bHWPwrPindetect)
	{
	#ifdef CONFIG_AUTOSUSPEND
		if(padapter->registrypriv.usbss_enable)
		{
			if(pwrpriv->rf_pwrstate == rf_on)
			{
				if(padapter->net_closed == _TRUE)
					pwrpriv->ps_flag = _TRUE;

				rfpwrstate = RfOnOffDetect(padapter);
				DBG_871X("@@@@- #1  %s==> rfstate:%s \n",__FUNCTION__,(rfpwrstate==rf_on)?"rf_on":"rf_off");
				if(rfpwrstate!= pwrpriv->rf_pwrstate)
				{
					if(rfpwrstate == rf_off)
					{
						pwrpriv->change_rfpwrstate = rf_off;

						pwrpriv->bkeepfwalive = _TRUE;
						pwrpriv->brfoffbyhw = _TRUE;

						autosuspend_enter(padapter);
					}
				}
			}
		}
		else
	#endif //CONFIG_AUTOSUSPEND
		{
			rfpwrstate = RfOnOffDetect(padapter);
			DBG_871X("@@@@- #2  %s==> rfstate:%s \n",__FUNCTION__,(rfpwrstate==rf_on)?"rf_on":"rf_off");

			if(rfpwrstate!= pwrpriv->rf_pwrstate)
			{
				if(rfpwrstate == rf_off)
				{
					pwrpriv->change_rfpwrstate = rf_off;
					pwrpriv->brfoffbyhw = _TRUE;
					padapter->bCardDisableWOHSM = _TRUE;
					rtw_hw_suspend(padapter );
				}
				else
				{
					pwrpriv->change_rfpwrstate = rf_on;
					rtw_hw_resume(padapter );
				}
				DBG_871X("current rf_pwrstate(%s)\n",(pwrpriv->rf_pwrstate == rf_off)?"rf_off":"rf_on");
			}
		}
		pwrpriv->pwr_state_check_cnts ++;
	}
#endif //SUPPORT_HW_RFOFF_DETECTED

	if (pwrpriv->ips_mode_req == IPS_NONE
		#ifdef CONFIG_CONCURRENT_MODE
		|| adapter_to_pwrctl(padapter->pbuddy_adapter)->ips_mode_req == IPS_NONE
		#endif
	)
		goto exit;

	if (rtw_pwr_unassociated_idle(padapter) == _FALSE)
		goto exit;

	if((pwrpriv->rf_pwrstate == rf_on) && ((pwrpriv->pwr_state_check_cnts%4)==0))
	{
		DBG_871X("==>%s .fw_state(%x)\n",__FUNCTION__,get_fwstate(pmlmepriv));
#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
#else
		pwrpriv->change_rfpwrstate = rf_off;
#endif
#ifdef CONFIG_AUTOSUSPEND
		if(padapter->registrypriv.usbss_enable)
		{
			if(pwrpriv->bHWPwrPindetect)
				pwrpriv->bkeepfwalive = _TRUE;

			if(padapter->net_closed == _TRUE)
				pwrpriv->ps_flag = _TRUE;

#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
			if(_TRUE==pwrpriv->bInternalAutoSuspend){
				DBG_871X("<==%s .pwrpriv->bInternalAutoSuspend)(%x)\n",__FUNCTION__,pwrpriv->bInternalAutoSuspend);
			} else {
				pwrpriv->change_rfpwrstate = rf_off;
				padapter->bCardDisableWOHSM = _TRUE;
				DBG_871X("<==%s .pwrpriv->bInternalAutoSuspend)(%x) call autosuspend_enter\n",__FUNCTION__,pwrpriv->bInternalAutoSuspend);
				autosuspend_enter(padapter);
			}
#else
			padapter->bCardDisableWOHSM = _TRUE;
			autosuspend_enter(padapter);
#endif	//if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
		}
		else if(pwrpriv->bHWPwrPindetect)
		{
		}
		else
#endif //CONFIG_AUTOSUSPEND
		{
#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
			pwrpriv->change_rfpwrstate = rf_off;
#endif	//defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)

			#ifdef CONFIG_IPS
			ips_enter(padapter);
			#endif
		}
	}
exit:
	rtw_set_pwr_state_check_timer(pwrpriv);
	pwrpriv->ps_processing = _FALSE;
	return;
}

void pwr_state_check_handler(void *FunctionContext);
void pwr_state_check_handler(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	rtw_ps_cmd(padapter);
}
#endif


#ifdef CONFIG_LPS
u8 rtw_wait_cpwm_interrupt(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	u8 ret = _TRUE;
	u32 count = 0;

	if (pwrpriv->cpwm > PS_STATE_S2)
		return ret;

	while (pwrpriv->cpwm < PS_STATE_S2) {
		if (count >= 25) {
			DBG_871X("%s: wait CPWM too long(%dms)! cpwm=0x%02x\n",
					__func__, count, pwrpriv->cpwm);
			break;
		}

		if ((padapter->bSurpriseRemoved == _TRUE) ||
			(padapter->hw_init_completed == _FALSE)) {
			pwrpriv->cpwm = PS_STATE_S4;
			break;
		}

		count ++;
		rtw_msleep_os(1);
	}

	if(count >= 25 && pwrpriv->cpwm < PS_STATE_S2) {
		ret = _FALSE;
	}

	return ret;
}

/*
 *
 * Parameters
 *	padapter
 *	pslv			power state level, only could be PS_STATE_S0 ~ PS_STATE_S4
 *
 */
void rtw_set_rpwm(PADAPTER padapter, u8 pslv)
{
	u8	rpwm;
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
	u8 tog = 0;
#endif

_func_enter_;
#if defined(CONFIG_RTL8188E) && !defined(CONFIG_WOWLAN)
	return;
#endif
	pslv = PS_STATE(pslv);


#ifdef CONFIG_BT_COEXIST
	if(BT_IsLowPwrDisable(padapter)) {
		if (pslv < PS_STATE_S4)
			pslv = PS_STATE_S3;
	}
#endif

#ifdef CONFIG_LPS_RPWM_TIMER
	if (pwrpriv->brpwmtimeout == _TRUE)
	{
		DBG_871X("%s: RPWM timeout, force to set RPWM(0x%02X) again!\n", __FUNCTION__, pslv);
	}
	else
#endif // CONFIG_LPS_RPWM_TIMER
	{
		/* here >= S2 can IO, but here we don't just for IO, so we need to set S3/S4 some times */
		/* e.g, we should set S4 before leave LPS, or RF maybe OFF after leave LPS */
		if (pwrpriv->rpwm == pslv)
		{
			RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_info_,
				("%s: Already set rpwm[0x%02X], new=0x%02X!\n", __FUNCTION__, pwrpriv->rpwm, pslv));
			return;
		}
	}

	if ((padapter->bSurpriseRemoved == _TRUE) ||
		(padapter->hw_init_completed == _FALSE))
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_err_,
				 ("%s: SurpriseRemoved(%d) hw_init_completed(%d)\n",
				  __FUNCTION__, padapter->bSurpriseRemoved, padapter->hw_init_completed));

		pwrpriv->cpwm = PS_STATE_S4;

		return;
	}

	if (padapter->bDriverStopped == _TRUE)
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_err_,
				 ("%s: change power state(0x%02X) when DriverStopped\n", __FUNCTION__, pslv));

		if (pslv < PS_STATE_S2) {
			RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_err_,
					 ("%s: Reject to enter PS_STATE(0x%02X) lower than S2 when DriverStopped!!\n", __FUNCTION__, pslv));
			return;
		}
	}

#if defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
	/* we find that tog bit sometimes will change no reason */
	/* so read tog bit before write instead of remember it in driver is right */
	rtw_hal_get_hwreg(padapter, HW_VAR_RPWM_TOG, &tog);
 	pwrpriv->tog = tog + 0x80;
#endif

	rpwm = pslv | pwrpriv->tog;
#ifdef CONFIG_LPS_LCLK
	// only when from PS_STATE S0/S1 to S2 and higher needs ACK
	if ((pwrpriv->cpwm < PS_STATE_S2) && (pslv >= PS_STATE_S2))
		rpwm |= PS_ACK;
#endif
	RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_info_,
			 ("rtw_set_rpwm: rpwm=0x%02x cpwm=0x%02x\n", rpwm, pwrpriv->cpwm));

	pwrpriv->rpwm = pslv;

#ifdef CONFIG_LPS_RPWM_TIMER
	if (rpwm & PS_ACK)
		_set_timer(&pwrpriv->pwr_rpwm_timer, LPS_RPWM_WAIT_MS);
#endif // CONFIG_LPS_RPWM_TIMER
	rtw_hal_set_hwreg(padapter, HW_VAR_SET_RPWM, (u8 *)(&rpwm));

#if (!defined CONFIG_SDIO_HCI) && (!defined CONFIG_GSPI_HCI)
	pwrpriv->tog += 0x80;
#endif

#ifdef CONFIG_LPS_LCLK
	// No LPS 32K, No Ack
	if (!(rpwm & PS_ACK))
#endif
	{
		pwrpriv->cpwm = pslv;
	}

#ifdef CONFIG_WAIT_PS_ACK
#ifdef CONFIG_LPS_LCLK
	if (rpwm & PS_ACK) {
#ifdef CONFIG_WOWLAN
		/* interrupt diabled, so we delay here */
		if (pwrpriv->wowlan_mode) {
			rtw_msleep_os(10);
			pwrpriv->cpwm = pslv;
		} else if (!rtw_wait_cpwm_interrupt(padapter)) {
			pwrpriv->cpwm = pslv;
			DBG_871X_LEVEL(_drv_err_, "cpwm time out, but wait enough time now \n");
		}
#else
		if (!rtw_wait_cpwm_interrupt(padapter)) {
			pwrpriv->cpwm = pslv;
			DBG_871X_LEVEL(_drv_err_, "cpwm time out, but wait enough time now \n");
		}
#endif
	}
#endif
#endif

_func_exit_;
}

u8 rtw_check_iot_peer_ps_should_pspoll(PADAPTER padapter)
{
	struct mlme_ext_info	*pmlmeinfo = &(padapter->mlmeextpriv.mlmext_info);
	u8 should_pspoll_mode = 0;

	//WAPI AP can not open smart PS
	if(is_IWN2410_AP(&(pmlmeinfo->network.MacAddress)))
	{
		DBG_871X("%s(): WAPI AP. close smart_ps \n", __func__);
		should_pspoll_mode = 1;
	}

	//Tenda W311R AP need receive ps-poll to send queued unicast data. Set smart_ps to 0.
	if (pmlmeinfo->assoc_AP_vendor == HT_IOT_PEER_TENDA)
	{
		DBG_871X("%s(): Tenda W311R. close smart_ps \n", __func__);
		should_pspoll_mode = 1;
	}

	DBG_871X("%s: should_pspoll_mode=%d\n", __func__, should_pspoll_mode);
	return should_pspoll_mode;
}

u8 PS_RDY_CHECK(_adapter * padapter);
u8 PS_RDY_CHECK(_adapter * padapter)
{
	u32 curr_time, delta_time;
	struct pwrctrl_priv	*pwrpriv = adapter_to_pwrctl(padapter);
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);


	curr_time = rtw_get_current_time();
	delta_time = curr_time -pwrpriv->DelayLPSLastTimeStamp;

	//under BT coex, we should enter LPS when we want
	//or, TDMA will wrong
#ifdef CONFIG_WOWLAN
	if ((delta_time < LPS_DELAY_TIME) && (_FALSE == pwrpriv->wowlan_mode))
#else
	if (delta_time < LPS_DELAY_TIME)
#endif //CONFIG_WOWLAN
	{
		return _FALSE;
	}

	if ((check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE) ||
		(check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) )
		return _FALSE;
#ifdef CONFIG_WOWLAN
	//if(_TRUE == pwrpriv->bInSuspend && pwrpriv->wowlan_mode)
//	if(pwrpriv->wowlan_mode)
//		return _TRUE;
//	else
//		return _FALSE;
	//if(pwrpriv->wowlan_mode)
	//	return _TRUE;
#else
	if(_TRUE == pwrpriv->bInSuspend )
		return _FALSE;
#endif //CONFIG_WOWLAN
	if( (padapter->securitypriv.dot11AuthAlgrthm == dot11AuthAlgrthm_8021X ||
		padapter->securitypriv.dot11AuthAlgrthm == dot11AuthAlgrthm_WAPI ||
		check_fwstate(pmlmepriv, WIFI_UNDER_WPS) == _TRUE) &&
		(padapter->securitypriv.bStaInstallPairwiseKey == _FALSE) )
	{
		//when bStaInstallPairwiseKey, hand shake is complete
		DBG_871X("Group handshake still in progress !!!\n");
		return _FALSE;
	}

	return _TRUE;
}

void rtw_set_ps_mode(PADAPTER padapter, u8 ps_mode, u8 smart_ps, u8 bcn_ant_mode)
{
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
#ifdef CONFIG_P2P
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo );
#endif //CONFIG_P2P
#ifdef CONFIG_TDLS
	struct sta_priv *pstapriv = &padapter->stapriv;
	_irqL irqL;
	int i, j;
	_list	*plist, *phead;
	struct sta_info *ptdls_sta;
#endif //CONFIG_TDLS

_func_enter_;

	DBG_871X("%s: PowerMode=%d Smart_PS=%d\n", __FUNCTION__, ps_mode, smart_ps);

	if(ps_mode >= PS_MODE_NUM) {
		RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("ps_mode:%d error\n", ps_mode));
		return;
	}

	if (pwrpriv->pwr_mode == ps_mode)
	{
		if (PS_MODE_ACTIVE == ps_mode) return;

		if ((pwrpriv->smart_ps == smart_ps) &&
			(pwrpriv->bcn_ant_mode == bcn_ant_mode))
		{
			return;
		}
	}

	//if(pwrpriv->pwr_mode == PS_MODE_ACTIVE)
	if(ps_mode == PS_MODE_ACTIVE)
	{
#ifdef CONFIG_P2P
		if(pwdinfo->opp_ps == 0)
#endif //CONFIG_P2P
		{
			DBG_871X("rtw_set_ps_mode: Leave 802.11 power save\n");

#ifdef CONFIG_TDLS
			_enter_critical_bh(&pstapriv->sta_hash_lock, &irqL);

			for(i=0; i< NUM_STA; i++)
			{
				phead = &(pstapriv->sta_hash[i]);
				plist = get_next(phead);

				while ((rtw_end_of_queue_search(phead, plist)) == _FALSE)
				{
					ptdls_sta = LIST_CONTAINOR(plist, struct sta_info, hash_list);

					if( ptdls_sta->tdls_sta_state & TDLS_LINKED_STATE )
						issue_nulldata_to_TDLS_peer_STA(padapter, ptdls_sta, 0);
					plist = get_next(plist);
				}
			}

			_exit_critical_bh(&pstapriv->sta_hash_lock, &irqL);
#endif //CONFIG_TDLS

			pwrpriv->smart_ps = 0;
			pwrpriv->bcn_ant_mode = 0;
			pwrpriv->pwr_mode = ps_mode;

			_enter_pwrlock(&pwrpriv->lps32klock);
			rtw_set_rpwm(padapter, PS_STATE_S4);
			_exit_pwrlock(&pwrpriv->lps32klock);

			rtw_hal_set_hwreg(padapter, HW_VAR_H2C_FW_PWRMODE, (u8 *)(&ps_mode));
#ifdef CONFIG_BT_COEXIST
			BT_LpsNotify(padapter, PS_STATE_S4);//BTC_LPS_DISABLE=0
#endif
			pwrpriv->bFwCurrentInPSMode = _FALSE;
		}
	}
	else
	{
		if (PS_RDY_CHECK(padapter)
#ifdef CONFIG_BT_COEXIST
#ifdef CONFIG_RTL8723A
			|| (BT_1Ant(padapter) == _TRUE)
#else
			|| BT_ForceExecPwrCmd(padapter)
#endif
#endif
			)
		{
			DBG_871X("rtw_set_ps_mode: Enter 802.11 power save\n");

#ifdef CONFIG_TDLS
			_enter_critical_bh(&pstapriv->sta_hash_lock, &irqL);

			for(i=0; i< NUM_STA; i++)
			{
				phead = &(pstapriv->sta_hash[i]);
				plist = get_next(phead);

				while ((rtw_end_of_queue_search(phead, plist)) == _FALSE)
				{
					ptdls_sta = LIST_CONTAINOR(plist, struct sta_info, hash_list);

					if( ptdls_sta->tdls_sta_state & TDLS_LINKED_STATE )
						issue_nulldata_to_TDLS_peer_STA(padapter, ptdls_sta, 1);
					plist = get_next(plist);
				}
			}

			_exit_critical_bh(&pstapriv->sta_hash_lock, &irqL);
#endif //CONFIG_TDLS

			pwrpriv->pwr_mode = ps_mode;
			if (rtw_check_iot_peer_ps_should_pspoll(padapter))
				pwrpriv->smart_ps = 0;
			else
				pwrpriv->smart_ps = smart_ps;
			pwrpriv->bcn_ant_mode = bcn_ant_mode;
			rtw_hal_set_hwreg(padapter, HW_VAR_H2C_FW_PWRMODE, (u8 *)(&ps_mode));
			pwrpriv->bFwCurrentInPSMode = _TRUE;

#ifdef CONFIG_P2P
			// Set CTWindow after LPS
			if(pwdinfo->opp_ps == 1)
			//if(pwdinfo->p2p_ps_enable == _TRUE)
				p2p_ps_wk_cmd(padapter, P2P_PS_ENABLE, 0);
#endif //CONFIG_P2P

#ifdef CONFIG_LPS_LCLK
			if (pwrpriv->alives == 0) {
				_enter_pwrlock(&pwrpriv->lps32klock);
				rtw_set_rpwm(padapter, PS_STATE_S0);
				_exit_pwrlock(&pwrpriv->lps32klock);
			}
			else
			{
				DBG_871X("%s(): alives=0x%x is not zero!!\n", __func__, pwrpriv->alives);
			}
#else
			rtw_set_rpwm(padapter, PS_STATE_S2);
#endif

#ifdef CONFIG_BT_COEXIST
			BT_LpsNotify(padapter, PS_STATE_S0);// BTC_LPS_ENABLE = 1
#endif
		}
		//else
		//{
		//	pwrpriv->pwr_mode = PS_MODE_ACTIVE;
		//}
	}

_func_exit_;
}

/*
 * Return:
 *	0:	Leave OK
 *	-1:	Timeout
 *	-2:	Other error
 */
s32 LPS_RF_ON_check(PADAPTER padapter, u32 delay_ms)
{
	u32 start_time;
	u8 bAwake = _FALSE;
	s32 err = 0;


	start_time = rtw_get_current_time();
	while (1)
	{
		rtw_hal_get_hwreg(padapter, HW_VAR_FWLPS_RF_ON, &bAwake);
		if (_TRUE == bAwake)
			break;

		if (_TRUE == padapter->bSurpriseRemoved)
		{
			err = -2;
			DBG_871X("%s: device surprise removed!!\n", __FUNCTION__);
			break;
		}

		if (rtw_get_passing_time_ms(start_time) > delay_ms)
		{
			err = -1;
			DBG_871X_LEVEL(_drv_err_, "%s: Wait for FW LPS leave more than %u ms!!!\n", __FUNCTION__, delay_ms);
			break;
		}
		rtw_usleep_os(100);
	}

	return err;
}

//
//	Description:
//		Enter the leisure power save mode.
//
void LPS_Enter(PADAPTER padapter)
{
	struct pwrctrl_priv	*pwrpriv = adapter_to_pwrctl(padapter);
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	_adapter *buddy = padapter->pbuddy_adapter;

_func_enter_;

//	DBG_871X("+LeisurePSEnter\n");
#ifdef CONFIG_WOWLAN
	if(pwrpriv->wowlan_mode == _TRUE) {
		DBG_871X("%s(): wowlan_mode is TRUE return",  __func__);
		return;
	}
#endif

#ifdef CONFIG_CONCURRENT_MODE
	if (padapter->iface_type != IFACE_PORT0)
		return; /* Skip power saving for concurrent mode port 1*/

	/* consider buddy, if exist */
	if (buddy) {
		struct mlme_priv *b_pmlmepriv = &(buddy->mlmepriv);
		#ifdef CONFIG_P2P
		struct wifidirect_info *b_pwdinfo = &(buddy->wdinfo);
		#ifdef CONFIG_IOCTL_CFG80211
		struct cfg80211_wifidirect_info *b_pcfg80211_wdinfo = &buddy->cfg80211_wdinfo;
		#endif
		#endif

		if (check_fwstate(b_pmlmepriv, WIFI_ASOC_STATE|WIFI_SITE_MONITOR)
			|| check_fwstate(b_pmlmepriv, WIFI_UNDER_LINKING|WIFI_UNDER_WPS)
			|| check_fwstate(b_pmlmepriv, WIFI_AP_STATE)
			|| check_fwstate(b_pmlmepriv, WIFI_ADHOC_MASTER_STATE|WIFI_ADHOC_STATE)
			#if defined(CONFIG_P2P) && defined(CONFIG_IOCTL_CFG80211) && defined(CONFIG_P2P_IPS)
			|| b_pcfg80211_wdinfo->is_ro_ch
			#elif defined(CONFIG_P2P)
			|| !rtw_p2p_chk_state(b_pwdinfo, P2P_STATE_NONE)
			#endif
			|| rtw_is_scan_deny(buddy)
		) {
			return;
		}
	}
#endif

#ifdef CONFIG_BT_COEXIST
	if(BT_IsBtControlLps(padapter))
	{
		return;
	}
#endif

	if (PS_RDY_CHECK(padapter) == _FALSE)
		return;

	if (pwrpriv->bLeisurePs)
	{
		if (pwrpriv->fw_psmode_iface_id != padapter->iface_id) {
			u8 mstatus = 1;//connect
			rtw_hal_set_hwreg(padapter, HW_VAR_H2C_FW_JOINBSSRPT, (u8 *)(&mstatus));
		}

		// Idle for a while if we connect to AP a while ago.
		if(pwrpriv->LpsIdleCount >= 2) //  4 Sec
		{
			if(pwrpriv->pwr_mode == PS_MODE_ACTIVE)
			{
				pwrpriv->bpower_saving = _TRUE;
				DBG_871X("%s smart_ps:%d\n", __func__, pwrpriv->smart_ps);
				rtw_set_ps_mode(padapter, pwrpriv->power_mgnt, padapter->registrypriv.smart_ps,0);
			}
		}
		else
			pwrpriv->LpsIdleCount++;
	}

//	DBG_871X("-LeisurePSEnter\n");

_func_exit_;
}


//
//	Description:
//		Leave the leisure power save mode.
//
void LPS_Leave(PADAPTER padapter)
{
	#define LPS_LEAVE_TIMEOUT_MS 100

	struct pwrctrl_priv	*pwrpriv = adapter_to_pwrctl(padapter);
	u32 start_time;
	u8 bAwake = _FALSE;

_func_enter_;

#ifdef CONFIG_BT_COEXIST
#ifdef CONFIG_RTL8723A
	/* 8723AS 1ant coex control all LPS action by set_ps_mode under linked */
	if (check_fwstate(&(padapter->mlmepriv), _FW_LINKED) == _TRUE &&
		BT_1Ant(padapter) == _TRUE) {
		return;
	}
#endif
#endif

#ifdef CONFIG_BT_COEXIST
	/* 8723BS 1ant coex just control TDMA LPS under bBtCtrlLps */
	/* 8723AS have set BT_IsBtControlLps FALSE */
	if(BT_IsBtControlLps(padapter))
	{
		return;
	}
#endif

#if defined(CONFIG_CONCURRENT_MODE)
	if (padapter->iface_type != IFACE_PORT0)
		return; /* Skip power saving for concurrent mode */
#endif

//	DBG_871X("+LeisurePSLeave\n");

	if (pwrpriv->bLeisurePs)
	{
		if(pwrpriv->pwr_mode != PS_MODE_ACTIVE)
		{
			rtw_set_ps_mode(padapter, PS_MODE_ACTIVE, 0, 0);

			LPS_RF_ON_check(padapter, LPS_LEAVE_TIMEOUT_MS);
		}
	}

	pwrpriv->bpower_saving = _FALSE;

//	DBG_871X("-LeisurePSLeave\n");

_func_exit_;
}

#endif

//
// Description: Leave all power save mode: LPS, FwLPS, IPS if needed.
// Move code to function by tynli. 2010.03.26.
//
void LeaveAllPowerSaveMode(IN PADAPTER Adapter)
{
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(Adapter);
	u8	enqueue = 0;

_func_enter_;

	//DBG_871X("%s.....\n",__FUNCTION__);
	if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
	{ //connect
#ifdef CONFIG_LPS_LCLK
		enqueue = 1;
#ifdef CONFIG_WOWLAN
		if(pwrctrlpriv->wowlan_mode)
			enqueue = 0;
#endif
#endif

#ifdef CONFIG_LPS
		//we dont need enqueue leave lps here because:
		//LPS_Leave_check can not wait it some times
		//under set_ps_mode we will leave 32k first
		rtw_lps_ctrl_wk_cmd(Adapter, LPS_CTRL_LEAVE, 0);//enqueue);
#endif

#ifdef CONFIG_P2P
		//after leave leave lps we can call P2P_PS_DISABLE directly without 32k
		p2p_ps_wk_cmd(Adapter, P2P_PS_DISABLE, 0);//enqueue);
#endif //CONFIG_P2P

#ifdef CONFIG_LPS_LCLK
		LPS_Leave_check(Adapter);
#endif
	}
	else
	{
		if(pwrctrlpriv->rf_pwrstate== rf_off)
		{
			#ifdef CONFIG_AUTOSUSPEND
			if(Adapter->registrypriv.usbss_enable)
			{
				#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,35))
				usb_disable_autosuspend(adapter_to_dvobj(Adapter)->pusbdev);
				#elif (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,22) && LINUX_VERSION_CODE<=KERNEL_VERSION(2,6,34))
				adapter_to_dvobj(Adapter)->pusbdev->autosuspend_disabled = Adapter->bDisableAutosuspend;//autosuspend disabled by the user
				#endif
			}
			else
			#endif
			{
#ifndef CONFIG_SUSPEND_NEW_FLOW
#if defined(CONFIG_PLATFORM_SPRD) // && (defined(CONFIG_RTL8723B) || defined(CONFIG_RTL8188E))
				#ifdef CONFIG_IPS
				if(_FALSE == ips_leave(Adapter))
				{
					DBG_871X("======> ips_leave fail.............\n");
				}
				#endif
#endif //CONFIG_PLATFORM_SPRD && CONFIG_RTL8188E
#endif
			}
		}
	}

_func_exit_;
}

#ifdef CONFIG_LPS_LCLK
void LPS_Leave_check(
	PADAPTER padapter)
{
	struct pwrctrl_priv *pwrpriv;
	u32	start_time;
	u8	bReady;

_func_enter_;

	pwrpriv = adapter_to_pwrctl(padapter);

	bReady = _FALSE;
	start_time = rtw_get_current_time();

	rtw_yield_os();

	while(1)
	{
		_enter_pwrlock(&pwrpriv->lock);

		if ((padapter->bSurpriseRemoved == _TRUE)
			|| (padapter->hw_init_completed == _FALSE)
#ifdef CONFIG_USB_HCI
			|| (padapter->bDriverStopped== _TRUE)
#endif
			|| (pwrpriv->pwr_mode == PS_MODE_ACTIVE)
			)
		{
			bReady = _TRUE;
		}

		_exit_pwrlock(&pwrpriv->lock);

		if(_TRUE == bReady)
			break;

		if(rtw_get_passing_time_ms(start_time)>100)
		{
			DBG_871X_LEVEL(_drv_err_, "Wait for cpwm event  than 100 ms!!!\n");
			break;
		}
		rtw_msleep_os(1);
	}

_func_exit_;
}

/*
 * Caller:ISR handler...
 *
 * This will be called when CPWM interrupt is up.
 *
 * using to update cpwn of drv; and drv willl make a decision to up or down pwr level
 */
void cpwm_int_hdl(
	PADAPTER padapter,
	struct reportpwrstate_parm *preportpwrstate)
{
	struct pwrctrl_priv *pwrpriv;

_func_enter_;

	pwrpriv = adapter_to_pwrctl(padapter);

#ifdef CONFIG_LPS_RPWM_TIMER
	_enter_pwrlock(&pwrpriv->lps32klock);
	if (pwrpriv->rpwm < PS_STATE_S2)
	{
		DBG_871X("%s: Redundant CPWM Int. RPWM=0x%02X CPWM=0x%02x\n", __func__, pwrpriv->rpwm, pwrpriv->cpwm);
		_exit_pwrlock(&pwrpriv->lps32klock);
		goto exit;
	}
#endif // CONFIG_LPS_RPWM_TIMER

	pwrpriv->cpwm = PS_STATE(preportpwrstate->state);
	pwrpriv->cpwm_tog = preportpwrstate->state & PS_TOGGLE;

	if (pwrpriv->cpwm >= PS_STATE_S2)
	{
#ifndef CONFIG_WAIT_PS_ACK
		if (pwrpriv->alives & CMD_ALIVE)
			_rtw_up_sema(&padapter->cmdpriv.cmd_queue_sema);

		if (pwrpriv->alives & XMIT_ALIVE)
			_rtw_up_sema(&padapter->xmitpriv.xmit_sema);
#endif
	}

#ifdef CONFIG_LPS_RPWM_TIMER
	_exit_pwrlock(&pwrpriv->lps32klock);
#endif

exit:
	RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_info_,
			 ("cpwm_int_hdl: cpwm=0x%02x\n", pwrpriv->cpwm));

_func_exit_;
}

static void cpwm_event_callback(struct work_struct *work)
{
	struct pwrctrl_priv *pwrpriv = container_of(work, struct pwrctrl_priv, cpwm_event);
	struct dvobj_priv *dvobj = pwrctl_to_dvobj(pwrpriv);
	_adapter *adapter = dvobj->if1;
	struct reportpwrstate_parm report;

	//DBG_871X("%s\n",__FUNCTION__);

	report.state = PS_STATE_S2;
	cpwm_int_hdl(adapter, &report);
}

#ifdef CONFIG_LPS_RPWM_TIMER
static void rpwmtimeout_workitem_callback(struct work_struct *work)
{
	PADAPTER padapter;
	struct dvobj_priv *dvobj;
	struct pwrctrl_priv *pwrpriv;


	pwrpriv = container_of(work, struct pwrctrl_priv, rpwmtimeoutwi);
	dvobj = pwrctl_to_dvobj(pwrpriv);
	padapter = dvobj->if1;
//	DBG_871X("+%s: rpwm=0x%02X cpwm=0x%02X\n", __func__, pwrpriv->rpwm, pwrpriv->cpwm);

	_enter_pwrlock(&pwrpriv->lps32klock);
	if ((pwrpriv->rpwm == pwrpriv->cpwm) || (pwrpriv->cpwm >= PS_STATE_S2))
	{
		DBG_871X("%s: rpwm=0x%02X cpwm=0x%02X CPWM done!\n", __func__, pwrpriv->rpwm, pwrpriv->cpwm);
		goto exit;
	}
	_exit_pwrlock(&pwrpriv->lps32klock);

	if (rtw_read8(padapter, 0x100) != 0xEA)
	{
#if 1
		struct reportpwrstate_parm report;

		report.state = PS_STATE_S2;
		DBG_871X("\n%s: FW already leave 32K!\n\n", __func__);
		cpwm_int_hdl(padapter, &report);
#else
		DBG_871X("\n%s: FW already leave 32K!\n\n", __func__);
		cpwm_event_callback(&pwrpriv->cpwm_event);
#endif
		return;
	}

	_enter_pwrlock(&pwrpriv->lps32klock);

	if ((pwrpriv->rpwm == pwrpriv->cpwm) || (pwrpriv->cpwm >= PS_STATE_S2))
	{
		DBG_871X("%s: cpwm=%d, nothing to do!\n", __func__, pwrpriv->cpwm);
		goto exit;
	}
	pwrpriv->brpwmtimeout = _TRUE;
	rtw_set_rpwm(padapter, pwrpriv->rpwm);
	pwrpriv->brpwmtimeout = _FALSE;

exit:
	_exit_pwrlock(&pwrpriv->lps32klock);
}

/*
 * This function is a timer handler, can't do any IO in it.
 */
static void pwr_rpwm_timeout_handler(void *FunctionContext)
{
	PADAPTER padapter;
	struct pwrctrl_priv *pwrpriv;


	padapter = (PADAPTER)FunctionContext;
	pwrpriv = adapter_to_pwrctl(padapter);
//	DBG_871X("+%s: rpwm=0x%02X cpwm=0x%02X\n", __func__, pwrpriv->rpwm, pwrpriv->cpwm);

	if ((pwrpriv->rpwm == pwrpriv->cpwm) || (pwrpriv->cpwm >= PS_STATE_S2))
	{
		DBG_871X("+%s: cpwm=%d, nothing to do!\n", __func__, pwrpriv->cpwm);
		return;
	}

	_set_workitem(&pwrpriv->rpwmtimeoutwi);
}
#endif // CONFIG_LPS_RPWM_TIMER

__inline static void register_task_alive(struct pwrctrl_priv *pwrctrl, u32 tag)
{
	pwrctrl->alives |= tag;
}

__inline static void unregister_task_alive(struct pwrctrl_priv *pwrctrl, u32 tag)
{
	pwrctrl->alives &= ~tag;
}

/*
 * Caller: FillH2CCmd
 *
 * Check if the fw_pwrstate is okay for H2C.
 * If not (cpwm is less than S3), then the sub-routine
 * will raise the cpwm to be greater than or equal to S3.
 *
 * Calling Context: Passive
 *
 * Return Value:
 *	 _SUCCESS	FillH2CCmd can set H2C to fw.
 *	 _FAIL		FillH2CCmd may can not set H2C to fw.
 */
s32 rtw_register_h2c_alive(PADAPTER padapter)
{
	s32 res;
	struct pwrctrl_priv *pwrctrl;
	u8 pslv;

	_func_enter_;

	res = _SUCCESS;
	pwrctrl = adapter_to_pwrctl(padapter);
#ifdef CONFIG_BT_COEXIST
	if (_TRUE == pwrctrl->btcoex_rfon)
		pslv = PS_STATE_S3;
	else
#endif
	{
		pslv = PS_STATE_S2;
	}

	_enter_pwrlock(&pwrctrl->lps32klock);

	register_task_alive(pwrctrl, H2C_ALIVE);

	if (pwrctrl->bFwCurrentInPSMode == _TRUE)
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_notice_,
				("rtw_register_h2c_alive: cpwm=0x%02x alives=0x%08x\n",
				 pwrctrl->cpwm, pwrctrl->alives));

		if (pwrctrl->cpwm < pslv)
		{
			if (pwrctrl->rpwm < pslv)
				rtw_set_rpwm(padapter, pslv);
			if (pwrctrl->cpwm < PS_STATE_S2) {
				DBG_871X("%s fail\n", __func__);
				res = _FAIL;
			}
		}
	}

	_exit_pwrlock(&pwrctrl->lps32klock);

	_func_exit_;

	return res;
}

/*
 * Caller: rtw_xmit_thread
 *
 * Check if the fw_pwrstate is okay for xmit.
 * If not (cpwm is less than S3), then the sub-routine
 * will raise the cpwm to be greater than or equal to S3.
 *
 * Calling Context: Passive
 *
 * Return Value:
 *	 _SUCCESS	rtw_xmit_thread can write fifo/txcmd afterwards.
 *	 _FAIL		rtw_xmit_thread can not do anything.
 */
s32 rtw_register_tx_alive(PADAPTER padapter)
{
	s32 res;
	struct pwrctrl_priv *pwrctrl;
	u8 pslv;

_func_enter_;

	res = _SUCCESS;
	pwrctrl = adapter_to_pwrctl(padapter);
#ifdef CONFIG_BT_COEXIST
	if (_TRUE == pwrctrl->btcoex_rfon)
		pslv = PS_STATE_S3;
	else
#endif
	{
		pslv = PS_STATE_S2;
	}

	_enter_pwrlock(&pwrctrl->lps32klock);

	register_task_alive(pwrctrl, XMIT_ALIVE);

	if (pwrctrl->bFwCurrentInPSMode == _TRUE)
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_info_,
				 ("rtw_register_tx_alive: cpwm=0x%02x alives=0x%08x\n",
				  pwrctrl->cpwm, pwrctrl->alives));

		if (pwrctrl->cpwm < pslv)
		{
			if (pwrctrl->rpwm < pslv)
				rtw_set_rpwm(padapter, pslv);
			if (pwrctrl->cpwm < PS_STATE_S2) {
				DBG_871X("%s fail\n", __func__);
				res = _FAIL;
			}
		}
	}

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;

	return res;
}

/*
 * Caller: rtw_cmd_thread
 *
 * Check if the fw_pwrstate is okay for issuing cmd.
 * If not (cpwm should be is less than S2), then the sub-routine
 * will raise the cpwm to be greater than or equal to S2.
 *
 * Calling Context: Passive
 *
 * Return Value:
 *	_SUCCESS	rtw_cmd_thread can issue cmds to firmware afterwards.
 *	_FAIL		rtw_cmd_thread can not do anything.
 */
s32 rtw_register_cmd_alive(PADAPTER padapter)
{
	s32 res;
	struct pwrctrl_priv *pwrctrl;
	u8 pslv;

_func_enter_;

	res = _SUCCESS;
	pwrctrl = adapter_to_pwrctl(padapter);
#ifdef CONFIG_BT_COEXIST
	if (_TRUE == pwrctrl->btcoex_rfon)
		pslv = PS_STATE_S3;
	else
#endif
	{
		pslv = PS_STATE_S2;
	}

	_enter_pwrlock(&pwrctrl->lps32klock);

	register_task_alive(pwrctrl, CMD_ALIVE);

	if (pwrctrl->bFwCurrentInPSMode == _TRUE)
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_info_,
				 ("rtw_register_cmd_alive: cpwm=0x%02x alives=0x%08x\n",
				  pwrctrl->cpwm, pwrctrl->alives));
		//DBG_871X("rtw_register_cmd_alive: cpwm=0x%02x alives=0x%08x pslv=%x \n",
		//		  pwrctrl->cpwm, pwrctrl->alives,pslv);

		if (pwrctrl->cpwm < pslv)
		{
			if (pwrctrl->rpwm < pslv)
				rtw_set_rpwm(padapter, pslv);
			if (pwrctrl->cpwm < PS_STATE_S2) {
				DBG_871X("%s fail\n", __func__);
				res = _FAIL;
			}
		}
	}

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;

	return res;
}

/*
 * Caller: rx_isr
 *
 * Calling Context: Dispatch/ISR
 *
 * Return Value:
 *	_SUCCESS
 *	_FAIL
 */
s32 rtw_register_rx_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	_enter_pwrlock(&pwrctrl->lps32klock);

	register_task_alive(pwrctrl, RECV_ALIVE);
	RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_notice_,
			 ("rtw_register_rx_alive: cpwm=0x%02x alives=0x%08x\n",
			  pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;

	return _SUCCESS;
}

/*
 * Caller: evt_isr or evt_thread
 *
 * Calling Context: Dispatch/ISR or Passive
 *
 * Return Value:
 *	_SUCCESS
 *	_FAIL
 */
s32 rtw_register_evt_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	_enter_pwrlock(&pwrctrl->lps32klock);

	register_task_alive(pwrctrl, EVT_ALIVE);
	RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_notice_,
			 ("rtw_register_evt_alive: cpwm=0x%02x alives=0x%08x\n",
			  pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;

	return _SUCCESS;
}

/*
 * Caller: FillH2CCmd
 *
 * If H2C done,
 * Then driver shall call this fun. to power down firmware again.
 */
void rtw_unregister_h2c_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

	_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	_enter_pwrlock(&pwrctrl->lps32klock);

	unregister_task_alive(pwrctrl, H2C_ALIVE);

	_exit_pwrlock(&pwrctrl->lps32klock);

	_func_exit_;
}

/*
 * Caller: ISR
 *
 * If ISR's txdone,
 * No more pkts for TX,
 * Then driver shall call this fun. to power down firmware again.
 */
void rtw_unregister_tx_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	_enter_pwrlock(&pwrctrl->lps32klock);

	unregister_task_alive(pwrctrl, XMIT_ALIVE);

	if ((pwrctrl->pwr_mode != PS_MODE_ACTIVE) &&
		(pwrctrl->bFwCurrentInPSMode == _TRUE))
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_info_,
				 ("%s: cpwm=0x%02x alives=0x%08x\n",
				  __FUNCTION__, pwrctrl->cpwm, pwrctrl->alives));

		if ((pwrctrl->alives == 0) &&
			(pwrctrl->cpwm > PS_STATE_S0))
		{
			rtw_set_rpwm(padapter, PS_STATE_S0);
		}
	}

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;
}

/*
 * Caller: ISR
 *
 * If all commands have been done,
 * and no more command to do,
 * then driver shall call this fun. to power down firmware again.
 */
void rtw_unregister_cmd_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	_enter_pwrlock(&pwrctrl->lps32klock);

	unregister_task_alive(pwrctrl, CMD_ALIVE);

	if ((pwrctrl->pwr_mode != PS_MODE_ACTIVE) &&
		(pwrctrl->bFwCurrentInPSMode == _TRUE))
	{
		RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_info_,
				 ("%s: cpwm=0x%02x alives=0x%08x\n",
				  __FUNCTION__, pwrctrl->cpwm, pwrctrl->alives));

		if ((pwrctrl->alives == 0) &&
			(pwrctrl->cpwm > PS_STATE_S0))
		{
			rtw_set_rpwm(padapter, PS_STATE_S0);
		}
	}

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;
}

/*
 * Caller: ISR
 */
void rtw_unregister_rx_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	_enter_pwrlock(&pwrctrl->lps32klock);

	unregister_task_alive(pwrctrl, RECV_ALIVE);

	RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_notice_,
			 ("rtw_unregister_rx_alive: cpwm=0x%02x alives=0x%08x\n",
			  pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lps32klock);

_func_exit_;
}

void rtw_unregister_evt_alive(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrl;

_func_enter_;

	pwrctrl = adapter_to_pwrctl(padapter);

	unregister_task_alive(pwrctrl, EVT_ALIVE);

	RT_TRACE(_module_rtl871x_pwrctrl_c_, _drv_notice_,
			 ("rtw_unregister_evt_alive: cpwm=0x%02x alives=0x%08x\n",
			  pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;
}
#endif	/* CONFIG_LPS_LCLK */

#ifdef CONFIG_RESUME_IN_WORKQUEUE
static void resume_workitem_callback(struct work_struct *work);
#endif //CONFIG_RESUME_IN_WORKQUEUE

void rtw_init_pwrctrl_priv(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(padapter);

#if defined(CONFIG_CONCURRENT_MODE)
	if (padapter->adapter_type != PRIMARY_ADAPTER)
		return;
#endif

_func_enter_;

#ifdef PLATFORM_WINDOWS
	pwrctrlpriv->pnp_current_pwr_state=NdisDeviceStateD0;
#endif

	_init_pwrlock(&pwrctrlpriv->lock);
	_init_pwrlock(&pwrctrlpriv->lps32klock);
	pwrctrlpriv->rf_pwrstate = rf_on;
	pwrctrlpriv->ips_enter_cnts=0;
	pwrctrlpriv->ips_leave_cnts=0;
	pwrctrlpriv->bips_processing = _FALSE;

	pwrctrlpriv->ips_mode = padapter->registrypriv.ips_mode;
	pwrctrlpriv->ips_mode_req = padapter->registrypriv.ips_mode;

	pwrctrlpriv->pwr_state_check_interval = RTW_PWR_STATE_CHK_INTERVAL;
	pwrctrlpriv->pwr_state_check_cnts = 0;
	pwrctrlpriv->bInternalAutoSuspend = _FALSE;
	pwrctrlpriv->bInSuspend = _FALSE;
	pwrctrlpriv->bkeepfwalive = _FALSE;

#ifdef CONFIG_AUTOSUSPEND
#ifdef SUPPORT_HW_RFOFF_DETECTED
	pwrctrlpriv->pwr_state_check_interval = (pwrctrlpriv->bHWPwrPindetect) ?1000:2000;
#endif
#endif

	pwrctrlpriv->LpsIdleCount = 0;
	//pwrctrlpriv->FWCtrlPSMode =padapter->registrypriv.power_mgnt;// PS_MODE_MIN;
	if (padapter->registrypriv.mp_mode == 1)
		pwrctrlpriv->power_mgnt =PS_MODE_ACTIVE ;
	else
		pwrctrlpriv->power_mgnt =padapter->registrypriv.power_mgnt;// PS_MODE_MIN;
	pwrctrlpriv->bLeisurePs = (PS_MODE_ACTIVE != pwrctrlpriv->power_mgnt)?_TRUE:_FALSE;

	pwrctrlpriv->bFwCurrentInPSMode = _FALSE;

	pwrctrlpriv->rpwm = 0;
	pwrctrlpriv->cpwm = PS_STATE_S4;

	pwrctrlpriv->pwr_mode = PS_MODE_ACTIVE;
	pwrctrlpriv->smart_ps = padapter->registrypriv.smart_ps;
	pwrctrlpriv->bcn_ant_mode = 0;

	pwrctrlpriv->tog = 0x80;

	pwrctrlpriv->btcoex_rfon = _FALSE;

#ifdef CONFIG_LPS_LCLK
	rtw_hal_set_hwreg(padapter, HW_VAR_SET_RPWM, (u8 *)(&pwrctrlpriv->rpwm));

	_init_workitem(&pwrctrlpriv->cpwm_event, cpwm_event_callback, NULL);

#ifdef CONFIG_LPS_RPWM_TIMER
	pwrctrlpriv->brpwmtimeout = _FALSE;
	_init_workitem(&pwrctrlpriv->rpwmtimeoutwi, rpwmtimeout_workitem_callback, NULL);
	_init_timer(&pwrctrlpriv->pwr_rpwm_timer, padapter->pnetdev, pwr_rpwm_timeout_handler, padapter);
#endif // CONFIG_LPS_RPWM_TIMER
#endif // CONFIG_LPS_LCLK

#ifdef PLATFORM_LINUX
	_init_timer(&(pwrctrlpriv->pwr_state_check_timer), padapter->pnetdev, pwr_state_check_handler, (u8 *)padapter);
#endif

	#ifdef CONFIG_RESUME_IN_WORKQUEUE
	_init_workitem(&pwrctrlpriv->resume_work, resume_workitem_callback, NULL);
	pwrctrlpriv->rtw_workqueue = create_freezable_workqueue("rtw_workqueue");
	#endif //CONFIG_RESUME_IN_WORKQUEUE

	#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_ANDROID_POWER)
	pwrctrlpriv->early_suspend.suspend = NULL;
	rtw_register_early_suspend(pwrctrlpriv);
	#endif //CONFIG_HAS_EARLYSUSPEND || CONFIG_ANDROID_POWER
	pwrctrlpriv->in_early_suspend = _FALSE;


_func_exit_;

}


void rtw_free_pwrctrl_priv(PADAPTER adapter)
{
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(adapter);

_func_enter_;

	//_rtw_memset((unsigned char *)pwrctrlpriv, 0, sizeof(struct pwrctrl_priv));

#if defined(CONFIG_CONCURRENT_MODE)
		if (adapter->adapter_type != PRIMARY_ADAPTER)
			return;
#endif

	#ifdef CONFIG_RESUME_IN_WORKQUEUE
	if (pwrctrlpriv->rtw_workqueue) {
		flush_workqueue(pwrctrlpriv->rtw_workqueue);
		destroy_workqueue(pwrctrlpriv->rtw_workqueue);
	}
	#endif


	#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_ANDROID_POWER)
	rtw_unregister_early_suspend(pwrctrlpriv);
	#endif //CONFIG_HAS_EARLYSUSPEND || CONFIG_ANDROID_POWER

	_free_pwrlock(&pwrctrlpriv->lock);
	_free_pwrlock(&pwrctrlpriv->lps32klock);

_func_exit_;
}

#ifdef CONFIG_RESUME_IN_WORKQUEUE
#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
extern int rtw_resume_process(_adapter *padapter);
#endif
static void resume_workitem_callback(struct work_struct *work)
{
	struct pwrctrl_priv *pwrpriv = container_of(work, struct pwrctrl_priv, resume_work);
	struct dvobj_priv *dvobj = pwrctl_to_dvobj(pwrpriv);
	_adapter *adapter = dvobj->if1;

	DBG_871X("%s\n",__FUNCTION__);

	#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
	rtw_resume_process(adapter);
	#endif

}

void rtw_resume_in_workqueue(struct pwrctrl_priv *pwrpriv)
{
	// accquire system's suspend lock preventing from falliing asleep while resume in workqueue
	rtw_lock_suspend();

	#if 1
	queue_work(pwrpriv->rtw_workqueue, &pwrpriv->resume_work);
	#else
	_set_workitem(&pwrpriv->resume_work);
	#endif
}
#endif //CONFIG_RESUME_IN_WORKQUEUE

#ifdef CONFIG_HAS_EARLYSUSPEND
#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
extern int rtw_resume_process(_adapter *padapter);
#endif
static void rtw_early_suspend(struct early_suspend *h)
{
	struct pwrctrl_priv *pwrpriv = container_of(h, struct pwrctrl_priv, early_suspend);

	struct dvobj_priv *dvobj = pwrctl_to_dvobj(pwrpriv);
	_adapter *adapter = dvobj->if1;
#ifdef SOFTAP_PS_DURATION
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct sta_priv *pstapriv = &adapter->stapriv;
	struct softap_ps_ioctl_param poidparam;

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) && pstapriv->asoc_sta_count == 2) {
		DBG_871X_LEVEL(_drv_always_, "%s SOFTAP_PS enable\n" , __func__);
		poidparam.subcode=SOFTAP_PS_ENABLE;
		poidparam.duration= SOFTAP_PS_DURATION; //(ms)
		rtw_hal_set_hwreg(adapter, HW_VAR_SOFTAP_PS,(u8 *)&poidparam);
	}
#endif

	DBG_871X("%s\n",__FUNCTION__);

	//jeff: do nothing but set do_late_resume to false
	pwrpriv->do_late_resume = _FALSE;
	pwrpriv->in_early_suspend = _TRUE;
#if  ((defined CONFIG_WOWLAN) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)))
	adapter->rtw_wdev->wiphy->registered=FALSE;
#endif


}

static void rtw_late_resume(struct early_suspend *h)
{
	struct pwrctrl_priv *pwrpriv = container_of(h, struct pwrctrl_priv, early_suspend);
	struct dvobj_priv *dvobj = pwrctl_to_dvobj(pwrpriv);
	_adapter *adapter = dvobj->if1;

#ifdef SOFTAP_PS_DURATION
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct softap_ps_ioctl_param poidparam;

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE)) {
		DBG_871X_LEVEL(_drv_always_, "%s SOFTAP_PS disable\n", __func__);
		poidparam.subcode=SOFTAP_PS_DISABLE;
		poidparam.duration= 0;
		rtw_hal_set_hwreg(adapter, HW_VAR_SOFTAP_PS,(u8 *)&poidparam);
	}
#endif
	DBG_871X("%s\n",__FUNCTION__);

	if(pwrpriv->do_late_resume) {
		#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
		rtw_resume_process(adapter);
		pwrpriv->do_late_resume = _FALSE;
		#endif
	}

	pwrpriv->in_early_suspend = _FALSE;
#if  ((defined CONFIG_WOWLAN) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)))
	adapter->rtw_wdev->wiphy->registered=TRUE;
#endif

}

void rtw_register_early_suspend(struct pwrctrl_priv *pwrpriv)
{
	DBG_871X("%s\n", __FUNCTION__);

	//jeff: set the early suspend level before blank screen, so we wll do late resume after scree is lit
	pwrpriv->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 20;
	pwrpriv->early_suspend.suspend = rtw_early_suspend;
	pwrpriv->early_suspend.resume = rtw_late_resume;
	register_early_suspend(&pwrpriv->early_suspend);


}

void rtw_unregister_early_suspend(struct pwrctrl_priv *pwrpriv)
{
	DBG_871X("%s\n", __FUNCTION__);

	pwrpriv->do_late_resume = _FALSE;

	if (pwrpriv->early_suspend.suspend)
		unregister_early_suspend(&pwrpriv->early_suspend);

	pwrpriv->early_suspend.suspend = NULL;
	pwrpriv->early_suspend.resume = NULL;
}
#endif //CONFIG_HAS_EARLYSUSPEND

#ifdef CONFIG_ANDROID_POWER
#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
extern int rtw_resume_process(PADAPTER padapter);
#endif
static void rtw_early_suspend(android_early_suspend_t *h)
{
	struct pwrctrl_priv *pwrpriv = container_of(h, struct pwrctrl_priv, early_suspend);
	DBG_871X("%s\n",__FUNCTION__);

	//jeff: do nothing but set do_late_resume to false
	pwrpriv->do_late_resume = _FALSE;
}

static void rtw_late_resume(android_early_suspend_t *h)
{
	struct pwrctrl_priv *pwrpriv = container_of(h, struct pwrctrl_priv, early_suspend);
	struct dvobj_priv *dvobj = pwrctl_to_dvobj(pwrpriv);
	_adapter *adapter = dvobj->if1;

	DBG_871X("%s\n",__FUNCTION__);
	if(pwrpriv->do_late_resume) {
		#if defined(CONFIG_USB_HCI) || defined(CONFIG_SDIO_HCI) || defined(CONFIG_GSPI_HCI)
		rtw_resume_process(adapter);
		pwrpriv->do_late_resume = _FALSE;
		#endif
	}
}

void rtw_register_early_suspend(struct pwrctrl_priv *pwrpriv)
{
	DBG_871X("%s\n", __FUNCTION__);

	//jeff: set the early suspend level before blank screen, so we wll do late resume after scree is lit
	pwrpriv->early_suspend.level = ANDROID_EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 20;
	pwrpriv->early_suspend.suspend = rtw_early_suspend;
	pwrpriv->early_suspend.resume = rtw_late_resume;
	android_register_early_suspend(&pwrpriv->early_suspend);
}

void rtw_unregister_early_suspend(struct pwrctrl_priv *pwrpriv)
{
	DBG_871X("%s\n", __FUNCTION__);

	pwrpriv->do_late_resume = _FALSE;

	if (pwrpriv->early_suspend.suspend)
		android_unregister_early_suspend(&pwrpriv->early_suspend);

	pwrpriv->early_suspend.suspend = NULL;
	pwrpriv->early_suspend.resume = NULL;
}
#endif //CONFIG_ANDROID_POWER

u8 rtw_interface_ps_func(_adapter *padapter,HAL_INTF_PS_FUNC efunc_id,u8* val)
{
	u8 bResult = _TRUE;
	rtw_hal_intf_ps_func(padapter,efunc_id,val);

	return bResult;
}

/*
* rtw_pwr_wakeup - Wake the NIC up from: 1)IPS. 2)USB autosuspend
* @adapter: pointer to _adapter structure
* @ips_deffer_ms: the ms wiil prevent from falling into IPS after wakeup
* Return _SUCCESS or _FAIL
*/
int _rtw_pwr_wakeup(_adapter *padapter, u32 ips_deffer_ms, const char *caller)
{
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	u32 start = rtw_get_current_time();
	int ret = _SUCCESS;

#ifdef CONFIG_CONCURRENT_MODE
	if (padapter->iface_type != IFACE_PORT0 && padapter->pbuddy_adapter)
		LeaveAllPowerSaveMode(padapter->pbuddy_adapter);
	else
#endif //CONFIG_CONCURRENT_MODE
		LeaveAllPowerSaveMode(padapter);

#ifdef CONFIG_CONCURRENT_MODE
	if ((padapter->isprimary == _FALSE) && padapter->pbuddy_adapter){
		padapter = padapter->pbuddy_adapter;
		pwrpriv = adapter_to_pwrctl(padapter);
		pmlmepriv = &padapter->mlmepriv;
	}
#endif

	pwrpriv->ips_deny_time = rtw_get_current_time() + rtw_ms_to_systime(ips_deffer_ms);
	if (pwrpriv->ps_processing) {
		DBG_871X("%s wait ps_processing...\n", __func__);
		while (pwrpriv->ps_processing && rtw_get_passing_time_ms(start) <= 3000) {
			DBG_871X("%s wait ps_processing...\n", __func__);
			rtw_msleep_os(10);
		}
		if (pwrpriv->ps_processing)
			DBG_871X("%s wait ps_processing timeout\n", __func__);
		else
			DBG_871X("%s wait ps_processing done\n", __func__);
	}


	//System suspend is not allowed to wakeup
	if((pwrpriv->bInternalAutoSuspend == _FALSE) && (_TRUE == pwrpriv->bInSuspend )){
		ret = _FAIL;
		goto exit;
	}

	//block???
	if((pwrpriv->bInternalAutoSuspend == _TRUE)  && (padapter->net_closed == _TRUE)) {
		ret = _FAIL;
		goto exit;
	}

	//I think this should be check in IPS, LPS, autosuspend functions...
	if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
	{
#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
		if(_TRUE==pwrpriv->bInternalAutoSuspend){
			if(0==pwrpriv->autopm_cnt){
			#if (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,33))
				if (usb_autopm_get_interface(adapter_to_dvobj(padapter)->pusbintf) < 0)
				{
					DBG_871X( "can't get autopm: \n");
				}
			#elif (LINUX_VERSION_CODE>=KERNEL_VERSION(2,6,20))
				usb_autopm_disable(adapter_to_dvobj(padapter)->pusbintf);
			#else
				usb_autoresume_device(adapter_to_dvobj(padapter)->pusbdev, 1);
			#endif
			pwrpriv->autopm_cnt++;
			}
#endif	//#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
		ret = _SUCCESS;
		goto exit;
#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
		}
#endif	//#if defined (CONFIG_BT_COEXIST)&& defined (CONFIG_AUTOSUSPEND)
	}
	if(rf_off == pwrpriv->rf_pwrstate )
	{
#ifdef CONFIG_USB_HCI
#ifdef CONFIG_AUTOSUSPEND
		 if(pwrpriv->brfoffbyhw==_TRUE)
		{
			DBG_8192C("hw still in rf_off state ...........\n");
			ret = _FAIL;
			goto exit;
		}
		else if(padapter->registrypriv.usbss_enable)
		{
			DBG_8192C("%s call autoresume_enter....\n",__FUNCTION__);
			if(_FAIL ==  autoresume_enter(padapter))
			{
				DBG_8192C("======> autoresume fail.............\n");
				ret = _FAIL;
				goto exit;
			}
		}
		else
#endif
#endif
		{
#ifdef CONFIG_IPS
			DBG_8192C("%s call ips_leave....\n",__FUNCTION__);
			if(_FAIL ==  ips_leave(padapter))
			{
				DBG_8192C("======> ips_leave fail.............\n");
				ret = _FAIL;
				goto exit;
			}
#endif
		}
	}

	//TODO: the following checking need to be merged...
	if(padapter->bDriverStopped
		|| !padapter->bup
		|| !padapter->hw_init_completed
	){
		DBG_8192C("%s: bDriverStopped=%d, bup=%d, hw_init_completed=%u\n"
			, caller
		   	, padapter->bDriverStopped
		   	, padapter->bup
		   	, padapter->hw_init_completed);
		ret= _FALSE;
		goto exit;
	}

exit:
	pwrpriv->ips_deny_time = rtw_get_current_time() + rtw_ms_to_systime(ips_deffer_ms);
	return ret;

}

int rtw_pm_set_lps(_adapter *padapter, u8 mode)
{
	int	ret = 0;
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(padapter);

	if ( mode < PS_MODE_NUM )
	{
		if(pwrctrlpriv->power_mgnt !=mode)
		{
			if(PS_MODE_ACTIVE == mode)
			{
				LeaveAllPowerSaveMode(padapter);
			}
			else
			{
				pwrctrlpriv->LpsIdleCount = 2;
			}
			pwrctrlpriv->power_mgnt = mode;
			pwrctrlpriv->bLeisurePs = (PS_MODE_ACTIVE != pwrctrlpriv->power_mgnt)?_TRUE:_FALSE;
		}
	}
	else
	{
		ret = -EINVAL;
	}

	return ret;
}

int rtw_pm_set_ips(_adapter *padapter, u8 mode)
{
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(padapter);

	if( mode == IPS_NORMAL || mode == IPS_LEVEL_2 ) {
		rtw_ips_mode_req(pwrctrlpriv, mode);
		DBG_871X("%s %s\n", __FUNCTION__, mode == IPS_NORMAL?"IPS_NORMAL":"IPS_LEVEL_2");
		return 0;
	}
	else if(mode ==IPS_NONE){
		rtw_ips_mode_req(pwrctrlpriv, mode);
		DBG_871X("%s %s\n", __FUNCTION__, "IPS_NONE");
		if(_FAIL == rtw_pwr_wakeup(padapter))
			return -EFAULT;
	}
	else {
		return -EINVAL;
	}
	return 0;
}


