#include "mpu6050Uitls.h"
#include "esp_timer.h"
#include "communication.h"

float accel_bias[3] = {0, 0, 0};
float gyro_bias[3] = {0, 0, 0};

float quart[4] = {1.0f, 0.0f, 0.0f, 0.0f};
float delta_t = 0.0f;
#define AVG_BUFF_SIZE 20

sensors_event_t a, g, temp;
uint64_t now_time = 0, prev_time = 0;
int8_t range = 0;
float accel_g_x, accel_g_y, accel_g_z;
float accel_int_x, accel_int_y, accel_int_z;
float gyro_ds_x, gyro_ds_y, gyro_ds_z, accel_res, gyro_res, temp_c;
int accel_x_avg_buff[AVG_BUFF_SIZE];
int accel_y_avg_buff[AVG_BUFF_SIZE];
int accel_z_avg_buff[AVG_BUFF_SIZE];
int accel_x_avg_buff_count = 0;
int accel_y_avg_buff_count = 0;
int accel_z_avg_buff_count = 0;
int accel_x_avg, accel_y_avg, accel_z_avg;
int min_reg_accel_x = 0, min_reg_accel_y = 0, min_reg_accel_z = 0;
int max_reg_accel_x = 0, max_reg_accel_y = 0, max_reg_accel_z = 0;
int min_curr_accel_x, min_curr_accel_y, min_curr_accel_z;
int max_curr_accel_x, max_curr_accel_y, max_curr_accel_z;
int dy_thres_accel_x = 0, dy_thres_accel_y = 0, dy_thres_accel_z = 0;
int dy_chan_accel_x, dy_chan_accel_y, dy_chan_accel_z;
int sample_new = 0, sample_old = 0;
int step_size = 200;
int active_axis = 0, interval = 500000;
int step_changed = 0;

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  Serial.println("MPU6050 calibrating...");
  mpu6050_calibrate(accel_bias, gyro_bias);
  Serial.println("MPU6050 calibrate finished!");
  Serial.print("Accel Bias :");
  Serial.print(accel_bias[0]);
  Serial.print(",");
  Serial.print(accel_bias[1]);
  Serial.print(",");
  Serial.print(accel_bias[2]);
  Serial.print(", ");
  Serial.print("Gyro Bias :");
  Serial.print(gyro_bias[0]);
  Serial.print(",");
  Serial.print(gyro_bias[1]);
  Serial.print(",");
  Serial.print(gyro_bias[2]);
  Serial.println("");

  // setupt motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(1);
  mpu.setInterruptPinLatch(true); // Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  Serial.println("");
  delay(100);
  commSetup();
}

