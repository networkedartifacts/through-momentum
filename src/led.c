#include <driver/ledc.h>

void led_init() {
  // prepare timer config
  ledc_timer_config_t ledc_timer = {
      .bit_num = LEDC_TIMER_10_BIT,
      .freq_hz = 5000,
      .speed_mode = LEDC_HIGH_SPEED_MODE,
      .timer_num = LEDC_TIMER_0
  };

  // configure timer
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // prepare channel config
  ledc_channel_config_t ledc_channel = {
      .duty = 0,
      .intr_type = LEDC_INTR_FADE_END,
      .speed_mode = LEDC_HIGH_SPEED_MODE,
      .timer_sel = LEDC_TIMER_0,
  };

  // configure red channel
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.gpio_num = GPIO_NUM_25;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // configure green channel
  ledc_channel.channel = LEDC_CHANNEL_1;
  ledc_channel.gpio_num = GPIO_NUM_26;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // configure blue channel
  ledc_channel.channel = LEDC_CHANNEL_2;
  ledc_channel.gpio_num = GPIO_NUM_27;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void led_set(int r, int g, int b) {
  // set red
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

  // set green
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

  // set blue
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
}
