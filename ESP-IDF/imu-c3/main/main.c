#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "icm42670.h"

#define TAG "MAIN_ICM42670"

// Configuração do I2C para ESP32-C3
#define I2C_MASTER_SCL_IO           8    // Pino SCL
#define I2C_MASTER_SDA_IO           7    // Pino SDA
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000

// Endereço do ICM42670 no barramento I2C
#define ICM42670_ADDR               ICM42670_I2C_ADDRESS

static icm42670_handle_t imu_handle = NULL;

// Inicializa o I2C
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    return ESP_OK;
}

// Tarefa para ler os dados do IMU
static void imu_task(void *arg)
{
    icm42670_value_t acce;
    icm42670_value_t gyro;
    complimentary_angle_t angle = {0};
    float temp;

    while (1) {
        if (icm42670_get_acce_value(imu_handle, &acce) != ESP_OK) {
            ESP_LOGE(TAG, "Falha leitura acelerômetro");
            continue;
        }
        if (icm42670_get_gyro_value(imu_handle, &gyro) != ESP_OK) {
            ESP_LOGE(TAG, "Falha leitura giroscópio");
            continue;
        }
        if (icm42670_get_temp_value(imu_handle, &temp) != ESP_OK) {
            ESP_LOGE(TAG, "Falha leitura temperatura");
        }

        // Calcula ângulos com filtro complementar
        if (icm42670_complimentory_filter(imu_handle, &acce, &gyro, &angle) != ESP_OK) {
            ESP_LOGE(TAG, "Falha no cálculo do filtro complementar");
        }

        ESP_LOGI(TAG, "ACCEL  X: %.2f g  Y: %.2f g  Z: %.2f g", acce.x, acce.y, acce.z);
        ESP_LOGI(TAG, "GYRO   X: %.2f dps Y: %.2f dps Z: %.2f dps", gyro.x, gyro.y, gyro.z);
        ESP_LOGI(TAG, "TEMP: %.2f °C", temp);
        ESP_LOGI(TAG, "ROLL: %.2f°  PITCH: %.2f°", angle.roll, angle.pitch);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());

    // Cria handle do sensor
    imu_handle = icm42670_create(I2C_MASTER_NUM, ICM42670_ADDR);
    if (imu_handle == NULL) {
        ESP_LOGE(TAG, "Falha ao criar handle do ICM42670");
        return;
    }

    // Configuração inicial do IMU
    icm42670_cfg_t cfg = {
        .gyro_fs = GYRO_FS_2000DPS,
        .gyro_odr = GYRO_ODR_100HZ,
        .acce_fs = ACCE_FS_16G,
        .acce_odr = ACCE_ODR_100HZ
    };

    if (icm42670_config(imu_handle, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao configurar sensor");
        icm42670_delete(imu_handle);
        return;
    }

    if (icm42670_acce_set_pwr(imu_handle, ACCE_PWR_LOWNOISE) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ligar acelerômetro");
    }

    if (icm42670_gyro_set_pwr(imu_handle, GYRO_PWR_LOWNOISE) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ligar giroscópio");
    }

    ESP_LOGI(TAG, "Sensor inicializado com sucesso!");

    // Cria tarefa para leitura periódica
    xTaskCreate(imu_task, "imu_task", 4096, NULL, 5, NULL);
}