void step_counter() {

  mpu.getEvent(&a, &g, &temp);
  range = static_cast<int>(mpu.getAccelerometerRange());
  accel_res = 1; // mpu6050_get_accel_res(range);

  accel_g_x = a.acceleration.x * accel_res - accel_bias[0];
  accel_g_y = a.acceleration.x * accel_res - accel_bias[1];
  accel_g_z = a.acceleration.x * accel_res - accel_bias[2];

  if (accel_g_x < 0)
    accel_g_x *= -1;
  if (accel_g_y < 0)
    accel_g_y *= -1;
  if (accel_g_z < 0)
    accel_g_z *= -1;

  range = mpu.getGyroRange();
  gyro_res = 1; // mpu6050_get_gyro_res(range);

  gyro_ds_x = g.gyro.x * gyro_res - gyro_bias[0];
  gyro_ds_y = g.gyro.y * gyro_res - gyro_bias[1];
  gyro_ds_z = g.gyro.z * gyro_res - gyro_bias[2];

  mpu6050_madgwick_quaternion_update(accel_g_x,
                                     accel_g_y,
                                     accel_g_z,
                                     gyro_ds_x * PI / 180.0f,
                                     gyro_ds_y * PI / 180.0f,
                                     gyro_ds_z * PI / 180.0f,
                                     quart,
                                     delta_t);

  accel_int_x = 1000 * accel_g_x;
  accel_int_y = 1000 * accel_g_y;
  accel_int_z = 1000 * accel_g_z;

  accel_x_avg_buff[accel_x_avg_buff_count] = accel_int_x;
  accel_x_avg_buff_count++;
  accel_x_avg_buff_count %= AVG_BUFF_SIZE;
  accel_x_avg = 0;

  for (int i = 0; i < AVG_BUFF_SIZE; i++)
    accel_x_avg += accel_x_avg_buff[i];

  accel_x_avg /= AVG_BUFF_SIZE;

  accel_y_avg_buff[accel_y_avg_buff_count] = accel_int_y;
  accel_y_avg_buff_count++;
  accel_y_avg_buff_count %= AVG_BUFF_SIZE;
  accel_y_avg = 0;

  for (int i = 0; i < AVG_BUFF_SIZE; i++)
    accel_y_avg += accel_y_avg_buff[i];

  accel_y_avg /= AVG_BUFF_SIZE;

  accel_z_avg_buff[accel_z_avg_buff_count] = accel_int_z;
  accel_z_avg_buff_count++;
  accel_z_avg_buff_count %= AVG_BUFF_SIZE;
  accel_z_avg = 0;

  for (int i = 0; i < AVG_BUFF_SIZE; i++)
    accel_z_avg += accel_z_avg_buff[i];

  accel_z_avg /= AVG_BUFF_SIZE;

  now_time = esp_timer_get_time();

  if (now_time - prev_time >= interval) {
    Serial.println("1");
    prev_time = now_time;

    min_curr_accel_x = min_reg_accel_x;
    max_curr_accel_x = max_reg_accel_x;
    dy_thres_accel_x = (min_curr_accel_x + max_curr_accel_x) / 2;
    dy_chan_accel_x = (max_curr_accel_x - min_curr_accel_x);
    min_reg_accel_x = accel_x_avg;
    max_reg_accel_x = accel_x_avg;
    min_curr_accel_y = min_reg_accel_y;
    max_curr_accel_y = max_reg_accel_y;
    dy_thres_accel_y = (min_curr_accel_y + max_curr_accel_y) / 2;
    dy_chan_accel_y = (max_curr_accel_y - min_curr_accel_y);
    min_reg_accel_y = accel_y_avg;
    max_reg_accel_y = accel_y_avg;
    min_curr_accel_z = min_reg_accel_z;
    max_curr_accel_z = max_reg_accel_z;
    dy_thres_accel_z = (min_curr_accel_z + max_curr_accel_z) / 2;
    dy_chan_accel_z = (max_curr_accel_z - min_curr_accel_z);
    min_reg_accel_z = accel_z_avg;
    max_reg_accel_z = accel_z_avg;

    if (dy_chan_accel_x >= dy_chan_accel_y && dy_chan_accel_x >= dy_chan_accel_z) {
      if (active_axis != 0) {
        sample_old = 0;
        sample_new = accel_x_avg;
      }
      active_axis = 0;
    } else if (dy_chan_accel_y >= dy_chan_accel_x && dy_chan_accel_y >= dy_chan_accel_z) {
      if (active_axis != 1) {
        sample_old = 0;
        sample_new = accel_y_avg;
      }
      active_axis = 1;
    } else {
      if (active_axis != 2) {
        sample_old = 0;
        sample_new = accel_z_avg;
      }
      active_axis = 2;
    }

  } else if (now_time < 500) {
    Serial.println("2");
    if (min_reg_accel_x > accel_x_avg)
      min_reg_accel_x = accel_x_avg;
    if (max_reg_accel_x < accel_x_avg)
      max_reg_accel_x = accel_x_avg;
    if (min_reg_accel_y > accel_y_avg)
      min_reg_accel_y = accel_y_avg;
    if (max_reg_accel_y < accel_y_avg)
      max_reg_accel_y = accel_y_avg;
    if (min_reg_accel_z > accel_z_avg)
      min_reg_accel_z = accel_z_avg;
    if (max_reg_accel_z < accel_z_avg)
      max_reg_accel_z = accel_z_avg;
  }

  sample_old = sample_new;
  switch (active_axis) {
    case 0:
      // Serial.println("3");
      if (accel_x_avg - sample_old > step_size || accel_x_avg - sample_old < -step_size) {
        sample_new = accel_x_avg;
        // Serial.println("3.1");
        if (sample_old > dy_thres_accel_x && sample_new < dy_thres_accel_x) {
          Serial.println("3.2");
          step_count++;
          step_changed = 1;
        }
      }
      break;
    case 1:
      Serial.println("4");
      if (accel_y_avg - sample_old > step_size || accel_y_avg - sample_old < -step_size) {
        sample_new = accel_y_avg;
        Serial.println("4.1");
        if (sample_old > dy_thres_accel_y && sample_new < dy_thres_accel_y) {
          Serial.println("4.2");
          step_count++;
          step_changed = 1;
        }
      }
      break;
    case 2:
      Serial.println("5");
      if (accel_z_avg - sample_old > step_size || accel_z_avg - sample_old < -step_size) {
        sample_new = accel_z_avg;
        Serial.println("5.1");
        if (sample_old > dy_thres_accel_z && sample_new < dy_thres_accel_z) {
          Serial.println("5.2");
          step_count++;
          step_changed = 1;
        }
      }
      break;
  }

  if (step_changed) {
    Serial.print("Step Counter change to:");
    Serial.println(step_count);
    step_changed = 0;
  }
}

void loop() {
  // if (mpu.getMotionInterruptStatus()) {
  step_counter();
  // Serial.println("Calling step Counter");
  // }
  vTaskDelay(10 / portTICK_PERIOD_MS);
}