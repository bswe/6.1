/**
 * \file
 *
 * \brief Data protocol handler implementation
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

#include "data_protocol.h"
#include <string.h>

/* First invalid bitmask. This and values higher are invalid */
#define FIRST_INVALID_BITMASK (1 << (NUM_CHANNELS))

/* Value used to indicate start of packet. */
#define START_SYMBOL         0xFF

/* Size of packet header */
#define HEADER_SIZE          3 /* Start symbol + node ID + bitmask */

#define MAX_TX_BUFFER_SIZE  ((TOTAL_DATA_SIZE*2)+HEADER_SIZE)

static uint8_t channel_size[NUM_CHANNELS] = CHANNEL_SIZE;
static uint8_t channel_offset[NUM_CHANNELS] = CHANNEL_OFFSET;

static uint8_t tx_channel_bitmask = 0;
static uint8_t tx_data[TOTAL_DATA_SIZE];
static bool    tx_busy = false;

static uint8_t rx_channel_bitmask = 0;
static uint8_t rx_buffer[TOTAL_DATA_SIZE];

static uint8_t rx_channel_bitmask_valid = 0;
static uint8_t rx_data_valid[NUM_NODES][TOTAL_DATA_SIZE];
static uint8_t rx_data_len = 0;
static uint8_t rx_data_size = 0;
static bool    rx_busy = false;

static uint8_t node_id;

tx_buf_func tx_func;

/** States in receive state machine */
enum rx_state_e {
	/** We are idle, waiting for a new packet */
	RX_STATE_IDLE,
	/** Start symbol received, wait for node ID */
	RX_STATE_WAIT_NODE_ID,
	/** Node ID received, wait for bitmask */
	RX_STATE_WAIT_BITMASK,
	/** Bitmask received; we are receiving packet data */
	RX_STATE_GET_DATA,
	/** Start symbol received */
	RX_STATE_GOT_SYMBOL,
};

/** Current state in our receive state machine */
static enum rx_state_e rx_state = RX_STATE_IDLE;

/**
 * \brief Initialize protocol transmission
 *
 * Set the ID of our node, and function to use for sending data.
 *
 * \param[in]  func  Transmit function to use when sending
 * \param[in]  id    The ID of our node
 */
void protocol_tx_init(tx_buf_func func, uint8_t id)
{
	tx_func = func;
	node_id = id;
}

/**
 * \brief Set data for given channel
 *
 * Set data to be sent next time for a channel
 *
 * \param[in]  channel_num  Channel number
 * \param[in]  data         Pointer to data to set
 */
enum status_code protocol_set_channel_data(uint8_t channel_num, void* data)
{
	/* Make sure we're not busy transmitting */
	if (tx_busy) {
		return STATUS_ERR_BUSY;
	}
	/* Update bitmask */
	tx_channel_bitmask |= (1 << channel_num);
	/* Copy data */
	memcpy(tx_data + channel_offset[channel_num], data,
			channel_size[channel_num]);
	return STATUS_OK;
}

/**
 * \brief Send data packet
 *
 * Send the data set by \ref protocol_set_channel_data
 *
 */
void protocol_send_packet(void)
{
	uint8_t channel_num;
	uint8_t byte_num;
	uint8_t data_byte;

	uint8_t send_buffer[MAX_TX_BUFFER_SIZE];
	uint8_t send_buffer_cnt = 0;

	/* Add header */
	send_buffer[send_buffer_cnt++] = START_SYMBOL;
	send_buffer[send_buffer_cnt++] = node_id;
	send_buffer[send_buffer_cnt++] = tx_channel_bitmask;

	/* Make sure we don't update our tx buffer while transmitting */
	tx_busy = true;
	/* Send channel data */
	for (channel_num = 0; channel_num < NUM_CHANNELS; channel_num++) {
		if (tx_channel_bitmask & (1 << channel_num)) {
			/* Send data on this channel */
			for (byte_num = 0; byte_num < channel_size[channel_num];
					byte_num++) {
				data_byte = tx_data[ channel_offset[channel_num] + byte_num ];
				send_buffer[send_buffer_cnt++] = data_byte;
				/* If sending START_SYMBOL, send it twice */
				if ( data_byte == START_SYMBOL) {
					send_buffer[send_buffer_cnt++] = START_SYMBOL;
				}
				/* Reset buffer */
				tx_data[ channel_offset[channel_num] + byte_num ] = 0;
			}
		}
	}

	/* Send the packet */
	tx_func(send_buffer, send_buffer_cnt);

	/** Reset channel bitmask */
	tx_channel_bitmask = 0;
	tx_busy = false;
}


