/**
 * The example demonstrates how to port LVGL.
 *
 * ## How to Use
 *
 * To use this example, please firstly install `ESP32_Display_Panel` (including its dependent libraries) and
 * `lvgl` (v8.3.x) libraries, then follow the steps to configure them:
 *
 * 1. [Configure ESP32_Display_Panel](https://github.com/esp-arduino-libs/ESP32_Display_Panel#configure-esp32_display_panel)
 * 2. [Configure LVGL](https://github.com/esp-arduino-libs/ESP32_Display_Panel#configure-lvgl)
 * 3. [Configure Board](https://github.com/esp-arduino-libs/ESP32_Display_Panel#configure-board)
 *
 * ## Example Output
 *
 * ```bash
 * ...
 * Hello LVGL! V8.3.8
 * I am ESP32_Display_Panel
 * Starting LVGL task
 * Setup done
 * Loop
 * Loop
 * Loop
 * Loop
 * ...
 * ```
 */

#include <Arduino.h>
#include <lvgl.h>
#include <lv_conf.h>
#include <ESP_Panel_Library.h>
#include <ESP_IOExpander_Library.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include "uifile.h"
#include "driver/twai.h"
// Extend IO Pin define
#define TP_RST 1
#define LCD_BL 2
#define LCD_RST 3
#define SD_CS 4
#define USB_SEL 5
#define CAN_RX 19
#define CAN_TX 20
// I2C Pin define
#define I2C_MASTER_NUM 0
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_SCL_IO 9

#define GREEN 1
#define RED 0
typedef enum
{
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

static disp_size_t disp_size;
static const lv_font_t *font_large;
static const lv_font_t *font_normal;
uint8_t ctr;
bool startst_123 = true;
bool stopst_123 = true;
bool startst_126 = true;
bool stopst_126 = true;

static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;

static bool driver_installed;

/**
/* To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 * You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 */
// #include <demos/lv_demos.h>
// #include <examples/lv_examples.h>

// function prototypes

void vTaskdrawimage1();
void vTaskdrawimage2();

SemaphoreHandle_t msgsemaphore = xSemaphoreCreateMutex();
/* LVGL porting configurations */
#define LVGL_TICK_PERIOD_MS (2)
#define LVGL_TASK_MAX_DELAY_MS (500)
#define LVGL_TASK_MIN_DELAY_MS (1)
#define LVGL_TASK_STACK_SIZE (4 * 1024)
#define LVGL_TASK_PRIORITY (2)
#define LVGL_BUF_SIZE (ESP_PANEL_LCD_H_RES * 20)

ESP_Panel *panel = NULL;
SemaphoreHandle_t lvgl_mux = NULL; // LVGL mutex

#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
/* Display flushing */
void lvgl_port_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp);
}
#else
/* Display flushing */
void lvgl_port_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
}

bool notify_lvgl_flush_ready(void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}
#endif /* ESP_PANEL_LCD_BUS_TYPE */

#if ESP_PANEL_USE_LCD_TOUCH
/* Read the touchpad */
void lvgl_port_tp_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    panel->getLcdTouch()->readData();

    bool touched = panel->getLcdTouch()->getTouchState();
    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        TouchPoint point = panel->getLcdTouch()->getPoint();

        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates*/
        data->point.x = point.x;
        data->point.y = point.y;

        Serial.printf("Touch point: x %d, y %d\n", point.x, point.y);
    }
}
#endif

void lvgl_port_lock(int timeout_ms)
{
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks);
}

void lvgl_port_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}

