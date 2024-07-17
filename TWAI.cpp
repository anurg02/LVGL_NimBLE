#include <Arduino.h>
#include <lvgl.h>
#include "driver/twai.h"

static void drawserialprints(const char *text)
{
    lv_textarea_add_text(serTextarea, text);
    lv_obj_align(serTextarea, LV_ALIGN_TOP_LEFT, 10, 0);
}
static void handle_rx_message(twai_message_t &message)
{
    uint8_t msglen = message.data_length_code;
    char datastr[msglen + 2];
    if (message.extd)
    {
        drawserialprints("Message is in Extended Format\n");
    }
    else
    {
        drawserialprints("Message is in Standard Format\n");
    }
    // getting transmitter's id
    uint32_t id = message.identifier;
    char myid[12];
    sprintf(myid, "%x\n", id);
    drawserialprints("receiving msgs from id: ");
    drawserialprints(myid);
    if (!(message.rtr))
    {
        for (int i = 0; i < msglen; i++)
        {
            datastr[i] = (char)message.data[i];
        }
        datastr[msglen] = '\n';
        datastr[msglen + 1] = '\0';
        drawserialprints(datastr);
    }
}
void twaidrvinit()
{
    // TWAI init
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_43, GPIO_NUM_44, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        drawserialprints("driver installed\n");
    }
    else
    {
        drawserialprints("Failed to install driver\n");
        return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        driver_installed = true;
        drawserialprints("Driver started\n");
    }
    else
    {
        drawserialprints("Failed to start driver\n");
        return;
    }
}
void transmitCB(lv_event_t *event)
{
    twai_message_t transmitmsg;
    transmitmsg.identifier = 0x16;
    transmitmsg.extd = 0;
    transmitmsg.data_length_code = 8;

    transmitmsg.data[0] = 'A';
    transmitmsg.data[1] = 'N';
    transmitmsg.data[2] = 'U';
    transmitmsg.data[3] = 'R';
    transmitmsg.data[4] = 'A';
    transmitmsg.data[5] = 'G';
    transmitmsg.data[6] = '0';
    transmitmsg.data[7] = '2';
    // Queue message for transmission
    esp_err_t trcode = twai_transmit(&transmitmsg, pdMS_TO_TICKS(0));
    if (trcode == ESP_OK)
    {
        drawserialprints("Message queued for transmission\n");
    }
    else
    {
        char transcode[6];
        sprintf(transcode, "%x\n", trcode);
        drawserialprints(transcode);
        drawserialprints("Failed to queue message for transmission\n");
    }
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600); // Init Display'
    Serial.println("here");

    // WiFi.begin("sanjay", "sanjaydeshpande");
    // while (WiFi.status() != WL_CONNECTED)
    // {
    //   Serial.print(".");
    //   delay(500);
    // }
    // Serial.println("connected");
    lcd->begin();
    lcd->fillScreen(BLACK);
    lcd->setTextSize(2);
    delay(200);
    lv_init();
    touch_init();
    screenWidth = lcd->width();
    screenHeight = lcd->height();

    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 10);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the (dummy) input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif
    lcd->fillScreen(BLACK);
    // createscreenOne();
    create_screen();
    serTextarea = lv_textarea_create(mainPanel);
    Transmitbtn = lv_btn_create(mainPanel);
    lv_obj_align(Transmitbtn, LV_ALIGN_TOP_RIGHT, -10, 0);
    lv_obj_add_event_cb(Transmitbtn, transmitCB, LV_EVENT_CLICKED, NULL);
    lv_obj_t *transmit = lv_label_create(Transmitbtn);
    lv_label_set_text(transmit, "Transmit");

    twaidrvinit();

}

void loop()
{
    lv_timer_handler();
    if (driver_installed)
    {
        twai_message_t message;
        esp_err_t code = twai_receive(&message, pdMS_TO_TICKS(0));
        if (code == ESP_OK)
        {
            handle_rx_message(message);
        }
    }
    else
    {
        drawserialprints("Driver not installed\n");
    }

    delay(5);
}