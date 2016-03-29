#include <string.h>
#include "ll_ifc_ensemble.h"
#include "ll_ifc_private.h"

            // this table is a lookup table for property size(s) for a give ensemble property ID
            // as new properties are defined this table must also be updated to contain the size-mapping
            // NOTE: the ORDER MUST MATCH THE ENUM ORDER

            // ZZ TODO this table is ending up in Host IFC and and ensemble code, which is BAD, need to reconcile later
static  const uint16_t   propertySizeTable[ENSEMBLE_PROP_MAX] = 
    {
        sizeof(uint32_t),   // Frequency
        sizeof(uint32_t),   // Bandwidth
        sizeof(uint32_t),   // Spreading factor
        sizeof(uint32_t),   // Coding Rate
        sizeof(uint32_t),   // Enumerated Role for module
        sizeof(uint32_t),   // Module Tx power (dBm)
        sizeof(uint32_t),   // Message preamble length (symbols)
        sizeof(uint32_t),   // Max Tx attempts per msg
        sizeof(uint32_t),   // Max app payload size / msg
        sizeof(uint32_t),   // max msgs buffered in a gateway
        sizeof(uint32_t),   // Delay between Tx attempts on error (sec)
        sizeof(uint32_t),   // Notify method when Gateway has msgs available
        sizeof(uint32_t),   // Mail ack transmission delay (waitig for mail from connected host)
        sizeof(uint32_t),   // Repeater available flag
        sizeof(uint8_t) * ENSEMBLE_AES_KEY_LENGTH, // AES encryption key (AES128) - largest property so far
                            // additional properties to be added here ......
     };

int32_t ll_ensemble_property_size( ensemble_property_t property_id )
{
    if( property_id >= ENSEMBLE_PROP_MAX )
    {
        return 0;
    }
    return propertySizeTable[property_id];
}

int32_t ll_set_ensemble_config_property(ensemble_property_t propertyId, void* propertyValue)
{
    uint8_t buffer[MAX_ENSEMBLE_PROPERTY_LENGTH + 1];
    // validate input arguments
    if( propertyId >= ENSEMBLE_PROP_MAX )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    if( propertyValue == NULL )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;

    // get property size
    int size = ll_ensemble_property_size(propertyId);

    // put everthing in the buffer in network order
    // since all we have now for types is a byte string (already in the proper order) or uint32_t we'll simply
    // handle those specific cases until we have more data types to deal with more generically

    // remember that inital buffer slot contains the property ID being updated
    buffer[0] = (uint8_t)propertyId;

    if( size == sizeof(uint32_t) )
    {
        buffer[1] = (*((uint32_t *)propertyValue) >> 24) & 0xFF;
        buffer[2] = (*((uint32_t *)propertyValue) >> 16) & 0xFF;
        buffer[3] = (*((uint32_t *)propertyValue) >> 8) & 0xFF;
        buffer[4] = (*((uint32_t *)propertyValue) & 0xFF);
    }
    else
    {
        memcpy(&buffer[1], propertyValue, size);
    }
    // and send it cmd for property to be set
    return hal_read_write(OP_UMODE_PROP_SET_REQ, buffer, size + 1, NULL, 0);
}

