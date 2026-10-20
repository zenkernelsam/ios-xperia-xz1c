#ifndef WDT_H
#define WDT_H

void wdt_arm(unsigned bark_s, unsigned bite_s);
void wdt_pet(void);
void wdt_disarm(void);

#endif
