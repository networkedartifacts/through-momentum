#ifndef LED_H
#define LED_H

typedef struct {
  /**
   * The individual channels in values from 0 to 1023.
   */
  int r, g, b, w;
} led_color_t;

/**
 * Initialize LED module.
 */
void led_init();

/**
 * Set constant led color.
 *
 * @param c The constant color.
 * @param t The fade time.
 */
void led_fade(led_color_t c, int t);

/**
 * Perform brief led flash.
 *
 * @param c The flash color.
 * @param t The flash time.
 */
void led_flash(led_color_t c, int t);

/**
 * Mix a color.
 *
 * @param r Red.
 * @param g Green.
 * @param b Blue.
 * @param w White.
 * @return The color.
 */
led_color_t led_color(int r, int g, int b, int w);

/**
 * Mix a mono color.
 *
 * @param b Brightness.
 * @return The color.
 */
led_color_t led_mono(int b);

/**
 * Mix a white color.
 *
 * @param w The whiteness.
 * @return The color.
 */
led_color_t led_white(int w);

#endif  // LED_H
