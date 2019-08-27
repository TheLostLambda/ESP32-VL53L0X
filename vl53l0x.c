// VL53L0X control
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
// Based on https://github.com/pololu/vl53l0x-arduino

#include "vl53l0x.h"
#include "esp_log.h"
#include <driver/i2c.h>

enum
{
   SYSRANGE_START = 0x00,

   SYSTEM_THRESH_HIGH = 0x0C,
   SYSTEM_THRESH_LOW = 0x0E,

   SYSTEM_SEQUENCE_CONFIG = 0x01,
   SYSTEM_RANGE_CONFIG = 0x09,
   SYSTEM_INTERMEASUREMENT_PERIOD = 0x04,

   SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0A,

   GPIO_HV_MUX_ACTIVE_HIGH = 0x84,

   SYSTEM_INTERRUPT_CLEAR = 0x0B,

   RESULT_INTERRUPT_STATUS = 0x13,
   RESULT_RANGE_STATUS = 0x14,

   RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN = 0xBC,
   RESULT_CORE_RANGING_TOTAL_EVENTS_RTN = 0xC0,
   RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF = 0xD0,
   RESULT_CORE_RANGING_TOTAL_EVENTS_REF = 0xD4,
   RESULT_PEAK_SIGNAL_RATE_REF = 0xB6,

   ALGO_PART_TO_PART_RANGE_OFFSET_MM = 0x28,

   I2C_SLAVE_DEVICE_ADDRESS = 0x8A,

   MSRC_CONFIG_CONTROL = 0x60,

   PRE_RANGE_CONFIG_MIN_SNR = 0x27,
   PRE_RANGE_CONFIG_VALID_PHASE_LOW = 0x56,
   PRE_RANGE_CONFIG_VALID_PHASE_HIGH = 0x57,
   PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT = 0x64,

   FINAL_RANGE_CONFIG_MIN_SNR = 0x67,
   FINAL_RANGE_CONFIG_VALID_PHASE_LOW = 0x47,
   FINAL_RANGE_CONFIG_VALID_PHASE_HIGH = 0x48,
   FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44,

   PRE_RANGE_CONFIG_SIGMA_THRESH_HI = 0x61,
   PRE_RANGE_CONFIG_SIGMA_THRESH_LO = 0x62,

   PRE_RANGE_CONFIG_VCSEL_PERIOD = 0x50,
   PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x51,
   PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO = 0x52,

   SYSTEM_HISTOGRAM_BIN = 0x81,
   HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT = 0x33,
   HISTOGRAM_CONFIG_READOUT_CTRL = 0x55,

   FINAL_RANGE_CONFIG_VCSEL_PERIOD = 0x70,
   FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x71,
   FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO = 0x72,
   CROSSTALK_COMPENSATION_PEAK_RATE_MCPS = 0x20,

   MSRC_CONFIG_TIMEOUT_MACROP = 0x46,

   SOFT_RESET_GO2_SOFT_RESET_N = 0xBF,
   IDENTIFICATION_MODEL_ID = 0xC0,
   IDENTIFICATION_REVISION_ID = 0xC2,

   OSC_CALIBRATE_VAL = 0xF8,

   GLOBAL_CONFIG_VCSEL_WIDTH = 0x32,
   GLOBAL_CONFIG_SPAD_ENABLES_REF_0 = 0xB0,
   GLOBAL_CONFIG_SPAD_ENABLES_REF_1 = 0xB1,
   GLOBAL_CONFIG_SPAD_ENABLES_REF_2 = 0xB2,
   GLOBAL_CONFIG_SPAD_ENABLES_REF_3 = 0xB3,
   GLOBAL_CONFIG_SPAD_ENABLES_REF_4 = 0xB4,
   GLOBAL_CONFIG_SPAD_ENABLES_REF_5 = 0xB5,

   GLOBAL_CONFIG_REF_EN_START_SELECT = 0xB6,
   DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD = 0x4E,
   DYNAMIC_SPAD_REF_EN_START_OFFSET = 0x4F,
   POWER_MANAGEMENT_GO1_POWER_FORCE = 0x80,

   VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV = 0x89,

   ALGO_PHASECAL_LIM = 0x30,
   ALGO_PHASECAL_CONFIG_TIMEOUT = 0x30,
};

typedef enum
{ VcselPeriodPreRange, VcselPeriodFinalRange } vcselPeriodType;


struct vl53l0x_s
{
   uint8_t port;
   uint8_t address;
   int64_t io_timeout;
   uint8_t did_timeout:1;
};

typedef struct
{
   uint8_t tcc:1;
   uint8_t msrc:1;
   uint8_t dss:1;
   uint8_t pre_range:1;
   uint8_t final_range:1;
} SequenceStepEnables;