/**
 * \brief Get data for a given channel / node.
 *
 * Get data received for a given channel from a given node
 *
 * \param[in]   channel_num  Channel number
 * \param[in]   nodeid       Node ID
 * \param[out]  data         Pointer to place data
 */
enum status_code protocol_get_channel_data(
		uint8_t channel_num,
		uint8_t nodeid,
		void* data)
{
	/* Make sure we're not busy */
	if (rx_busy) {
		return STATUS_ERR_BUSY;
	}
	/* Make sure node ID is sane */
	if (node_id >= NUM_NODES) {
		return ERR_BAD_DATA;
	}
	/* Do we have valid data on this channel? */
	if (!(rx_channel_bitmask_valid & (1 << channel_num))) {
		/* No valid data */
		return ERR_BAD_DATA;
	}

	/* Copy data from rx buffer */
	memcpy(data, rx_data_valid[nodeid] + channel_offset[channel_num],
			channel_size[channel_num]);
	return STATUS_OK;
}


/**
 * \brief Inform protocol handler of received data
 *
 * State machine handling incoming data. Write data to correct
 * buffers to make them available using \ref protocol_get_channel_data
 *
 * \param[in]  data         Received data
 */
void protocol_byte_received (uint8_t data)
{
	uint8_t channel_num;
	uint8_t valid_offset;
	uint8_t offset;
	static uint8_t current_node_id;

	if ((rx_state == RX_STATE_GOT_SYMBOL) && (data != START_SYMBOL)) {
		/* Abort packet reception, new packet incoming */
		rx_state = RX_STATE_WAIT_BITMASK;
	}

	switch (rx_state) {

	case RX_STATE_IDLE:
		/* We are waiting for a new packet. */
		if (data != START_SYMBOL) {
			return;
		}
		/* Got start symbol, wait for node ID */
		rx_state = RX_STATE_WAIT_NODE_ID;
		return;

	case RX_STATE_WAIT_NODE_ID:
		if (data == START_SYMBOL) {
			/* Restart. Don't change state. Wait for new node ID */
			return;
		}
		if (data >= NUM_NODES) {
			/* Invalid node number, restart */
			rx_state = RX_STATE_IDLE;
			return;
		}
		current_node_id = data;
		rx_state = RX_STATE_WAIT_BITMASK;
		return;

	case RX_STATE_WAIT_BITMASK:
		if (data == START_SYMBOL) {
			/* Restart packet reception. Wait for new node ID */
			rx_state = RX_STATE_WAIT_NODE_ID;
			return;
		}
		if	(data >= FIRST_INVALID_BITMASK) {
			/* Invalid channel bitmask.. start over */
			rx_state = RX_STATE_IDLE;
			return;
		}
		/* Got valid bitmask, wait for data */
		rx_channel_bitmask = data;
		if (rx_channel_bitmask == 0) {
			/* No data here, wait for next packet */
			rx_state = RX_STATE_IDLE;
			return;
		}

		/* Calculate how many bytes to expect */
		rx_data_size = 0;
		for (channel_num = 0; channel_num < NUM_CHANNELS; channel_num++) {
			if (rx_channel_bitmask & (1 << channel_num)) {
				rx_data_size += channel_size[channel_num];
			}
		}

		rx_state = RX_STATE_GET_DATA;
		rx_data_len = 0;
		return;

	case RX_STATE_GET_DATA:
	case RX_STATE_GOT_SYMBOL:
		if ((data == START_SYMBOL) && (rx_state == RX_STATE_GET_DATA)) {
			rx_state = RX_STATE_GOT_SYMBOL;
			return;
		}
		/* Add new data to rx buffer */
		rx_buffer[rx_data_len++] = data;
		/* Are we done yet? */
		if (rx_data_len == rx_data_size) {
			/* Yes we are! Update valid data buffer */
			rx_busy = true;
			rx_channel_bitmask_valid = rx_channel_bitmask;
			/* Put the data on the right offsets */
			valid_offset = 0;
			offset = 0;
			for (channel_num = 0; channel_num < NUM_CHANNELS; channel_num++) {
				if (rx_channel_bitmask & (1 << channel_num)) {
					/* We got this channel, copy */
					memcpy(rx_data_valid[current_node_id] + valid_offset,
							rx_buffer + offset, channel_size[channel_num]);
					valid_offset += channel_size[channel_num];
					offset += channel_size[channel_num];
				} else {
					/* Channel not here, increase valid_offset */
					valid_offset += channel_size[channel_num];
				}
			}

			rx_busy = false;
			rx_state = RX_STATE_IDLE;
			return;
		}
		/* Not done yet.. keep on receiving */
		rx_state = RX_STATE_GET_DATA;
		return;
	}

}
