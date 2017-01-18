 /* 
  * The library is not extensively tested and only 
  * meant as a simple explanation and for inspiration. 
  * NO WARRANTY of ANY KIND is provided. 
  */
#if (defined(MPU60x0) || defined(MPU9150) || defined(MPU9255))
#ifndef MPU_H
#define MPU_H


#include <stdbool.h>
#include <stdint.h>
#include "nrf_drv_twi.h"
#include "nrf_log.h"

#if defined(MPU60x0)
    #include "mpu60x0_register_map.h"
#elif defined(MPU9150)
    #include "mpu9150_register_map.h"
#elif defined(MPU9255)
    #include "mpu9255_register_map.h"
#else 
    #error "No MPU defined. Please define MPU in Target Options C/C++ Defines"
#endif

#define MPU_TWI_TIMEOUT 5000 
#define MPU_ADDRESS     0x68 

#define MPU_MPU_BASE_NUM    0x4000
#define MPU_SUCCESS             (MPU_MPU_BASE_NUM + 0)
#define MPU_BAD_PARAMETER       (MPU_MPU_BASE_NUM + 1)

#if defined(MPU9150)
    #define MPU_MG_PR_LSB_FF_THR    32
#elif defined(MPU9255)
    #define MPU_MG_PR_LSB_FF_THR    4
#elif defined(MPU60x0)
    #define MPU_MG_PR_LSB_FF_THR    1
#endif

/**@brief Enum defining Accelerometer's Full Scale range posibillities in Gs. */
enum accel_range {
  AFS_2G = 0,       // 2 G
  AFS_4G,           // 4 G
  AFS_8G,           // 8 G
  AFS_16G           // 16 G
};

/**@brief Enum defining Gyroscopes� Full Scale range posibillities in Degrees Pr Second. */
enum gyro_range {
  GFS_250DPS = 0,   // 250 deg/s
  GFS_500DPS,       // 500 deg/s
  GFS_1000DPS,      // 1000 deg/s
  GFS_2000DPS       // 2000 deg/s
};

typedef struct {
		int16_t az;
    int16_t ay;
    int16_t ax;
		int16_t gz;
    int16_t gy;
    int16_t gx;
} combined_values_t;

/**@brief Structure to hold acceleromter values. 
 * Sequence of z, y, and x is important to correspond with 
 * the sequence of which z, y, and x data are read from the sensor.
 * All values are unsigned 16 bit integers
*/
typedef struct
{
    int16_t z;
    int16_t y;
    int16_t x;
}accel_values_t;


/**@brief Structure to hold gyroscope values. 
 * Sequence of z, y, and x is important to correspond with 
 * the sequence of which z, y, and x data are read from the sensor.
 * All values are unsigned 16 bit integers
*/
typedef struct
{
    int16_t z;
    int16_t y;
    int16_t x;
}gyro_values_t;
//struct holding magnetometer values

typedef struct
{
    int16_t z;
    int16_t y;
    int16_t x;
}magn_values_t;

/**@brief Simple typedef to hold temperature values */
typedef int16_t temp_value_t;

/**@brief MPU driver digital low pass fileter and external Frame Synchronization (FSYNC) pin sampling configuration structure */
typedef struct
{
    uint8_t dlpf_cfg     :3; // 3-bit unsigned value. Configures the Digital Low Pass Filter setting.
    uint8_t ext_sync_set :3; // 3-bit unsigned value. Configures the external Frame Synchronization (FSYNC) pin sampling.
#if defined(MPU9255)
    uint8_t fifo_mode    :1; // When set to ‘1’, when the fifo is full, additional writes will not be written to fifo. When set to ‘0’, when the fifo is full, additional writes will be written to the fifo, replacing the oldest data.
    uint8_t              :1;
#else
    uint8_t              :2;
#endif
}sync_dlpf_config_t;

/**@brief MPU driver gyro configuration structure. */
typedef struct
{
#if defined(MPU9255)
    uint8_t f_choice        :2;
    uint8_t                 :1;
#else
    uint8_t                 :3;
#endif
    uint8_t fs_sel          :2; // FS_SEL 2-bit unsigned value. Selects the full scale range of gyroscopes.
    
#if defined(MPU9255)
    uint8_t gz_st           :1;
    uint8_t gy_st           :1;
    uint8_t gx_st           :1;
#else
    uint8_t                 :3;
#endif
}gyro_config_t;

