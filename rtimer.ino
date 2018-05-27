#include "rt.h"

rtimer::RTimer rtm(rtimer::lcp, keys::P_KEYBOARD, rtimer::P_BEEPER);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  rtm.run();
}
