#include <lvgl.h>
#include <demos/lv_demos.h>
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf[800 * 480 / 10];
static lv_disp_drv_t disp_drv;
// UI
#include <ui.h>
static int first_flag = 0;
extern int zero_clean;
extern int goto_widget_flag;
extern int bar_flag;
extern lv_obj_t *ui_MENU;
extern lv_obj_t *ui_TOUCH;
extern lv_obj_t *ui_JIAOZHUN;
extern lv_obj_t *ui_Label2;
static lv_obj_t *ui_Label;  // TOUCH界面label
static lv_obj_t *ui_Label3; // TOUCH界面label3
static lv_obj_t *ui_Labe2;  // Menu界面进度条label
static lv_obj_t *bar;       // Menu界面进度条

#include <SPI.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <jsonfile.h>
#include <HTTPClient.h>
const char *wifissid = "sanjay";
const char *pass = "sanjaydeshpande";
const char *serverurl = "http://192.168.43.200:5000//get_json";

String jsonstring;
char jsonbuffer[2048];
HTTPClient client;
DynamicJsonDocument doc(2048);
SPIClass &spi = SPI;
uint16_t touchCalibration_x0 = 300, touchCalibration_x1 = 3600, touchCalibration_y0 = 300, touchCalibration_y1 = 3600;
uint8_t touchCalibration_rotate = 1, touchCalibration_invert_x = 2, touchCalibration_invert_y = 0;
static int val = 100;
#include <Ticker.h> //Call the ticker. H Library
Ticker ticker1;
Ticker ticker2;
int i = 0;
#include <Arduino_GFX_Library.h>
#define TFT_BL 2

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *lcd = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    41 /* DE */, 40 /* VSYNC */, 39 /* HSYNC */, 0 /* PCLK */,
    14 /* R0 */, 21 /* R1 */, 47 /* R2 */, 48 /* R3 */, 45 /* R4 */,
    9 /* G0 */, 46 /* G1 */, 3 /* G2 */, 8 /* G3 */, 16 /* G4 */, 1 /* G5 */,
    15 /* B0 */, 7 /* B1 */, 6 /* B2 */, 5 /* B3 */, 4 /* B4 */
);

// option 1:
// 7寸 50PIN 800*480
Arduino_RPi_DPI_RGBPanel *lcd = new Arduino_RPi_DPI_RGBPanel(
    bus,
    //  800 /* width */, 0 /* hsync_polarity */, 8/* hsync_front_porch */, 2 /* hsync_pulse_width */, 43/* hsync_back_porch */,
    //  480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 2/* vsync_pulse_width */, 12 /* vsync_back_porch */,
    //  1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

    //  800 /* width */, 0 /* hsync_polarity */, 210 /* hsync_front_porch */, 30 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
    //  480 /* height */, 0 /* vsync_polarity */, 22 /* vsync_front_porch */, 13 /* vsync_pulse_width */, 10 /* vsync_back_porch */,
    //  1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);
    //  800 /* width */, 1 /* hsync_polarity */, 80 /* hsync_front_porch */, 48 /* hsync_pulse_width */,  40/* hsync_back_porch */,
    //  480 /* height */, 1 /* vsync_polarity */, 50 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 31 /* vsync_back_porch */,
    //  0 /* pclk_active_neg */, 30000000 /* prefer_speed */, true /* auto_flush */);
    800 /* width */, 1 /* hsync_polarity */, 40 /* hsync_front_porch */, 48 /* hsync_pulse_width */, 40 /* hsync_back_porch */,
    480 /* height */, 1 /* vsync_polarity */, 13 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 31 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */, true /* auto_flush */);

#endif /* !defined(DISPLAY_DEV_KIT) */
#include "touch.h"

// function prototypes//
// Event callback for handling button clicks
void jsonCB(lv_event_t *e);

// Creates the initial screen with a button
void create_screen();

// Draws a button on the screen
void drawButton(lv_coord_t width, lv_coord_t height, lv_align_t align, lv_coord_t xoff, lv_coord_t yoff);