/**@brief MPU driver accelerometer configuration structure. */
typedef struct
{
#if defined(MPU9255) || defined(MPU60x0)
    uint8_t                 :3;
#else
    uint8_t accel_hpf       :3; // 3-bit unsigned value. Selects the Digital High Pass Filter configuration.
#endif
    uint8_t afs_sel         :2; // 2-bit unsigned value. Selects the full scale range of accelerometers.
    uint8_t za_st           :1; // When set to 1, the Z- Axis accelerometer performs self test.
    uint8_t ya_st           :1; // When set to 1, the Y- Axis accelerometer performs self test.
    uint8_t xa_st           :1; // When set to 1, the X- Axis accelerometer performs self test.
}accel_config_t;

#if defined(MPU9255)
/**@brief MPU9255 driver accelerometer second configuration structure. */
typedef struct
{
    uint8_t a_dlpf_cfg      :2; // 3-bit unsigned value. Selects the Digital High Pass Filter configuration.
    uint8_t accel_f_choice_b :2; // 2-bit unsigned value. Selects the full scale range of accelerometers.
    uint8_t                 :4;
}accel_config_2_t;

#define MPU_DEFAULT_2_CONFIG()                          \
    {                                                     \
        .a_dlpf_cfg             = 0,              \
        .accel_f_choice_b       = 0,              \
    }
#endif

/**@brief MPU driver general configuration structure. */
typedef struct
{
    uint8_t             smplrt_div;         // Divider from the gyroscope output rate used to generate the Sample Rate for the MPU-9150. Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
    sync_dlpf_config_t  sync_dlpf_gonfig;   // Digital low pass fileter and external Frame Synchronization (FSYNC) configuration structure
    gyro_config_t       gyro_config;        // Gyro configuration structure
    accel_config_t      accel_config;       // Accelerometer configuration structure
}mpu_config_t;

#if defined(MPU9255) 
#define MPU_DEFAULT_CONFIG()                          \
    {                                                     \
        .smplrt_div                     = 7,              \
        .sync_dlpf_gonfig.dlpf_cfg      = 1,              \
        .sync_dlpf_gonfig.ext_sync_set  = 0,              \
        .gyro_config.fs_sel             = GFS_2000DPS,    \
        .gyro_config.f_choice           = 0,              \
        .gyro_config.gz_st              = 0,              \
        .gyro_config.gy_st              = 0,              \
        .gyro_config.gx_st              = 0,              \
        .accel_config.afs_sel           = AFS_16G,        \
        .accel_config.za_st             = 0,              \
        .accel_config.ya_st             = 0,              \
        .accel_config.xa_st             = 0,              \
    }
#elif defined(MPU9150)
/**@brief MPU instance default configuration. */
#define MPU_DEFAULT_CONFIG()                          \
    {                                                     \
        .smplrt_div                     = 7,              \
        .sync_dlpf_gonfig.dlpf_cfg      = 1,              \
        .sync_dlpf_gonfig.ext_sync_set  = 0,              \
        .gyro_config.fs_sel             = GFS_2000DPS,    \
        .accel_config.accel_hpf         = 0,              \
        .accel_config.afs_sel           = AFS_16G,        \
        .accel_config.za_st             = 0,              \
        .accel_config.ya_st             = 0,              \
        .accel_config.xa_st             = 0,              \
    }
#elif defined(MPU60x0)
#define MPU_DEFAULT_CONFIG()                          \
    {                                                     \
        .smplrt_div                     = 7,              \
        .sync_dlpf_gonfig.dlpf_cfg      = 1,              \
        .sync_dlpf_gonfig.ext_sync_set  = 0,              \
        .gyro_config.fs_sel             = GFS_2000DPS,    \
        .accel_config.afs_sel           = AFS_16G,        \
        .accel_config.za_st             = 0,              \
        .accel_config.ya_st             = 0,              \
        .accel_config.xa_st             = 0,              \
    }    
#else
    #error "No/unknown MPU defined"
#endif
 
