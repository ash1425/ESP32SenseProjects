/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "speech_commands_action.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"

#include "servo.h"
#include "clap_detect.h"
#include "driver/usb_serial_jtag.h"
#include "driver/gpio.h"

#define LED_GPIO 21

#define CMD_GOOD_BOY 0
#define CMD_SIT_DOWN 1
#define CMD_LIE_DOWN 2
#define CMD_STRETCH 3
#define CMD_WALK 4
#define CMD_DANCE 5
#define CMD_WIGGLE 99
#define CMD_TEST   98
#define CMD_SOLO_FL 90
#define CMD_SOLO_FR 91
#define CMD_SOLO_BL 92
#define CMD_SOLO_BR 93

int wakeup_flag = 0;
static volatile int clap_wakeup = 0;
static const esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
srmodel_list_t *models = NULL;

QueueHandle_t servo_cmd_queue;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch == feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag)
    {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        if (!wakeup_flag && clap_detect_feed(i2s_buff, audio_chunksize, feed_channel))
        {
            clap_wakeup = 1;
        }

        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff)
    {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    printf("multinet:%s\n", mn_name);
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 6000);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data); // Add speech commands from sdkconfig
    assert(mu_chunksize == afe_chunksize);
    // print active speech commands
    multinet->print_active_speech_commands(model_data);

    printf("============ PET DOG v2 — clap + wiggle ============\n");
    printf("------------detect start------------\n");
    while (task_flag)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL)
        {
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED)
        {
            printf("WAKEWORD DETECTED\n");
            multinet->clean(model_data);
        }

        if (res->raw_data_channels == 1 && res->wakeup_state == WAKENET_DETECTED)
        {
            wakeup_flag = 1;
            gpio_set_level(LED_GPIO, 1);
            int w = CMD_WIGGLE;
            xQueueSend(servo_cmd_queue, &w, 0);
        }
        else if (res->raw_data_channels > 1 && res->wakeup_state == WAKENET_CHANNEL_VERIFIED)
        {
            printf("AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
            wakeup_flag = 1;
            gpio_set_level(LED_GPIO, 1);
            int w = CMD_WIGGLE;
            xQueueSend(servo_cmd_queue, &w, 0);
        }

        if (clap_wakeup == 1 && wakeup_flag == 0)
        {
            printf("CLAP WAKEUP — listening for commands\n");
            afe_handle->disable_wakenet(afe_data);
            multinet->clean(model_data);
            wakeup_flag = 1;
            gpio_set_level(LED_GPIO, 1);
            clap_wakeup = 0;
            int w = CMD_WIGGLE;
            xQueueSend(servo_cmd_queue, &w, 0);
        }

        if (wakeup_flag == 1)
        {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING)
            {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++)
                {
                    printf("TOP %d, command_id: %d, phrase_id: %d, string: %s, prob: %f\n",
                           i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                }

                // Send top command_id to servo task — non-blocking
                int cmd_id = mn_result->command_id[0];
                xQueueSend(servo_cmd_queue, &cmd_id, 0);
                printf("-----------listening-----------\n");
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT)
            {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                printf("timeout, string:%s\n", mn_result->string);
                afe_handle->enable_wakenet(afe_data);
                wakeup_flag = 0;
                gpio_set_level(LED_GPIO, 0);
                printf("\n-----------awaits to be waken up-----------\n");
                continue;
            }
        }
    }
    if (model_data)
    {
        multinet->destroy(model_data);
        model_data = NULL;
    }
    printf("detect exit\n");
    vTaskDelete(NULL);
}

