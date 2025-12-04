#include <libpynq.h>
#include <pthread.h>
#include <pwm.h>

uint8_t USE_DISPLAY = 1;

uint32_t iicaddress = 0x70;
uint32_t regs[2] = {1, 1};
const uint32_t reglen = sizeof(regs)/sizeof(uint32_t);
display_t display;
FontxFile fonts[1][2];

uint32_t min(uint32_t o1, uint32_t o2) { return o1 < o2 ? o1 : o2; }
uint32_t max(uint32_t o1, uint32_t o2) { return o1 > o2 ? o1 : o2; }

struct print_data { uint32_t len; char **pre; uint32_t **nums; char **post; uint32_t *dtype; };
struct print_data make_data(uint32_t len) { 
  struct print_data d = {0};
  d.len = len;
  d.nums = malloc(sizeof(d.nums[0]) * len + 1);
  d.pre = malloc(sizeof(d.pre[0]) * len + 1);
  d.post = malloc(sizeof(d.post[0]) * len + 1);
  d.dtype = calloc(sizeof(d.dtype[0]) * len + 1, 1);
  for (uint32_t i = 0; i < len; ++i) { d.pre[i] = calloc(200, 0); d.post[i] = calloc(200, 0); }
  return d;
}
 
void *displayloop(void *arg) {
  struct print_data *pd = arg;
  char res[128];
  int32_t ypos = 120;
  int32_t ystride = -20;

  while (1) {
    int cy = ypos;
    for (uint32_t i = 0; i < pd->len; ++i) {
      displayDrawFillRect(&display, cy, 0, cy + 24, 230, RGB_WHITE);
      snprintf(res, 20, pd->dtype[i] == 1 ? "%s0.%02i%s" : "%s%i%s", pd->pre[i], *pd->nums[i], pd->post[i]);
      displayDrawString(&display, fonts[0], cy, 0, (uint8_t*)res, RGB_RED);
      cy += ystride;
    }
    sleep_msec(1000);
  }
  pthread_exit(NULL);
  return arg;
}

void init_sys() {
  pynq_init();
  buttons_init();
  switches_init();

  if (USE_DISPLAY) {
    display_init(&display);
  }
  switchbox_init();
  switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
  switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
  switchbox_set_pin(IO_PMODA3, SWB_IIC1_SCL);
  switchbox_set_pin(IO_PMODA4, SWB_IIC1_SDA);
  iic_init(IIC0);
  iic_reset(IIC0);

  iic_init(IIC1);
  iic_reset(IIC1);

  if (USE_DISPLAY) {
    InitFontx(fonts[0], "../../fonts/ILGH24XB.FNT", "");
    displayFillScreen(&display, RGB_WHITE);
    displaySetFontDirection(&display, TEXT_DIRECTION90);
  }
}

void init_print_thread(struct print_data *value) {
  pthread_t thread;
  pthread_create(&thread, NULL, displayloop, (void*)value);
}

#define SET_TITLE(str) {uint8_t text[21] = {str};displayDrawString(&display, fonts[0], 164, 0, text, RGB_RED);}

#define CRYING_ADDR 0x10
#define HEART_ADDR 0x20
#define DECISION_ADDR 0x42
#define MOTORS_ADDR 0x69

void run_crying() {
  SET_TITLE("Crying submodule:")
  uint32_t crying_volume = 80;
  struct print_data pd = make_data(1);
  pd.nums[0] = &regs[0];
  strcpy(pd.pre[0], "Crying: ");
  strcpy(pd.post[0], "dB");
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
  SET_TITLE("Heartbeat submodule")
  uint32_t heartbeat = 140;
  struct print_data pd = make_data(1);
  pd.nums[0] = &regs[0];
  strcpy(pd.pre[0], "Heartrate: ");
  strcpy(pd.post[0], "Bpm");
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
  SET_TITLE("Decision making:")
  uint32_t crying_volume = 0;
  uint32_t heartbeat = 0;
  uint32_t freq = 5, amp = 50;
  // uint32_t stress = 0;

  struct print_data pd = make_data(4);
  pd.nums[0] = &crying_volume;
  strcpy(pd.pre[0], "Crying: ");
  strcpy(pd.post[0], "dB");
  pd.nums[1] = &heartbeat;
  strcpy(pd.pre[1], "Heartrate: ");
  strcpy(pd.post[1], "Bpm");
  pd.nums[2] = &regs[0];
  strcpy(pd.pre[2], "Freq: ");
  strcpy(pd.post[2], "Hz");
  pd.dtype[2] = 1;
  pd.nums[3] = &regs[1];
  strcpy(pd.pre[3], "Amp: ");
  strcpy(pd.post[3], "%");
  // pd.nums[2] = &stress;
  init_print_thread(&pd);
  iic_set_slave_mode(IIC1, DECISION_ADDR, &(regs[0]), reglen);
  uint32_t delay = 0;
  while (1) {
    if (iic_read_register(IIC0, HEART_ADDR, 0, (uint8_t*)&heartbeat, 4)) { heartbeat = -1; }
    if (iic_read_register(IIC0, CRYING_ADDR, 0, (uint8_t*)&crying_volume, 4)) { crying_volume = -1; }

    if (delay > 100) {
      freq += 1;
      amp += 5;
      if (freq > 7) { freq = 2; }
      if (amp > 100) { amp = 20; }
      delay = 0;
    }
    ++delay;

    regs[0] = freq;
    regs[1] = amp;

    fprintf(stdout, "heart=%i cry=%i\n", heartbeat, crying_volume);
    iic_slave_mode_handler(IIC1);
    sleep_msec(10);
  }
}

