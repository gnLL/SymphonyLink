#ifndef __LL_IFC_LORAWAN_H
#define __LL_IFC_LORAWAN_H

#include <stdint.h>
#include <string.h>  // memcpy
#include "ll_ifc_consts.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LL_ARG_CHECK(x) if (!(x)) { return LL_IFC_ERROR_INCORRECT_PARAMETER; }


/* ------------------ LoRaWAN ------------------------------------------ */

    /**
     * @addtogroup Link_Labs_Interface_Library
     * @{
     */

    /**
     * @defgroup LoRaWAN_Interface LoRaWAN
     *
     * @brief Communicate with a LoRaWAN network.
     *
     * LoRaWAN is a Low-Power Wide Area Network (LPWAN) based upon the same
     * Semtech radios as the Link Lab's Symphony Link.  The specification
     * is maintained by the
     * <a href="http://www.lora-alliance.org/">LoRa Alliance</a>.
     * The ll_ifc LoRaWAN mode allows the external host to connect to LoRaWAN
     * networks using Link Lab's modules.
     *
     * LoRaWAN consists of nodes, gateways and a server.  The nodes are the
     * low-power end devices that send RF packets. Gateways receive the packets
     * and blindly forward them to the server.  The server inspects the packet
     * and optionally responds to the node.  The system consisting of the
     * external host and the Link Lab's module forms a LoRaWAN node.
     *
     * LoRaWAN supports three difference classes of operation:
     *
     * - Bi-directional end-devices (Class A): End-devices of Class A allow for
     *   directional communications whereby each end-device's uplink
     *   transmission is followed by two short downlink receive windows.  The
     *   transmission slot scheduled by the end-device is based on its own
     *   communication needs with a small variation based on a random time
     *   basis (ALOHA-type of protocol).  This Class A operation is the lowest
     *   power end-device system for applications that only require downlink
     *   communication from the server shortly after the end-device has sent
     *   an uplink transmission. Downlink communications from the server at
     *   any other time will have to wait until the next scheduled uplink.
     * - Bi-directional end-devices with scheduled receive slots (Class B):
     *   End-devices of Class B allow for more receive slots. In addition to
     *   the Class A random receive windows, Class B devices open extra
     *   receive windows at scheduled times. In order for the End-device to
     *   open it receive window at the scheduled time it receives a time
     *   synchronized Beacon from the gateway. This allows the server to
     *   know when the end-device is listening.
     * - Bi-directional end-devices with maximal receive slots (Class C):
     *   End-devices of Class C have nearly continuously open receive windows,
     *   only closed when transmitting. Class C end-device will use more power
     *   to operate than Class A or Class B but they offer the lowest latency
     *   for server to end-device communication.
     *
     * credit: section 2.1 of the LoRaWAN specification v1.0
     *
     * Nodes must be activated before communicating on the network, and
     * LoRaWAN has two different activation types.
     * - Activation by Personalization (ABP) assigns the provisioned
     *   network information directly to the device, usually at the time of
     *   manufacturing.  This information is stored to the external host
     *   and provided to the network using ll_lorawan_activate_personalization()
     * - Over-The-Air Activation (OTAA) uses higher level network
     *   information which can allow devices to connect to networks in
     *   the field.  The devEui is still commonly assigned at the time
     *   of manufacturing, but the remaining information can be determined
     *   later.  The external host provides this information to the network
     *   using ll_lorawan_activate_over_the_air().
     *
     * Once the module is connected to the network, the external host can
     * send packets over the network using either ll_lorawan_send_confirmed()
     * or ll_lorawan_send_unconfirmed().  The module receives packets using
     * ll_lorawan_receive().  The module also supports several parameters
     * which can be retrieved using ll_lorawan_param_get_i32().
     * ll_lorawan_param_set_i32() allows the external host to modify parameters
     * which allow modification.
     *
     * @{
     */

    /**
     * @brief
     *   Activate the LoRaWAN network using the over-the-air join procedure.
     *
     * @details
     *   Dynamically connect to a LoRaWAN network.  Any existing LoRaWAN
     *   connection will be dropped.
     *
     * @param[in] network_type
     *   The ::ll_lorawan_network_type_e which is usually LL_LORAWAN_PUBLIC.
     *
     * @param[in] device_class
     *   The ::ll_lorawan_device_class_e.  LL_LORAWAN_CLASS_A consumes the
     *   lowest power and is a good first choice when developing a device.
     *   See the LoRaWAN specification for details.
     *
     * @param[in] devEui
     *   Pointer to the device EUI array ( 8 bytes ).  When all bytes are 0,
     *   the preconfigured device EUI will be used.
     *
     * @param[in] appEui
     *   Pointer to the application EUI array ( 8 bytes ).
     *
     * @param[in] appKey
     *   Pointer to the application AES128 key array ( 16 bytes ).
     *
     * @return
     *   0 - success, negative otherwise.
     */
    int32_t ll_lorawan_activate_over_the_air(
            enum ll_lorawan_network_type_e network_type,
            enum ll_lorawan_device_class_e device_class,
            uint8_t const * devEui,
            uint8_t const * appEui,
            uint8_t const * appKey );

    /**
     * @brief
     *   Activate LoRaWAN network using the pre-configured personalization
     *   procedure.
     *
     * @details
     *   Statically connect to a LoRaWAN network.  Using
     *   ll_lorawan_activate_by_join() is preferred whenever possible.  Any
     *   existing LoRaWAN connection will be dropped.
     *
     * @param[in] network_type
     *   The ::ll_lorawan_network_type_e which is usually LL_LORAWAN_PUBLIC.
     *
     * @param[in] device_class
     *   The ::ll_lorawan_device_class_e.  LL_LORAWAN_CLASS_A consumes the
     *   lowest power and is a good first choice when developing a device.
     *   See the LoRaWAN specification for details.
     *
     * @param[in] netID
     *   The 24 bit network identifier as provided by the network operator.
     *
     * @param[in] devAddr
     *   The 32 bit device address which must be unique to the network.
     *
     * @param[in] netSKey
     *   The AES128 network session key ( 16 bytes ).
     *
     * @param[in] appSKey
     *   The AES128 application session key ( 16 bytes ).
     *
     * @return
     *   0 - success, negative otherwise.
     */
    int32_t ll_lorawan_activate_personalization(
            enum ll_lorawan_network_type_e network_type,
            enum ll_lorawan_device_class_e device_class,
            uint32_t netID,
            uint32_t devAddr,
            uint8_t const * netSKey,
            uint8_t const * appSKey );

    /**
     * @brief
     *   Get a LoRaWAN parameter value.
     *
     * @param[in] param
     *   The ::ll_lorawan_param_e parameter to get.
     *
     * @param[out] value
     *   The parameter value.
     *
     * @return
     *   0 on success or a negative error code value.
     */
    int32_t ll_lorawan_param_get_i32(enum ll_lorawan_param_e param, int32_t * value);

    /**
     * @brief
     *   Set a LoRaWAN parameter value.
     *
     * @param[in] param
     *   The ::ll_lorawan_param_e parameter to get.
     *
     * @param[in] value
     *   The parameter value.
     *
     * @return
     *   0 on success or a negative error code value.
     */
    int32_t ll_lorawan_param_set_i32(enum ll_lorawan_param_e param, int32_t value);

    /**
     * @brief
     *   Send a packet over LoRaWAN.
     *
     * @param[in] flags
     *   The optional ::ll_lorawan_send_flags_e flags or 0.
     *
     * @param[in] fPort
     *   The LoRaWAN MAC payload port where 0 < fPort < 254.
     *
     * @param[in] buffer
     *   The packet payload.
     *
     * @param[in] buffer_length
     *   The number of bytes in buffer.  The maximum value is region specific.
     *   See the LoRaWAN specification for details.
     *
     * @return
     *   0 on success or a negative error code value.
     */
    int32_t ll_lorawan_send_unconfirmed(
            uint8_t flags,
            uint8_t fPort,
            uint8_t const * buffer,
            uint8_t buffer_length);

    /**
     * @brief
     *   Send a packet over LoRaWAN.
     *
     * @details
     *   This function just queues the packet for transmission.  The actual
     *   packet transmission time is dependent upon LoRaWAN.  Poll using
     *   ll_lorawan_receive to get the gateway's response.
     *
     * @param[in] flags
     *   The optional ::ll_lorawan_send_flags_e flags or 0.
     *
     * @param[in] fPort
     *   The LoRaWAN MAC payload port where 0 < fPort < 254.
     *
     * @param[in] buffer
     *   The packet payload.
     *
     * @param[in] buffer_length
     *   The number of bytes in buffer.  The maximum value is region specific.
     *   See the LoRaWAN specification for details.
     *
     * @param[in] retries
     *   The number of times to attempt buffer transmission before giving up.
     *
     * @return
     *   0 on success or a negative error code value.
     *   -LL_IFC_NACK_BUSY_TRY_AGAIN is expected when the transmit FIFO is
     *   full.
     */
    int32_t ll_lorawan_send_confirmed(
            uint8_t flags,
            uint8_t fPort,
            uint8_t const * buffer,
            uint8_t buffer_length,
            uint8_t retries);

    /**
     * @brief
     *   The receive packet along with metadata information.
     */
    typedef struct ll_lorawan_rx_s {
        /**
         * @brief
         *   The ::ll_lorawan_receive_flags_e that indicate features of the
         *   received packet.
         */
        uint8_t flags;
        uint8_t TxAckReceived;
        uint8_t TxNbRetries;
        uint8_t bytes_received;
        uint8_t RxPort;
        int16_t RxRssi;
        uint8_t RxSnr;
        uint8_t DemodMargin;
        uint8_t NbGateways;
    } ll_lorawan_rx_t;

    /**
     * @brief
     *   Receive a packet over LoRaWAN.
     *
     * @details
     *   This function transfers a received packet from the module to the
     *   external host.  The packet must have been previously received over
     *   LoRaWAN.
     *
     * @param[out] buf
     *   The buffer that contains copy of the received packet on success.
     *
     * @param[out] len
     *   The number of bytes available in buf for a message.
     *
     * @param[out] rx
     *   The pointer to the receive packet metadata.
     *
     * @return
     *   0 on success or a negative error code value.  If no packet was
     *   received, then RxBufferSize will be 0.
     */
    int32_t ll_lorawan_receive(uint8_t * buf, uint16_t len, ll_lorawan_rx_t * rx);

    /** @} (end defgroup LoRaWAN_Interface) */

    /** @} (end addtogroup Link_Labs_Interface_Library) */

#ifdef __cplusplus
}
#endif

#endif /* __LL_IFC_LORAWAN_H */
