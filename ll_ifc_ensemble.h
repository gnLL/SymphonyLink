#ifndef __LL_IFC_ENSEMBLE_H
#define __LL_IFC_ENSEMBLE_H

#include "ll_ifc_consts.h"
#include "ll_ifc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup Link_Labs_Interface_Library
 * @{
 */

/**
 * @defgroup Ensemble_Interface Ensemble
 *
 * @brief Create your own Ensemble network.
 *
 * Ensemble allows the module to act as the gateway, repeater, or endpoint
 * for low capacity, low power networks.
 *
 * @{
 */

/**
 * Link Labs ensemble MAC role identifiers
 */
typedef enum
{
    ENSEMBLE_ROLE_ENDPOINT = 0,				// Node assigend as endpoint
    ENSEMBLE_ROLE_GATEWAY  = 1,				// Node assigned as gateway
    ENSEMBLE_ROLE_REPEATER = 2,				// Node assigned as repeater
    ENSEMBLE_ROLE_MAX
} ensemble_role_t;

#define ENSEMBLE_AES_KEY_LENGTH        (16)    // length of AES key (note this is storage size - not its entry size as a string of characters)
#define MAX_ENSEMBLE_PROPERTY_LENGTH   (16)    // set this to the largest property (size) used for ensemble MAC protocol - currently the AES key
#define MAX_ENSEMBLE_TRANSFER_SIZE     (255)   // max length of any msg that can ever be send <=> over host insterface - include payload and header overhead
#define MAX_ENSEMBLE_PAYLOAD_LENGTH    (128)   // max allowable message payload can be be configure for OTA transfer
#define ENSEMBLE_DBG_BUFFER_SIZE       (32)    // debug buffer size (used to return debug information to host - will be deprecated

/**
 * Link Labs ensemble MAC property identifiers
 */
typedef enum
{
    ENSEMBLE_PROP_FREQUENCY            = 0,    // Frequency                    : type uint32_t
    ENSEMBLE_PROP_BW                   = 1,    // Enumerated Bandwidth         : type uint32_t (enumeration: property_bw_t)
    ENSEMBLE_PROP_SF                   = 2,	// Enumerated Spreading factor  : type uint32_t (enumeration: property_sf_t)
    ENSEMBLE_PROP_CR                   = 3,	// Enumerated Coding Rate       : type uint32_t (enumeration: property_cr_t)
    ENSEMBLE_PROP_NODE_ROLE            = 4,	// Enumerated Role for module   : type uint32_t (enumeration: ensemble_role_t)
    ENSEMBLE_PROP_TX_POWER             = 5,    // Module Tx power (dBm)        : type uint32_t
    ENSEMBLE_PROP_PREAMBLE_LENGTH      = 6,    // Message preamble length (symbols) : type uint32_t
    ENSEMBLE_PROP_MAX_TX_ATTEMPTS      = 7,    // Max Tx attempts per msg      : type uint32_t
    ENSEMBLE_PROP_MAX_PAYLOAD_SIZE     = 8,    // Max app payload size / msg   : type uint32_t
    ENSEMBLE_PROP_GATEWAY_MAX_MSG_CNT  = 9,    // Max msgs buffered in a gateway:              type uint32_t
    ENSEMBLE_PROP_TX_FAIL_RETRY_DELAY  = 10,   // Delay between Tx attempts on error (sec):    type uint32_t
    ENSEMBLE_PROP_MSG_AVAIL_NOTIFY_MODE  = 11, // Notify method when Gateway has msgs available: type uint32_t
    ENSEMBLE_PROP_MAIL_ACK_DELAY       = 12,   // acknowledge time delay (in gateway module) waiting for Mail from Host before ACK is sent
    ENSEMBLE_PROP_REPEATER_AVAILABLE   = 13,   // Repeater available for use by EP:            type uint32_t
                                                // Recommendation: add new properties prior to the Traffic Key
    ENSEMBLE_PROP_MSG_TRAFFIC_KEY      = 14,   // AES encryption key (AES128)  : type 16 byte array of uint8_t's
    ENSEMBLE_PROP_MAX
} ensemble_property_t;

/**
 * Link Labs ensemble receive message characteristics enumeration
 */
typedef enum            // inbound Rx message characteristics field 
{
    ENSEMBLE_MSG_ROUTED_VIA_REPEATER = 0x8000,           // if set (bit 15), then msg was inbound from a repeater
                                                          // all other bits/values (bit 0 - 14) are currently unused
} ensemble_msg_characteristics_t;

/**
 * Link Labs ensemble receive message descriptor structure
 */
typedef struct ensemble_msg_descriptor
{
    uint16_t    msginfo;        // attributes/characteristics of the msg
                                // currently this is limited to whether the msg was route via a Repeater / came directly from an EP
                                // see enumeration: ensemble_msg_characteristics_t
    int16_t     rssi;           // RSSI of the captured msg
    uint64_t    utc_time;       // timestamp of UTC time when msg was Rx'ed in Gateway
    uint64_t    mui;            // module unique ID (mac address of EP module that sent the msg)
} ensemble_msg_descriptor_t;

                                // these aren't ideal, but represent how msg descriptor fields are serialize in host XFER
#define ENSEMBLE_SERIALIZED_OFFSET_MSGINFO (0)
#define ENSEMBLE_SERIALIZED_OFFSET_RSSI    (ENSEMBLE_SERIALIZED_OFFSET_MSGINFO + sizeof (uint16_t))
#define ENSEMBLE_SERIALIZED_OFFSET_UTCTIME (ENSEMBLE_SERIALIZED_OFFSET_RSSI + sizeof (uint16_t))
#define ENSEMBLE_SERIALIZED_OFFSET_MUI     (ENSEMBLE_SERIALIZED_OFFSET_UTCTIME + sizeof (uint64_t))
#define ENSEMBLE_SERIALIZED_OFFSET_TO_PAYLOAD     (ENSEMBLE_SERIALIZED_OFFSET_MUI + sizeof (uint64_t))

    /**
    * @brief
    *  Get/return the size of a ensemble property (in bytes)
    *
    * @param[in] propertyId - The id (ensemble_property_t) of the property
    *
    * @return
	*   0 - the size of the property if the ID is valid. Zero (0) otherwise,
    *
    */
    int32_t ll_ensemble_property_size( ensemble_property_t property_id );

    /**
    * @brief
    *  Sets an individual configuration property for a module that is used when
    *  the module is running the ensemble MAC protocol.
    *
    *  The entire set of configuration parameters for the ensemble MAC protocol
    *  are represented by a set of properties that can be written by this interface
    *  to configure the protocol for use.
    *
    * @param[in] propertyId - The id (ensemble_property_t) of the property to configure.
    * @param[in] propertyValue - a pointer to the property value. Note that different properties
    *                           may (and will) be of different types (and sizes).
	*
    *
    * @details
    *
	*     The following list of properties are available for configuration of the ensemble MAC protocol:
    *   <ol>
    *   <li> <b>Frequency</b> The configured Frequency. Specified as a <I>uint32_t</I>.
    *   <li> <b>Bandwidth</b> The enumerated Bandwidth. Specified as a <I>uint32_t</I>. Is an enumeration of type <I>property_bw_t</I>.
    *   <li> <b>Spreading Factor</b> The enumerated spreading factor. Specified as a <I>uint32_t</I>. Is an enumeration of type <I>property_sf_t</I>.
    *   <li> <b>Coding Rate</b> The enumerated coding rate. Specified as a <I>uint32_t</I>. Is an enumeration of type <I>property_cr_t</I>.
    *   <li> <b>Module Role</b> Choices are: Endpoint, Gateway, or Repeater. Specified as a <I>uint32_t</I>. Is an enumeration of type <I>ensemble_role_t</I>.
    *   <li> <b>Tx Power</b> The module Tx power level specified in dBm. Specified as a <I>uint32_t</I>.
    *   <li> <b>Preamble Length</b> The preamble length (number of symbols) to be used for all Tx'ed messages. Specified as a <I>uint32_t</I>.
    *   <li> <b>Max Tx Attempts</b> (per message). Specified as a <I>uint32_t</I>.
    *   <li> <b>Max Application Payload Size</b> (per msg). Specified as a <I>uint32_t</I>.
    *   <li> <b>Max Msg Storage Capacity</b> (in a Gateway - prior to transfer to a host). Specified as a <I>uint32_t</I>.
    *   <li> <b>Delay Between Tx Attempts On Tx Error</b> (seconds). Specified as a <I>uint32_t</I>.
    *   <li> <b>Notification (of Host) Method</b> (0 - Host must poll, 1 - Assert GPIO HOST_IO0 if data available, 2 - Assert GPIO HOST_IO0 if data available
    *           to time is availble for Host to craft a mail message to return to the source module). Specified as a <I>uint32_t</I>.
    *   <li> <b>Mail Acknowledgement Delay</b> The amount of delay (prior to a Gateway module returning an ACK) the Gateway module
    *           will wait for Host (connected to Gateway) to provide a mail message for return to the endpoint as part of the ACK.
    *           a zero (0) is valid and indicates there is no delay (prior) to returning the ACK.
    *   <li> <b>Message Encryption Key </b> AES encryption key (AES128). Specified as a 16 byte array of <I>uint8_t/I>'s
    *   <li> <b>Repeater Available </b> 0 - No Repeater availaible. 1 - Repeater availble to relay messages if GW module cannot be reached 
    *           directly. Parameter relevant to Endpoint only. Specified as a <I>uint32_t</I>.
    *   </ol>
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_set_ensemble_config_property(ensemble_property_t propertyId, void* propertyValue);

    /**
    * @brief
    *  Gets (returns) the current value for a specified configuration property defined
    *  for the ensemble MAC protocol
    *
    * @param[in] propertyId - The id (ensemble_property_t) of the property to return.
    * @param[in] propertyValue - a pointer to buffer to hold the returned property value. Note that different properties
    *                           may (and will) be of different types (and sizes).
	*
    * @details
    *
	*     See the desciption for ll_set_ensemble_config_property() for a complete
    *     listing of available parameters. NOTE: The Message Encryption Key property cannot
    *     be accessed by this interface.
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_get_ensemble_config_property(ensemble_property_t propertyId, void* propertyValue);

   /**
    * @brief
    *   Gets the number of currently buffered (stored) messages
	*	awaiting transfer to the host application.
	*
	*   This parameter is only relevant when the module is in Link Labs 
	*	ensemble mode and it is configured in the GATEWAY role.
	*
    * @param[out] ptr to a returned message count. 
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_get_ensemble_stored_msg_count(uint32_t* message_count);

   /**
    * @brief
    *   Gets the number of messages (missed) by gateway due to 
    *   storage being full when the msg(s) came in or number of messages
    *   in an endpoint that were discarded after ACK failure
	*
	*   This parameter is only relevant when the module is in Link Labs 
	*	ensemble mode and it is configured in the GATEWAY or ENDPOINT role.
	*
    * @param[out] ptr to a returned message count.
    * @param[in] whether module reset's the count in the module after reading it. 
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_get_ensemble_lost_msg_count(uint32_t* message_count, uint8_t reset);

   /**
    * @brief
    *   Return the next stored message (contained in a module
	*	designated as a Gateway) running in Link Labs Micro MAC mode.
	*
	* @param[out] ptr to a ensemble_msg_descriptor_t characteristics structure that returns information
    * about the message. 
    * @param[out] ptr to a buffer in which to return message. Note that the max size of this
	* messaage is configured via the ll_set_ensemble_payload_size() method.
    * @param[out] bufferSize - size of the buffer (in bytes) available to hold the msg being returned
	*
	* The timestamp is specified as a uint64_t time (in seconds) relative to the UTC epoch.
    * @details
    *
	*     The following is a list of fields contained within the returned ensemble_msg_descriptor_t
    *      strucure and what they represent.
    *   <ol>
    *   <li> <b>msginfo</b> The attributes/characteristics of the msg. See see enumeration: ensemble_msg_characteristics_t.
    *   <li> <b>rssi</b> The rssi measurement (in dBm) associated with the message
    *   <li> <b>utc_time</b> The time (specified in UTC as a <I>uint64_t</I>) of when the message was received.
    *   <li> <b>mui</b> The module unique ID (i.e. MAC address who sent the msg). Specified as a <I>uint64_t</I>.
    *   </ol>
    *
	* Note: For the timestamp to have relevance, the time must have been previously
	* set via the ll_set_time() method.
    *
    * @return
	*   > 0 - success (value represent the number of bytes returned as a message
    *   = 0 - no message returned (none available)
    *   < 0 - error
    */
    int32_t ll_get_ensemble_get_stored_msg(ensemble_msg_descriptor_t* msg_descriptor, uint8_t* buffer, uint32_t bufferSize);

   /**
    * @brief
    *   Return the next mail message (contained in an EP module)
	*
    * @param[out] ptr to a buffer in which to return message. Note that the max size of this
	* messaage is configured via the ll_set_ensemble_payload_size() method.
    * @param[out] bufferSize - size of the buffer (in bytes) available to hold the msg being returned
	*
    * Initial implementation only holds a single mail msg (at-a-time).
    *
    * @return
	*   > 0 - success (value represent the number of bytes returned as a message
    *   = 0 - no message returned (none available)
    *   < 0 - error
    */
    int32_t ll_get_ensemble_get_mail_msg(uint8_t* buffer, uint32_t bufferSize);

   /**
    * @brief
    *   Set UTC time.
	*	Currently used to timestamp incoming messages in a module configured
	*	to use the ensemble mode as a Gateway.
	*
    * @param[in] ptr to UTC time specified as a uint64_t
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_set_utc_time(uint64_t* utc_time);

   /**
    * @brief
    *   Get UTC time.
	*	Currently used to get the UTC time currently set. Note: the module must have been previously
    *   configured for ensemble with the GATEWAY role, and the UTC time must have been previously set
    *   for this value to have meaning.
	*
    * @param[out] ptr to returned UTC time (seconds) specified as a uint64_t
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_get_utc_time(uint64_t* utc_time);

   /**
    * @brief
    *   Send data to Gateway.
	*	Used by endpoint application to encode/enqueue data to be sent to gateway.
    *   The specified size must be less then > the configured maximum payload size
    *   for the endpoint to use. Since the data is queued to the ensemble MAC, if
    *   the queue is full, an error is returned.
	*
    * @param[in] payload - a pointer to buffer to holding the data to send to gateway
    * @param[in] payloadSize - The size of the payload data enqueue/send to gateway
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_send_payload_to_gw(uint8_t* payload, uint8_t payload_size);

   /**
    * @brief
    *   Send mail to Endpoint
	*	Used by Host application to send "mail" to and endpoint after an incoming
    *   msg message was received from the target endpoint.
    *   These messaages are targeted towards a specific endpoint by specifying
    *   its MAC address as part of this API.
    *
    *   Note: Since the the message can only sent in response to an incoming
    *   data message from the endpoint, it is necessary for the message to be 
    *   enqueued PRIOR to the next incoming OR within one second of the incoming
    *   messaage. The Gateway will hold off the ACK for a configurable period (allowing
    *   the connected host time to determine whether a mail response should be set).
    *
    *   Note: a specified MAC that is zero (all 0s) shall be interpreted as a assocated with
    *   the next incoming endpoint message regardless of its MAC address
    *
    *   Note: a zero length message is permissible and used to immediately acknowledge
    *   the message without sending actually sending a mail message to the endpoint
	*
    * @param[in] ep_mac_address - a pointer to destination endpoint MAC address
    * @param[in] payload - a pointer to buffer to holding the data to send to gateway
    * @param[in] payloadSize - The size of the payload data enqueue/send to gateway
    *
    * @return
	*   0 - success, negative otherwise
    */
    int32_t ll_send_mail_to_ep(uint64_t* ep_mac_address, uint8_t* payload, uint8_t payload_size);

   /**
    * @brief
    *   Get debug data (dump) from module
	*
    * @param[out] ptr to a buffer in which to return debug data. 
    * @param[out] bufferSize - size of the buffer (in bytes) available to hold the msg being returned
	*
    * @return
	*   > 0 - success (value represent the number of bytes returned as a message
    *   = 0 - no message returned (none available)
    *   < 0 - error
    */
    int32_t ll_get_ensemble_get_debuginfo(uint8_t* buffer, uint32_t bufferSize);

/** @} (end defgroup Ensemble_Interface) */

/** @} (end addtogroup Link_Labs_Interface_Library) */
#ifdef __cplusplus
}
#endif

#endif
