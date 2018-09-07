#include <art32/motion.h>
#include <art32/numbers.h>
#include <art32/strconv.h>
#include <driver/adc.h>
#include <esp_system.h>
#include <math.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "dst.h"
#include "enc.h"
#include "end.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

// TODO: Make pir async.

/* state */

typedef enum {
  OFFLINE,    // offline state
  STANDBY,    // waits for external commands
  MOVE_UP,    // moves up
  MOVE_DOWN,  // moves down
  MOVE_TO,    // moved to position
  AUTOMATE,   // moves according to sensors
  ZERO,       // zero position
  RESET,      // resets position
  REPOSITION  // reposition after a reset
} state_t;

state_t state = OFFLINE;

/* parameters */

static bool automate = false;
static double reset_height = 0;
static double winding_length = 0;
static double base_height = 0;
static double idle_height = 0;
static double rise_height = 0;
static int idle_light = 0;
static int flash_intensity = 0;
static int move_up_speed = 0;
static int move_down_speed = 0;
static int zero_speed = 0;
static bool zero_switch = false;
static bool invert_encoder = false;
static double move_precision = 0;
static int pir_sensitivity = 0;
static int pir_interval = 0;

/* variables */

static bool motion = false;
static double distance = 0;
static double position = 0;
static double move_to = 0;

static a32_motion_t mp;

/* helpers */

bool approach(double target) {
  // configure motion profile
  mp.max_velocity = 12.0 /* cm */ / 1000 /* s */ * 1.2;
  mp.max_acceleration = 0.005 /* cm */ / 1000 /* s */;

  // provide measured position
  mp.position = position;

  // update motion profile (for next ms)
  a32_motion_update(&mp, target, 1);

  // check if target has been reached (within 0.2cm and velocity < 2cm/s)
  if (position < target + 0.2 && position > target - 0.2 && mp.velocity < 0.002) {
    // stop motor
    mot_hard_stop();

    return true;
  }

  // move depending on position
  if (mp.velocity > 0) {
    mot_move_up(mp.velocity * 1000 * 0.8);
  } else {
    mot_move_down(fabs(mp.velocity) * 1000 * 0.8);
  }

  return false;
}

/* state machine */

const char *state_str(state_t s) {
  switch (s) {
    case OFFLINE:
      return "OFFLINE";
    case STANDBY:
      return "STANDBY";
    case MOVE_UP:
      return "MOVE_UP";
    case MOVE_DOWN:
      return "MOVE_DOWN";
    case MOVE_TO:
      return "MOVE_TO";
    case AUTOMATE:
      return "AUTOMATE";
    case ZERO:
      return "ZERO";
    case RESET:
      return "RESET";
    case REPOSITION:
      return "REPOSITION";
  }

  return "";
}

static void state_feed();

static void state_transition(state_t new_state) {
  // return if already in state
  if (new_state == state) {
    return;
  }

  // log state change
  naos_log("transition: %s", state_str(new_state));

  // transition state
  switch (new_state) {
    case OFFLINE: {
      // stop motor
      mot_hard_stop();

      // turn of led
      led_set(led_mono(0), 100);

      // set state
      state = OFFLINE;

      break;
    }

    case STANDBY: {
      // stop motor
      mot_hard_stop();

      // enable idle light
      led_set(led_mono(idle_light), 100);

      // set state
      state = STANDBY;

      break;
    }

    case MOVE_UP: {
      // stop motor
      mot_set(move_up_speed);

      // set state
      state = MOVE_UP;

      break;
    }

    case MOVE_DOWN: {
      // stop motor
      mot_set(-move_down_speed);

      // set state
      state = MOVE_DOWN;

      break;
    }

    case MOVE_TO: {
      // stop motor
      mot_hard_stop();

      // reset motion profile
      mp = (a32_motion_t){0};

      // set state
      state = MOVE_TO;

      break;
    }

    case AUTOMATE: {
      // set state
      state = AUTOMATE;

      // reset motion profile
      mp = (a32_motion_t){0};

      break;
    }

    case ZERO: {
      // move up
      mot_set(zero_speed);

      // set state
      state = ZERO;

      break;
    }

    case RESET: {
      // stop motor
      mot_hard_stop();

      // reset position
      position = reset_height;

      // set state
      state = RESET;

      break;
    }

    case REPOSITION: {
      // stop motor
      mot_hard_stop();

      // reset motion profile
      mp = (a32_motion_t){0};

      // set state
      state = REPOSITION;

      break;
    }
  }

  // publish new state
  naos_publish("state", state_str(state), 0, false, NAOS_LOCAL);

  // feed state machine
  state_feed();
}

