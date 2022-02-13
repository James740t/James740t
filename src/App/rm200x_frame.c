#include "app/rm200x_frame.h"

/******************************************************************************************/
// RM200x FRAME
/******************************************************************************************/

//FREERTOS CONTROL ITEMS

//Task Tags
const char *FRAME_TASK_TAG = "FRAME_TASK";

//Local prototypes

/******************************************************************************************/
// RM200x FRAME - Definitions
/******************************************************************************************/

/*****************************************************************************/
// RM200x FRAME BUILDING
/*****************************************************************************/
uint8_t RollingCounter(void)
{
    static uint8_t counter;
    if (counter == UINT8_MAX)
    {
        counter = 0;
        return counter;
    }
    else
    {
        counter++;
        return counter;
    }
}

uint8_t Calculate_Checksum(const uint8_t *pt_frame_array)
{
    int _crc = 0x100;

    for (int i = 0; i < pt_frame_array[2]; i++)
    {
        _crc -= (int8_t)pt_frame_array[i + 3];
    }

    return (uint8_t)_crc;
}

uint8_t Get_Frame_Length (const uint8_t *pt_frame_array)
{
    uint8_t length_in = pt_frame_array[2];
    return length_in + 5;
}

uint8_t Get_Frame_Intent (const uint8_t *pt_frame_array)
{
    return pt_frame_array[4];
}

uint8_t Get_Frame_Payload (const uint8_t *pt_frame_array)
{
    return pt_frame_array[2];
}

uint8_t Get_Frame_CRC (const uint8_t *pt_frame_array)
{
    return pt_frame_array[2 + pt_frame_array[2] + 1];
}

/*
bit 0 == bad frame start
bit 1 == length error
bit 2 == bad crc
bit 3 == unknown intent
*/
uint8_t CheckFrame(const uint8_t *pt_frame_array)
{
    byte error;
    error.byte = 0x00;
    // Frame start is good ?
    if (pt_frame_array[0] != 0xFF) error.bits.bit0 = 1;
    if (pt_frame_array[1] != 0x55) error.bits.bit0 = 1;

    //Length and CRC are good
    // FF 55 --- LL --- xx yy zz --- crc 
    uint8_t length_in = pt_frame_array[2];
    if (length_in < 3) error.bits.bit1 = 1;

    uint8_t crc_idx = 2 + length_in + 1;
    uint8_t crc_in = pt_frame_array[crc_idx];
    uint8_t crc_calc = Calculate_Checksum(pt_frame_array);

    ESP_LOGI(FRAME_TASK_TAG, "Pay. Len: 0x%02X, Msg. Len:: 0x%02X", length_in, crc_idx + 1);
    ESP_LOGI(FRAME_TASK_TAG, "CRC in: 0x%02X, CRC calc: 0x%02X", crc_in, crc_calc);

    if (crc_in != crc_calc) error.bits.bit2 = 1;

    return error.byte;
}

int CreateFrame(uint8_t *pt_frame, const uint8_t pt_intent, const uint8_t data_size, const uint8_t *pt_data)
{    
    uint8_t index = 0;
    
    pt_frame[index++] = 0xFF;
    pt_frame[index++] = 0x55;
    pt_frame[index++] = data_size;
    pt_frame[index++] = RollingCounter();
    pt_frame[index++] = pt_intent;
    //Add data here
    memcpy(&pt_frame[index], pt_data, data_size);
    //Add the checksum finally
    index += data_size;
    uint8_t crc = Calculate_Checksum(pt_frame);
    pt_frame[index++] = crc;

    // Check the frame
    byte check;
    check.byte = CheckFrame(pt_frame);
    if (check.byte != 0) return -1;

    ESP_LOG_BUFFER_HEXDUMP(FRAME_TASK_TAG, pt_frame, index, ESP_LOG_INFO);

    return index;
   //Build OK returns the whole frame length else returns -1 == NOK 
}
/*****************************************************************************/
// RM200x FRAME BUILDING  -- END --
/*****************************************************************************/



/******************************************************************************************/
// RM200x FRAME -- END --
/******************************************************************************************/