int32_t ll_get_ensemble_config_property(ensemble_property_t propertyId, void* propertyValue)
{
    uint8_t buffer[MAX_ENSEMBLE_PROPERTY_LENGTH + 1];
    uint8_t localPropId = (uint8_t)propertyId;

    // validate input arguments         - note key is not to be readable via this API
    if( (propertyId >= ENSEMBLE_PROP_MAX) || (propertyId == ENSEMBLE_PROP_MSG_TRAFFIC_KEY) )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    if( propertyValue == NULL )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;

    // get property size
    int size = ll_ensemble_property_size(propertyId);

    // what we are doing is sending across command specifying the property to get
    // and what is returned is the property + 1 initial ID byte indicating what its ID is (should be same)
	int32_t ret = hal_read_write(OP_UMODE_PROP_GET_REQ, &localPropId, 1, buffer, size + 1);

    // if it worked we do the same network order swapping or just copying based on the properties type
    // which for now is inferred based on size (same as set code does)
	if( ret >= 0 )
	{
        if( size == sizeof(uint32_t) )
        {
            // property ID is also encoded in the response, we just skip that
            *((uint32_t *)propertyValue) = 0;
            *((uint32_t *)propertyValue) |= (uint32_t)buffer[1] << 24;
            *((uint32_t *)propertyValue) |= (uint32_t)buffer[2] << 16;
            *((uint32_t *)propertyValue) |= (uint32_t)buffer[3] << 8;
            *((uint32_t *)propertyValue) |= (uint32_t)buffer[4];
        }
        else
        {
            memcpy(propertyValue , &buffer[1], size);
        }
        return size;        // return property size unless an error is returned in 1st place
	}
	else
    {
        return ret;
    }
}

int32_t ll_get_ensemble_stored_msg_count(uint32_t* message_count)
{
    uint8_t buffer[sizeof(uint32_t)];
                                // range check things
    if( message_count == NULL )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;

    // request the count
	int32_t ret = hal_read_write(OP_UMODE_GET_MSG_CNT_REQ, NULL, 0, buffer, sizeof(uint32_t));

    // if it worked swap data out of network order
	if( ret >= 0 )
	{
        *message_count = 0;
        *message_count |= (uint32_t)buffer[0] << 24;
        *message_count |= (uint32_t)buffer[1] << 16;
        *message_count |= (uint32_t)buffer[2] << 8;
        *message_count |= (uint32_t)buffer[3];
    }
	return ret;
}

int32_t ll_get_ensemble_get_stored_msg(ensemble_msg_descriptor_t* msg_descriptor, uint8_t* buffer, uint32_t bufferSize)
{
    uint8_t local_buffer[MAX_ENSEMBLE_TRANSFER_SIZE + 1];
    uint8_t swap;
    uint8_t i;

    if( msg_descriptor == NULL || buffer == NULL )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;

	int32_t ret = hal_read_write(OP_UMODE_GET_NEXT_MSG_REQ, NULL, 0, local_buffer, MAX_ENSEMBLE_TRANSFER_SIZE);

    // range check the results (must be bigger than the descriptor)
    if( ret < 0 )
    {
        return ret;     // error came from module
    }
    else if( ret <= (int32_t)ENSEMBLE_SERIALIZED_OFFSET_TO_PAYLOAD )
    {
                        // data returned was not sufficiently large enough to represent a true payload
        return -LL_IFC_NACK_NODATA;
    }

    // xfer size represents size of payload (including the fields contained in the message descriptor buffer)
    if( ret > 0 )
    {
        // locate and copy payload, trim copy size if buffer too small
        uint32_t payload_size = ret - (int32_t)ENSEMBLE_SERIALIZED_OFFSET_TO_PAYLOAD;
        if( bufferSize < payload_size )
        {
            payload_size = bufferSize;
        }
        memcpy( buffer, &local_buffer[ENSEMBLE_SERIALIZED_OFFSET_TO_PAYLOAD], payload_size);

        // swap the descriptor fields out one at a time
                        // 1st field is the msg info field - place in network order
        swap = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MSGINFO];
        local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MSGINFO] = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MSGINFO + 1];
        local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MSGINFO + 1] = swap;
        msg_descriptor->msginfo = *((int16_t*)(&local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MSGINFO]));
                        // 2nd field is the rssi field - place in network order
        swap = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_RSSI];
        local_buffer[ENSEMBLE_SERIALIZED_OFFSET_RSSI] = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_RSSI + 1];
        local_buffer[ENSEMBLE_SERIALIZED_OFFSET_RSSI + 1] = swap;
        msg_descriptor->rssi = *((int16_t*)(&local_buffer[ENSEMBLE_SERIALIZED_OFFSET_RSSI]));
                        // 3rd field is the time field which must be placed in network order
        for( i = 0; i < sizeof(uint64_t) / 2; i++ )
        {
            swap = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_UTCTIME + (7 - i)];
            local_buffer[ENSEMBLE_SERIALIZED_OFFSET_UTCTIME + (7 - i)] = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_UTCTIME + i];
            local_buffer[ENSEMBLE_SERIALIZED_OFFSET_UTCTIME + i] =  swap;
        }
        msg_descriptor->utc_time = *((uint64_t*)(&local_buffer[ENSEMBLE_SERIALIZED_OFFSET_UTCTIME]));
                        // 4th field is the mac address (uint64_t) which must be placed in network order
        for( i = 0; i < sizeof(uint64_t) / 2; i++ )
        {
            swap = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MUI + (7 - i)];
            local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MUI + (7 - i)] = local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MUI + i];
            local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MUI + i] =  swap;
        }
        msg_descriptor->mui = *((uint64_t*)(&local_buffer[ENSEMBLE_SERIALIZED_OFFSET_MUI]));
        // note that the response can be smaller than the max size buffer we are specifying here...
        return payload_size;
    }
    else
    {
        return ret;
    }
}

