/**
 * This code is based on a sample from Nordic Semiconductor ASA (see license below),
 * with modifications made by Microsoft (see the README.md in this directory).
 *
 * Modified version of ble_peripheral\ble_app_uart example from Nordic nRF5 SDK version 15.2.0
 * (https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.2.0_9412b96.zip)
 *
 * Original file: {SDK_ROOT}\examples\ble_peripheral\ble_app_uart\main.c
 **/

/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_assert.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"

#include "ble_db_discovery.h"
#include "nrf_ble_scan.h"
//#include "ble_lbs_c.h"
#include "ble_thingy_uis_c.h"

#include "peer_manager.h"
#include "fds.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_lesc.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"
#include "peer_manager_handler.h"

#include "ble_agg_config_service.h"
#include "app_aggregator.h"
#include "ble_lbs_c_extended.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "message_protocol.h"
#include "blecontrol_message_protocol.h"

#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */

#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED       /**< The advertising duration in units of 10 milliseconds (0 means forever). */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(20, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(75, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define CENTRAL_SCANNING_LED            BSP_BOARD_LED_0                     /**< Scanning LED will be on when the device is scanning. */
#define CENTRAL_CONNECTED_LED           BSP_BOARD_LED_1                     /**< Connected LED will be on when the device is connected. */
#define LEDBUTTON_LED                   BSP_BOARD_LED_2                     /**< LED to indicate a change of state of the the Button characteristic on the peer. */
#define CODED_PHY_LED                   BSP_BOARD_LED_3                     /**< connected to atleast one CODED phy */

#define SCAN_INTERVAL                   0x00A0                              /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                     0x0050                              /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_DURATION                   0x0000                              /**< Timout when scanning. 0x0000 disables timeout. */
#define SCAN_TIMEOUT                    0x0200                              /**< Timout when scanning. 0x0000 disables timeout. */


#define MIN_CONNECTION_INTERVAL         MSEC_TO_UNITS(7.5, UNIT_1_25_MS)    /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL         MSEC_TO_UNITS(30, UNIT_1_25_MS)     /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY                   0                                   /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT             MSEC_TO_UNITS(4000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 milliseconds. */

#define LEDBUTTON_BUTTON_PIN            BSP_BUTTON_0                        /**< Button that will write to the LED characteristic of the peer */

//#define CENTRAL_DISCONNECT_BUTTON   BSP_BUTTON_0                        /**< Button that will write to the LED characteristic of the peer. */
//#define SCAN_START_STOP_BUTTON      BSP_BUTTON_1
//#define LEDBUTTON_BUTTON            BSP_BUTTON_2

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                 /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */


#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  1                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  1                                           /**< LE Secure Connections enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_DISPLAY_ONLY                /**< Display I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define PASSKEY_LENGTH                  6                                           /**< Length of pass-key received by the stack for display. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_SCAN_DEF(m_scan);                                                 /**< Scanning module instance. */ 
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

BLE_AGG_CFG_SERVICE_DEF(m_agg_cfg_service);                             /**< BLE NUS service instance. */

//BLE_LBS_C_DEF(m_ble_lbs_c);                                     /**< Main structure used by the LBS client module. */
BLE_LBS_C_ARRAY_DEF(m_lbs_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);           /**< LED Button client instances. */
BLE_THINGY_UIS_C_ARRAY_DEF(m_thingy_uis_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);
BLE_DB_DISCOVERY_ARRAY_DEF(m_db_disc, NRF_SDH_BLE_CENTRAL_LINK_COUNT);  /**< Database discovery module instances. */


static char const m_target_periph_name[] = "Nordic_Blinky";     /**< Name of the device we try to connect to. This name is searched in the scan report data*/

static uint16_t     m_conn_handle          = BLE_CONN_HANDLE_INVALID;               /**< Handle of the current connection. */
static pm_peer_id_t m_peer_id;                                                      /**< Device reference handle to the current bonded central. */
static uint16_t     m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;          /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t   m_adv_uuids[]          =                                        /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

static uint16_t   m_coded_phy_conn_handle[NRF_SDH_BLE_TOTAL_LINK_COUNT];


//static char const m_target_periph_name[] = "NT:";                       /**< Name of the device we try to connect to. This name is searched for in the scan report data*/
static char const m_target_blinky_name[] = "MLThingy";
//static volatile bool m_service_discovery_in_process = false;

static uint16_t   m_service_discovery_conn_handle = BLE_CONN_HANDLE_INVALID;
typedef enum {DEVTYPE_NONE, DEVTYPE_BLINKY, DEVTYPE_THINGY} device_type_t;

//device_type_t    m_device_type_being_connected_to = DEVTYPE_NONE;
char             m_device_name_being_connected_to[30];
connected_device_info_t m_device_being_connected_info = {DEVTYPE_NONE, m_device_name_being_connected_to, 0};