static void state_feed() {
  switch (state) {
    case OFFLINE: {
      // do nothing

      break;
    }

    case STANDBY: {
      // transition to automate if enabled
      if (automate) {
        state_transition(AUTOMATE);
      }

      break;
    }

    case MOVE_UP:
    case MOVE_DOWN: {
      // wait for stop command or reset

      break;
    }

    case MOVE_TO: {
      // approach target and transition to standby if reached
      if (approach(move_to)) {
        state_transition(STANDBY);
      }

      break;
    }

    case AUTOMATE: {
      // transition to standby if disabled
      if (!automate) {
        state_transition(STANDBY);
      }

      // calculate target
      double target = motion ? rise_height : idle_height;

      // approach new target
      approach(target);

      break;
    }

    case ZERO: {
      // wait for reset signal

      break;
    }

    case RESET: {
      // transition to reposition state
      state_transition(REPOSITION);

      break;
    }

    case REPOSITION: {
      // approach target and transition to standby if reached
      if (approach(reset_height - 5)) {
        state_transition(STANDBY);
      }

      break;
    }
  }
}

/* naos callbacks */

static void ping() {
  // flash white for 100ms
  led_flash(led_white(512), 100);
}

static void online() {
  // subscribe local topics
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("stop", 0, NAOS_LOCAL);
  naos_subscribe("zero", 0, NAOS_LOCAL);
  naos_subscribe("flash", 0, NAOS_LOCAL);
  naos_subscribe("flash-color", 0, NAOS_LOCAL);
  naos_subscribe("disco", 0, NAOS_LOCAL);

  // transition to standby state
  state_transition(STANDBY);
}

static void offline() {
  // transition into offline state
  state_transition(OFFLINE);
}

static void update(const char *param, const char *value) {
  // feed state machine
  state_feed();
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // set target
  if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    // check for keywords
    if (strcmp((const char *)payload, "up") == 0) {
      state_transition(MOVE_UP);
    } else if (strcmp((const char *)payload, "down") == 0) {
      state_transition(MOVE_DOWN);
    } else {
      // set new position
      move_to = a32_constrain_d(strtod((const char *)payload, NULL), base_height, reset_height);

      // change state
      state_transition(MOVE_TO);
    }
  }

  // stop motor
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    naos_set_b("automate", false);
    state_transition(STANDBY);
  }

  // zero object
  else if (strcmp(topic, "zero") == 0 && scope == NAOS_LOCAL) {
    state_transition(ZERO);
  }

  // perform flash
  else if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
    int time = a32_str2i((const char *)payload);
    led_flash(led_mono(flash_intensity), time);
  }

  // perform flash
  else if (strcmp(topic, "flash-color") == 0 && scope == NAOS_LOCAL) {
    // read colors and time
    int red = 0;
    int green = 0;
    int blue = 0;
    int white = 0;
    int time = 0;
    sscanf((const char *)payload, "%d %d %d %d %d", &red, &green, &blue, &white, &time);

    // set flash
    led_flash(led_color(red, green, blue, white), time);
  }

  // perform disco
  else if (strcmp(topic, "disco") == 0 && scope == NAOS_LOCAL) {
    int r = esp_random() / 4194304;
    int g = esp_random() / 4194304;
    int b = esp_random() / 4194304;
    int w = esp_random() / 4194304;
    led_set(led_color(r, g, b, w), 100);
  }
}