// Draws a text area (textbox) on the screen
void drawTextbox(const char *tempText, lv_coord_t w, lv_coord_t h, lv_align_t align, lv_coord_t xoff, lv_coord_t yoff);

// Draws a label on the screen
void drawlabel(const char *txt);

// Draws a switch on the screen
void drawswitch(lv_align_t align, lv_coord_t xoff, lv_coord_t yoff);

// Iterates through a JSON object to create UI elements based on the JSON data
void iterateJson(JsonVariant jsonVariant);

// Draws the display based on the JSON data
void drawdisplay();

// Gets JSON data from the server
void getJson();

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  lcd->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  lcd->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;

      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
      // Serial.print("Data x ");
      // Serial.println(data->point.x);
      // Serial.print("Data y ");
      // Serial.println(data->point.y);
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void callback1() // Callback function
{
  if (bar_flag == 6)
  {
    if (val > 1)
    {
      val--;
      lv_bar_set_value(bar, val, LV_ANIM_OFF);
      lv_label_set_text_fmt(ui_Labe2, "%d %%", val);
    }
    else
    {
      lv_obj_clear_flag(ui_touch, LV_OBJ_FLAG_CLICKABLE);
      lv_label_set_text(ui_Labe2, "Loading");
      delay(150);
      val = 100;
      bar_flag = 0;         // 停止进度条标志
      goto_widget_flag = 1; // 进入widget标志
    }
  }
}

