#ifndef STEP_DOG_COUNTER_MPU_UTILS_H
#define STEP_DOG_COUNTER_MPU_UTILS_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define GYRO_MEAS_ERROR (PI * (60.0f / 180.0f))
#define GYRO_MEAS_DRIFT (PI * (1.0f / 180.0f))
#define BETA            (std::sqrt(3.0f / 4.0f) * GYRO_MEAS_ERROR)
#define ZETA            (std::sqrt(3.0f / 4.0f) * GYRO_MEAS_DRIFT)

Adafruit_MPU6050 mpu;

inline void mpu6050_calibrate(float* accel_bias_res, float* gyro_bias_res) {
  double accel_bias[3] = {0, 0, 0};
  double gyro_bias[3] = {0, 0, 0};

  float accel_bias_reg[3] = {0, 0, 0};
  uint16_t accel_temp[3] = {0, 0, 0};
  uint16_t gyro_temp[3] = {0, 0, 0};
  uint8_t mask_bit[3] = {0, 0, 0};
  uint32_t mask = 1uL;
  uint16_t gyro_sensitivity = 131;
  uint16_t accel_sensitivity = 16384;
  uint8_t tmp_data[12];
  uint16_t packet_count = 100;

  // Configure device for bias calculation:
  mpu.setClock(clock_select::MPU6050_INTR_8MHz);
  mpu.setFilterBandwidth(mpu6050_bandwidth_t::MPU6050_BAND_184_HZ);
  mpu.setAccelerometerRange(mpu6050_accel_range_t::MPU6050_RANGE_2_G);
  mpu.setGyroRange(mpu6050_gyro_range_t::MPU6050_RANGE_250_DEG);

  // Accumulate 80 samples
  for (int i = 0; i < packet_count; i++) {
    // Read data for averaging:
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    accel_bias[0] += a.acceleration.x;
    accel_bias[1] += a.acceleration.y;
    accel_bias[2] += a.acceleration.z;
    gyro_bias[0] += g.gyro.x;
    gyro_bias[1] += g.gyro.y;
    gyro_bias[2] += g.gyro.z;

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  // Normalize sums to get average count biases:
  accel_bias[0] /= packet_count;
  accel_bias[1] /= packet_count;
  accel_bias[2] /= packet_count;
  gyro_bias[0] /= packet_count;
  gyro_bias[1] /= packet_count;
  gyro_bias[2] /= packet_count;

  accel_bias_res[0] = accel_bias[0];
  accel_bias_res[1] = accel_bias[1];
  accel_bias_res[2] = accel_bias[2];

  gyro_bias_res[0] = gyro_bias[0];
  gyro_bias_res[1] = gyro_bias[1];
  gyro_bias_res[2] = gyro_bias[2];
}

inline void mpu6050_madgwick_quaternion_update(float accel_x,
                                               float accel_y,
                                               float accel_z,
                                               float gyro_x,
                                               float gyro_y,
                                               float gyro_z,
                                               float* quart,
                                               float& delta_t) {
  float func_1, func_2, func_3;
  float j_11o24, j_12o23, j_13o22, j_14o21, j_32, j_33;
  float q_dot_1, q_dot_2, q_dot_3, q_dot_4;
  float hat_dot_1, hat_dot_2, hat_dot_3, hat_dot_4;
  float gyro_x_err, gyro_y_err, gyro_z_err;
  float gyro_x_bias, gyro_y_bias, gyro_z_bias;
  float norm;

  float half_q1 = 0.5f * quart[0];
  float half_q2 = 0.5f * quart[1];
  float half_q3 = 0.5f * quart[2];
  float half_q4 = 0.5f * quart[3];
  float double_q1 = 2.0f * quart[0];
  float double_q2 = 2.0f * quart[1];
  float double_q3 = 2.0f * quart[2];
  float double_q4 = 2.0f * quart[3];

  // Normalise accelerometer measurement:
  norm = std::sqrt(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);

  // Handle NaN:
  if (norm == 0.0f)
    return;

  norm = 1.0f / norm;
  accel_x *= norm;
  accel_y *= norm;
  accel_z *= norm;

  // Compute the objective function and Jacobian:
  func_1 = double_q2 * quart[3] - double_q1 * quart[2] - accel_x;
  func_2 = double_q1 * quart[1] + double_q3 * quart[3] - accel_y;
  func_3 = 1.0f - double_q2 * quart[3] - double_q3 * quart[2] - accel_z;
  j_11o24 = double_q3;
  j_12o23 = double_q4;
  j_13o22 = double_q1;
  j_14o21 = double_q2;
  j_32 = 2.0f * j_14o21;
  j_33 = 2.0f * j_11o24;

  // Compute the gradient (matrix multiplication):
  hat_dot_1 = j_14o21 * func_2 - j_11o24 * func_1;
  hat_dot_2 = j_12o23 * func_1 + j_13o22 * func_2 - j_32 * func_3;
  hat_dot_3 = j_12o23 * func_2 - j_33 * func_3 - j_13o22 * func_1;
  hat_dot_4 = j_14o21 * func_1 + j_11o24 * func_2;

  // Normalize the gradient:
  norm = std::sqrt(hat_dot_1 * hat_dot_1 + hat_dot_2 * hat_dot_2 + hat_dot_3 * hat_dot_3 +
                   hat_dot_4 * hat_dot_4);
  hat_dot_1 /= norm;
  hat_dot_2 /= norm;
  hat_dot_3 /= norm;
  hat_dot_4 /= norm;

  // Compute estimated gyroscope biases:
  gyro_x_err =
      double_q1 * hat_dot_2 - double_q2 * hat_dot_1 - double_q3 * hat_dot_4 + double_q4 * hat_dot_3;
  gyro_y_err =
      double_q1 * hat_dot_3 + double_q2 * hat_dot_4 - double_q3 * hat_dot_1 - double_q4 * hat_dot_2;
  gyro_z_err =
      double_q1 * hat_dot_4 - double_q2 * hat_dot_3 + double_q3 * hat_dot_2 - double_q4 * hat_dot_1;

  // Compute and remove gyroscope biases:
  gyro_x_bias += gyro_x_err * delta_t * ZETA;
  gyro_y_bias += gyro_y_err * delta_t * ZETA;
  gyro_z_bias += gyro_z_err * delta_t * ZETA;

  // Compute the quaternion derivative:
  q_dot_1 = -half_q2 * gyro_x - half_q3 * gyro_y - half_q4 * gyro_z;
  q_dot_2 = half_q1 * gyro_x + half_q3 * gyro_z - half_q4 * gyro_y;
  q_dot_3 = half_q1 * gyro_y - half_q2 * gyro_z + half_q4 * gyro_x;
  q_dot_4 = half_q1 * gyro_z + half_q2 * gyro_y - half_q3 * gyro_x;

  // Compute then integrate estimated quaternion derivative:
  quart[0] += (q_dot_1 - (BETA * hat_dot_1)) * delta_t;
  quart[1] += (q_dot_2 - (BETA * hat_dot_2)) * delta_t;
  quart[2] += (q_dot_3 - (BETA * hat_dot_3)) * delta_t;
  quart[3] += (q_dot_4 - (BETA * hat_dot_4)) * delta_t;

  // Normalize the quaternion:
  norm = std::sqrt(quart[0] * quart[0] + quart[1] * quart[1] + quart[2] * quart[2] +
                   quart[3] * quart[3]);
  norm = 1.0f / norm;
  quart[0] *= norm;
  quart[1] *= norm;
  quart[2] *= norm;
  quart[3] *= norm;
}

inline float mpu6050_get_accel_res(uint8_t accel_scale) {
  float accel_res = 0;

  switch (accel_scale) {
    case 0: {
      accel_res = 2.0 / 32768.0;
      break;
    }
    case 1: {
      accel_res = 4.0 / 32768.0;
      break;
    }
    case 2: {
      accel_res = 8.0 / 32768.0;
      break;
    }
    case 3: {
      accel_res = 16.0 / 32768.0;
      break;
    }
  }

  return (accel_res);
}

inline float mpu6050_get_gyro_res(uint8_t gyro_scale) {
  float gyro_res = 0;

  switch (gyro_scale) {
    case 0: {
      gyro_res = 250.0 / 32768.0;
      break;
    }
    case 1: {
      gyro_res = 500.0 / 32768.0;
      break;
    }
    case 2: {
      gyro_res = 1000.0 / 32768.0;
      break;
    }
    case 3: {
      gyro_res = 2000.0 / 32768.0;
      break;
    }
  }

  return (gyro_res);
}

#endif /* STEP_DOG_COUNTER_MPU_UTILS_H */