typedef struct
{
   uint16_t pre_range_vcsel_period_pclks,
     final_range_vcsel_period_pclks;
   uint16_t msrc_dss_tcc_mclks,
     pre_range_mclks,
     final_range_mclks;
   uint32_t msrc_dss_tcc_us,
     pre_range_us,
     final_range_us;
}
SequenceStepTimeouts;

static uint8_t stop_variable;
static int64_t timeout_start_ms;
static uint32_t measurement_timing_budget_us;
#define millis() (esp_timer_get_time()/1000)

// Record the current time to check an upcoming timeout against
#define startTimeout() (timeout_start_ms = millis())

// Check if timeout is enabled (set to nonzero value) and has expired
#define checkTimeoutExpired() (v->io_timeout > 0 && ((uint16_t)millis() - timeout_start_ms) > v->io_timeout)

// Encode VCSEL pulse period register value from period in PCLKs
// based on VL53L0X_encode_vcsel_period()
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)

#define writeReg(reg,val) writeRegPort(v->port,reg,val)
int
writeRegPort (uint8_t port, uint8_t reg, uint8_t val)
{
   // TODO
   return -1;
}

#define writeReg16Bit(reg,val) writeRegPort(v->port,reg,val)
int
writeReg16Port (uint8_t port, uint8_t reg, uint16_t val)
{
   // TODO
   return -1;
}

#define writeReg32Bit(reg,value) writeReg32Port(v,reg,value)
int
writeReg32Port (vl53l0x_t * v, uint8_t reg, uint32_t value)
{
   // TODO
   return -1;
}


#define readReg(reg) readRegPort(v->port,reg)
int
readRegPort (uint8_t port, uint8_t reg)
{
   // TODO
   return -1;
}

#define readReg16Bit(reg) readReg16Port(v->port,reg)
int
readReg16Port (uint8_t port, uint8_t reg)
{
   // TODO
   return -1;
}

#define readReg32Bit(reg) readReg32Port(v,reg)
uint32_t
readReg32Port (uint8_t port, uint8_t reg)
{
   // TODO
   return -1;
}

// Get the return signal rate limit check value in MCPS

#define	readMulti(reg,dst,count) readMultiPort(v->port,reg,dst,count)
// Read an arbitrary number of bytes from the sensor, starting at the given
// register, into the given array
int
readMultiPort (uint8_t port, uint8_t reg, uint8_t * dst, uint8_t count)
{
   // TODO
   return -1;
}

#define writeMulti(reg,src,count) writeMultiPort(v->port,reg,src,count)
// Write an arbitrary number of bytes from the given array to the sensor,
// starting at the given register
int
writeMultiPort (uint8_t port, uint8_t reg, uint8_t const *src, uint8_t count)
{
   // TODO
   return -1;
}

// Decode VCSEL (vertical cavity surface emitting laser) pulse period in PCLKs
// from register value
// based on VL53L0X_decode_vcsel_period()
#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)

// Calculate macro period in *nanoseconds* from VCSEL period in PCLKs
// based on VL53L0X_calc_macro_period_ps()
// PLL_period_ps = 1655; macro_period_vclks = 2304
#define calcMacroPeriod(vcsel_period_pclks) ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)

// Decode sequence step timeout in MCLKs from register value
// based on VL53L0X_decode_timeout()
// Note: the original function returned a uint32_t, but the return value is
// always stored in a uint16_t.
uint16_t
decodeTimeout (uint16_t reg_val)
{
   // format: "(LSByte * 2^MSByte) + 1"
   return (uint16_t) ((reg_val & 0x00FF) << (uint16_t) ((reg_val & 0xFF00) >> 8)) + 1;
}

// Convert sequence step timeout from MCLKs to microseconds with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_us()
static uint32_t
timeoutMclksToMicroseconds (uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks)
{
   uint32_t macro_period_ns = calcMacroPeriod (vcsel_period_pclks);

   return ((timeout_period_mclks * macro_period_ns) + (macro_period_ns / 2)) / 1000;
}

// based on VL53L0X_perform_single_ref_calibration()
static int
performSingleRefCalibration (vl53l0x_t * v, uint8_t vhv_init_byte)
{
   writeReg (SYSRANGE_START, 0x01 | vhv_init_byte);     // VL53L0X_REG_SYSRANGE_MODE_START_STOP

   startTimeout ();
   while ((readReg (RESULT_INTERRUPT_STATUS) & 0x07) == 0)
   {
      if (checkTimeoutExpired ())
         return -1;
   }

   writeReg (SYSTEM_INTERRUPT_CLEAR, 0x01);

   writeReg (SYSRANGE_START, 0x00);

   return 0;
}

