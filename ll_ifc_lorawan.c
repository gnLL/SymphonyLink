#include "ll_ifc_lorawan.h"
#include "ll_ifc.h"
#include "ll_ifc_private.h"


#define LL_LORAWAN_ACTIVATE_TIMEOUT_S 60

static uint32_t lorawan_active_wait()
{
    uint8_t rsp[] = {0};
    uint8_t msg[] = {LL_LORAWAN_ACTIVATION_QUERY};
    struct time time_start;
    struct time time_current;
    uint32_t irq_flags_read = 0;
    static const uint32_t FLAGS_TO_CLEAR = IRQ_FLAGS_CONNECTED |
                                           IRQ_FLAGS_RX_DONE |
                                           IRQ_FLAGS_TX_ERROR |
                                           IRQ_FLAGS_TX_DONE;

    gettime(&time_start);

    while (1)
    {
        int32_t rw_response = hal_read_write(OP_LORAWAN_ACTIVATE, msg, sizeof(msg), rsp, sizeof(rsp));
        if (rw_response < 0)
        {
            return((int8_t) rw_response);
        }
        if (rw_response != sizeof(rsp))
        {
            return LL_IFC_ERROR_INCORRECT_MESSAGE_SIZE;
        }

        switch (rsp[0])
        {
            case LL_LORAWAN_ACTIVATION_STATUS_COMPLETED: // unexpected but ok
                ll_irq_flags(FLAGS_TO_CLEAR, &irq_flags_read);
                return 0;
            case LL_LORAWAN_ACTIVATION_STATUS_PENDING: // expected
                break;
            case LL_LORAWAN_ACTIVATION_STATUS_FAILED: // expected error
                return -LL_IFC_NACK_OTHER;
            default: // unexpected (should never happen)
                return -LL_IFC_NACK_OTHER;
        }

        gettime(&time_current);
        if ((time_current.tv_sec - time_start.tv_sec) > LL_LORAWAN_ACTIVATE_TIMEOUT_S)
        {
            return LL_IFC_ERROR_TIMEOUT;
        }

        sleep_ms(1000); // 1 second, LoRaWAN takes a while
    }
}

static int32_t lorawan_activate(uint8_t * msg, uint16_t msg_size)
{
    uint8_t rsp[] = {0};

    int32_t rw_response = hal_read_write(OP_LORAWAN_ACTIVATE, msg, msg_size, rsp, sizeof(rsp));
    if (rw_response < 0)
    {
        return((int8_t) rw_response);
    }
    if (rw_response != sizeof(rsp))
    {
        return LL_IFC_ERROR_INCORRECT_MESSAGE_SIZE;
    }

    switch (rsp[0])
    {
        case LL_LORAWAN_ACTIVATION_STATUS_COMPLETED: // unexpected but ok
            return 0;
        case LL_LORAWAN_ACTIVATION_STATUS_PENDING: // expected
            break;
        case LL_LORAWAN_ACTIVATION_STATUS_FAILED: // unexpected and error
            return -LL_IFC_NACK_OTHER;
        default: // unexpected (should never happen)
            return -LL_IFC_NACK_OTHER;
    }

    return lorawan_active_wait();
}


int32_t ll_lorawan_activate_over_the_air(
        enum ll_lorawan_network_type_e network_type,
        enum ll_lorawan_device_class_e device_class,
        uint8_t const * devEui,
        uint8_t const * appEui,
        uint8_t const * appKey)
{
    uint8_t msg[1 + 1 + 1 + 8 + 8 + 16];

    LL_ARG_CHECK((network_type == LL_LORAWAN_PUBLIC) || (network_type == LL_LORAWAN_PRIVATE));
    LL_ARG_CHECK((device_class == LL_LORAWAN_CLASS_A) ||
                 (device_class == LL_LORAWAN_CLASS_B) ||
                 (device_class == LL_LORAWAN_CLASS_C));
    LL_ARG_CHECK(NULL != devEui);
    LL_ARG_CHECK(NULL != appEui);
    LL_ARG_CHECK(NULL != appKey);

    msg[0] = LL_LORAWAN_ACTIVATION_OVER_THE_AIR;
    msg[1] = (uint8_t) network_type;
    msg[2] = (uint8_t) device_class;
    memcpy(&msg[3], devEui, 8);
    memcpy(&msg[3 + 8], appEui, 8);
    memcpy(&msg[3 + 8 + 8], appKey, 16);

    return lorawan_activate(msg, sizeof(msg));
}

int32_t ll_lorawan_activate_personalization(
        enum ll_lorawan_network_type_e network_type,
        enum ll_lorawan_device_class_e device_class,
        uint32_t netID,
        uint32_t devAddr,
        uint8_t const * netSKey,
        uint8_t const * appSKey )
{
    uint8_t msg[1 + 1 + 1 + 4 + 4 + 16 + 16];
    uint8_t *p = &msg[3];

    LL_ARG_CHECK((network_type == LL_LORAWAN_PUBLIC) || (network_type == LL_LORAWAN_PRIVATE));
    LL_ARG_CHECK((device_class == LL_LORAWAN_CLASS_A) ||
                 (device_class == LL_LORAWAN_CLASS_B) ||
                 (device_class == LL_LORAWAN_CLASS_C));
    LL_ARG_CHECK(netID <= 0xffffff);
    LL_ARG_CHECK(NULL != netSKey);
    LL_ARG_CHECK(NULL != appSKey);

    msg[0] = LL_LORAWAN_ACTIVATION_PERSONALIZATION;
    msg[1] = (uint8_t) network_type;
    msg[2] = (uint8_t) device_class;
    write_uint32(netID, &p);
    write_uint32(devAddr, &p);
    memcpy(p, netSKey, 16);
    memcpy(p + 16, appSKey, 16);

    return lorawan_activate(msg, sizeof(msg));
}