void servo_Task(void *arg)
{
    int cmd_id;
    while (1)
    {
        // Block here until a command arrives — no CPU waste
        if (xQueueReceive(servo_cmd_queue, &cmd_id, portMAX_DELAY))
        {
            printf("servo_Task received command: %d\n", cmd_id);
            switch (cmd_id)
            {
            case CMD_GOOD_BOY:
                anim_good_boy();
                break;
            case CMD_SIT_DOWN:
                anim_sit_down();
                break;
            case CMD_LIE_DOWN:
                anim_lie_down();
                break;
            case CMD_STRETCH:
                anim_stretch();
                break;
            case CMD_WALK:
                anim_walk();
                break;
            case CMD_DANCE:
                anim_dance();
                break;
            case CMD_WIGGLE:
                anim_wiggle();
                break;
            case CMD_TEST:
                anim_test();
                break;
            case CMD_SOLO_FL:
                anim_solo(0);
                break;
            case CMD_SOLO_FR:
                anim_solo(1);
                break;
            case CMD_SOLO_BL:
                anim_solo(2);
                break;
            case CMD_SOLO_BR:
                anim_solo(3);
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

void serial_cmd_Task(void *arg)
{
    usb_serial_jtag_driver_config_t usb_cfg = {
        .tx_buffer_size = 256,
        .rx_buffer_size = 256,
    };
    usb_serial_jtag_driver_install(&usb_cfg);

    uint8_t buf[1];
    while (1)
    {
        int len = usb_serial_jtag_read_bytes(buf, 1, pdMS_TO_TICKS(100));
        if (len <= 0)
            continue;

        int c = buf[0];
        if (c >= '0' && c <= '5')
        {
            int cmd_id = c - '0';
            printf("Serial command: %d\n", cmd_id);
            xQueueSend(servo_cmd_queue, &cmd_id, 0);
        }
        else if (c == '8')
        {
            int t = CMD_TEST;
            printf("Serial command: test (all servos full swing)\n");
            xQueueSend(servo_cmd_queue, &t, 0);
        }
        else if (c == '9')
        {
            int w = CMD_WIGGLE;
            printf("Serial command: wiggle\n");
            xQueueSend(servo_cmd_queue, &w, 0);
        }
        else if (c == 'a' || c == 'A')
        {
            int s = CMD_SOLO_FL;
            printf("Serial command: solo FL\n");
            xQueueSend(servo_cmd_queue, &s, 0);
        }
        else if (c == 'b' || c == 'B')
        {
            int s = CMD_SOLO_FR;
            printf("Serial command: solo FR\n");
            xQueueSend(servo_cmd_queue, &s, 0);
        }
        else if (c == 'c' || c == 'C')
        {
            int s = CMD_SOLO_BL;
            printf("Serial command: solo BL\n");
            xQueueSend(servo_cmd_queue, &s, 0);
        }
        else if (c == 'd' || c == 'D')
        {
            int s = CMD_SOLO_BR;
            printf("Serial command: solo BR\n");
            xQueueSend(servo_cmd_queue, &s, 0);
        }
    }
}

void app_main()
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    printf("STEP 1 — servo_init\n");
    servo_init();
    printf("STEP 2 — servo_all_neutral\n");
    servo_all_neutral();
    printf("STEP 3 — clap_detect_init\n");
    clap_detect_init(16000);

    servo_cmd_queue = xQueueCreate(5, sizeof(int));

    printf("STEP 4 — srmodel_init\n");
    models = esp_srmodel_init("model");
    printf("STEP 5 — esp_board_init\n");
    ESP_ERROR_CHECK(esp_board_init(16000, 2, 16));
    printf("STEP 6 — board_init done\n");
    // ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

#if CONFIG_IDF_TARGET_ESP32
    printf("This demo only support ESP32S3\n");
    return;
#else
    printf("STEP 7 — AFE config\n");
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_handle = esp_afe_handle_from_config(afe_config);
    printf("STEP 8 — AFE create\n");
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);
    printf("STEP 9 — AFE ready\n");
#endif

    task_flag = 1;
    printf("STEP 10 — creating tasks\n");
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void *)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(servo_Task, "servo", 4 * 1024, NULL, 5, NULL, 0);
    xTaskCreate(serial_cmd_Task, "serial_cmd", 2 * 1024, NULL, 3, NULL);

    printf("STEP 11 — all tasks started\n");
    printf("============ PET DOG v2 — clap + wiggle ============\n");
    printf("Serial commands: 0=good boy  1=sit  2=lie down  3=stretch  4=walk  5=dance  8=test  9=wiggle\n");
    printf("Solo servo:      a=FL  b=FR  c=BL  d=BR\n");
}