/**@brief MPU driver interrupt pin configuration structure. */    
typedef struct
{
    uint8_t clkout_en       :1;  // When this bit is equal to 1, a reference clock output is provided at the CLKOUT pin. When this bit is equal to 0, the clock output is disabled. For further information regarding CLKOUT, please refer to the MPU-9150 Product Specification document.
    uint8_t i2c_bypass_en   :1;  // When this bit is equal to 1 and I2C_MST_EN (Register 106 bit[5]) is equal to 0, the host application processor will be able to directly access the auxiliary I2C bus of the MPU-9150. When this bit is equal to 0, the host application processor will not be able to directly access the auxiliary I2C bus of the MPU-9150 regardless of the state of I2C_MST_EN (Register 106 bit[5]).
    uint8_t fsync_int_en    :1;  // When equal to 0, this bit disables the FSYNC pin from causing an interrupt to the host processor. When equal to 1, this bit enables the FSYNC pin to be used as an interrupt to the host processor.
    uint8_t fsync_int_level :1;  // When this bit is equal to 0, the logic level for the FSYNC pin (when used as an interrupt to the host processor) is active high. When this bit is equal to 1, the logic level for the FSYNC pin (when used as an interrupt to the host processor) is active low.
    uint8_t int_rd_clear    :1;  // When this bit is equal to 0, interrupt status bits are cleared only by reading INT_STATUS (Register 58). When this bit is equal to 1, interrupt status bits are cleared on any read operation.
    uint8_t latch_int_en    :1;  // When this bit is equal to 0, the INT pin emits a 50us long pulse. When this bit is equal to 1, the INT pin is held high until the interrupt is cleared.
    uint8_t int_open        :1;  // When this bit is equal to 0, the INT pin is configured as push-pull. When this bit is equal to 1, the INT pin is configured as open drain.
    uint8_t int_level       :1; // When this bit is equal to 0, the logic level for the INT pin is active high. When this bit is equal to 1, the logic level for the INT pin is active low.
}mpu_int_pin_cfg_t;

/**@brief MPU interrupt pin default configuration. */
#define MPU_DEFAULT_INT_PIN_CONFIG()    \
{                                       \
    .clkout_en          = 0,    \
    .i2c_bypass_en      = 0,    \
    .fsync_int_en       = 0,    \
    .fsync_int_level    = 0,    \
    .int_rd_clear       = 1,    \
    .latch_int_en       = 0,    \
    .int_open           = 0,    \
    .int_level          = 0,    \
}

/**@brief MPU driver interrupt source configuration structure. */
typedef struct
{
    uint8_t data_rdy_en     :1; // When set to 1, this bit enables the Data Ready interrupt, which occurs each time a write operation to all of the sensor registers has been completed.
    uint8_t                 :2; // 
    uint8_t i2c_mst_int_en  :1; // When set to 1, this bit enables any of the I2C Master interrupt sources to generate an interrupt
    uint8_t fifo_oflow_en   :1; // When set to 1, this bit enables a FIFO buffer overflow to generate an interrupt.
    uint8_t zmot_en         :1; // When set to 1, this bit enables Zero Motion detection to generate an interrupt.
    uint8_t mot_en          :1; // When set to 1, this bit enables Motion detection to generate an interrupt.
    uint8_t ff_en           :1; // When set to 1, this bit enables Free Fall detection to generate an interrupt.
}mpu_int_enable_t;

/**@brief MPU interrupt sources default configuration. */
#define MPU_DEFAULT_INT_ENABLE_CONFIG() \
{                           \
    .data_rdy_en    = 0,    \
    .i2c_mst_int_en = 0,    \
    .fifo_oflow_en  = 0,    \
    .zmot_en        = 0,    \
    .mot_en         = 0,    \
    .ff_en          = 0,    \
}
 
/**@brief Event handler used to pass TWI events from main application to the MPU library
 *
 * @param[in]   evt             TWI driver event
 * @retval      uint32_t        Error code
 */
void mpu_twi_event_handler(const nrf_drv_twi_evt_t *evt);
  