uint32_t pwm_get_duty(uint32_t proc) { return proc * 1000;}

uint32_t pwm_freq_duty(uint32_t freq) {
  if (freq > 68) { return 80; }
  if (freq > 57) { return 60; }
  if (freq > 40) { return 40; }
  if (freq > 30) { return 20; }
  return 5;
}

uint32_t pwm_amp_duty(uint32_t amp) {
  if (amp > 90) { return 80; }
  if (amp > 70) { return 60; }
  if (amp > 50) { return 40; }
  if (amp > 30) { return 20; }
  return 5;
}

void pwm_set_cycle(uint32_t n, uint32_t dut) {
  pwm_set_duty_cycle((n == 0) ? PWM0 : PWM1, pwm_get_duty(dut));
}

void run_motors() {
  uint32_t rock_freq = 0, rock_amp = 0;
  struct print_data pd = make_data(4);
  uint32_t pwm_freq = 0, pwm_amp = 0;
  if (USE_DISPLAY) {
    SET_TITLE("Motor driver:")
    pd.nums[0] = &rock_freq;
    strcpy(pd.pre[0], "Freq: ");
    strcpy(pd.post[0], "Hz");
    pd.dtype[0] = 1;
    pd.nums[1] = &pwm_freq;
    strcpy(pd.pre[1], "Freq PWM: ");
    strcpy(pd.post[1], "%");
    pd.nums[2] = &rock_amp;
    strcpy(pd.pre[2], "Amp: ");
    strcpy(pd.post[2], "%");
    pd.nums[3] = &pwm_amp;
    strcpy(pd.pre[3], "Amp PWM: ");
    strcpy(pd.post[3], "%");
    init_print_thread(&pd);
  }

  switchbox_set_pin(IO_AR0, SWB_PWM0);
  switchbox_set_pin(IO_AR1, SWB_PWM1);
  pwm_init(PWM0, 100000);
  pwm_init(PWM1, 100000);
  pwm_set_duty_cycle(PWM0, 0);
  pwm_set_duty_cycle(PWM1, 0);
  int cp = 0;
  while (1) {
    cp += 1000;
    cp %= 100000;
    rock_freq = cp / 1000;
    rock_amp = cp / 1000;
    /*
    pwm_set_duty_cycle(PWM0, cp);
    pwm_set_duty_cycle(PWM1, 100000 - cp);
    fprintf(stdout, "%u\n", cp);
    sleep_msec(100);*/
    /*if (iic_read_register(IIC1, DECISION_ADDR, 0, (uint8_t*)&rock_freq, 4)) { rock_freq = -1; }
    if (iic_read_register(IIC1, DECISION_ADDR, 1, (uint8_t*)&rock_amp, 4)) { rock_amp = -1; }*/
    pwm_freq = pwm_freq_duty(rock_freq);
    pwm_amp  = pwm_amp_duty (rock_amp );
    // pwm_set_cycle(0, pwm_freq);
    // pwm_set_cycle(1, pwm_amp);
    pwm_set_cycle(0, cp / 1000);
    pwm_set_cycle(1, cp / 1000);
  
    fprintf(stdout, "freq=%i amp=%i\n", rock_freq, rock_amp);
    sleep_msec(10);
    sleep_msec(50);
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
