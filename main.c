#include <libpynq.h>
#include <pthread.h>

uint32_t iicaddress = 0x70;
uint32_t regs[32] = {1};
const uint32_t reglen = sizeof(regs)/sizeof(uint32_t);
display_t display;
FontxFile fonts[1][2];
 
void *displayloop(void *arg) {
  char res[128];
  while (1) {
    snprintf(res, sizeof(res), "%u", regs[0]);
    displayDrawFillRect(&display, 80, 0, 120, 150, RGB_BLACK);
    displayDrawString(&display, fonts[0], 80, 0, (uint8_t*)res, RGB_RED);
    sleep_msec(200);
  }
  pthread_exit(NULL);
  return arg;
}

void init_sys() {
  pynq_init();
  display_init(&display);
  switchbox_init();
  switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
  switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
  iic_init(IIC0);
  iic_reset(IIC0);

  InitFontx(fonts[0], "../../fonts/ILGH24XB.FNT", "");
  displayFillScreen(&display, RGB_BLACK);

  displaySetFontDirection(&display, TEXT_DIRECTION90);
}

void run_master() {
  uint32_t i;
  uint32_t slave_address = 0x70;
  for (int reg=0; reg < 32; reg++) {
    if (iic_read_register(IIC0, slave_address, reg, (uint8_t *) &i, 4)) {
      printf("register[%d]=error\n",reg); } else {
      printf("register[%d]=%d\n",reg,i);
    }
  }
}

void run_slave() {
  uint32_t fib[2] = {1, 1};
  iic_set_slave_mode(IIC0, iicaddress, &(regs[0]), reglen);

  pthread_t thread;
  pthread_create(&thread, NULL, displayloop, NULL);
  while (1) {
    iic_slave_mode_handler(IIC0);

    uint32_t nfib = fib[0] + fib[1];
    fib[0] = fib[1];
    fib[1] = nfib % 10000007;
    regs[0] = fib[0];

    sleep_msec(10);
  }
  pthread_join(thread, NULL);
}

void run_old_slave() {
  iic_set_slave_mode(IIC0, iicaddress, &(regs[0]), reglen);
  while (1) {
    iic_slave_mode_handler(IIC0);
    sleep_msec(10);
  }
}

int main(void)
{
  init_sys();

  uint8_t text[] = {"We are all birds:"};
  displayDrawString(&display, fonts[0], 124, 0, text, RGB_RED);

  run_slave();
  // run_old_slave();

  iic_destroy(IIC0);
  switchbox_destroy();
  pynq_destroy();
  return EXIT_SUCCESS;
}
