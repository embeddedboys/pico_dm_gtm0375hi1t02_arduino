#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <Wire.h>
#include <demos/lv_demos.h>

#define MY_DISP_HOR_RES     480
#define MY_DISP_VER_RES     320
#define MY_DISP_BUF_SIZE    (MY_DISP_HOR_RES * MY_DISP_VER_RES / 2)

TFT_eSPI tft = TFT_eSPI();

/* NS2009U Spec */
#define NS2009_ADDR         0x48
#define NS2009_PIN_SCL      27
#define NS2009_PIN_SDA      26
#define NS2009_PIN_IRQ      21
#define NS2009_CMD_READ_X   0xC0
#define NS2009_CMD_READ_Y   0xD0
#define NS2009_DISABLE_IRQ  (1 << 2)

#define NS2009_RTP_X_WIDTH  80
#define NS2009_RTP_Y_WIDTH  54
#define NS2009_RTP_X_RES    415
#define NS2009_RTP_Y_RES    285
#define NS2009_RTP_X_OFFS   5
#define NS2009_RTP_Y_OFFS   -20

float rtp_x_sc = (float)((float)MY_DISP_HOR_RES / (float)NS2009_RTP_X_RES);
float rtp_y_sc = (float)((float)MY_DISP_VER_RES / (float)NS2009_RTP_Y_RES);

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

static uint8_t i2c_read_reg(TwoWire &twi, uint8_t dev_addr, uint8_t reg) {
  uint8_t rd;
  do {
    twi.beginTransmission(dev_addr);
    twi.write(reg);
    twi.endTransmission(false);
    rd = twi.requestFrom(dev_addr, 1);
  } while (rd == 0);

  while (!twi.available())
    ;
  return twi.read();
}
#define read_reg(v) i2c_read_reg(Wire1, NS2009_ADDR, v)

static bool ns2009_is_pressed(void) {
  return !digitalRead(NS2009_PIN_IRQ);
}

static uint16_t ns2009_read_x(void) {
  uint8_t val = read_reg(NS2009_CMD_READ_Y);
  uint16_t this_y = 0;

  this_y = ((val * MY_DISP_VER_RES) / (1 << 8));
  this_y += NS2009_RTP_Y_OFFS;
  this_y *= rtp_y_sc;

  return this_y;
}

static uint16_t ns2009_read_y(void) {
  uint8_t val = read_reg(NS2009_CMD_READ_X);
  uint16_t this_x = 0;

  this_x = (MY_DISP_HOR_RES - (val * MY_DISP_HOR_RES) / (1 << 8));
  this_x += NS2009_RTP_X_OFFS;
  this_x *= rtp_x_sc;

  return this_x;
}

static void ns2009_init(void) {
  pinMode(NS2009_PIN_IRQ, INPUT_PULLUP);

  Wire1.setSDA(NS2009_PIN_SDA);
  Wire1.setSCL(NS2009_PIN_SCL);
  Wire1.begin();
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
  tft.setAddrWindow(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area));
  tft.pushPixels((void *)color_p, lv_area_get_size(area));

  lv_disp_flush_ready(disp_drv);
}

static void tp_read(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
  if (ns2009_is_pressed()) {
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }

  data->point.x = ns2009_read_x();
  data->point.y = ns2009_read_y();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);

  lv_init();

  static lv_disp_draw_buf_t draw_buf_dsc_1;
  static lv_color_t buf_1[MY_DISP_BUF_SIZE];
  lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_BUF_SIZE);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = MY_DISP_HOR_RES;
  disp_drv.ver_res = MY_DISP_VER_RES;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &draw_buf_dsc_1;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  ns2009_init();
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = tp_read;
  lv_indev_drv_register(&indev_drv);

  lv_demo_widgets();
  // lv_demo_benchmark();
}

void loop() {
  // put your main code here, to run repeatedly:
  lv_timer_handler();
  delay(5);
}
