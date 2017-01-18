 /* 
  * The library is not extensively tested and only 
  * meant as a simple explanation and for inspiration. 
  * NO WARRANTY of ANY KIND is provided. 
  */
#if (defined(MPU60x0) || defined(MPU9150) || defined(MPU9255))
#include <stdbool.h>
#include <stdint.h>
#include "nrf_drv_twi.h"
#include "mpu_register_map.h"
#include "mpu.h"

static nrf_drv_twi_t const * m_twi_instance;
volatile static bool twi_tx_done = false;
volatile static bool twi_rx_done = false;


void mpu_twi_event_handler(const nrf_drv_twi_evt_t *evt)
{ 
    switch(evt->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
						switch(evt->xfer_desc.type)
            {
                case NRF_DRV_TWI_XFER_TX:
                    twi_tx_done = true;
                    break;
                case NRF_DRV_TWI_XFER_TXTX:
                    twi_tx_done = true;
                    break;
                case NRF_DRV_TWI_XFER_RX:
                    twi_rx_done = true;
                    break;
                case NRF_DRV_TWI_XFER_TXRX:
                    twi_rx_done = true;
                    break;
                default:
                    break;
            }
            break;
        case NRF_DRV_TWI_EVT_ADDRESS_NACK:
            break;
        case NRF_DRV_TWI_EVT_DATA_NACK:
            break;
        default:
            break;
    }      
}

// The new SDK 11 TWI driver is not able to do two transmits without repeating the ADDRESS + Write bit byte
// Hence we need to merge the MPU register address with the buffer and then transmit all as one transmission  
static void buffer_merger(uint8_t * new_buffer, uint8_t reg, uint8_t * p_data, uint32_t length)
{
    uint8_t *ptr_new_buffer;
    uint8_t *ptr_data_place_holder;
    
    ptr_data_place_holder = p_data;
    ptr_new_buffer = new_buffer;
    *ptr_new_buffer = reg;
    ptr_new_buffer++;
    
    for(int i = 0; i < length; i++)
    {
        *ptr_new_buffer = *ptr_data_place_holder;
        ptr_new_buffer++;
        ptr_data_place_holder++;
    }
}


static uint32_t mpu_write_burst(uint8_t reg, uint8_t * p_data, uint32_t length)
{    
    // This burst write function is not optimal and needs improvement. 
    // The new SDK 11 TWI driver is not able to do two transmits without repeating the ADDRESS + Write bit byte
    uint32_t err_code;
    uint32_t timeout = MPU_TWI_TIMEOUT;
    
    // Merging MPU register address and p_data into one buffer.
    uint8_t buffer[20];
    buffer_merger(buffer, reg, p_data, length);
    
    // Setting up transfer
    nrf_drv_twi_xfer_desc_t xfer_desc;
    xfer_desc.address = MPU_ADDRESS;
    xfer_desc.type = NRF_DRV_TWI_XFER_TX;
    xfer_desc.primary_length = length + 1;
    xfer_desc.p_primary_buf = buffer;
    
    // Transfering
    err_code = nrf_drv_twi_xfer(m_twi_instance, &xfer_desc, 0);
    
    while((!twi_tx_done) && --timeout);  
    if(!timeout) return NRF_ERROR_TIMEOUT;
    twi_tx_done = false;
    
    return err_code;
}

uint32_t mpu_write_register(uint8_t reg, uint8_t data)
{
    uint32_t err_code;
    uint32_t timeout = MPU_TWI_TIMEOUT;
    
    uint8_t packet[2] = {reg, data};
    
    err_code = nrf_drv_twi_tx(m_twi_instance, MPU_ADDRESS, packet, 2, false);
		NRF_LOG_PRINTF("(mpu.c)ERRCODE nrf_drv_twi_tx: %d \r\n", err_code);
    if(err_code != NRF_SUCCESS) return err_code;
    while((!twi_tx_done) && --timeout);
    if(!timeout) return NRF_ERROR_TIMEOUT;
    
    twi_tx_done = false;
    
    return err_code;
}


uint32_t mpu_read_registers(uint8_t reg, uint8_t * p_data, uint32_t length)
{
    uint32_t err_code;
    uint32_t timeout = MPU_TWI_TIMEOUT;
    
    err_code = nrf_drv_twi_tx(m_twi_instance, MPU_ADDRESS, &reg, 1, false);
    if(err_code != NRF_SUCCESS) return err_code;
    
    while((!twi_tx_done) && --timeout);  
    if(!timeout) return NRF_ERROR_TIMEOUT;
    twi_tx_done = false;
    
    err_code = nrf_drv_twi_rx(m_twi_instance, MPU_ADDRESS, p_data, length);
    if(err_code != NRF_SUCCESS) return err_code;
    
    timeout = MPU_TWI_TIMEOUT;
    while((!twi_rx_done) && --timeout);
    if(!timeout) return NRF_ERROR_TIMEOUT;
    twi_rx_done = false;
    
    return err_code;
}


