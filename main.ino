#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <Wire.h>
#include <demos/lv_demos.h>

#define MY_DISP_HOR_RES 480
#define MY_DISP_VER_RES 320
#define MY_DISP_BUF_SIZE    (MY_DISP_HOR_RES * MY_DISP_VER_RES / 2)

TFT_eSPI tft = TFT_eSPI();

/* FT6236U Spec */
#define FT6236_ADDR      0x38
#define FT6236_PIN_SCL   27
#define FT6236_PIN_SDA   26
#define FT6236_PIN_RST   18
/* FT Registers */
#define FT_REG_TD_STATUS  0x02  // Touch point status
#define FT_REG_TOUCH1_XH  0x03  // Touch point 1 X high 8-bit
#define FT_REG_TOUCH1_XL  0x04  // Touch point 1 X low 8-bit
#define FT_REG_TOUCH1_YH  0x05  // Touch point 1 Y high 8-bit
#define FT_REG_TOUCH1_YL  0x06  // Touch point 1 Y low 8-bit
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
#define read_reg(v) i2c_read_reg(Wire1, FT6236_ADDR, v)

static bool ft6236_is_pressed(void) {
  return read_reg(FT_REG_TD_STATUS);
}

static uint16_t ft6236_read_x(void) {
  uint8_t val_h = read_reg(FT_REG_TOUCH1_YH);
  uint8_t val_l = read_reg(FT_REG_TOUCH1_YL);
  uint16_t val = (val_h << 8) | val_l;
  return val;
}

static uint16_t ft6236_read_y(void) {
  uint8_t val_h = read_reg(FT_REG_TOUCH1_XH) & 0x1f; /* the MSB is always high, but it shouldn't */
  uint8_t val_l = read_reg(FT_REG_TOUCH1_XL);
  uint16_t val = (val_h << 8) | val_l;
  return (320 - val);
}

static void ft6236_reset(void) {
  digitalWrite(FT6236_PIN_RST, HIGH);
  delay(10);
  digitalWrite(FT6236_PIN_RST, LOW);
  delay(10);
  digitalWrite(FT6236_PIN_RST, HIGH);
}

static void ft6236_init(void) {
  ft6236_reset();

  Wire1.setSDA(FT6236_PIN_SDA);
  Wire1.setSCL(FT6236_PIN_SCL);
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
  if (ft6236_is_pressed()) {
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }

  data->point.x = ft6236_read_x();
  data->point.y = ft6236_read_y();
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
  ft6236_init();
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
