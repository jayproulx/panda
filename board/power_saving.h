#define POWER_SAVE_STATUS_DISABLED 0
#define POWER_SAVE_STATUS_ENABLED 1

int power_save_status = POWER_SAVE_STATUS_DISABLED;

void set_power_save_state(int state) {

  bool is_valid_state = (state == POWER_SAVE_STATUS_ENABLED) || (state == POWER_SAVE_STATUS_DISABLED);
  if (is_valid_state && (state != power_save_status)) {
    bool enable = false;
    if (state == POWER_SAVE_STATUS_ENABLED) {
      puts("enable power savings\n");
      if (board_has_gps()) {
        char UBLOX_SLEEP_MSG[] = "\xb5\x62\x06\x04\x04\x00\x01\x00\x08\x00\x17\x78";
        uart_ring *ur = get_ring_by_number(1);
        for (unsigned int i = 0; i < sizeof(UBLOX_SLEEP_MSG) - 1U; i++) while (!putc(ur, UBLOX_SLEEP_MSG[i]));
      }
    } else {
      puts("disable power savings\n");
      if (board_has_gps()) {
        char UBLOX_WAKE_MSG[] = "\xb5\x62\x06\x04\x04\x00\x01\x00\x09\x00\x18\x7a";
        uart_ring *ur = get_ring_by_number(1);
        for (unsigned int i = 0; i < sizeof(UBLOX_WAKE_MSG) - 1U; i++) while (!putc(ur, UBLOX_WAKE_MSG[i]));
      }
      enable = true;
    }

    current_board->enable_can_transcievers(enable);

    // Switch EPS/GPS
    if (enable) {
      current_board->set_esp_gps_mode(ESP_GPS_ENABLED);
    } else {
      current_board->set_esp_gps_mode(ESP_GPS_DISABLED);
    }

    if(board_has_gmlan()){
      // turn on GMLAN
      set_gpio_output(GPIOB, 14, enable);
      set_gpio_output(GPIOB, 15, enable);
    }

    if(board_has_lin()){
      // turn on LIN
      set_gpio_output(GPIOB, 7, enable);
      set_gpio_output(GPIOA, 14, enable);
    }

    // Switch IR and fan
    // TODO: Remove powering these back up. Should be done on device
    current_board->set_ir_power(enable ? 50U : 0U);
    current_board->set_fan_power(enable ? 20U : 0U);

    power_save_status = state;
  }
}