int32_t ll_get_ensemble_get_mail_msg(uint8_t* buffer, uint32_t bufferSize)
{
    uint8_t local_buffer[MAX_ENSEMBLE_TRANSFER_SIZE + 1];
    uint8_t adjusted_size;

    if( buffer == NULL )
    {
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    }
	int32_t ret = hal_read_write(OP_GET_MAIL_FROM_GW, NULL, 0, local_buffer, MAX_ENSEMBLE_TRANSFER_SIZE);
    // xfer size represents size of mail msg
    if( ret > 0 )
    {
        if( bufferSize < (uint32_t)ret )
        {
            adjusted_size = bufferSize;
        }
        else
        {
            adjusted_size = ret;
        }
        memcpy( buffer, local_buffer, adjusted_size);
        return adjusted_size;
    }
    else
    {
        return ret;
    }
}

int32_t ll_set_utc_time(uint64_t* utc_time)
{
    uint8_t buffer[sizeof(uint64_t)];       // time storage buffer
    uint8_t* ptr = (uint8_t*)utc_time;      // byte-wide pointer used later

    if( utc_time == NULL || *utc_time == 0 )
    {
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    }
                                            // we copy in...in network order
    buffer[0] = ptr[7];                     // brute force, but it works
    buffer[1] = ptr[6];
    buffer[2] = ptr[5];
    buffer[3] = ptr[4];
    buffer[4] = ptr[3];
    buffer[5] = ptr[2];
    buffer[6] = ptr[1];
    buffer[7] = ptr[0];
                                            // and send it across
    return hal_read_write(OP_UMODE_SET_TIME_REQ, buffer, sizeof(uint64_t), NULL, 0);
}

int32_t ll_get_utc_time(uint64_t* utc_time)
{
    uint8_t buffer[sizeof(uint64_t)];       // returned time storage buffer
    uint8_t* ptr = (uint8_t*)utc_time;      // byte-wide pointer used later

    if( utc_time == NULL )
    {
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    }

    // request the time
	int32_t ret = hal_read_write(OP_UMODE_GET_TIME_REQ, NULL, 0, buffer, sizeof(uint64_t));

    // if it worked swap data out of network order
	if( ret >= 0 )
	{
                                            // we copy back...converting back from network order
        ptr[0] = buffer[7];                 // brute force, but it works
        ptr[1] = buffer[6];
        ptr[2] = buffer[5];
        ptr[3] = buffer[4];
        ptr[4] = buffer[3];
        ptr[5] = buffer[2];
        ptr[6] = buffer[1];
        ptr[7] = buffer[0];
    }
	return ret;
}

