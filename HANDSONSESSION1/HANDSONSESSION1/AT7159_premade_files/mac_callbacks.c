/**
 * \file
 *
 * \brief MAC stack callback implementation for smart sensor application
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 */
#include <asf.h>
#include "temp_sensor.h"

/** Flag to set when radio is ready to operate */
extern bool radio_ready;

/**
 * \brief Callback function indicating network search
 *
 * \param[out] parameter
 */

/**
 * Callback function usr_mcps_data_conf
 *
 * \param[in] msduHandle  Handle of MSDU handed over to MAC earlier
 * \param[in] status      Result for requested data transmission request
 * \param[in] Timestamp   The time, in symbols, at which the data were transmitted
 *                    (only if timestamping is enabled).
 *
 */
#ifdef ENABLE_TSTAMP
void usr_mcps_data_conf(
		uint8_t msduHandle,
		uint8_t status,
		uint32_t Timestamp)
#else
void usr_mcps_data_conf(
		uint8_t msduHandle,
		uint8_t status)
#endif  /* ENABLE_TSTAMP */
{
}

/**
 * \brief Callback function usr_mlme_scan_conf
 *
 * \param[in] status            Result of requested scan operation
 * \param[in] ScanType          Type of scan performed
 * \param[in] ChannelPage       Channel page on which the scan was performed
 * \param[in] UnscannedChannels Bitmap of unscanned channels
 * \param[in] ResultListSize    Number of elements in ResultList
 * \param[in] ResultList        Pointer to array of scan results
 */
void usr_mlme_scan_conf(
		uint8_t status,
		uint8_t ScanType,
		uint8_t ChannelPage,
		uint32_t UnscannedChannels,
		uint8_t ResultListSize,
		void *ResultList)
{
}

/**
 * \brief Callback function usr_mlme_associate_conf
 *
 * \param[in] AssocShortAddress    Short address allocated by the coordinator
 * \param[in] status               Result of requested association operation
 */
void usr_mlme_associate_conf(
		uint16_t AssocShortAddress,
		uint8_t status)
{
}

/**
 * \brief Callback function usr_mlme_associate_ind
 *
 * \param[in] DeviceAddress         Extended address of device requesting association
 * \param[in] CapabilityInformation Capabilities of device requesting association
 */
void usr_mlme_associate_ind(
		uint64_t DeviceAddress,
		uint8_t CapabilityInformation)
{
}

/**
 * \brief Callback function usr_mlme_comm_status_ind
 *
 * \param[in] SrcAddrSpec      Pointer to source address specification
 * \param[in] DstAddrSpec      Pointer to destination address specification
 * \param[in] status           Result for related response operation
 */
void usr_mlme_comm_status_ind(
		wpan_addr_spec_t *SrcAddrSpec,
		wpan_addr_spec_t *DstAddrSpec,
		uint8_t status)
{
}

/**
 * \brief Callback function usr_mlme_start_conf
 *
 * \param[in] status        Result of requested start operation
 */
void usr_mlme_start_conf(
		uint8_t status)
{
}

#if ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1))
void usr_mcps_purge_conf(
		uint8_t msduHandle,
		uint8_t status)
{
}
#endif  /* ((MAC_PURGE_REQUEST_CONFIRM == 1) && (MAC_INDIRECT_DATA_BASIC == 1)) */

#if (MAC_BEACON_NOTIFY_INDICATION == 1)
void usr_mlme_beacon_notify_ind(
		uint8_t BSN,
		wpan_pandescriptor_t *PANDescriptor,
		uint8_t PendAddrSpec,
		uint8_t *AddrList,
		uint8_t sduLength,
		uint8_t *sdu)
{
}
#endif  /* (MAC_BEACON_NOTIFY_INDICATION == 1) */

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
void usr_mlme_disassociate_conf(
		uint8_t status,
		wpan_addr_spec_t *DeviceAddrSpec)
{
}
#endif /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)*/

#if (MAC_DISASSOCIATION_BASIC_SUPPORT == 1)
void usr_mlme_disassociate_ind(
		uint64_t DeviceAddress,
		uint8_t DisassociateReason)
{
}
#endif  /* (MAC_DISASSOCIATION_BASIC_SUPPORT == 1) */

/**
 * \brief Callback function usr_mlme_get_conf
 *
 * \param[in] status            Result of requested PIB attribute set operation
 * \param[in] PIBAttribute      Updated PIB attribute
 * \param[in] PIBAttributeValue Pointer to data containing retrieved PIB attribute
 */
void usr_mlme_get_conf(
		uint8_t status,
		uint8_t PIBAttribute,
		void *PIBAttributeValue)
{
	uint8_t current_page;
	uint8_t current_channel;

	if ((status == MAC_SUCCESS) && (PIBAttribute == phyCurrentPage)) {
		current_page = DEFAULT_CHANNEL_PAGE;
		wpan_mlme_set_req(phyCurrentPage, &current_page);
	} else if ((status == MAC_SUCCESS) && (PIBAttribute == phyCurrentChannel)) {
		/* Ask for new channel. */
		current_channel = DEFAULT_CHANNEL;
		wpan_mlme_set_req(phyCurrentChannel, &current_channel);
	}
}

