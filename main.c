#include <libpynq.h>
#include <pthread.h>

uint32_t iicaddress = 0x70;
uint32_t regs[2] = {1, 1};
const uint32_t reglen = sizeof(regs)/sizeof(uint32_t);
display_t display;
FontxFile fonts[1][2];

struct print_data { uint32_t len; uint32_t **nums; };
struct print_data make_data(uint32_t len) { 
  struct print_data d = {0};
  d.len = len;
  d.nums = malloc(sizeof(d.nums[0]) * len + 1);
  return d;
}
 
void *displayloop(void *arg) {
  struct print_data *pd = arg;
  char res[128];
  uint32_t ypos = 80;
  while (1) {
    snprintf(res, sizeof(res), "%u", *pd->nums[0]);
    displayDrawFillRect(&display, ypos, 0, ypos + 40, 200, RGB_WHITE);
    displayDrawString(&display, fonts[0], ypos, 0, (uint8_t*)res, RGB_RED);
    sleep_msec(200);
  }
  pthread_exit(NULL);
  return arg;
}

void init_sys() {
  pynq_init();
  buttons_init();
  switches_init();

  display_init(&display);
  switchbox_init();
  switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
  switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
  iic_init(IIC0);
  iic_reset(IIC0);

  InitFontx(fonts[0], "../../fonts/ILGH16XB.FNT", "");
  displayFillScreen(&display, RGB_BLACK);

  displaySetFontDirection(&display, TEXT_DIRECTION90);
}

void init_print_thread(struct print_data *value) {
  pthread_t thread;
  pthread_create(&thread, NULL, displayloop, (void*)value);
}

#define SET_TITLE(str) {uint8_t text[] = {str};displayDrawString(&display, fonts[0], 124, 0, text, RGB_RED);}

#define CRYING_ADDR 0x10
#define HEART_ADDR 0x20
#define DECISION_ADDR 0x42
#define MOTORS_ADDR 0x69

void run_crying() {
  SET_TITLE("Crying submodule:")
  uint32_t crying_volume = 80;
  struct print_data pd = make_data(1);
  pd.nums[0] = &regs[0];
  init_print_thread(&pd);

  iic_set_slave_mode(IIC0, CRYING_ADDR, &(regs[0]), reglen);
  uint32_t delay = 0;
  while (1) {
    if (delay > 100) {
      crying_volume += 1;
      if (crying_volume > 100) { crying_volume = 0; }
      delay = 0;
    }
    ++delay;

    regs[0] = crying_volume;
    iic_slave_mode_handler(IIC0);
    sleep_msec(10);
  }
}

void run_heartbeat() {
  SET_TITLE("Heartbeat submodule:")
  uint32_t heartbeat = 140;
  struct print_data pd = make_data(1);
  pd.nums[0] = &regs[0];
  init_print_thread(&pd);

  iic_set_slave_mode(IIC0, HEART_ADDR, &(regs[0]), reglen);
  uint32_t delay = 0;
  while (1) {
    if (delay > 100) {
      heartbeat -= 1;
      if (heartbeat < 60) { heartbeat = 240; }
      delay = 0;
    }
    ++delay;

    regs[0] = heartbeat;
    iic_slave_mode_handler(IIC0);
    sleep_msec(10);
  }
}

void run_decision() {
  SET_TITLE("Decision making submodule:")
  uint32_t crying_volume = 0;
  uint32_t heartbeat = 0;
  uint32_t stress = 0;

  struct print_data pd = make_data(3);
  pd.nums[0] = &crying_volume;
  pd.nums[1] = &heartbeat;
  pd.nums[2] = &stress;
  init_print_thread(&pd);
  while (1) {
    if (iic_read_register(IIC0, HEART_ADDR, 0, (uint8_t*)&heartbeat, 4)) { heartbeat = -1; }
    if (iic_read_register(IIC0, CRYING_ADDR, 0, (uint8_t*)&crying_volume, 4)) { crying_volume = -1; }
    fprintf(stdout, "heart=%i cry=%i\n", heartbeat, crying_volume);

    sleep_msec(100);
  }
}

void run_motors() {
  SET_TITLE("Motor driver submodule:")
  uint32_t crying_volume = 80;
  struct print_data pd = make_data(1);
  pd.nums[0] = &regs[0];
  init_print_thread(&pd);

  iic_set_slave_mode(IIC0, CRYING_ADDR, &(regs[0]), reglen);
  while (1) {
    crying_volume += 1;

    regs[0] = crying_volume;
    iic_slave_mode_handler(IIC0);
    sleep_msec(100);
  }
}

int main(void) {
  init_sys();
  
  uint32_t board_id = get_switch_state(0) * 2 + get_switch_state(1);
  fprintf(stdout, "Board id %u!\n", board_id);
  switch (board_id) {
    case 0: run_crying(); break;
    case 1: run_heartbeat(); break;
    case 2: run_decision(); break;
    default: run_motors(); break;
  }

  iic_destroy(IIC0);
  switchbox_destroy();
  pynq_destroy();
  return EXIT_SUCCESS;
}