void lvgl_port_task(void *arg)
{
    Serial.println("Starting LVGL task");

    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    while (1)
    {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        lvgl_port_lock(-1);
        task_delay_ms = lv_timer_handler();
        // Release the mutex
        lvgl_port_unlock();
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS)
        {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        }
        else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS)
        {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void Init_WS_Panel()
{
    if (LV_HOR_RES <= 320)
        disp_size = DISP_SMALL;
    else if (LV_HOR_RES < 720)
        disp_size = DISP_MEDIUM;
    else
        disp_size = DISP_LARGE;

    font_large = LV_FONT_DEFAULT;
    font_normal = LV_FONT_DEFAULT;

    lv_coord_t tab_h;
    if (disp_size == DISP_LARGE)
    {
        tab_h = 70;
#if LV_FONT_MONTSERRAT_24
        font_large = &lv_font_montserrat_24;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
        font_normal = &lv_font_montserrat_16;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else if (disp_size == DISP_MEDIUM)
    {
        tab_h = 45;
#if LV_FONT_MONTSERRAT_20
        font_large = &lv_font_montserrat_20;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_20 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_14
        font_normal = &lv_font_montserrat_14;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_14 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else
    { /* disp_size == DISP_SMALL */
        tab_h = 45;
#if LV_FONT_MONTSERRAT_18
        font_large = &lv_font_montserrat_18;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_12
        font_normal = &lv_font_montserrat_12;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }

#if LV_USE_THEME_DEFAULT
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                          font_normal);
#endif

    Serial.println("Init WS Panel 1");
    lv_style_init(&style_text_muted);
    lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

    Serial.println("Init WS Panel 2");
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_large);
    /*
        Serial.println("Init WS Panel 3");
        lv_style_init(&style_icon);
        lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
        lv_style_set_text_font(&style_icon, font_large);
    */
    Serial.println("Init WS Panel 4");
    lv_style_init(&style_bullet);
    lv_style_set_border_width(&style_bullet, 0);
    lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

    Serial.println("Init WS Panel 5");
}
static void transmitCB(lv_event_t *event)
{
    twai_message_t transmitmsg;
    const char *datastr;
    datastr = lv_textarea_get_text(serTextarea);

    transmitmsg.identifier = 0x32;
    transmitmsg.extd = 0;
    transmitmsg.data_length_code = 8;

    strcpy((char *)transmitmsg.data, datastr);

    // Queue message for transmission
    esp_err_t trcode = twai_transmit(&transmitmsg, pdMS_TO_TICKS(0));
    if (trcode == ESP_OK)
    {
        terminal("Message queued for transmission\n");
        Serial.println("Message queued for transmission\n");
    }
    else
    {
        char transcode[6];
        sprintf(transcode, "%x\n", trcode);
        Serial.println(transcode);
        Serial.println("Failed to queue message for transmission\n");
    }
    lv_textarea_set_text(serTextarea, "");
}
static void transmit2CB(lv_event_t *e)
{
    twai_message_t transmitmsg;
    transmitmsg.identifier = 0x34;
    transmitmsg.extd = 0;
    transmitmsg.data_length_code = 8;

    const char *datastr;
    datastr = lv_textarea_get_text(serTextarea);

    strcpy((char *)transmitmsg.data, datastr);

    // Queue message for transmission
    esp_err_t trcode = twai_transmit(&transmitmsg, pdMS_TO_TICKS(0));
    if (trcode == ESP_OK)
    {
        terminal("Message queued for transmission\n");
        Serial.println("Message queued for transmission\n");
    }
    else
    {
        char transcode[6];
        sprintf(transcode, "%x\n", trcode);
        Serial.println(transcode);
        Serial.println("Failed to queue message for transmission\n");
        terminal(transcode);
        terminal("Failed to queue message for transmission\n");
    }
    lv_textarea_set_text(serTextarea, "");
}
static void transmitst1(lv_event_t *e)
{
    lv_obj_t *parentbtn = lv_event_get_target(e);
    if (parentbtn == startbtn1)
    {
        startst_123 = 0;
    }
    else if (parentbtn == stopbtn1)
    {
        stopst_123 = 0;
    }

    twai_message_t transmitmsg;

    transmitmsg.identifier = 0x21;
    transmitmsg.extd = 0;
    transmitmsg.data_length_code = 3;

    transmitmsg.data[0] = ctr;
    transmitmsg.data[1] = startst_123;
    transmitmsg.data[2] = stopst_123;

    // Queue message for transmission
    esp_err_t trcode = twai_transmit(&transmitmsg, pdMS_TO_TICKS(0));
    if (trcode == ESP_OK)
    {
        terminal("Message queued for transmission\n");
        Serial.println("Message queued for transmission\n");
    }
    else
    {
        char transcode[6];
        sprintf(transcode, "%x\n", trcode);
        Serial.println(transcode);
        Serial.println("Failed to queue message for transmission\n");
    }
    vTaskdrawimage1();
    lv_textarea_set_text(serTextarea, "");
}
static void transmitst2(lv_event_t *e)
{
    lv_obj_t *parentbtn = lv_event_get_target(e);
    if (parentbtn == startbtn2)
    {
        startst_126 = 0;
    }
    else if (parentbtn == stopbtn2)
    {
        stopst_126 = 0;
    }

    twai_message_t transmitmsg;

    transmitmsg.identifier = 0x22;
    transmitmsg.extd = 0;
    transmitmsg.data_length_code = 3;

    transmitmsg.data[0] = ctr;
    transmitmsg.data[1] = startst_126;
    transmitmsg.data[2] = stopst_126;

    // Queue message for transmission
    esp_err_t trcode = twai_transmit(&transmitmsg, pdMS_TO_TICKS(0));
    if (trcode == ESP_OK)
    {
        terminal("Message queued for transmission\n");
        Serial.println("Message queued for transmission\n");
    }
    else
    {
        char transcode[6];
        sprintf(transcode, "%x\n", trcode);
        Serial.println(transcode);
        Serial.println("Failed to queue message for transmission\n");
    }
    vTaskdrawimage2();
    lv_textarea_set_text(serTextarea, "");
}
static void twaidrvinit()
{

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // g_config.alerts_enabled = TWAI_ALERT_ALL;

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        terminal("Driver installed\n");
        Serial.println("Driver installed\n");
    }
    else
    {
        Serial.println("Failed to install driver\n");
        return;
    }

    if (twai_start() == ESP_OK)
    {
        terminal("Driver started\n");
        Serial.println("Driver started\n");
        driver_installed = true;

        if (twai_reconfigure_alerts(TWAI_ALERT_ALL, NULL) == ESP_OK)
        {
            Serial.println("alerts enabled\n");
        }
        else
        {
            Serial.println("Couldn't enable alerts\n");
        }

        twai_status_info_t status_info;
        if (twai_get_status_info(&status_info) == ESP_OK)
        {
            Serial.println("Bus state: ");
            switch (status_info.state)
            {
            case TWAI_STATE_RUNNING:
                Serial.println("Running");
                break;
            case TWAI_STATE_STOPPED:
                Serial.println("Stopped");
                break;
            case TWAI_STATE_BUS_OFF:
                Serial.println("Bus Off");
                break;
            case TWAI_STATE_RECOVERING:
                Serial.println("Recovering");
                break;
            default:
                Serial.println("Unknown");
                break;
            }
        }
        else
        {
            Serial.println("Failed to get bus status");
        }
    }
    else
    {
        Serial.println("Failed to start driver\n");
        return;
    }
}
void vTaskdrawimage1()
{
    // while (1)
    // {
    if (startst_123 == 0)
    {
        if (lvimg != nullptr)
        {
            lv_obj_del(lvimg);
            lvimg = nullptr;
        }
        lvimg = imageCreate(tab1, GREEN, LV_ALIGN_TOP_LEFT, 10, 0);
    }
    if (stopst_123 == 0)
    {
        if (lvimg != nullptr)
        {
            lv_obj_del(lvimg);
            lvimg = nullptr;
        }
        lvimg = imageCreate(tab1, RED, LV_ALIGN_TOP_LEFT, 10, 0);
    }
    //     vTaskDelay(pdMS_TO_TICKS(20));
    // }
}
void vTaskdrawimage2()
{
    // while (1)
    // {
    if (startst_126 == 0)
    {
        if (lvimg2 != nullptr)
        {
            lv_obj_del(lvimg2);
            lvimg2 = nullptr;
        }
        lvimg2 = imageCreate(tab1, GREEN, LV_ALIGN_TOP_RIGHT, -20, 0);
    }
    if (stopst_126 == 0)
    {
        if (lvimg2 != nullptr)
        {
            lv_obj_del(lvimg2);
            lvimg2 = nullptr;
        }
        lvimg2 = imageCreate(tab1, RED, LV_ALIGN_TOP_RIGHT, -20, 0);
    }
    // vTaskDelay(pdMS_TO_TICKS(20));
    //}
}
void handlemsg1(twai_message_t message)
{
    uint8_t msglen = message.data_length_code;
    char datastr[msglen + 2], datactr[3];

    // getting transmitter's id
    uint32_t id = message.identifier;
    char myid[12];
    sprintf(myid, "%x\n", id);
    // Serial.print("receiving msgs from id: ");
    // Serial.println(myid);

    if (!(message.rtr))
    {
        ctr = message.data[0];
        startst_123 = message.data[1];
        stopst_123 = message.data[2];
        sprintf(datactr, "%d", ctr);
        for (int i = 0; i < message.data_length_code; i++)
        {
            Serial.printf("the data at index %d is: ", i);
            Serial.println(message.data[i]);
        }
        if (xSemaphoreTake(msgsemaphore, portMAX_DELAY) == pdTRUE)
        {
            lv_label_set_text(ctrlabelL, datactr);
            vTaskdrawimage1();
            xSemaphoreGive(msgsemaphore);
        }
        
    }
}
void handlemsg2(twai_message_t message)
{
    uint8_t ctr;
    uint8_t msglen = message.data_length_code;
    char datastr[msglen + 2], datactr[3];

    // getting transmitter's id
    uint32_t id = message.identifier;
    char myid[12];
    sprintf(myid, "%x\n", id);
    // Serial.print("receiving msgs from id: ");
    // Serial.println(myid);

    if (!(message.rtr))
    {
        ctr = message.data[0];
        startst_126 = message.data[1];
        stopst_126 = message.data[2];
        sprintf(datactr, "%d", ctr);
        for (int i = 0; i < message.data_length_code; i++)
        {
            Serial.printf("the data at index %d is: ", i);
            Serial.println(message.data[i]);
        }
        if (xSemaphoreTake(msgsemaphore, portMAX_DELAY) == pdTRUE)
        {
            lv_label_set_text(ctrlabelR, datactr);
            vTaskdrawimage2();
            xSemaphoreGive(msgsemaphore);
        }
        
    }
}
void vTaskHandleMsg1(void *pvParameters)
{
    while (1)
    {
        twai_message_t messagerx1;

        esp_err_t rxcode = twai_receive(&messagerx1, pdMS_TO_TICKS(400));
        Serial.println("task 1");
        if (rxcode == ESP_OK && messagerx1.identifier == 0x123)
        {
            handlemsg1(messagerx1);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void vTaskHandleMsg2(void *pvParameters)
{
    while (1)
    {
        twai_message_t messagerx2;
        esp_err_t rxcode = twai_receive(&messagerx2, pdMS_TO_TICKS(400));
        Serial.println("task 2");
        if (rxcode == ESP_OK && messagerx2.identifier == 0x126)
        {
            handlemsg2(messagerx2);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void alertread()
{
    uint32_t alert;
    if (twai_read_alerts(&alert, pdMS_TO_TICKS(100)) == ESP_OK)
    {
        char Alerts[12];
        sprintf(Alerts, "%x\n", alert);
        Serial.print("The alert is: ");
        Serial.println(Alerts);
    }
}
void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello LVGL! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println(LVGL_Arduino);
    Serial.println("I am ESP32_Display_Panel");

    Init_WS_Panel();
    Serial.println("after Init WS Panel ");

    panel = new ESP_Panel();

    /* Initialize LVGL core */
    lv_init();

    /* Initialize LVGL buffers */
    static lv_disp_draw_buf_t draw_buf;
    /* Using double buffers is more faster than single buffer */
    /* Using internal SRAM is more fast than PSRAM (Note: Memory allocated using `malloc` may be located in PSRAM.) */
    uint8_t *buf = (uint8_t *)heap_caps_calloc(1, LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
    assert(buf);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_BUF_SIZE);

    /* Initialize the display device */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = ESP_PANEL_LCD_H_RES;
    disp_drv.ver_res = ESP_PANEL_LCD_V_RES;
    disp_drv.flush_cb = lvgl_port_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

#if ESP_PANEL_USE_LCD_TOUCH
    /* Initialize the input device */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_port_tp_read;
    lv_indev_drv_register(&indev_drv);
#endif
    /* Initialize bus and device of panel */
    panel->init();
#if ESP_PANEL_LCD_BUS_TYPE != ESP_PANEL_BUS_TYPE_RGB
    /* Register a function to notify LVGL when the panel is ready to flush */
    /* This is useful for refreshing the screen using DMA transfers */
    panel->getLcd()->setCallback(notify_lvgl_flush_ready, &disp_drv);
#endif

    /**
     * These development boards require the use of an IO expander to configure the screen,
     * so it needs to be initialized in advance and registered with the panel for use.
     *
     */
    Serial.println("Initialize IO expander");
    /* Initialize IO expander */
    // ESP_IOExpander *expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    ESP_IOExpander *expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
    expander->init();
    expander->begin();
    expander->multiPinMode(TP_RST | LCD_BL | LCD_RST | SD_CS | USB_SEL, OUTPUT);
    expander->multiDigitalWrite(TP_RST | LCD_BL | LCD_RST | SD_CS, HIGH);

    // Turn off backlight
    // expander->digitalWrite(LCD_BL, LOW);

    // When USB_SEL is HIGH, it enables FSUSB42UMX chip and gpio19,gpio20 wired CAN_TX CAN_RX, and then dont use USB Function
    expander->digitalWrite(USB_SEL, HIGH);
    /* Add into panel */
    panel->addIOExpander(expander);

    /* Start panel */
    panel->begin();

    /* Create a task to run the LVGL task periodically */
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    xTaskCreatePinnedToCore(lvgl_port_task, "lvgl", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 0);

    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    lvgl_port_lock(-1);
    /**
     * Try an example. Don't forget to uncomment header.
     * See all the examples online: https://docs.lvgl.io/master/examples.html
     * source codes: https://github.com/lvgl/lvgl/tree/e7f88efa5853128bf871dde335c0ca8da9eb7731/examples
     */
    //  lv_example_btn_1();

    /**
     * Or try out a demo.
     * Don't forget to uncomment header and enable the demos in `lv_conf.h`. E.g. `LV_USE_DEMOS_WIDGETS`
     */
    // lv_demo_widgets();
    createScreen();
    styleinit();
    kb = lv_keyboard_create(lv_scr_act());
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    Serial.println("here 1");
    if (tab1 != nullptr)
    {
        Serial.println("tab 1 not nullptr");
        serTextarea = createTextarea(tab1, "Write here to transmit", LV_ALIGN_BOTTOM_LEFT, LV_HOR_RES * 2 / 3, LV_VER_RES * 1 / 6, 10, -40);
        Serial.println("here 2");
        lv_obj_add_event_cb(serTextarea, keyboardEvent, LV_EVENT_ALL, kb);
        Serial.println("here 3");
        terminaltxt = createTextarea(tab1, "terminal", LV_ALIGN_CENTER, LV_HOR_RES - 40, LV_VER_RES * 1 / 6, 0, -10);
        Serial.println("here 4");
        transmitbutton = createbtn(tab1, "ToNode-1", LV_ALIGN_BOTTOM_RIGHT, -10, -100);
        transmitbutton2 = createbtn(tab1, "ToNode-2", LV_ALIGN_BOTTOM_RIGHT, -10, -40);
        Serial.println("here 5");
        lv_obj_add_event_cb(transmitbutton, transmitCB, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(transmitbutton2, transmit2CB, LV_EVENT_CLICKED, NULL);
        ctrbtnl = createbtn(tab1, NULL, LV_ALIGN_TOP_LEFT, 150, 10);
        ctrbtnr = createbtn(tab1, NULL, LV_ALIGN_TOP_RIGHT, -150, 10);
        ctrlabelL = lv_obj_get_child(ctrbtnl, 0);
        ctrlabelR = lv_obj_get_child(ctrbtnr, 0);
        Serial.println("here 6");
        startbtn1 = createbtn(tab1, "Start", LV_ALIGN_TOP_LEFT, 20, 80);
        stopbtn1 = createbtn(tab1, "Stop", LV_ALIGN_TOP_LEFT, 100, 80);
        startbtn2 = createbtn(tab1, "Start", LV_ALIGN_TOP_RIGHT, -100, 80);
        stopbtn2 = createbtn(tab1, "Stop", LV_ALIGN_TOP_RIGHT, -20, 80);

        Serial.println("here 7");
        lv_obj_add_event_cb(startbtn1, transmitst1, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(stopbtn1, transmitst1, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(startbtn2, transmitst2, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(stopbtn2, transmitst2, LV_EVENT_CLICKED, NULL);
    }
    else
    {
        Serial.println("tab nullptr");
    }
    // CAN init
    twaidrvinit();
    if (driver_installed)
    {
        xTaskCreatePinnedToCore(vTaskHandleMsg1, "for id 0x123", 4096, NULL, 1, NULL, 1);
        xTaskCreatePinnedToCore(vTaskHandleMsg2, "for id 0x126", 4096, NULL, 1, NULL, 1);
        // xTaskCreatePinnedToCore(vTaskdrawimage1, "image 1", 2048, NULL, 1, NULL, 1);
        // xTaskCreatePinnedToCore(vTaskdrawimage2,"image 2", 2048, NULL , 1, NULL, 1);
    }
    else
    {
        Serial.println("driver not installed");
    }
    /* Release the mutex */
    lvgl_port_unlock();

    Serial.println("Setup done");
}
void loop()
{
    // if (driver_installed)
    // {
    //     twai_message_t mymessage;
    //     esp_err_t code = twai_receive(&mymessage, pdMS_TO_TICKS(0));
    //     if (code == ESP_OK && mymessage.identifier == 0x123)
    //     {
    //         handlemsg1(mymessage);
    //         // Serial.print("startst is: ");
    //         // Serial.println(startst);
    //         // Serial.print("stop is: ");
    //         // Serial.println(stopst);
    //     }
    // }
    sleep(1);
}