#if (MAC_ORPHAN_INDICATION_RESPONSE == 1)
void usr_mlme_orphan_ind(
		uint64_t OrphanAddress)
{
}
#endif  /* (MAC_ORPHAN_INDICATION_RESPONSE == 1) */


#if (MAC_INDIRECT_DATA_BASIC == 1)
void usr_mlme_poll_conf(
		uint8_t status)
{
}
#endif  /* (MAC_INDIRECT_DATA_BASIC == 1) */


void usr_mlme_sync_loss_ind(
		uint8_t LossReason,
		uint16_t PANId,
		uint8_t LogicalChannel,
		uint8_t ChannelPage)
{
}

#if (MAC_RX_ENABLE_SUPPORT == 1)
void usr_mlme_rx_enable_conf(
		uint8_t status)
{
}
#endif  /* (MAC_RX_ENABLE_SUPPORT == 1) */


/**
 * @brief Callback function usr_mcps_data_ind
 *
 * @param SrcAddrSpec      Pointer to source address specification
 * @param DstAddrSpec      Pointer to destination address specification
 * @param msduLength       Number of octets contained in MSDU
 * @param msdu             Pointer to MSDU
 * @param mpduLinkQuality  LQI measured during reception of the MPDU
 * @param DSN              DSN of the received data frame.
 * @param Timestamp        The time, in symbols, at which the data were received
 *                         (only if timestamping is enabled).
 */
void usr_mcps_data_ind(
		wpan_addr_spec_t *SrcAddrSpec,
		wpan_addr_spec_t *DstAddrSpec,
		uint8_t msduLength,
		uint8_t *msdu,
		uint8_t mpduLinkQuality,
#ifdef ENABLE_TSTAMP
		uint8_t DSN,
		uint32_t Timestamp)
#else
		uint8_t DSN)
#endif /* ENABLE_TSTAMP */
{
}

void usr_mlme_set_conf(
		uint8_t status,
		uint8_t PIBAttribute)
{
	if ((status == MAC_SUCCESS) && (PIBAttribute == macShortAddress)) {
		/* Set the short address of this node.
		* Use: bool wpan_mlme_set_req(uint8_t PIBAttribute,
		*                             void *PIBAttributeValue);
		*
		* This request leads to a set confirm message -> usr_mlme_set_conf
		*/
		uint8_t panid[2];

		panid[0] = (uint8_t)SOURCE_PAN_ID;          // low byte
		panid[1] = (uint8_t)(SOURCE_PAN_ID >> 8);   // high byte

		wpan_mlme_set_req(macPANId, panid); // Task2.2
	} else if ((status == MAC_SUCCESS) && (PIBAttribute == macPANId)) {
		/* Set the short address of this node.
		* Use: bool wpan_mlme_set_req(uint8_t PIBAttribute,
		*                             void *PIBAttributeValue);
		*
		* This request leads to a set confirm message -> usr_mlme_set_conf
		*/
		uint8_t config_channel = DEFAULT_CHANNEL;
		wpan_mlme_set_req(phyCurrentChannel, &config_channel); // Task2.2

	} else if ((status == MAC_SUCCESS) && (PIBAttribute == phyCurrentChannel)) {
		/*
		* Set RX on when idle to enable the receiver as default.
		* Use: bool wpan_mlme_set_req(uint8_t PIBAttribute,
		*                             void *PIBAttributeValue);
		*
		* This request leads to a set confirm message -> usr_mlme_set_conf
		*/
		bool rx_on_when_idle = true;

		wpan_mlme_set_req(macRxOnWhenIdle, &rx_on_when_idle);
	} else if ((status == MAC_SUCCESS) && (PIBAttribute == macRxOnWhenIdle)) {
		radio_ready = true;
	} else {
		// something went wrong; restart
		wpan_mlme_reset_req(true);
	}
}


/**
 * @brief Callback function usr_mlme_reset_conf
 *
 * @param status Result of the reset procedure
 */
void usr_mlme_reset_conf(
		uint8_t status)
{
	if (status == MAC_SUCCESS) {
		/* Set the short address of this node.
		* Use: bool wpan_mlme_set_req(uint8_t PIBAttribute,
		*                             void *PIBAttributeValue);
		*
		* This request leads to a set confirm message -> usr_mlme_set_conf
		*/
		uint8_t short_addr[2];

		short_addr[0] = (uint8_t)SOURCE_SHORT_ADDR;          // low byte
		short_addr[1] = (uint8_t)(SOURCE_SHORT_ADDR >> 8);   // high byte
		wpan_mlme_set_req(macShortAddress, short_addr);
	} else {
		// something went wrong; restart
		wpan_mlme_reset_req(true);
	}
}