static ret_code_t led_status_send_by_mask(uint8_t button_state, uint8_t r, uint8_t g, uint8_t b, uint32_t mask);
static ret_code_t led_status_on_off_send_by_mask(bool on, uint32_t mask);
//static ret_code_t post_connect_message(uint8_t conn_handle);

static bool m_initialization_completed = false;
static bool m_advertising_with_whitelist = true;

#define BLE_DEVICE_ALREADY_INITIALIZED 1


static uint8_t m_scan_buffer_data[BLE_GAP_SCAN_BUFFER_EXTENDED_MIN]; /**< buffer where advertising reports will be stored by the SoftDevice. */

/**@brief Pointer to the buffer where advertising reports will be stored by the SoftDevice. */
static ble_data_t m_scan_buffer =
{
    m_scan_buffer_data,
    BLE_GAP_SCAN_BUFFER_EXTENDED_MIN
};
static bool m_scanning_enabled = true;

/**@brief Scan parameters requested for scanning and connection. */
static ble_gap_scan_params_t m_scan_params =
{
    .active   = 1,
    .interval = SCAN_INTERVAL,
    .window   = SCAN_WINDOW,
    .report_incomplete_evts = 0, 
    .extended = 0,
    .timeout           = SCAN_TIMEOUT,
    .scan_phys         = BLE_GAP_PHY_1MBPS,
    .filter_policy     = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .channel_mask      = {0,0,0,0,0},
};

/**@brief Connection parameters requested for connection. */
static ble_gap_conn_params_t const m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,
    (uint16_t)MAX_CONNECTION_INTERVAL,
    (uint16_t)SLAVE_LATENCY,
    (uint16_t)SUPERVISION_TIMEOUT
};

static bool m_scan_mode_coded_phy = false;

enum {APPCMD_ERROR, APPCMD_SET_LED_ALL, APPCMD_SET_LED_ON_OFF_ALL, 
      APPCMD_POST_CONNECT_MESSAGE, APPCMD_DISCONNECT_PERIPHERALS,
      APPCMD_DISCONNECT_CENTRAL, APPCMD_TEST_CMD};

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for setting filtered whitelist.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void whitelist_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    uint32_t     peer_id_count = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
                   peer_id_count,
                   BLE_GAP_WHITELIST_ADDR_MAX_COUNT);

    err_code = pm_whitelist_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for setting filtered device identities.
 *
 * @param[in] skip  Filter passed to @ref pm_peer_id_list.
 */
static void identities_set(pm_peer_id_list_skip_t skip)
{
    pm_peer_id_t peer_ids[BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT];
    uint32_t     peer_id_count = BLE_GAP_DEVICE_IDENTITIES_MAX_COUNT;

    ret_code_t err_code = pm_peer_id_list(peer_ids, &peer_id_count, PM_PEER_ID_INVALID, skip);
    APP_ERROR_CHECK(err_code);

    err_code = pm_device_identities_list_set(peer_ids, peer_id_count);
    APP_ERROR_CHECK(err_code);
}

static int ble_start_advertising_handler(bool use_whitelist)
{
    ret_code_t ret = 0;
    m_advertising_with_whitelist = use_whitelist;

    if(!use_whitelist)
    {
        // Disconnect currently connected device before starting advertising to all.
        if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            ret = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if(ret != NRF_SUCCESS)
            {
                NRF_LOG_INFO("ERROR: Fail to disconnect central device with error: %d.", ret);
                return ret;
            }
        }
        m_advertising.adv_mode_current = BLE_ADV_MODE_FAST;
        ret = ble_advertising_restart_without_whitelist(&m_advertising);
    }
    else
    {
        // Start advertising to bonded devices only, if there is at least one bonded device and no device is currently connected.
        if(m_advertising.adv_mode_current != BLE_ADV_MODE_IDLE)
        {
            (void) sd_ble_gap_adv_stop(m_advertising.adv_handle);
        }
        if(pm_peer_count() > 0 && m_conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);
            m_advertising.whitelist_temporarily_disabled = false;
            m_advertising.whitelist_in_use               = true;
            ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        }
    }
    return ret;
}

/**@brief Clear bond information from persistent storage.
 */
static int delete_bonds(void)
{
    // Disconnect currently connected device before deleting all bonds.
    if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ret_code_t ret = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if(ret != NRF_SUCCESS)
        {
            NRF_LOG_INFO("ERROR: Fail to disconnect central device with error: %d.\n", ret);
            return ret;
        }
    }

    if(pm_peer_count() > 0)
    {
        NRF_LOG_INFO("Erase bonds!");
        ret_code_t ret = pm_peers_delete();
        if(ret != NRF_SUCCESS)
        {
            NRF_LOG_INFO("ERROR: Fail to erase bonds with error: %d.\n", ret);
            return ret;
        }
        return ble_start_advertising_handler(m_advertising_with_whitelist);
    }
    NRF_LOG_INFO("No bonds to erase!");
    return 0;
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            ble_start_advertising_handler(m_advertising_with_whitelist);
            break;

        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
            if (     p_evt->params.peer_data_update_succeeded.flash_changed
                 && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
            {
                NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
                // Note: You should check on what kind of white list policy your application should use.

                whitelist_set(PM_PEER_ID_LIST_SKIP_NO_ID_ADDR);
            }
            break;

        default:
            break;
    }
}