// Encode sequence step timeout register value from timeout in MCLKs
// based on VL53L0X_encode_timeout()
// Note: the original function took a uint16_t, but the argument passed to it
// is always a uint16_t.
static uint16_t
encodeTimeout (uint16_t timeout_mclks)
{
   // format: "(LSByte * 2^MSByte) + 1"

   uint32_t ls_byte = 0;
   uint16_t ms_byte = 0;

   if (timeout_mclks > 0)
   {
      ls_byte = timeout_mclks - 1;

      while ((ls_byte & 0xFFFFFF00) > 0)
      {
         ls_byte >>= 1;
         ms_byte++;
      }

      return (ms_byte << 8) | (ls_byte & 0xFF);
   } else
   {
      return 0;
   }
}

// Get the VCSEL pulse period in PCLKs for the given period type.
// based on VL53L0X_get_vcsel_pulse_period()
static uint8_t
getVcselPulsePeriod (vl53l0x_t * v, vcselPeriodType type)
{
   if (type == VcselPeriodPreRange)
   {
      return decodeVcselPeriod (readReg (PRE_RANGE_CONFIG_VCSEL_PERIOD));
   } else if (type == VcselPeriodFinalRange)
   {
      return decodeVcselPeriod (readReg (FINAL_RANGE_CONFIG_VCSEL_PERIOD));
   } else
   {
      return 255;
   }
}

// Get sequence step enables
// based on VL53L0X_GetSequenceStepEnables()
static void
getSequenceStepEnables (vl53l0x_t * v, SequenceStepEnables * enables)
{
   uint8_t sequence_config = readReg (SYSTEM_SEQUENCE_CONFIG);

   enables->tcc = (sequence_config >> 4) & 0x1;
   enables->dss = (sequence_config >> 3) & 0x1;
   enables->msrc = (sequence_config >> 2) & 0x1;
   enables->pre_range = (sequence_config >> 6) & 0x1;
   enables->final_range = (sequence_config >> 7) & 0x1;
}