uint32_t mpu_config(mpu_config_t * config)
{
    uint8_t *data; 
    data = (uint8_t*)config;
    return mpu_write_burst(MPU_REG_SMPLRT_DIV, data, 4);
}

uint32_t mpu_int_cfg_pin(mpu_int_pin_cfg_t *cfg)
{
    uint8_t *data; 
    data = (uint8_t*)cfg;
    return mpu_write_register(MPU_REG_INT_PIN_CFG, *data);
}

uint32_t mpu_int_enable(mpu_int_enable_t *cfg)
{
    uint8_t *data; 
    data = (uint8_t*)cfg;
    return mpu_write_register(MPU_REG_INT_ENABLE, *data);
}
    
    

uint32_t mpu_init(nrf_drv_twi_t const * const p_instance)
{
    uint32_t err_code;
    m_twi_instance = p_instance;
    
    uint8_t reset_value = 7; // Resets gyro, accelerometer and temperature sensor signal paths.
    err_code = mpu_write_register(MPU_REG_SIGNAL_PATH_RESET, reset_value);
		NRF_LOG_PRINTF("(mpu.c)ERRCODE: mpu_write_register(1): %d \r\n", err_code);
    if(err_code != NRF_SUCCESS) return err_code;
    
    // Chose  PLL with X axis gyroscope reference as clock source
    err_code = mpu_write_register(MPU_REG_PWR_MGMT_1, 1);
		NRF_LOG_PRINTF("(mpu.c)ERRCODE: mpu_write_register(2): %d \r\n", err_code);
    if(err_code != NRF_SUCCESS) return err_code;
    
    return NRF_SUCCESS;
}


uint32_t mpu_read_accel(accel_values_t * accel_values)
{
    uint32_t err_code;
    uint8_t raw_values[6];
    err_code = mpu_read_registers(MPU_REG_ACCEL_XOUT_H, raw_values, 6);
    if(err_code != NRF_SUCCESS) return err_code;
    
    // Reorganize read sensor values and put them into value struct
    //uint8_t *data;
    //data = (uint8_t*)accel_values;
    uint8_t *data;
		data = (uint8_t*)accel_values;
		//for(uint8_t i = 0; i<6; i++)
    for(uint8_t i=0; i<6; i++)
		{
        *data = raw_values[5-i];
        data++;
    }
    return NRF_SUCCESS;
}


uint32_t mpu_read_gyro(gyro_values_t * gyro_values)
{
    uint32_t err_code;
    uint8_t raw_values[6];
    err_code = mpu_read_registers(MPU_REG_GYRO_XOUT_H, raw_values, 6);
    if(err_code != NRF_SUCCESS) return err_code;
    
    // Reorganize read sensor values and put them into value struct
    uint8_t *data;
    data = (uint8_t*)gyro_values;
    for(uint8_t i = 0; i<6; i++)
    {
        *data = raw_values[5-i];
        data++;
    }
    return NRF_SUCCESS;
}
/**@brief This really may not work:*/ 
uint32_t mpu_read_magn(magn_values_t * magn_values)
{
    uint32_t err_code;
    uint8_t raw_values[6];
    err_code = mpu_read_registers(MPU_REG_GYRO_XOUT_H, raw_values, 6);
		if(err_code != NRF_SUCCESS) return err_code;
    
    // Reverse byte order
    uint8_t *data;
    data = (uint8_t*)magn_values;
    for(uint8_t i = 0; i<6; i++)
    {
        *data = raw_values[i];
        data++;
    }
    return err_code;
}

uint32_t mpu_read_temp(temp_value_t * temperature)
{
    uint32_t err_code;
    uint8_t raw_values[2];
    err_code = mpu_read_registers(MPU_REG_TEMP_OUT_H, raw_values, 2);
    if(err_code != NRF_SUCCESS) return err_code;  
    
    *temperature = (temp_value_t)(raw_values[0] << 8) + raw_values[1];
    
    return NRF_SUCCESS;    
}

uint32_t mpu_read_int_source(uint8_t * int_source)
{
    return mpu_read_registers(MPU_REG_INT_STATUS, int_source, 1);
}

// Function does not work on MPU60x0 and MPU9255
#if defined(MPU9150)
uint32_t mpu_config_ff_detection(uint16_t mg, uint8_t duration)
{
    uint32_t err_code;
    uint8_t threshold = (uint8_t)(mg/MPU_MG_PR_LSB_FF_THR);
    if(threshold > 255) return MPU_BAD_PARAMETER;
    
    err_code = mpu_write_register(MPU_REG_FF_THR, threshold);
    if(err_code != NRF_SUCCESS) return err_code;
    
    return mpu_write_register(MPU_REG_FF_DUR, duration);
}
#endif
#endif /**@(defined(MPU60x0) || defined(MPU9150) || defined(MPU9255))*/
/**
  @}
*/