/**@brief Function for initializing the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
/**@snippet [Handling the data received over BLE] */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        NRF_LOG_DEBUG("Received data from BLE NUS. Writing data on UART.");
        NRF_LOG_HEXDUMP_DEBUG(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
        int result = message_protocol_send_data_via_uart(p_evt->params.rx_data.p_data, 
                                                         p_evt->params.rx_data.length);

        if(result != 0)
        {
            NRF_LOG_ERROR("Failed to send UART data.");
        }
    }

}

/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Clean up message protocols
    ble_control_message_protocol_clean_up();
    message_protocol_clean_up();

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();

    // NOTE: If the chip is in debug interface mode when sd_power_system_off() is called,
    // the chip will enter emulated system off mode, and this will lead to the CPU executing
    // code after the call. It is recommended, therefore, to add an infinite loop after calling
    // sd_power_system_off().
    #ifdef DEBUG_NRF
    while(1);
    #else
    APP_ERROR_CHECK(err_code);
    #endif
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_DIRECTED_HIGH_DUTY:
            NRF_LOG_INFO("High Duty Directed advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_DIRECTED);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_DIRECTED:
            NRF_LOG_INFO("Directed advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_DIRECTED);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            if(m_advertising_with_whitelist)
            {
                NRF_LOG_INFO("Stop advertising, start fast advertising with whitelist");
                ble_start_advertising_handler(true);
            }
            break;

        case BLE_ADV_EVT_SLOW:
            NRF_LOG_INFO("Slow advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_SLOW);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_FAST_WHITELIST:
            NRF_LOG_INFO("Fast advertising with whitelist.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
            APP_ERROR_CHECK(err_code);
            if(!m_advertising_with_whitelist)
            {
                NRF_LOG_INFO("Stop advertising, start fast advertising without whitelist");
                ble_advertising_restart_without_whitelist(&m_advertising);
            }
            else if(pm_peer_count() == 0 && m_advertising.adv_mode_current != BLE_ADV_MODE_IDLE)
            {
                (void) sd_ble_gap_adv_stop(m_advertising.adv_handle);
            }
            break;

        case BLE_ADV_EVT_SLOW_WHITELIST:
            NRF_LOG_INFO("Slow advertising with whitelist.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        case BLE_ADV_EVT_WHITELIST_REQUEST:
        {
            ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            ble_gap_irk_t  whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            uint32_t       addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
            uint32_t       irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

            err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                        whitelist_irks,  &irk_cnt);
            if(err_code != NRF_ERROR_NOT_FOUND && err_code != NRF_SUCCESS)
            {
                APP_ERROR_CHECK(err_code);
            }
            NRF_LOG_DEBUG("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist",
                          addr_cnt, irk_cnt);

            // Set the correct identities list (no excluding peers with no Central Address Resolution).
            identities_set(PM_PEER_ID_LIST_SKIP_NO_IRK);

            // Apply the whitelist.
            err_code = ble_advertising_whitelist_reply(&m_advertising,
                                                       whitelist_addrs,
                                                       addr_cnt,
                                                       whitelist_irks,
                                                       irk_cnt);
            APP_ERROR_CHECK(err_code);
        } break; //BLE_ADV_EVT_WHITELIST_REQUEST

        case BLE_ADV_EVT_PEER_ADDR_REQUEST:
        {
            pm_peer_data_bonding_t peer_bonding_data;

            // Only Give peer address if we have a handle to the bonded peer.
            if (m_peer_id != PM_PEER_ID_INVALID)
            {
                err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
                if (err_code != NRF_ERROR_NOT_FOUND)
                {
                    APP_ERROR_CHECK(err_code);

                    // Manipulate identities to exclude peers with no Central Address Resolution.
                    identities_set(PM_PEER_ID_LIST_SKIP_ALL);

                    ble_gap_addr_t * p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
                    err_code = ble_advertising_peer_addr_reply(&m_advertising, p_peer_addr);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; //BLE_ADV_EVT_PEER_ADDR_REQUEST

        default:
            break;
    }
}

/**@brief Function to start scanning. */
static void scan_start(bool coded_phy)
{
    ret_code_t ret;
#ifndef NRF52840_XXAA
    coded_phy = false;
#endif
    //(void) sd_ble_gap_scan_stop();
    if(m_scanning_enabled)
    {
        NRF_LOG_DEBUG("Scan start: Name - %s, phy - %s", (uint32_t)m_target_periph_name, coded_phy ? "Coded" : "1Mbps");
        m_scan_buffer.len = BLE_GAP_SCAN_BUFFER_EXTENDED_MIN;
        m_scan_params.scan_phys = coded_phy ? BLE_GAP_PHY_CODED : BLE_GAP_PHY_1MBPS;
        m_scan_params.extended = coded_phy ? 1 : 0;
        ret = sd_ble_gap_scan_start(&m_scan_params, &m_scan_buffer);
        if(ret == NRF_ERROR_INVALID_STATE)
        {
            NRF_LOG_INFO("scan start invalid state");
        }
        else APP_ERROR_CHECK(ret);

//        scan_led_state_set(true, coded_phy);
        m_scan_mode_coded_phy = coded_phy;
    }
}




static uint8_t peer_addr_LR[NRF_SDH_BLE_CENTRAL_LINK_COUNT][6];

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;
    static uint8_t coded_phy_conn_count = 0;

    // For readability.
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    //NRF_LOG_INFO("Evt: %i", p_ble_evt->header.evt_id);
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        // Handle central connections
            if(p_gap_evt->params.connected.role == BLE_GAP_ROLE_CENTRAL)
            {
                NRF_LOG_INFO("Peer addr  %02x%02x%02x%02x%02x%02x",
                    peer_addr_LR[p_gap_evt->conn_handle][0],
                    peer_addr_LR[p_gap_evt->conn_handle][1],
                    peer_addr_LR[p_gap_evt->conn_handle][2],
                    peer_addr_LR[p_gap_evt->conn_handle][3],
                    peer_addr_LR[p_gap_evt->conn_handle][4],
                    peer_addr_LR[p_gap_evt->conn_handle][5]);

                NRF_LOG_INFO("Connection 0x%x established , starting DB discovery.",
                             p_gap_evt->conn_handle);

                memcpy(&peer_addr_LR[p_gap_evt->conn_handle][0], &p_gap_evt->params.connected.peer_addr.addr[0], 6);

                //APP_ERROR_CHECK_BOOL(p_gap_evt->conn_handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT);

                switch(m_device_being_connected_info.dev_type)
                {
                    case DEVTYPE_BLINKY:
                        err_code = ble_lbs_c_handles_assign(&m_lbs_c[p_gap_evt->conn_handle],
                                                            p_gap_evt->conn_handle, NULL);
                        APP_ERROR_CHECK(err_code);
                        break;
                    
                    case DEVTYPE_THINGY:
                        err_code = ble_thingy_uis_c_handles_assign(&m_thingy_uis_c[p_gap_evt->conn_handle],
                                                                   p_gap_evt->conn_handle, NULL);
                        APP_ERROR_CHECK(err_code);
                        break;
                    
                    default:
                        break;
                }

                m_service_discovery_conn_handle = p_gap_evt->conn_handle;
                memset(&m_db_disc[p_gap_evt->conn_handle], 0x00, sizeof(ble_db_discovery_t));
                err_code = ble_db_discovery_start(&m_db_disc[p_gap_evt->conn_handle],
                                                  p_gap_evt->conn_handle);
                if (err_code != NRF_ERROR_BUSY)
                {
                    APP_ERROR_CHECK(err_code);
                }

                // Notify the aggregator service
                app_aggregator_on_central_connect(p_gap_evt, &m_device_being_connected_info);
  
                // Update LEDs status, and check if we should be looking for more
                if (ble_conn_state_central_conn_count() == NRF_SDH_BLE_CENTRAL_LINK_COUNT)
                {
                    bsp_board_led_off(CENTRAL_SCANNING_LED);
                }
                else
                {
                    // Resume scanning.
                    //bsp_board_led_on(CENTRAL_SCANNING_LED);
                    //scan_start();
                }
                
                m_device_being_connected_info.dev_type = DEVTYPE_NONE;

                // check if it was a coded phy connection
                if(BLE_GAP_PHY_CODED == m_scan_params.scan_phys)
                {
                    coded_phy_conn_count++;
                    m_coded_phy_conn_handle[p_gap_evt->conn_handle] = p_gap_evt->conn_handle;
                    bsp_board_led_on(CODED_PHY_LED);
                }
                // Notify the Azure Sphere of an Aggregator Event
                ble_control_message_protocol_send_ble_client_event();
            }
            // Handle links as a peripheral here
            else
            {
                m_conn_handle = p_gap_evt->conn_handle;
                NRF_LOG_INFO("Peripheral connection 0x%x established.", m_conn_handle);
//                app_timer_start(m_post_message_delay_timer_id, APP_TIMER_TICKS(2000), 0);
//                app_timer_stop(m_adv_led_blink_timer_id);
//                bsp_board_led_on(PERIPHERAL_ADV_CON_LED);

                err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
                APP_ERROR_CHECK(err_code);
                
                err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
                APP_ERROR_CHECK(err_code);
                m_advertising_with_whitelist = true;
                ble_control_message_protocol_send_connected_event();
            }
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Peer addr  %02x%02x%02x%02x%02x%02x",
                    peer_addr_LR[p_gap_evt->conn_handle][0],
                    peer_addr_LR[p_gap_evt->conn_handle][1],
                    peer_addr_LR[p_gap_evt->conn_handle][2],
                    peer_addr_LR[p_gap_evt->conn_handle][3],
                    peer_addr_LR[p_gap_evt->conn_handle][4],
                    peer_addr_LR[p_gap_evt->conn_handle][5]);

            NRF_LOG_INFO("GAP_EVT_DISCONNECT: %i", p_gap_evt->conn_handle);

            // Handle central disconnections
            if(p_gap_evt->conn_handle != m_conn_handle)
            {
                NRF_LOG_INFO("LBS central link 0x%x disconnected (reason: 0x%x)",
                       p_gap_evt->conn_handle,
                       p_gap_evt->params.disconnected.reason);

                if(p_gap_evt->conn_handle == m_service_discovery_conn_handle)
                {
                    m_service_discovery_conn_handle = BLE_CONN_HANDLE_INVALID;
                }
                // Notify aggregator service
                app_aggregator_on_central_disconnect(p_gap_evt);
                // Notify the Azure Sphere of an Aggregator Event
                ble_control_message_protocol_send_ble_client_event();

                if(m_coded_phy_conn_handle[p_gap_evt->conn_handle] != BLE_CONN_HANDLE_INVALID)
                {
                    // This is a coded phy link that got disconnected
                    m_coded_phy_conn_handle[p_gap_evt->conn_handle] = BLE_CONN_HANDLE_INVALID;
                    if(--coded_phy_conn_count == 0)
                    {
                        bsp_board_led_off(CODED_PHY_LED);
                    }
                }
                
                // Start scanning, in case the disconnect happened during service discovery
                scan_start(m_scan_mode_coded_phy);
            }
            // Handle peripheral disconnect
            else
            {
                NRF_LOG_INFO("Peripheral connection disconnected (reason: 0x%x)", p_gap_evt->params.disconnected.reason);
                // LED indication will be changed when advertising starts.
                m_conn_handle = BLE_CONN_HANDLE_INVALID;

//                app_aggregator_clear_buffer();
//                app_timer_stop(m_post_message_delay_timer_id);
//                bsp_board_led_off(PERIPHERAL_ADV_CON_LED);
                
                // Start advertising
//                advertising_start();
                ble_control_message_protocol_send_disconnected_event();
            }

            break;

        case BLE_GAP_EVT_TIMEOUT:
        {
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                // This can only happen with central (initiator request timeout)
                NRF_LOG_INFO("Connection request timed out.");

                scan_start(m_scan_mode_coded_phy);
            }
            else if(p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                // On scan timeout, restart scanning in the opposite mode (1M vs coded)
                scan_start(!m_scan_mode_coded_phy);
            }
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE:
        {
            ble_gap_evt_phy_update_t phy_update = p_ble_evt->evt.gap_evt.params.phy_update;
            if(phy_update.status == BLE_HCI_STATUS_CODE_SUCCESS)
            {
                NRF_LOG_INFO("PHY updated: %i, %i", phy_update.tx_phy, phy_update.rx_phy);
//                app_aggregator_phy_update(p_ble_evt->evt.gap_evt.conn_handle, phy_update.tx_phy, phy_update.rx_phy);
            }
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;

        case BLE_GAP_EVT_PASSKEY_DISPLAY:
        {
            char passkey[PASSKEY_LENGTH + 1];
            memcpy(passkey, p_ble_evt->evt.gap_evt.params.passkey_display.passkey, PASSKEY_LENGTH);
            passkey[PASSKEY_LENGTH] = 0;

            NRF_LOG_INFO("Passkey: %s", nrf_log_push(passkey));
            ble_control_message_protocol_send_display_passkey_needed_event();
        } break;
        
        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;

         case BLE_GAP_EVT_AUTH_STATUS:
             NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x, lesc:%d",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer),
                          p_ble_evt->evt.gap_evt.params.auth_status.lesc);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}




/**@brief Handles events coming from the LED Button central module.
 */
static void lbs_c_evt_handler(ble_lbs_c_t * p_lbs_c, ble_lbs_c_evt_t * p_lbs_c_evt)
{
    switch (p_lbs_c_evt->evt_type)
    {
        case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
            {
                ret_code_t err_code;

                NRF_LOG_INFO("LED Button service discovered on conn_handle 0x%x",
                             p_lbs_c_evt->conn_handle);

                err_code = app_button_enable();
                APP_ERROR_CHECK(err_code);

                // LED Button service discovered. Enable notification of Button.
                err_code = ble_lbs_c_button_notif_enable(p_lbs_c);
                APP_ERROR_CHECK(err_code);
            
                ble_gap_conn_params_t conn_params;
                conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
                conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
                conn_params.slave_latency     = SLAVE_LATENCY;
                conn_params.conn_sup_timeout  = SUPERVISION_TIMEOUT;

                sd_ble_gap_conn_param_update(p_lbs_c_evt->conn_handle, &conn_params);
                
                scan_start(m_scan_mode_coded_phy);
            } 
            break; // BLE_LBS_C_EVT_DISCOVERY_COMPLETE

        case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
            {
                NRF_LOG_INFO("Link 0x%x, Button state changed on peer to 0x%x",
                             p_lbs_c_evt->conn_handle,
                             p_lbs_c_evt->params.button.button_state);

                if (p_lbs_c_evt->params.button.button_state)
                {
                    bsp_board_led_on(LEDBUTTON_LED);
                }
                else
                {
                    bsp_board_led_off(LEDBUTTON_LED);
                }
            
                ble_control_message_protocol_send_lbs_c_button_event();
                // Forward the data to the app aggregator module
//                app_aggregator_on_blinky_data(p_lbs_c_evt->conn_handle, p_lbs_c_evt->params.button.button_state);
            } 
            break; // BLE_LBS_C_EVT_BUTTON_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Handles events coming from the Thingy UI central module.
 *
 * @param[in] p_thingy_uis_c     The instance of THINGY_UIS_C that triggered the event.
 * @param[in] p_thingy_uis_c_evt The THINGY_UIS_C event.
 */
static void thingy_uis_c_evt_handler(ble_thingy_uis_c_t * p_thingy_uis_c, ble_thingy_uis_c_evt_t * p_thingy_uis_c_evt)
{
    ret_code_t err_code;
    switch (p_thingy_uis_c_evt->evt_type)
    {
        case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
        {
            NRF_LOG_INFO("Thingy UI service discovered on conn_handle 0x%x\r\n", p_thingy_uis_c_evt->conn_handle);
            
            // Thingy UI service discovered. Enable notification of Button.
            err_code = ble_thingy_uis_c_button_notif_enable(p_thingy_uis_c);
            APP_ERROR_CHECK(err_code);
            
            ble_thingy_uis_led_set_constant(p_thingy_uis_c, 255, 255, 255);
            
            ble_gap_conn_params_t conn_params;
            conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
            conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
            conn_params.slave_latency     = SLAVE_LATENCY;
            conn_params.conn_sup_timeout  = SUPERVISION_TIMEOUT;

            sd_ble_gap_conn_param_update(p_thingy_uis_c_evt->conn_handle, &conn_params);
            
            scan_start(m_scan_mode_coded_phy);

        } break; // BLE_LBS_C_EVT_DISCOVERY_COMPLETE

        case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
        {
            // Forward the data to the app aggregator module
//            app_aggregator_on_blinky_data(p_thingy_uis_c_evt->conn_handle, p_thingy_uis_c_evt->params.button.button_state);
        } break; // BLE_LBS_C_EVT_BUTTON_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}


static ret_code_t led_status_send_by_mask(uint8_t button_action, uint8_t r, uint8_t g, uint8_t b, uint32_t mask)
{
    ret_code_t err_code;
    uint8_t colors[3] = {r, g, b};
    
//    app_aggregator_on_led_color_set(r, g, b, mask);

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if((mask & (1 << i)) != 0)
        {
            // First, try to access the devices as a Blinky device
            err_code = ble_lbs_led_color_send(&m_lbs_c[i], colors);
            if(err_code != NRF_SUCCESS)
            {
                // If the blinky call fails, assume this is a Thingy device
                err_code = ble_thingy_uis_led_set_constant(&m_thingy_uis_c[i], button_action ? r : 0, button_action ? g : 0, button_action ? b : 0);
                if (err_code != NRF_SUCCESS &&
                    err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                    err_code != NRF_ERROR_INVALID_STATE)
                {
                    return err_code;
                }
            }
        }
    }
    return NRF_SUCCESS;    
}

ret_code_t led_status_on_off_send_by_mask(bool on, uint32_t mask)
{
    ret_code_t err_code;

//    app_aggregator_on_led_update(on, mask);
    
    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if((mask & (1 << i)) != 0)
        {
            err_code = ble_lbs_led_status_send(&m_lbs_c[i], on);
            if(err_code != NRF_SUCCESS)
            {
                err_code = ble_thingy_uis_led_set_on_off(&m_thingy_uis_c[i], on);
                if (err_code != NRF_SUCCESS &&
                    err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                    err_code != NRF_ERROR_INVALID_STATE)
                {
                    return err_code;
                }
            }
        }
    }
    return NRF_SUCCESS;      
}

ret_code_t post_connect_message(uint8_t conn_handle)
{
    ret_code_t err_code = NRF_SUCCESS;
    for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if(m_lbs_c[i].conn_handle == conn_handle)
        {
            err_code = ble_thingy_uis_led_set_on_off(&m_thingy_uis_c[i], true);
        }
        
        if(m_thingy_uis_c[i].conn_handle == conn_handle)
        {
            err_code = ble_thingy_uis_led_set_constant(&m_thingy_uis_c[i], 255, 255, 255);  
        }
    }
    return err_code;
}

ret_code_t disconnect_all_peripherals()
{
    ret_code_t err_code;
    for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if(m_lbs_c[i].conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            err_code = sd_ble_gap_disconnect(m_lbs_c[i].conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if(err_code != NRF_SUCCESS)
            {
                //return err_code;
            }
        }
        
        else if(m_thingy_uis_c[i].conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            err_code = sd_ble_gap_disconnect(m_thingy_uis_c[i].conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if(err_code != NRF_SUCCESS)
            {
                //return err_code;
            }
        }
    }    
    return NRF_SUCCESS;
}


/**@brief Function for initializing buttons and leds. */
static void leds_init(void)
{
    uint32_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling database discovery events.
 *
 * @details This function is callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function should forward the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    NRF_LOG_DEBUG("call to ble_lbs_on_db_disc_evt for instance %d and link 0x%x!",
                  p_evt->conn_handle,
                  p_evt->conn_handle);

    ble_lbs_on_db_disc_evt(&m_lbs_c[p_evt->conn_handle], p_evt);
    ble_thingy_uis_on_db_disc_evt(&m_thingy_uis_c[p_evt->conn_handle], p_evt);
}

/**@brief Database discovery initialization.
 */
static void db_discovery_init(void)
{
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
//    ret_code_t err_code = nrf_ble_lesc_request_handler();
//    APP_ERROR_CHECK(err_code);
//
//    UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());

    NRF_LOG_FLUSH();
    nrf_pwr_mgmt_run();
}

static uint32_t send_data_to_ble_nus(uint8_t *data, uint16_t length)
{
    return ble_nus_data_send(&m_nus, data, &length, m_conn_handle);
}


/**@brief LED Button client initialization.
 */
static void lbs_c_init(void)
{
    ret_code_t       err_code;
    ble_lbs_c_init_t lbs_c_init_obj;

    lbs_c_init_obj.evt_handler = lbs_c_evt_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_lbs_c_init(&m_lbs_c[i], &lbs_c_init_obj);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief LED Button collector initialization. */
static void thingy_uis_c_init(void)
{
    ret_code_t       err_code;
    ble_thingy_uis_c_init_t thingy_uis_c_init_obj;

    thingy_uis_c_init_obj.evt_handler = thingy_uis_c_evt_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_thingy_uis_c_init(&m_thingy_uis_c[i], &thingy_uis_c_init_obj);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for writing to the LED characteristic of all connected clients.
 *
 * @details Based on if the button is pressed or released, this function writes a high or low
 *          LED status to the server.
 *
 * @param[in] button_action The button action (press/release).
 *            Determines if the LEDs of the servers will be ON or OFF.
 *
 * @return If successful NRF_SUCCESS is returned. Otherwise, the error code from @ref ble_lbs_led_status_send.
 */
static ret_code_t led_status_send_to_all(uint8_t button_action)
{
    ret_code_t err_code;

    for (uint32_t i = 0; i< NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_lbs_led_status_send(&m_lbs_c[i], button_action);

        if(err_code != NRF_SUCCESS)
        {
            err_code = ble_thingy_uis_led_set_on_off(&m_thingy_uis_c[i], button_action);
            //err_code = ble_thingy_uis_led_set_constant(&m_thingy_uis_c[i], button_action ? 255 : 0, button_action ? 255 : 0, button_action ? 255 : 0);
            if (err_code != NRF_SUCCESS &&
                err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                err_code != NRF_ERROR_INVALID_STATE)
            {
                return err_code;
            }
        }
    }
    return NRF_SUCCESS;
}

/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    ret_code_t err_code;

    switch (pin_no)
    {
//        case CENTRAL_DISCONNECT_BUTTON:
//            if(m_per_con_handle != BLE_CONN_HANDLE_INVALID)
//            {
//                sd_ble_gap_disconnect(m_per_con_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
//            }
//            break;
//        
//        case SCAN_START_STOP_BUTTON:
//            if(button_action == APP_BUTTON_PUSH)
//            {
//                m_scanning_enabled = !m_scanning_enabled;
//                if(m_scanning_enabled)
//                {
//                    scan_start(false);
//                }
//                else
//                {
//                    scan_stop();
//                }
//            }
//            break;
        case LEDBUTTON_BUTTON_PIN:
            err_code = led_status_send_to_all(button_action);
            if (err_code == NRF_SUCCESS)
            {
                NRF_LOG_INFO("LBS write LED state %d", button_action);
            }
            break;
                   
        default:
            APP_ERROR_HANDLER(pin_no);
            break;
    }
}

/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
    ret_code_t err_code;

    //The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] =
    {
        {LEDBUTTON_BUTTON_PIN, false, BUTTON_PULL, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Scaning events.
 *
 * @param[in]   p_scan_evt   Scanning event.
 */
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;

    switch(p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
            err_code = p_scan_evt->params.connecting_err.err_code;
            APP_ERROR_CHECK(err_code);
            break;
        default:
          break;
    }
}


static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Setting filters for scanning.
    err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_NAME_FILTER, false);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_NAME_FILTER, m_target_periph_name);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(const uint8_t *name, uint8_t length)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, name, length);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

static int set_ble_passkey(const uint8_t *passkey)
{
    ble_opt_t ble_opt;
    // The passkey is a 6-character ASCII string containing numerals only ('0'-'9').
    ble_opt.gap_opt.passkey.p_passkey = passkey;
    ret_code_t err_code = sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &ble_opt);
    if(err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    return 0;
}

/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}

/**@snippet [Handling the data received over BLE] */


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_whitelist_enabled          = true;
    init.config.ble_adv_directed_high_duty_enabled = true;
    init.config.ble_adv_directed_enabled           = false;
    init.config.ble_adv_directed_interval          = 0;
    init.config.ble_adv_directed_timeout           = 0;
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@snippet [UART Initialization] */

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

static int init_ble_stack(const uint8_t *name, uint8_t length)
{
    if(m_initialization_completed)
    {
        return BLE_DEVICE_ALREADY_INITIALIZED;
    }
    NRF_LOG_INFO("INIT BLE STACK.");
//    ble_stack_init();
    gap_params_init(name, length);
//    gatt_init();
    services_init();
    app_aggregator_init(&m_agg_cfg_service);    // aggregator service
    advertising_init();
    conn_params_init();
    peer_manager_init();
    // Start execution.
    ble_start_advertising_handler(true);
    m_initialization_completed = true;
    return 0;
}

static int process_app_commands(uint8_t *p_data, uint16_t data_size)
{           

    uint8_t agg_cmd_received = p_data[0];      
    uint8_t agg_cmd[32];


    memcpy(agg_cmd, &p_data[1], data_size);

    if(agg_cmd_received != 0)
    {          
        uint32_t mask;
        NRF_LOG_INFO("APP COMMAND");
        switch(agg_cmd_received)
        {
            case APPCMD_SET_LED_ALL:
                for(int i = 2; i >= 0; i--)
                {
                    mask = mask << 8 | agg_cmd[4 + i];
                }
                led_status_send_by_mask(agg_cmd[0], agg_cmd[1], agg_cmd[2], agg_cmd[3], mask);
                break;
                
            case APPCMD_SET_LED_ON_OFF_ALL:
                for(int i = 2; i >= 0; i--)
                {
                    mask = mask << 8 | agg_cmd[1 + i];
                }
                led_status_on_off_send_by_mask(agg_cmd[0], mask);
                break;
                
            case APPCMD_POST_CONNECT_MESSAGE:
                post_connect_message(agg_cmd[0]);
                break;
            
            case APPCMD_DISCONNECT_PERIPHERALS:
                disconnect_all_peripherals();
                break;

            case APPCMD_DISCONNECT_CENTRAL:
                sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                break;
            
            case APPCMD_TEST_CMD:     //6
                led_status_send_to_all(agg_cmd[0]);
                break;

            default:
                break;
        }
        agg_cmd_received = 0;
    }
    else
    {
        NRF_LOG_WARNING("AGG CMD ZERO -ERROR!!\r\n");

    }
}

/**@brief Application main function.
 */
int main(void)

 {
    // Initialize.
    message_protocol_init(send_data_to_ble_nus);
    ble_control_message_protocol_init(init_ble_stack, \
                                    set_ble_passkey, \
                                    ble_start_advertising_handler, \
                                    delete_bonds, \
                                    process_app_commands, 
                                    get_aggregator_data);

    log_init();
    timers_init();
    leds_init();
    buttons_init();
    power_management_init();
    ble_stack_init();
    scan_init();
    gatt_init();
    db_discovery_init(); // discovering and handler attaching to ble led button...
    lbs_c_init();
    thingy_uis_c_init();

    NRF_LOG_INFO("Azure CENTRAL scan start");

    for (int i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; ++i){
        m_coded_phy_conn_handle[i] = BLE_CONN_HANDLE_INVALID;
    }
    scan_start(false);
    // Turn on the LED to signal scanning.
    bsp_board_led_on(CENTRAL_SCANNING_LED);

    ble_control_message_protocol_send_device_up_event();

    // Enter main loop.
    for (;;)
    {
//        if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
//        {
//            while(app_aggregator_flush_ble_commands());
//        }
        idle_state_handle();
    }
}

/**
 * @}
 */