// Get sequence step timeouts
// based on get_sequence_step_timeout(),
// but gets all timeouts instead of just the requested one, and also stores
// intermediate values
static void
getSequenceStepTimeouts (vl53l0x_t * v, SequenceStepEnables const *enables, SequenceStepTimeouts * timeouts)
{
   timeouts->pre_range_vcsel_period_pclks = getVcselPulsePeriod (v, VcselPeriodPreRange);

   timeouts->msrc_dss_tcc_mclks = readReg (MSRC_CONFIG_TIMEOUT_MACROP) + 1;
   timeouts->msrc_dss_tcc_us = timeoutMclksToMicroseconds (timeouts->msrc_dss_tcc_mclks, timeouts->pre_range_vcsel_period_pclks);

   timeouts->pre_range_mclks = decodeTimeout (readReg16Bit (PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
   timeouts->pre_range_us = timeoutMclksToMicroseconds (timeouts->pre_range_mclks, timeouts->pre_range_vcsel_period_pclks);

   timeouts->final_range_vcsel_period_pclks = getVcselPulsePeriod (v, VcselPeriodFinalRange);

   timeouts->final_range_mclks = decodeTimeout (readReg16Bit (FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

   if (enables->pre_range)
   {
      timeouts->final_range_mclks -= timeouts->pre_range_mclks;
   }

   timeouts->final_range_us = timeoutMclksToMicroseconds (timeouts->final_range_mclks, timeouts->final_range_vcsel_period_pclks);
}

// Convert sequence step timeout from microseconds to MCLKs with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_mclks()
static uint32_t
timeoutMicrosecondsToMclks (uint32_t timeout_period_us, uint8_t vcsel_period_pclks)
{
   uint32_t macro_period_ns = calcMacroPeriod (vcsel_period_pclks);

   return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

// Set the return signal rate limit check value in units of MCPS (mega counts
// per second). "This represents the amplitude of the signal reflected from the
// target and detected by the device"; setting this limit presumably determines
// the minimum measurement necessary for the sensor to report a valid reading.
// Setting a lower limit increases the potential range of the sensor but also
// seems to increase the likelihood of getting an inaccurate reading because of
// unwanted reflections from objects other than the intended target.
// Defaults to 0.25 MCPS as initialized by the ST API and this library.
int
setSignalRateLimit (vl53l0x_t * v, float limit_Mcps)
{
   if (limit_Mcps < 0 || limit_Mcps > 511.99)
      return -1;
   // Q9.7 fixed point format (9 integer bits, 7 fractional bits)
   writeReg16Bit (FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, limit_Mcps * (1 << 7));
   return 0;
}

// Get reference SPAD (single photon avalanche diode) count and type
// based on VL53L0X_get_info_from_device(),
// but only gets reference SPAD count and type
int
getSpadInfo (vl53l0x_t * v, uint8_t * count, bool * type_is_aperture)
{
   uint8_t tmp;

   writeReg (0x80, 0x01);
   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x00);

   writeReg (0xFF, 0x06);
   writeReg (0x83, readReg (0x83) | 0x04);
   writeReg (0xFF, 0x07);
   writeReg (0x81, 0x01);

   writeReg (0x80, 0x01);

   writeReg (0x94, 0x6b);
   writeReg (0x83, 0x00);
   startTimeout ();
   while (readReg (0x83) == 0x00)
   {
      if (checkTimeoutExpired ())
         return -1;
   }
   writeReg (0x83, 0x01);
   tmp = readReg (0x92);

   *count = tmp & 0x7f;
   *type_is_aperture = (tmp >> 7) & 0x01;

   writeReg (0x81, 0x00);
   writeReg (0xFF, 0x06);
   writeReg (0x83, readReg (0x83) & ~0x04);
   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x01);

   writeReg (0xFF, 0x00);
   writeReg (0x80, 0x00);

   return 0;
}

// Get the measurement timing budget in microseconds
// based on VL53L0X_get_measurement_timing_budget_micro_seconds()
// in us
uint32_t
getMeasurementTimingBudget (vl53l0x_t * v)
{
   SequenceStepEnables enables;
   SequenceStepTimeouts timeouts;

   uint16_t const StartOverhead = 1910; // note that this is different than the value in set_
   uint16_t const EndOverhead = 960;
   uint16_t const MsrcOverhead = 660;
   uint16_t const TccOverhead = 590;
   uint16_t const DssOverhead = 690;
   uint16_t const PreRangeOverhead = 660;
   uint16_t const FinalRangeOverhead = 550;

   // "Start and end overhead times always present"
   uint32_t budget_us = StartOverhead + EndOverhead;

   getSequenceStepEnables (v, &enables);
   getSequenceStepTimeouts (v, &enables, &timeouts);

   if (enables.tcc)
   {
      budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
   }

   if (enables.dss)
   {
      budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
   } else if (enables.msrc)
   {
      budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
   }

   if (enables.pre_range)
   {
      budget_us += (timeouts.pre_range_us + PreRangeOverhead);
   }

   if (enables.final_range)
   {
      budget_us += (timeouts.final_range_us + FinalRangeOverhead);
   }

   measurement_timing_budget_us = budget_us;    // store for internal reuse
   return budget_us;
}


// Set the measurement timing budget in microseconds, which is the time allowed
// for one measurement; the ST API and this library take care of splitting the
// timing budget among the sub-steps in the ranging sequence. A longer timing
// budget allows for more accurate measurements. Increasing the budget by a
// factor of N decreases the range measurement standard deviation by a factor of
// sqrt(N). Defaults to about 33 milliseconds; the minimum is 20 ms.
// based on VL53L0X_set_measurement_timing_budget_micro_seconds()
int
setMeasurementTimingBudget (vl53l0x_t * v, uint32_t budget_us)
{
   SequenceStepEnables enables;
   SequenceStepTimeouts timeouts;

   uint16_t const StartOverhead = 1320; // note that this is different than the value in get_
   uint16_t const EndOverhead = 960;
   uint16_t const MsrcOverhead = 660;
   uint16_t const TccOverhead = 590;
   uint16_t const DssOverhead = 690;
   uint16_t const PreRangeOverhead = 660;
   uint16_t const FinalRangeOverhead = 550;

   uint32_t const MinTimingBudget = 20000;

   if (budget_us < MinTimingBudget)
      return -1;

   uint32_t used_budget_us = StartOverhead + EndOverhead;

   getSequenceStepEnables (v, &enables);
   getSequenceStepTimeouts (v, &enables, &timeouts);

   if (enables.tcc)
   {
      used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
   }

   if (enables.dss)
   {
      used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
   } else if (enables.msrc)
   {
      used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
   }

   if (enables.pre_range)
   {
      used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
   }

   if (enables.final_range)
   {
      used_budget_us += FinalRangeOverhead;

      // "Note that the final range timeout is determined by the timing
      // budget and the sum of all other timeouts within the sequence.
      // If there is no room for the final range timeout, then an error
      // will be set. Otherwise the remaining time will be applied to
      // the final range."

      if (used_budget_us > budget_us)
         return -1;             // "Requested timeout too big."

      uint32_t final_range_timeout_us = budget_us - used_budget_us;

      // set_sequence_step_timeout() begin
      // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

      // "For the final range timeout, the pre-range timeout
      //  must be added. To do this both final and pre-range
      //  timeouts must be expressed in macro periods MClks
      //  because they have different vcsel periods."

      uint16_t
         final_range_timeout_mclks = timeoutMicrosecondsToMclks (final_range_timeout_us, timeouts.final_range_vcsel_period_pclks);

      if (enables.pre_range)
      {
         final_range_timeout_mclks += timeouts.pre_range_mclks;
      }

      writeReg16Bit (FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout (final_range_timeout_mclks));

      // set_sequence_step_timeout() end

      measurement_timing_budget_us = budget_us; // store for internal reuse
   }
   return 0;
}

// Initialize sensor using sequence based on VL53L0X_DataInit(),
// VL53L0X_StaticInit(), and VL53L0X_PerformRefCalibration().
// This function does not perform reference SPAD calibration
// (VL53L0X_PerformRefSpadManagement()), since the API user manual says that it
// is performed by ST on the bare modules; it seems like that should work well
// enough unless a cover glass is added.
// If io_2v8 (optional) is true or not given, the sensor is configured for 2V8
// mode.
vl53l0x_t *
vl53l0x_init (uint8_t port, uint8_t scl, uint8_t sda, uint8_t address, uint8_t io_2v8)
{
   if (i2c_driver_install (port, I2C_MODE_MASTER, 0, 0, 0))
      return NULL;
   i2c_config_t config = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = sda,
      .scl_io_num = scl,
      .sda_pullup_en = true,
      .scl_pullup_en = true,
      .master.clk_speed = 100000,
   };
   if (i2c_param_config (port, &config))
   {
      i2c_driver_delete (port);
      return NULL;
   }
   vl53l0x_t *v = malloc (sizeof (*v));
   if (!v)
   {
      i2c_driver_delete (port);
      return v;
   }
   memset (v, 0, sizeof (*v));
   v->port = port;
   v->address = (address ? : I2C_SLAVE_DEVICE_ADDRESS);

   // Set up the VL53L0X
   // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
   if (io_2v8)
      writeReg (VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, readReg (VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01); // set bit 0
   // "Set I2C standard mode"
   writeReg (0x88, 0x00);

   writeReg (0x80, 0x01);
   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x00);
   stop_variable = readReg (0x91);
   writeReg (0x00, 0x01);
   writeReg (0xFF, 0x00);
   writeReg (0x80, 0x00);

   // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
   writeReg (MSRC_CONFIG_CONTROL, readReg (MSRC_CONFIG_CONTROL) | 0x12);

   // set final range signal rate limit to 0.25 MCPS (million counts per second)
   setSignalRateLimit (v, 0.25);

   writeReg (SYSTEM_SEQUENCE_CONFIG, 0xFF);

   // VL53L0X_DataInit() end

   // VL53L0X_StaticInit() begin

   uint8_t spad_count;
   bool spad_type_is_aperture;
   if (!getSpadInfo (v, &spad_count, &spad_type_is_aperture))
   {
      return false;
   }
   // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
   // the API, but the same data seems to be more easily readable from
   // GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
   uint8_t ref_spad_map[6];
   readMulti (GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

   // -- VL53L0X_set_reference_spads() begin (assume NVM values are valid)

   writeReg (0xFF, 0x01);
   writeReg (DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
   writeReg (DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
   writeReg (0xFF, 0x00);
   writeReg (GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

   uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0;       // 12 is the first aperture spad
   uint8_t spads_enabled = 0;

   for (uint8_t i = 0; i < 48; i++)
   {
      if (i < first_spad_to_enable || spads_enabled == spad_count)
      {
         // This bit is lower than the first one that should be enabled, or
         // (reference_spad_count) bits have already been enabled, so zero this bit
         ref_spad_map[i / 8] &= ~(1 << (i % 8));
      } else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1)
      {
         spads_enabled++;
      }
   }

   writeMulti (GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

   // -- VL53L0X_set_reference_spads() end

   // -- VL53L0X_load_tuning_settings() begin
   // DefaultTuningSettings from vl53l0x_tuning.h

   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x00);

   writeReg (0xFF, 0x00);
   writeReg (0x09, 0x00);
   writeReg (0x10, 0x00);
   writeReg (0x11, 0x00);

   writeReg (0x24, 0x01);
   writeReg (0x25, 0xFF);
   writeReg (0x75, 0x00);

   writeReg (0xFF, 0x01);
   writeReg (0x4E, 0x2C);
   writeReg (0x48, 0x00);
   writeReg (0x30, 0x20);

   writeReg (0xFF, 0x00);
   writeReg (0x30, 0x09);
   writeReg (0x54, 0x00);
   writeReg (0x31, 0x04);
   writeReg (0x32, 0x03);
   writeReg (0x40, 0x83);
   writeReg (0x46, 0x25);
   writeReg (0x60, 0x00);
   writeReg (0x27, 0x00);
   writeReg (0x50, 0x06);
   writeReg (0x51, 0x00);
   writeReg (0x52, 0x96);
   writeReg (0x56, 0x08);
   writeReg (0x57, 0x30);
   writeReg (0x61, 0x00);
   writeReg (0x62, 0x00);
   writeReg (0x64, 0x00);
   writeReg (0x65, 0x00);
   writeReg (0x66, 0xA0);

   writeReg (0xFF, 0x01);
   writeReg (0x22, 0x32);
   writeReg (0x47, 0x14);
   writeReg (0x49, 0xFF);
   writeReg (0x4A, 0x00);

   writeReg (0xFF, 0x00);
   writeReg (0x7A, 0x0A);
   writeReg (0x7B, 0x00);
   writeReg (0x78, 0x21);

   writeReg (0xFF, 0x01);
   writeReg (0x23, 0x34);
   writeReg (0x42, 0x00);
   writeReg (0x44, 0xFF);
   writeReg (0x45, 0x26);
   writeReg (0x46, 0x05);
   writeReg (0x40, 0x40);
   writeReg (0x0E, 0x06);
   writeReg (0x20, 0x1A);
   writeReg (0x43, 0x40);

   writeReg (0xFF, 0x00);
   writeReg (0x34, 0x03);
   writeReg (0x35, 0x44);

   writeReg (0xFF, 0x01);
   writeReg (0x31, 0x04);
   writeReg (0x4B, 0x09);
   writeReg (0x4C, 0x05);
   writeReg (0x4D, 0x04);

   writeReg (0xFF, 0x00);
   writeReg (0x44, 0x00);
   writeReg (0x45, 0x20);
   writeReg (0x47, 0x08);
   writeReg (0x48, 0x28);
   writeReg (0x67, 0x00);
   writeReg (0x70, 0x04);
   writeReg (0x71, 0x01);
   writeReg (0x72, 0xFE);
   writeReg (0x76, 0x00);
   writeReg (0x77, 0x00);

   writeReg (0xFF, 0x01);
   writeReg (0x0D, 0x01);

   writeReg (0xFF, 0x00);
   writeReg (0x80, 0x01);
   writeReg (0x01, 0xF8);

   writeReg (0xFF, 0x01);
   writeReg (0x8E, 0x01);
   writeReg (0x00, 0x01);
   writeReg (0xFF, 0x00);
   writeReg (0x80, 0x00);

   // -- VL53L0X_load_tuning_settings() end

   // "Set interrupt config to new sample ready"
   // -- VL53L0X_SetGpioConfig() begin

   writeReg (SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
   writeReg (GPIO_HV_MUX_ACTIVE_HIGH, readReg (GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);       // active low
   writeReg (SYSTEM_INTERRUPT_CLEAR, 0x01);

   // -- VL53L0X_SetGpioConfig() end

   measurement_timing_budget_us = getMeasurementTimingBudget (v);

   // "Disable MSRC and TCC by default"
   // MSRC = Minimum Signal Rate Check
   // TCC = Target CentreCheck
   // -- VL53L0X_SetSequenceStepEnable() begin

   writeReg (SYSTEM_SEQUENCE_CONFIG, 0xE8);

   // -- VL53L0X_SetSequenceStepEnable() end

   // "Recalculate timing budget"
   setMeasurementTimingBudget (v, measurement_timing_budget_us);

   // VL53L0X_StaticInit() end

   // VL53L0X_PerformRefCalibration() begin (VL53L0X_perform_ref_calibration())

   // -- VL53L0X_perform_vhv_calibration() begin

   writeReg (SYSTEM_SEQUENCE_CONFIG, 0x01);
   if (!performSingleRefCalibration (v, 0x40))
   {
      i2c_driver_delete (port);
	  free(v);
	  return NULL;
   }
   // -- VL53L0X_perform_vhv_calibration() end

   // -- VL53L0X_perform_phase_calibration() begin

   writeReg (SYSTEM_SEQUENCE_CONFIG, 0x02);
   if (!performSingleRefCalibration (v, 0x00))
   {
      return false;
   }
   // -- VL53L0X_perform_phase_calibration() end

   // "restore the previous Sequence Config"
   writeReg (SYSTEM_SEQUENCE_CONFIG, 0xE8);

   // VL53L0X_PerformRefCalibration() end

   return v;
}

void
vl532l0x_end (vl53l0x_t * v)
{
   if (!v)
      return;
   i2c_driver_delete (v->port);
   free (v);
}

void
setAddress (vl53l0x_t * v, uint8_t new_addr)
{
   writeReg (I2C_SLAVE_DEVICE_ADDRESS, new_addr & 0x7F);
   v->address = new_addr;
}

float
getSignalRateLimit (vl53l0x_t * v)
{
   return (float) readReg16Bit (FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT) / (1 << 7);
}

// Set the VCSEL (vertical cavity surface emitting laser) pulse period for the
// given period type (pre-range or final range) to the given value in PCLKs.
// Longer periods seem to increase the potential range of the sensor.
// Valid values are (even numbers only):
//  pre:  12 to 18 (initialized default: 14)
//  final: 8 to 14 (initialized default: 10)
// based on VL53L0X_set_vcsel_pulse_period()
bool
setVcselPulsePeriod (vl53l0x_t * v, vcselPeriodType type, uint8_t period_pclks)
{
   uint8_t vcsel_period_reg = encodeVcselPeriod (period_pclks);

   SequenceStepEnables enables;
   SequenceStepTimeouts timeouts;

   getSequenceStepEnables (v, &enables);
   getSequenceStepTimeouts (v, &enables, &timeouts);

   // "Apply specific settings for the requested clock period"
   // "Re-calculate and apply timeouts, in macro periods"

   // "When the VCSEL period for the pre or final range is changed,
   // the corresponding timeout must be read from the device using
   // the current VCSEL period, then the new VCSEL period can be
   // applied. The timeout then must be written back to the device
   // using the new VCSEL period.
   //
   // For the MSRC timeout, the same applies - this timeout being
   // dependant on the pre-range vcsel period."


   if (type == VcselPeriodPreRange)
   {
      // "Set phase check limits"
      switch (period_pclks)
      {
      case 12:
         writeReg (PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x18);
         break;

      case 14:
         writeReg (PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x30);
         break;

      case 16:
         writeReg (PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x40);
         break;

      case 18:
         writeReg (PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x50);
         break;

      default:
         // invalid period
         return false;
      }
      writeReg (PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);

      // apply new VCSEL period
      writeReg (PRE_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg);

      // update timeouts

      // set_sequence_step_timeout() begin
      // (SequenceStepId == VL53L0X_SEQUENCESTEP_PRE_RANGE)

      uint16_t new_pre_range_timeout_mclks = timeoutMicrosecondsToMclks (timeouts.pre_range_us, period_pclks);

      writeReg16Bit (PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout (new_pre_range_timeout_mclks));

      // set_sequence_step_timeout() end

      // set_sequence_step_timeout() begin
      // (SequenceStepId == VL53L0X_SEQUENCESTEP_MSRC)

      uint16_t new_msrc_timeout_mclks = timeoutMicrosecondsToMclks (timeouts.msrc_dss_tcc_us, period_pclks);

      writeReg (MSRC_CONFIG_TIMEOUT_MACROP, (new_msrc_timeout_mclks > 256) ? 255 : (new_msrc_timeout_mclks - 1));

      // set_sequence_step_timeout() end
   } else if (type == VcselPeriodFinalRange)
   {
      switch (period_pclks)
      {
      case 8:
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x10);
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
         writeReg (GLOBAL_CONFIG_VCSEL_WIDTH, 0x02);
         writeReg (ALGO_PHASECAL_CONFIG_TIMEOUT, 0x0C);
         writeReg (0xFF, 0x01);
         writeReg (ALGO_PHASECAL_LIM, 0x30);
         writeReg (0xFF, 0x00);
         break;

      case 10:
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x28);
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
         writeReg (GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
         writeReg (ALGO_PHASECAL_CONFIG_TIMEOUT, 0x09);
         writeReg (0xFF, 0x01);
         writeReg (ALGO_PHASECAL_LIM, 0x20);
         writeReg (0xFF, 0x00);
         break;

      case 12:
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
         writeReg (GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
         writeReg (ALGO_PHASECAL_CONFIG_TIMEOUT, 0x08);
         writeReg (0xFF, 0x01);
         writeReg (ALGO_PHASECAL_LIM, 0x20);
         writeReg (0xFF, 0x00);
         break;

      case 14:
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x48);
         writeReg (FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
         writeReg (GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
         writeReg (ALGO_PHASECAL_CONFIG_TIMEOUT, 0x07);
         writeReg (0xFF, 0x01);
         writeReg (ALGO_PHASECAL_LIM, 0x20);
         writeReg (0xFF, 0x00);
         break;

      default:
         // invalid period
         return false;
      }

      // apply new VCSEL period
      writeReg (FINAL_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg);

      // update timeouts

      // set_sequence_step_timeout() begin
      // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

      // "For the final range timeout, the pre-range timeout
      //  must be added. To do this both final and pre-range
      //  timeouts must be expressed in macro periods MClks
      //  because they have different vcsel periods."

      uint16_t new_final_range_timeout_mclks = timeoutMicrosecondsToMclks (timeouts.final_range_us, period_pclks);

      if (enables.pre_range)
      {
         new_final_range_timeout_mclks += timeouts.pre_range_mclks;
      }

      writeReg16Bit (FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout (new_final_range_timeout_mclks));

      // set_sequence_step_timeout end
   } else
   {
      // invalid type
      return false;
   }

   // "Finally, the timing budget must be re-applied"

   setMeasurementTimingBudget (v, measurement_timing_budget_us);

   // "Perform the phase calibration. This is needed after changing on vcsel period."
   // VL53L0X_perform_phase_calibration() begin

   uint8_t sequence_config = readReg (SYSTEM_SEQUENCE_CONFIG);
   writeReg (SYSTEM_SEQUENCE_CONFIG, 0x02);
   performSingleRefCalibration (v, 0x0);
   writeReg (SYSTEM_SEQUENCE_CONFIG, sequence_config);

   // VL53L0X_perform_phase_calibration() end

   return true;
}

// Start continuous ranging measurements. If period_ms (optional) is 0 or not
// given, continuous back-to-back mode is used (the sensor takes measurements as
// often as possible); otherwise, continuous timed mode is used, with the given
// inter-measurement period in milliseconds determining how often the sensor
// takes a measurement.
// based on VL53L0X_StartMeasurement()
void
startContinuous (vl53l0x_t * v, uint32_t period_ms)
{
   writeReg (0x80, 0x01);
   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x00);
   writeReg (0x91, stop_variable);
   writeReg (0x00, 0x01);
   writeReg (0xFF, 0x00);
   writeReg (0x80, 0x00);

   if (period_ms != 0)
   {
      // continuous timed mode

      // VL53L0X_SetInterMeasurementPeriodMilliSeconds() begin

      uint16_t osc_calibrate_val = readReg16Bit (OSC_CALIBRATE_VAL);

      if (osc_calibrate_val != 0)
      {
         period_ms *= osc_calibrate_val;
      }

      writeReg32Bit (SYSTEM_INTERMEASUREMENT_PERIOD, period_ms);

      // VL53L0X_SetInterMeasurementPeriodMilliSeconds() end

      writeReg (SYSRANGE_START, 0x04);  // VL53L0X_REG_SYSRANGE_MODE_TIMED
   } else
   {
      // continuous back-to-back mode
      writeReg (SYSRANGE_START, 0x02);  // VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK
   }
}

// Stop continuous measurements
// based on VL53L0X_StopMeasurement()
void
stopContinuous (vl53l0x_t * v)
{
   writeReg (SYSRANGE_START, 0x01);     // VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT

   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x00);
   writeReg (0x91, 0x00);
   writeReg (0x00, 0x01);
   writeReg (0xFF, 0x00);
}

// Returns a range reading in millimeters when continuous mode is active
// (readRangeSingleMillimeters() also calls this function after starting a
// single-shot range measurement)
uint16_t
readRangeContinuousMillimeters (vl53l0x_t * v)
{
   startTimeout ();
   while ((readReg (RESULT_INTERRUPT_STATUS) & 0x07) == 0)
   {
      if (checkTimeoutExpired ())
      {
         v->did_timeout = 1;
         return 65535;
      }
   }

   // assumptions: Linearity Corrective Gain is 1000 (default);
   // fractional ranging is not enabled
   uint16_t range = readReg16Bit (RESULT_RANGE_STATUS + 10);

   writeReg (SYSTEM_INTERRUPT_CLEAR, 0x01);

   return range;
}

// Performs a single-shot range measurement and returns the reading in
// millimeters
// based on VL53L0X_PerformSingleRangingMeasurement()
uint16_t
readRangeSingleMillimeters (vl53l0x_t * v)
{
   writeReg (0x80, 0x01);
   writeReg (0xFF, 0x01);
   writeReg (0x00, 0x00);
   writeReg (0x91, stop_variable);
   writeReg (0x00, 0x01);
   writeReg (0xFF, 0x00);
   writeReg (0x80, 0x00);

   writeReg (SYSRANGE_START, 0x01);

   // "Wait until start bit has been cleared"
   startTimeout ();
   while (readReg (SYSRANGE_START) & 0x01)
   {
      if (checkTimeoutExpired ())
      {
         v->did_timeout = 1;
         return 65535;
      }
   }

   return readRangeContinuousMillimeters (v);
}

// Did a timeout occur in one of the read functions since the last call to
// timeoutOccurred()?
bool
timeoutOccurred (vl53l0x_t * v)
{
   bool tmp = v->did_timeout;
   v->did_timeout = false;
   return tmp;
}