int32_t ll_lorawan_param_get_i32(enum ll_lorawan_param_e param, int32_t * value)
{
    uint8_t msg[1 + 1 + 1];
    uint8_t rsp[1 + 1 + 1 + 4];
    uint8_t const * p = rsp + 3;

    LL_ARG_CHECK((param >= 0) && (param < 0xff));
    LL_ARG_CHECK(NULL != value);

    msg[0] = 3;
    msg[1] = param;
    msg[2] = 0;

    int32_t rw_response = hal_read_write(OP_LORAWAN_PARAM, msg, sizeof(msg), rsp, sizeof(rsp));
    if (rw_response < 0)
    {
        return((int8_t) rw_response);
    }
    if (rw_response != sizeof(rsp))
    {
        return LL_IFC_ERROR_INCORRECT_MESSAGE_SIZE;
    }
    *value = (int32_t) read_uint32(&p);
    return 0;
}

int32_t ll_lorawan_param_set_i32(enum ll_lorawan_param_e param, int32_t value)
{
    uint8_t msg[1 + 1 + 1 + 4];
    uint8_t rsp[1 + 1 + 1 + 4];
    uint8_t * p_msg = msg + 3;

    LL_ARG_CHECK((param >= 0) && (param < 0xff));
    msg[0] = 3;
    msg[1] = param;
    msg[2] = 4;
    write_uint32(value, &p_msg);

    int32_t rw_response = hal_read_write(OP_LORAWAN_PARAM, msg, sizeof(msg), rsp, sizeof(rsp));
    if (rw_response < 0)
    {
        return((int8_t) rw_response);
    }
    if (rw_response != sizeof(rsp))
    {
        return LL_IFC_ERROR_INCORRECT_MESSAGE_SIZE;
    }

    return 0;
}

static int32_t ll_lorawan_send_internal(
        uint8_t flags,
        uint8_t fPort,
        uint8_t const * buffer,
        uint8_t buffer_length,
        uint8_t retries)
{
    uint8_t msg[1 + 1 + 1 + 1 + 256];
    uint8_t rsp[] = {0};

    LL_ARG_CHECK(fPort > 0);
    LL_ARG_CHECK(NULL != buffer);
    LL_ARG_CHECK(buffer_length > 0);

    msg[0] = flags;
    msg[1] = fPort;
    msg[2] = retries;
    msg[3] = buffer_length;
    memcpy(msg + 4, buffer, buffer_length);

    int32_t rw_response = hal_read_write(OP_LORAWAN_MSG_SEND, msg, 4 + buffer_length, rsp, sizeof(rsp));
    if (rw_response < 0)
    {
        return rw_response;
    }
    return 0;
}

int32_t ll_lorawan_send_unconfirmed(
        uint8_t flags,
        uint8_t fPort,
        uint8_t const * buffer,
        uint8_t buffer_length)
{
    return ll_lorawan_send_internal(flags & ~LL_LORAWAN_SEND_CONFIRMED, fPort, buffer, buffer_length, 0);
}

int32_t ll_lorawan_send_confirmed(
        uint8_t flags,
        uint8_t fPort,
        uint8_t const * buffer,
        uint8_t buffer_length,
        uint8_t retries)
{
    return ll_lorawan_send_internal(flags | LL_LORAWAN_SEND_CONFIRMED, fPort, buffer, buffer_length, retries);
}

int32_t ll_lorawan_receive(uint8_t * buf, uint16_t len, ll_lorawan_rx_t * rx)
{
    uint8_t msg[256];

    LL_ARG_CHECK(NULL != buf);
    LL_ARG_CHECK(len > 0);
    LL_ARG_CHECK(NULL != rx);

    int32_t rw_response = hal_read_write(OP_LORAWAN_MSG_RECEIVE, 0, 0, msg, sizeof(msg));
    if (rw_response < 0)
    {
        return((int8_t) rw_response);
    }

    if (rw_response < 8)
    {
        return LL_IFC_ERROR_INCORRECT_MESSAGE_SIZE;
    }

    rx->flags = msg[0];
    rx->TxNbRetries = msg[1];
    rx->DemodMargin = msg[2];
    rx->NbGateways = msg[3];
    rx->RxRssi = LL_LORAWAN_RSSI_FROM_PKT(msg[4]);
    rx->RxSnr = msg[5];
    rx->RxPort = msg[6];
    rx->bytes_received = msg[7];

    if (rw_response != (8 + rx->bytes_received))
    {
        return LL_IFC_ERROR_INCORRECT_MESSAGE_SIZE;
    }

    if (rx->bytes_received > len)
    {
        return LL_IFC_ERROR_BUFFER_TOO_SMALL;
    }

    memcpy(buf, &msg[8], rx->bytes_received);

    return 0;
}