// 触摸Label控件
void label_xy()
{
  ui_Label = lv_label_create(ui_TOUCH);
  lv_obj_enable_style_refresh(true);
  lv_obj_set_width(ui_Label, LV_SIZE_CONTENT);  /// 1
  lv_obj_set_height(ui_Label, LV_SIZE_CONTENT); /// 1
  lv_obj_set_x(ui_Label, -55);
  lv_obj_set_y(ui_Label, -40);
  lv_obj_set_align(ui_Label, LV_ALIGN_CENTER);
  lv_obj_set_style_text_color(ui_Label, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_Label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_Label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

  ui_Label3 = lv_label_create(ui_TOUCH);
  lv_obj_enable_style_refresh(true);
  lv_obj_set_width(ui_Label3, LV_SIZE_CONTENT);  /// 1
  lv_obj_set_height(ui_Label3, LV_SIZE_CONTENT); /// 1
  lv_obj_set_x(ui_Label3, 85);
  lv_obj_set_y(ui_Label3, -40);
  lv_obj_set_align(ui_Label3, LV_ALIGN_CENTER);
  lv_obj_set_style_text_color(ui_Label3, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_Label3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_Label3, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// 进度条控件
void lv_example_bar(void)
{
  //////////////////////////////
  bar = lv_bar_create(ui_MENU);
  lv_bar_set_value(bar, 0, LV_ANIM_OFF);
  lv_obj_set_width(bar, 480);
  lv_obj_set_height(bar, 25);
  lv_obj_set_x(bar, 0);
  lv_obj_set_y(bar, 175);
  lv_obj_set_align(bar, LV_ALIGN_CENTER);
  lv_obj_set_style_bg_img_src(bar, &ui_img_bar_800_01_png, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_style_bg_img_src(bar, &ui_img_bar_800_02_png, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_color(bar, lv_color_hex(0x2D8812), LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_outline_opa(bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  //////////////////////
  ui_Labe2 = lv_label_create(bar); // 创建标签
  lv_obj_set_style_text_color(ui_Labe2, lv_color_hex(0x09BEFB), LV_STATE_DEFAULT);
  lv_label_set_text(ui_Labe2, "0%");
  lv_obj_center(ui_Labe2);
}

#define Z_THRESHOLD 350 // Touch pressure threshold for validating touches
#define _RAWERR 20      // Deadband error allowed in successive position samples
void begin_touch_read_write(void)
{
  digitalWrite(38, HIGH); // Just in case it has been left low
  spi.setFrequency(600000);
  digitalWrite(38, LOW);
}

void end_touch_read_write(void)
{
  digitalWrite(38, HIGH); // Just in case it has been left low
  spi.setFrequency(600000);
}

uint16_t getTouchRawZ(void)
{

  begin_touch_read_write();

  // Z sample request
  int16_t tz = 0xFFF;
  spi.transfer(0xb0);              // Start new Z1 conversion
  tz += spi.transfer16(0xc0) >> 3; // Read Z1 and start Z2 conversion
  tz -= spi.transfer16(0x00) >> 3; // Read Z2

  end_touch_read_write();

  return (uint16_t)tz;
}

uint8_t getTouchRaw(uint16_t *x, uint16_t *y)
{
  uint16_t tmp;

  begin_touch_read_write();

  // Start YP sample request for x position, read 4 times and keep last sample
  spi.transfer(0xd0); // Start new YP conversion
  spi.transfer(0);    // Read first 8 bits
  spi.transfer(0xd0); // Read last 8 bits and start new YP conversion
  spi.transfer(0);    // Read first 8 bits
  spi.transfer(0xd0); // Read last 8 bits and start new YP conversion
  spi.transfer(0);    // Read first 8 bits
  spi.transfer(0xd0); // Read last 8 bits and start new YP conversion

  tmp = spi.transfer(0); // Read first 8 bits
  tmp = tmp << 5;
  tmp |= 0x1f & (spi.transfer(0x90) >> 3); // Read last 8 bits and start new XP conversion

  *x = tmp;

  // Start XP sample request for y position, read 4 times and keep last sample
  spi.transfer(0);    // Read first 8 bits
  spi.transfer(0x90); // Read last 8 bits and start new XP conversion
  spi.transfer(0);    // Read first 8 bits
  spi.transfer(0x90); // Read last 8 bits and start new XP conversion
  spi.transfer(0);    // Read first 8 bits
  spi.transfer(0x90); // Read last 8 bits and start new XP conversion

  tmp = spi.transfer(0); // Read first 8 bits
  tmp = tmp << 5;
  tmp |= 0x1f & (spi.transfer(0) >> 3); // Read last 8 bits

  *y = tmp;

  end_touch_read_write();

  return true;
}

uint8_t validTouch(uint16_t *x, uint16_t *y, uint16_t threshold)
{
  uint16_t x_tmp, y_tmp, x_tmp2, y_tmp2;

  // Wait until pressure stops increasing to debounce pressure
  uint16_t z1 = 1;
  uint16_t z2 = 0;
  while (z1 > z2)
  {
    z2 = z1;
    z1 = getTouchRawZ();
    delay(1);
    Serial.print("z1:");
    Serial.println(z1);
  }

  if (z1 <= threshold)
    return false;

  getTouchRaw(&x_tmp, &y_tmp);

  delay(1); // Small delay to the next sample
  if (getTouchRawZ() <= threshold)
    return false;

  delay(2); // Small delay to the next sample
  getTouchRaw(&x_tmp2, &y_tmp2);

  if (abs(x_tmp - x_tmp2) > _RAWERR)
    return false;
  if (abs(y_tmp - y_tmp2) > _RAWERR)
    return false;

  *x = x_tmp;
  *y = y_tmp;

  return true;
}

void calibrateTouch(uint16_t *parameters, uint32_t color_fg, uint32_t color_bg, uint8_t size)
{
  int16_t values[] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t x_tmp, y_tmp;
  uint16_t _width = 800;
  uint16_t _height = 480;

  for (uint8_t i = 0; i < 4; i++)
  {
    lcd->fillRect(0, 0, size + 1, size + 1, color_bg);
    lcd->fillRect(0, _height - size - 1, size + 1, size + 1, color_bg);
    lcd->fillRect(_width - size - 1, 0, size + 1, size + 1, color_bg);
    lcd->fillRect(_width - size - 1, _height - size - 1, size + 1, size + 1, color_bg);

    if (i == 5)
      break; // used to clear the arrows

    switch (i)
    {
    case 0: // up left
      lcd->drawLine(0, 0, 0, size, color_fg);
      lcd->drawLine(0, 0, size, 0, color_fg);
      lcd->drawLine(0, 0, size, size, color_fg);
      break;
    case 1: // bot left
      lcd->drawLine(0, _height - size - 1, 0, _height - 1, color_fg);
      lcd->drawLine(0, _height - 1, size, _height - 1, color_fg);
      lcd->drawLine(size, _height - size - 1, 0, _height - 1, color_fg);
      break;
    case 2: // up right
      lcd->drawLine(_width - size - 1, 0, _width - 1, 0, color_fg);
      lcd->drawLine(_width - size - 1, size, _width - 1, 0, color_fg);
      lcd->drawLine(_width - 1, size, _width - 1, 0, color_fg);
      break;
    case 3: // bot right
      lcd->drawLine(_width - size - 1, _height - size - 1, _width - 1, _height - 1, color_fg);
      lcd->drawLine(_width - 1, _height - 1 - size, _width - 1, _height - 1, color_fg);
      lcd->drawLine(_width - 1 - size, _height - 1, _width - 1, _height - 1, color_fg);
      break;
    }

    // user has to get the chance to release
    if (i > 0)
      delay(1000);

    for (uint8_t j = 0; j < 8; j++)
    {
      while (touch_has_signal())
      {
        if (touch_touched())
        {
          // Serial.print("Data x :");
          // Serial.println(touch_last_x);
          // Serial.print("Data y :");
          // Serial.println(touch_last_y);
          break;
        }
      }
    }
  }
}

void touch_calibrate() // 屏幕校准
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;
  Serial.println("屏幕校准");

  // 校准
  //   lcd->fillScreen(BLACK);
  //   lcd->setCursor(20, 0);
  //   Serial.println("setCursor");
  //   lcd->setTextFont(2);
  //   Serial.println("setTextFont");
  //   lcd->setTextSize(1);
  //   Serial.println("setTextSize");
  //   lcd->setTextColor(TFT_WHITE, TFT_BLACK);

  //  lcd->println("按指示触摸角落");
  Serial.println("按指示触摸角落");
  //  lcd->setTextFont(1);
  //  lcd->println();
  //      lcd->setCursor(175, 100);
  //      lcd->printf("Touch Adjust");
  //  Serial.println("setTextFont(1)");
  lv_timer_handler();
  calibrateTouch(calData, MAGENTA, BLACK, 17);
  Serial.println("calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15)");
  Serial.println();
  Serial.println();
  Serial.println("//在setup()中使用此校准代码:");
  Serial.print("uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(calData[i]);
    if (i < 4)
      Serial.print(", ");
  }

  Serial.println(" };");
  //  Serial.print("  tft.setTouch(calData);");
  //  Serial.println(); Serial.println();
  //  lcd->fillScreen(BLACK);
  //  lcd->println("XZ OK!");
  //  lcd->println("Calibration code sent to Serial port.");
}

void setTouch(uint16_t *parameters)
{
  touchCalibration_x0 = parameters[0];
  touchCalibration_x1 = parameters[1];
  touchCalibration_y0 = parameters[2];
  touchCalibration_y1 = parameters[3];

  if (touchCalibration_x0 == 0)
    touchCalibration_x0 = 1;
  if (touchCalibration_x1 == 0)
    touchCalibration_x1 = 1;
  if (touchCalibration_y0 == 0)
    touchCalibration_y0 = 1;
  if (touchCalibration_y1 == 0)
    touchCalibration_y1 = 1;

  touchCalibration_rotate = parameters[4] & 0x01;
  touchCalibration_invert_x = parameters[4] & 0x02;
  touchCalibration_invert_y = parameters[4] & 0x04;
}

lv_coord_t width, height, x_off, y_off;
lv_align_t alignment;
const char *parent;
lv_obj_t *screen1, *button1, *label1, *txtArea1, *mainPanel;

void create_screen()
{
  screen1 = lv_obj_create(NULL);
  lv_scr_load(screen1); // Load the screen as the active screen

  lv_obj_t *jsonbtn = lv_btn_create(lv_scr_act());
  lv_obj_add_event_cb(jsonbtn, jsonCB, LV_EVENT_ALL, mainPanel);
  lv_obj_align(jsonbtn, LV_ALIGN_TOP_RIGHT, -20, 20);
  lv_obj_set_size(jsonbtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  mainPanel = lv_obj_create(screen1);
  lv_obj_align(mainPanel, LV_ALIGN_CENTER, 0, 50);
  lv_obj_set_size(mainPanel, 600, 350);

  lv_obj_t *jsonlabel = lv_label_create(jsonbtn);
  lv_label_set_text(jsonlabel, "Click to Get DisplayJSON");
  lv_obj_set_style_text_font(jsonlabel, &lv_font_montserrat_16, NULL);
}
void jsonCB(lv_event_t *e)
{
  lv_event_code_t evcode = lv_event_get_code(e);
  lv_obj_t *panel = (lv_obj_t *)lv_event_get_user_data(e);
  if (evcode == LV_EVENT_CLICKED)
  {

    getJson();
    lv_obj_t *child;
    while ((child = lv_obj_get_child(mainPanel, NULL)) != NULL)
    {
      lv_obj_del(child);
    }
    drawdisplay();
  }
}
void drawButton(lv_coord_t width, lv_coord_t height, lv_align_t align, lv_coord_t xoff, lv_coord_t yoff)
{
  button1 = lv_btn_create(mainPanel);
  lv_obj_set_size(button1, width, height);
  lv_obj_align(button1, align, xoff, yoff);
}
void drawTextbox(const char *tempText, lv_coord_t w, lv_coord_t h, lv_align_t align, lv_coord_t xoff, lv_coord_t yoff)
{
  txtArea1 = lv_textarea_create(mainPanel);
  lv_textarea_set_placeholder_text(txtArea1, tempText);
  lv_obj_align(txtArea1, align, xoff, yoff);
  lv_obj_set_size(txtArea1, w, h);
}
void drawlabel(const char *txt)
{
  label1 = lv_label_create(button1);
  lv_label_set_text(label1, txt);
  lv_obj_center(label1);
}
void drawswitch(lv_align_t align, lv_coord_t xoff, lv_coord_t yoff)
{
  lv_obj_t *sw = lv_switch_create(mainPanel);
  lv_obj_align(sw, align, xoff, yoff);
}
void iterateJson(JsonVariant jsonVariant)
{
  JsonArray child = jsonVariant["ui"]["root"]["children"];
  for (auto value : child)
  {
    if (value["type"] == "button")
    {
      width = value["options"]["width"];
      height = value["options"]["height"];
      alignment = value["options"]["align"];
      x_off = value["options"]["x_offset"];
      y_off = value["options"]["y_offset"];
      drawButton(width, height, alignment, x_off, y_off);
    }
    else if (value["type"] == "textarea")
    {
      const char *myText;
      myText = value["options"]["tempText"];
      alignment = value["options"]["alignment"];
      x_off = value["options"]["x_offset"];
      y_off = value["options"]["y_offset"];
      width = value["options"]["width"];
      height = value["options"]["height"];
      drawTextbox(myText, width, height, alignment, x_off, y_off);
    }
    else if (value["type"] == "label")
    {
      const char *txt = value["options"]["text"];
      drawlabel(txt);
    }
    else if (value["type"] == "switch")
    {
      alignment = value["options"]["align"];
      drawswitch(alignment, x_off, y_off);
    }
  }
}
void drawdisplay()
{
  DeserializationError error = deserializeJson(doc, jsonstring);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  iterateJson(doc);
}
void getJson()
{
  client.begin(serverurl);
  client.addHeader("Accept", "application/json");
  int code = client.GET();
  if (code)
  {
    Serial.print("The code is: ");
    Serial.println(code);
    jsonstring = client.getString();
    Serial.print("the string is: ");
    Serial.println(jsonstring.c_str());
  }
  else
  {
    Serial.print("Code not 1 it is: ");
    Serial.println(code);
  }
  client.end();
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600); // Init Display
  WiFi.begin(wifissid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("connected Successfully");
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
  create_screen();

  getJson();
  drawdisplay();
  // ui_init();//开机UI界面
  // lv_demo_widgets(); // 主UI界面
}

void loop()
{
  lv_timer_handler();
  delay(5);
}