int32_t ll_send_payload_to_gw(uint8_t* payload, uint8_t payload_size)
{
    uint8_t buffer[MAX_ENSEMBLE_TRANSFER_SIZE + 1];      // send payload + 1 byte size (preceding the payload)

    if( payload == NULL )
    {
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    }
    buffer[0] = payload_size;                           // build up the msg to send to ensemble MAC (size)
    memcpy( &buffer[1], payload, payload_size); // then the payload
    // and send cmd for message to be enqueued
    return hal_read_write(OP_SEND_MSG_TO_GW, buffer, payload_size + 1, NULL, 0);
}

int32_t ll_send_mail_to_ep(uint64_t* ep_mac_address, uint8_t* payload, uint8_t payload_size)
{
    uint8_t buffer[MAX_ENSEMBLE_TRANSFER_SIZE + 1 + sizeof(uint64_t)];      // send MAC + payload + 1 byte size (preceding the MAC and payload)
    uint8_t swap;
    uint8_t i;

    if( payload == NULL || ep_mac_address == NULL)
    {
        return LL_IFC_ERROR_INCORRECT_PARAMETER;
    }
    buffer[0] = payload_size;                                         // build up the msg to send to GW (payload size of zero is legit)
    memcpy( &buffer[1], ep_mac_address, sizeof(uint64_t));            // then the MAC
    if( payload_size > 0 )
    {
        memcpy( &buffer[1 + sizeof(uint64_t)], payload, payload_size);    // then the payload
    }
                                                                      // swap the MAC into network order (in bytes 1 - 8)
    for( i = 0; i < sizeof(uint64_t) / 2; i++ )
    {
        swap = buffer[8 - i];
        buffer[8 - i] = buffer[i + 1];
        buffer[i + 1] =  swap;
    }
    // and send cmd for message to be enqueued
    return hal_read_write(OP_SEND_MAIL_TO_EP, buffer, payload_size + sizeof(uint64_t) + 1, NULL, 0);
}


int32_t ll_get_ensemble_get_debuginfo(uint8_t* buffer, uint32_t bufferSize)
{
    uint8_t local_buffer[ENSEMBLE_DBG_BUFFER_SIZE + 1];

    if( buffer == NULL )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;

	int32_t ret = hal_read_write(OP_DEBUG_DUMP, NULL, 0, local_buffer, ENSEMBLE_DBG_BUFFER_SIZE);
    // not the most efficient (double copy, but lets the checking for length be simpler on module side)

    // adjust xfer size if buffer is too small on host side
    if( ret > 0 )
    {
        uint32_t adjusted_size = ENSEMBLE_DBG_BUFFER_SIZE;
        if( bufferSize < adjusted_size )
        {
            adjusted_size = bufferSize;
        }
        memcpy( buffer, &local_buffer[0], adjusted_size);
        // note that the response can be smaller than the max size buffer we are specifying here...
        return adjusted_size;
    }
    else
    {
        return ret;
    }
}

int32_t ll_get_ensemble_lost_msg_count(uint32_t* missed_message_count, uint8_t reset)
{
    uint8_t buffer[sizeof(uint32_t)];       // missed msg count
    uint8_t    resetFlag = reset;              // local copy of reset command flag

                                // range check things
    if( missed_message_count == NULL )
        return LL_IFC_ERROR_INCORRECT_PARAMETER;

    // request the lost count (reset flag is passed in)
    // what we get back is the real count
	int32_t ret = hal_read_write(OP_UMODE_LOST_MSG_REQ, &resetFlag, sizeof resetFlag, buffer, sizeof(uint32_t));

    // if it worked swap data out of network order
	if( ret >= 0 )
	{
        *missed_message_count = 0;
        *missed_message_count |= (uint32_t)buffer[0] << 24;
        *missed_message_count |= (uint32_t)buffer[1] << 16;
        *missed_message_count |= (uint32_t)buffer[2] << 8;
        *missed_message_count |= (uint32_t)buffer[3];
    }
	return ret;
}
