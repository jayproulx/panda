#define RCC_BDCR_OPTIONS (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_0 | RCC_BDCR_LSEON)
#define RCC_BDCR_MASK (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL | RCC_BDCR_LSEMOD | RCC_BDCR_LSEBYP | RCC_BDCR_LSEON)

#define YEAR_OFFSET 2000U

typedef struct __attribute__((packed)) timestamp_t {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} timestamp_t;

uint8_t to_bcd(uint16_t value){
    return (((value / 10U) & 0x0FU) << 4U) | ((value % 10U) & 0x0FU);
}

uint16_t from_bcd(uint8_t value){
    return (((value & 0xF0U) >> 4U) * 10U) + (value & 0x0FU);
}

void rtc_init(void){
    // Initialize RTC module and clock if not done already.
    if((RCC->BDCR & RCC_BDCR_MASK) != RCC_BDCR_OPTIONS){
        puts("Initializing RTC\n");
        // Reset backup domain
        RCC->BDCR |= RCC_BDCR_BDRST;

        // Disable write protection
        PWR->CR |= PWR_CR_DBP;

        // Clear backup domain reset
        RCC->BDCR &= ~(RCC_BDCR_BDRST);

        // Set RTC options
        RCC->BDCR = RCC_BDCR_OPTIONS | (RCC->BDCR & (~RCC_BDCR_MASK));

        // Enable write protection
        PWR->CR &= ~(PWR_CR_DBP);
    }
}

void rtc_set_time(timestamp_t time){
    puts("Setting RTC time\n");

    // Disable write protection
    PWR->CR |= PWR_CR_DBP;
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // Enable initialization mode
    RTC->ISR |= RTC_ISR_INIT;
    while((RTC->ISR & RTC_ISR_INITF) == 0){}
    
    // Set time
    RTC->TR = (to_bcd(time.hour) << RTC_TR_HU_Pos) | (to_bcd(time.minute) << RTC_TR_MNU_Pos) | (to_bcd(time.second) << RTC_TR_SU_Pos);
    RTC->DR = (to_bcd(time.year - YEAR_OFFSET) << RTC_DR_YU_Pos) | (time.weekday << RTC_DR_WDU_Pos) | (to_bcd(time.month) << RTC_DR_MU_Pos) | (to_bcd(time.day) << RTC_DR_DU_Pos);

    // Set options
    RTC->CR = 0U;
    
    // Disable initalization mode
    RTC->ISR &= ~(RTC_ISR_INIT);

    // Wait for synchronization
    while((RTC->ISR & RTC_ISR_RSF) == 0){}

    // Re-enable write protection
    RTC->WPR = 0x00;
    PWR->CR &= ~(PWR_CR_DBP);
}

timestamp_t rtc_get_time(void){
    // Wait until the register sync flag is set
    while((RTC->ISR & RTC_ISR_RSF) == 0){}

    // Read time and date registers. Since our HSE > 7*LSE, this should be fine.
    uint32_t time = RTC->TR;
    uint32_t date = RTC->DR;

    // Parse values
    timestamp_t result;
    result.year = from_bcd((date & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos) + YEAR_OFFSET;
    result.month = from_bcd((date & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos);
    result.day = from_bcd((date & (RTC_DR_DT | RTC_DR_DU)) >> RTC_DR_DU_Pos);
    result.weekday = ((date & RTC_DR_WDU) >> RTC_DR_WDU_Pos);
    result.hour = from_bcd((time & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos);
    result.minute = from_bcd((time & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos);
    result.second = from_bcd((time & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos);
    return result;
}