static void loop() {
  // TODO: Use separate task?

  // track last motion
  static uint32_t last_motion = 0;

  // calculate dynamic pir threshold
  int threshold = a32_safe_map_i((int)position, 0, (int)rise_height, 0, pir_sensitivity);

  // update timestamp if motion detected
  if (pir_read() > threshold) {
    last_motion = naos_millis();
  }

  // check if there was a motion in the last 8sec
  bool new_motion = last_motion > naos_millis() - pir_interval;

  // check motion
  if (motion != new_motion) {
    // update motion
    motion = new_motion;

    // publish update
    naos_publish_b("motion", motion, 0, false, NAOS_LOCAL);
  }

  // track last sent position
  static double sent = 0;

  // publish update if distance changed more than 2cm
  if (distance > sent + 2 || distance < sent - 2) {
    naos_publish_d("distance", distance, 0, false, NAOS_LOCAL);
    sent = distance;
  }

  // feed state machine
  state_feed();
}

/* custom callbacks */

static void end() {
  // ignore when already in reset or zero state
  if (state == RESET || state == REPOSITION || !zero_switch) {
    return;
  }

  // transition in reset state
  state_transition(RESET);
}

static void enc(double rot) {
  // track last sent position
  static double sent = 0;

  // apply rotation
  position += (invert_encoder ? rot * -1 : rot) * winding_length;

  // publish update if position changed more than 1cm
  if (position > sent + 1 || position < sent - 1) {
    naos_publish_d("position", position, 0, false, NAOS_LOCAL);
    sent = position;
  }

  // feed state machine
  state_feed();
}

static void dst(double d) {
  // update distance
  distance = d;
}

static naos_param_t params[] = {
    {.name = "automate", .type = NAOS_BOOL, .default_b = false, .sync_b = &automate},
    {.name = "winding-length", .type = NAOS_DOUBLE, .default_d = 7.5, .sync_d = &winding_length},
    {.name = "base-height", .type = NAOS_DOUBLE, .default_d = 50, .sync_d = &base_height},
    {.name = "idle-height", .type = NAOS_DOUBLE, .default_d = 100, .sync_d = &idle_height},
    {.name = "rise-height", .type = NAOS_DOUBLE, .default_d = 150, .sync_d = &rise_height},
    {.name = "reset-height", .type = NAOS_DOUBLE, .default_d = 200, .sync_d = &reset_height},
    {.name = "idle-light", .type = NAOS_LONG, .default_l = 127, .sync_l = &idle_light},
    {.name = "flash-intensity", .type = NAOS_LONG, .default_l = 1023, .sync_l = &flash_intensity},
    {.name = "move-up-speed", .type = NAOS_LONG, .default_l = 512, .sync_l = &move_up_speed},
    {.name = "move-down-speed", .type = NAOS_LONG, .default_l = 512, .sync_l = &move_down_speed},
    {.name = "zero-speed", .type = NAOS_LONG, .default_l = 500, .sync_l = &zero_speed},
    {.name = "zero-switch", .type = NAOS_BOOL, .default_b = true, .sync_b = &zero_switch},
    {.name = "invert-encoder", .type = NAOS_BOOL, .default_b = true, .sync_b = &invert_encoder},
    {.name = "move-precision", .type = NAOS_DOUBLE, .default_d = 1, .sync_d = &move_precision},
    {.name = "pir-sensitivity", .type = NAOS_LONG, .default_l = 300, .sync_l = &pir_sensitivity},
    {.name = "pir-interval", .type = NAOS_LONG, .default_l = 2000, .sync_l = &pir_interval},
};

static naos_config_t config = {.device_type = "tm-lo",
                               .firmware_version = "1.0.0",
                               .parameters = params,
                               .num_parameters = 16,
                               .ping_callback = ping,
                               .loop_callback = loop,
                               .loop_interval = 1,
                               .online_callback = online,
                               .offline_callback = offline,
                               .update_callback = update,
                               .message_callback = message,
                               .password = "tm2018"};

void app_main() {
  // install global interrupt service
  ESP_ERROR_CHECK(gpio_install_isr_service(0));

  // initialize end stop
  end_init(&end);

  // initialize motion sensor
  pir_init();

  // initialize motor
  mot_init();

  // initialize led
  led_init();

  // initialize encoder
  enc_init(enc);

  // initialize naos
  naos_init(&config);

  // initialize distance sensor
  dst_init(dst);
}