/**@brief Function for initiating MPU and MPU library
 * 
 * Resets gyro, accelerometer and temperature sensor signal paths.
 * Function resets the analog and digital signal paths of the gyroscope, accelerometer,
 * and temperature sensors.
 * The reset will revert the signal path analog to digital converters and filters to their power up
 * configurations.
 *
 * @param[in]   p_instance      Instance of nRF5x TWI module
 * @retval      uint32_t        Error code
 */
uint32_t mpu_init(nrf_drv_twi_t const * const p_instance);

/**@brief Function for reading an arbitrary register
 *
 * @param[in]   reg             Register to write
 * @param[in]   data            Value
 * @retval      uint32_t        Error code
 */
uint32_t mpu_write_register(uint8_t reg, uint8_t data);

/**@brief Function for reading arbitrary register(s)
 *
 * @param[in]   reg             Register to read
 * @param[in]   p_data          Pointer to place to store value(s)
 * @param[in]   length          Number of registers to read
 * @retval      uint32_t        Error code
 */
uint32_t mpu_read_registers(uint8_t reg, uint8_t * p_data, uint32_t length);
    

/**@brief Function for basic configuring of the MPU
 * 
 * Register 25 � Sample Rate Divider SMPRT_DIV. This register specifies 
 * the divider from the gyroscope output rate used to generate the Sample Rate for the MPU.
 * 
 * Register 26 � Configuration CONFIG. This register configures 
 * the external Frame Synchronization (FSYNC) pin sampling and the Digital Low Pass Filter (DLPF) 
 * setting for both the gyroscopes and accelerometers.
 * 
 * Register 27 � Gyroscope Configuration GYRO_CONFIG. This register is used to configure the gyroscopes� full scale range.
 * 
 * Register 28 � Accelerometer Configuration ACCEL_CONFIG. This register is 
 * used to trigger accelerometer self test and configure the accelerometer full scale range. 
 * This register also configures the Digital High Pass Filter (DHPF).
 *
 * @param[in]   config          Pointer to configuration structure
 * @retval      uint32_t        Error code
 */
uint32_t mpu_config(mpu_config_t * config);


/**@brief Function for configuring the behaviour of the interrupt pin of the MPU
 *
 * @param[in]   config          Pointer to configuration structure
 * @retval      uint32_t        Error code
 */
uint32_t mpu_int_cfg_pin(mpu_int_pin_cfg_t *cfg);


/**@brief Function for eneabling interrupts sources in MPU
 *
 * @param[in]   config          Pointer to configuration structure
 * @retval      uint32_t        Error code
 */
uint32_t mpu_int_enable(mpu_int_enable_t *cfg);
    

/**@brief Function for reading MPU accelerometer data.
 *
 * @param[in]   accel_values    Pointer to variable to hold accelerometer data
 * @retval      uint32_t        Error code
 */
uint32_t mpu_read_accel(accel_values_t * accel_values);


/**@brief Function for reading MPU gyroscope data.
 *
 * @param[in]   gyro_values     Pointer to variable to hold gyroscope data
 * @retval      uint32_t        Error code
 */
uint32_t mpu_read_gyro(gyro_values_t * gyro_values);
uint32_t mpu_read_magn(magn_values_t * magn_values);

/**@brief Function for reading MPU temperature data.
 *
 * @param[in]   temp_values     Pointer to variable to hold temperature data
 * @retval      uint32_t        Error code
 */
uint32_t mpu_read_temp(temp_value_t * temp_values);

/**@brief Function for reading the source of the MPU generated interrupts.
 *
 * @param[in]   int_source      Pointer to variable to hold interrupt source
 * @retval      uint32_t        Error code
 */
uint32_t mpu_read_int_source(uint8_t * int_source);

/**@brief Function for configuring free fall interrupts 
 *
 * THIS FUNCTION DOES NOT WORK ON MPU60x0 and MPU9255 AS THEY DON'T HAVE THE
 * THE SAME FREE FALL FUNCTIONALLITY AS THE MPU9150
 * 
 * @param[in]   uint16_t       Free fall threshold in mg
 * @retval      uint8_t        Required free fall duration in ms
 */
#if defined(MPU9150)
uint32_t mpu_config_ff_detection(uint16_t mg, uint8_t duration);
#endif

#endif /* MPU_H */

#endif /**@(defined(MPU60x0) || defined(MPU9150) || defined(MPU9255))*/

/**
  @}
*/

