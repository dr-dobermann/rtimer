#include "rt.h"
#include <EEPROM.h>


//------------------------------------------------------------------------------------------
rtimer::RTimer::RTimer(const uint16_t lc_pins[6], const uint16_t keyboard_port, const uint8_t beep_port) :
    kbd(keyboard_port),
    lcd(lc_pins),
    steps {  
          { mRoot, "MAIN MENU", "Use UP/DOWN to choose Timer or Settings and SELECT to enter into it. ", mRoot, {pTimer, mSettings, mRoot, mRoot, mRoot}, NULL},
          { pTimer, "MM>TIMER", "Press SELECT to run/stop timer", mRoot, {}, &RTimer::timer_run},
          { mSettings, "MM>SETTINGS", "Use UP/DOWN to choose settings and SELECT/RIGHT to configure it. Press LEFT to get back to main menu. ", mRoot, {pTimerSet, pDelaySet, pRepeatSet, pBeepSet, pReSet}, NULL},
          { pTimerSet, "SET>TMR", "", mSettings, {}, &RTimer::set_timer_run},
          { pDelaySet, "SET>DELAY", "", mSettings, {}, &RTimer::set_delay_run},
          { pRepeatSet, "SET>RPT", "", mSettings, {}, &RTimer::set_repeat_run},
          { pBeepSet, "SET>BEEPS", "", mSettings, {}, &RTimer::set_beep_run},
          { pReSet, "SET>RESET", "", mSettings, {}, &RTimer::set_reset_run} },
    beeper(beep_port)
{
    load();
    randomSeed(analogRead(0));
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::set_defaults() {
  
    tstate = tsNotStarted;
    last_millis = 0;
  
    tmode = tmRandom;
    tmin = TIMER_MIN_DEFAULT;
    tmax = TIMER_MAX_DEFAULT;
    
    dmode = tmRandom;
    dmin = DELAY_MIN_DEFAULT;
    dmax = DELAY_MAX_DEFAULT;
    
    trmode = trmRounds;
    trlimit = 3;
    
    rtValue = rtvMin;
    
    reset_flag = false;
  
    curr_step = mRoot;
    curr_menu_item = 0;
    last_key_code = keys::kcNone;
  
    tstart_cntdwn = true;
    tend_cntdwn = true;
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::load() {

    if ( EEPROM.read(0) == 73 ) { 
      tmode = TimerMode(EEPROM.read(1));
      tmin = EEPROM.read(2);
      tmax = EEPROM.read(3);
      dmode = TimerMode(EEPROM.read(4));
      dmin = EEPROM.read(5);
      dmax = EEPROM.read(6);
      trmode = TimerRepeatMode(EEPROM.read(7));
      trlimit = EEPROM.read(8);
      tstart_cntdwn = bool(EEPROM.read(9));
      tend_cntdwn = bool(EEPROM.read(10));  
    }
    else {
      EEPROM.update(0, 73);
      set_defaults();
      save();
    }
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::save() {
  
    EEPROM.update(1, uint8_t(tmode));
    EEPROM.update(2, tmin);
    EEPROM.update(3, tmax);
    EEPROM.update(4, uint8_t(dmode));
    EEPROM.update(5, dmin);
    EEPROM.update(6, dmax);
    EEPROM.update(7, uint8_t(trmode));
    EEPROM.update(8, trlimit);
    EEPROM.update(9, uint8_t(tstart_cntdwn));
    EEPROM.update(10, uint8_t(tend_cntdwn));
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::reset() {

    set_defaults();
    save();
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::run() {

    keys::Key key = kbd.get_key();
  
    // get current step info
    RTimer::Step *step = get_step(curr_step);
    if (step == NULL)
        return;
  
    if (key.code == keys::kcLeft) {
        curr_step = step->prev;
        return;
    }

    if (step->runner != NULL) {
        if (!(this->*(step->runner))(key))
            curr_step = step->prev;
      
        return;
    }
  
    Step *stp = NULL;
    if (step->next[curr_menu_item] != mRoot)
      stp = get_step(step->next[curr_menu_item]);
    if (stp)
      lcd.showLine(stp->name, 0);
    else
      lcd.showLine(step->name, 0);
    lcd.showLine(step->descr, 1);
  
    if (key.code == last_key_code)
      return;
    last_key_code = key.code;

    switch (key.code) {
      case keys::kcNone:
          break;
          
      // left current state and get back to the previous one
      case keys::kcLeft:
          curr_step = step->prev;
          break;       
      
      // enter into selected option or start the command runner
      case keys::kcSelect:
      case keys::kcRight:
          curr_step = step->next[curr_menu_item];
          curr_menu_item = 0;
          break;
        
      // change next/previous available option in the menu       
      case keys::kcUp:     
      case keys::kcDown:
          if (key.code == keys::kcDown)
              curr_menu_item++;
          else
              curr_menu_item--;

          if (curr_menu_item < 0 || curr_menu_item > 4 || step->next[curr_menu_item] == mRoot)
            curr_menu_item = 0;
        break;
    }
  
}


//------------------------------------------------------------------------------------------
rtimer::RTimer::LC::LC(const uint16_t lc_pins[6]) :
  _lcd(lc_pins[0], lc_pins[1], lc_pins[2], lc_pins[3], lc_pins[4], lc_pins[5]),
  display_tout(MIN_TOUT)
{
    // init LCD display
    _lcd.begin(16, 2);
    // set backlit value
    analogWrite(P_DISPLAY_BKLIT, DISPLAY_BKLIT);
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::LC::showLine(String str, uint8_t line) {
    if (line > 1)
        return;
  
    if (lines[line] == str && lines[line].length() <= 16)
        return;

    if ( lines[line] != str ) {
        lines[line] = str;
        pos[line] = 0;
    }

    _lcd.setCursor(0, line);
    if (lines[line].length() > 16) {
        if (millis() - last_disp_time >= display_tout) {
            _lcd.print(lines[line].substring(pos[line], pos[line] + 16));
            if (++pos[line] >= lines[line].length())    
                pos[line] = 0;
            last_disp_time = millis();
        }
    }
    else {
        // clearing by spaces prevents of screen blinking after LiquidScreen::clear()
        _lcd.print("                ");
        _lcd.setCursor(0, line);
        _lcd.print(lines[line]);
    }
}


//------------------------------------------------------------------------------------------
rtimer::RTimer::Beeper::Beeper(uint8_t bport) :
    beeper_port(bport),
    beeps{
        {1500, 300},   // btStart
        {1000, 300},   // btDelay
        {1200, 100},   // btStartCntdwn
        {800,  100},   // btEndCntdwn
        {100,  500} } // btEnd  
{
  stop_beep_millis = 0;
  noTone(beeper_port);  
}
//------------------------------------------------------------------------------------------


void rtimer::RTimer::Beeper::beep(TBeepType btype) {
    tone(beeper_port, beeps[btype].freq, beeps[btype].dur);
    stop_beep_millis = millis() + beeps[btype].dur;
}


//------------------------------------------------------------------------------------------
void rtimer::RTimer::Beeper::check_beeper() {
    if (stop_beep_millis > 0 && stop_beep_millis < millis()) {
      noTone(beeper_port);
      stop_beep_millis = 0;
    }
}


//------------------------------------------------------------------------------------------
bool rtimer::RTimer::timer_run(keys::Key k) {
  
    String fStr("TIMER:"),
           sStr("");
    switch (tstate) {
        case tsNotStarted:
            fStr += "NOT STRTD ";
            break;
  
        case tsStartCntdwn:
            fStr += "STARTS IN:";
            sStr = start_cntdwn;
            break;
        
        case tsStarted:
            fStr += "STARTED ";
            sStr += tleft;
            break;
   
        case tsDelayed:
            fStr += "DELAYED ";
            sStr += dleft;
            break; 
  
        case tsTPaused:
            fStr += "T.PAUSED ";
            sStr += tleft;
            break; 
  
        case tsDPaused:
            fStr += "D.PAUSED ";
            sStr += dleft;
            break; 
    }
      
    lcd.showLine(fStr, 0);
    lcd.showLine(sStr, 1);
    
    switch (k.code) {
        case keys::kcSelect:
            if (k.code == last_key_code)
                return true;
            ttime = get_ttime();
            tleft = ttime;
            trlim_left = trlimit;
            last_millis = millis();
            if (tstart_cntdwn) {
                tstate = tsStartCntdwn;
                start_cntdwn = START_CNTDWN;
            }
            else
                tstate = tsStarted;
            break;
  
        case keys::kcRight:
            if (k.code == last_key_code)
                return true;
            switch (tstate) {
                case tsStarted:
                    tstate = tsTPaused;
                    break;
                  
                case tsDelayed:
                    tstate = tsDPaused;
                    break;
                  
                case tsTPaused:
                    tstate = tsStarted;
                    break;
                  
                case tsDPaused:
                    tstate = tsDelayed;
                    break;
                  
                default:
                    break;
            }
            break;
  
        case keys::kcDown:
            if (k.code == last_key_code)
                return true;
            if (tstate == tsStarted) tleft = 0;
            else if (tstate == tsDelayed) dleft = 0;
            break;

        default:
            break;
    }
    last_key_code = k.code;
  
    // update time for timer or delay
    if (millis() - last_millis >= 1000) {
        switch (tstate) {
            case tsStartCntdwn:
                if (start_cntdwn > 1) {
                    start_cntdwn--;
                    beeper.beep(Beeper::btStartCntdwn);          
                }
                else {
                    tstate = tsStarted;
                    beeper.beep(Beeper::btStart);
                }
                break;
          
            case tsStarted:
                if (tleft > 0) {
                    tleft--;
                    if (tend_cntdwn && tleft < STEP_CNTDWN)
                        beeper.beep(Beeper::btEndCntdwn); 
                }
                else {
                    tstate = tsDelayed;
                    dtime = get_dtime();
                    dleft = dtime;
                    if (trmode == trmRounds)
                        trlim_left--;
                    beeper.beep(Beeper::btDelay);
                }
                if (trmode == trmTLimit)
                    trlim_left--;
                break;
  
            case tsDelayed:
                if (dleft > 0) {
                    dleft--;
                    if (tstart_cntdwn && dleft < STEP_CNTDWN)
                        beeper.beep(Beeper::btStartCntdwn);
                }
                else {
                    tstate = tsStarted;
                    ttime = get_ttime();
                    tleft = ttime;
                    beeper.beep(Beeper::btStart);
                }
                if (trmode == trmTLimit)
                    trlim_left--;
                break;
          
              default:
                  break;
        }
        last_millis = millis();
    }
    
    // check for round limits
    if (trmode != trmForever && trlim_left <= 0) {
        if (tstate == tsStarted || tstate == tsDelayed)
            beeper.beep(Beeper::btEnd);
        tstate = tsNotStarted;
    }
      
    return true;
}


//------------------------------------------------------------------------------------------
bool rtimer::RTimer::set_timer_run(keys::Key k) {

    String fStr("SET TMR: "),
           sStr("");
    if (tmode == tmFixed) {
        fStr += "FIX ";
        sStr += tmin;
        rtValue = rtvMin;
    } else {
        fStr += "RND ";
        if (rtValue == rtvMin) {
            sStr = "MIN: ";
            sStr += tmin;
        } else {
            sStr = "MAX: ";
            sStr += tmax;
        }
    }
    lcd.showLine(fStr, 0);
    lcd.showLine(sStr, 1);
  
    int delta = 0;
    bool updated = false;
  
    switch (k.code) {
        case keys::kcSelect:
            if (k.code == last_key_code)
                return true;
            if (tmode == tmFixed)
                tmode = tmRandom;
            else {
                tmode = tmFixed;
                rtValue = rtvMin;
            }
            break;
        
        case keys::kcRight:
            if (k.code == last_key_code)
                return true;
            if (tmode == tmRandom) {
                if (rtValue == rtvMin)
                    rtValue = rtvMax;
                else
                    rtValue = rtvMin;
            }
            else 
                rtValue = rtvMin;          
            break;
        
        case keys::kcUp:
        case keys::kcDown:  
            if (k.mode == keys::kmLong)
                delta = -5;
            else 
                if (k.code != last_key_code)
                    delta = -1;
            if (k.code == keys::kcUp) // invert delta if UP pressed 
                delta *= -1;
            if (rtValue == rtvMin) {
                tmin += delta;
                tmin = normalize(tmin, TIMER_MIN_DEFAULT, TIMER_MAX_DEFAULT);
                updated = true;
            }
            else {
                tmax += delta;
                tmax = normalize(tmax, TIMER_MIN_DEFAULT, TIMER_MAX_DEFAULT);
                updated = true;
            }
            // Align tmin and tmax according to timer mode
            if ((tmode == tmRandom && tmin > tmax) || tmode == tmFixed)
                tmax = tmin;
            break;

        default:
            break;
    }
    last_key_code = k.code;
    //save config to EEPROM
    if (updated)
        save();
  
    return true;
}


//------------------------------------------------------------------------------------------
bool rtimer::RTimer::set_delay_run(keys::Key k) {

    String fStr("SET DELAY:"),
           sStr("");
    if (dmode == tmFixed) {
        fStr += "FIX ";
        sStr += dmin;
        rtValue = rtvMin;
    } else {
        fStr += "RND ";
        if (rtValue == rtvMin) {
            sStr = "MIN: ";
            sStr += dmin;
        } else {
            sStr = "MAX: ";
            sStr += dmax;
        }
    }
    lcd.showLine(fStr, 0);
    lcd.showLine(sStr, 1);
  
    int delta = 0;
    bool updated = false;
      
    switch (k.code) {
        case keys::kcSelect:
            if (k.code == last_key_code)
                return true;
            if (dmode == tmFixed)
                dmode = tmRandom;
            else
                dmode = tmFixed;
            break;
        
        case keys::kcRight:
            if (k.code == last_key_code)
                return true;
            if (dmode == tmRandom) {
                if (rtValue == rtvMin)
                    rtValue = rtvMax;
                else
                    rtValue = rtvMin;
            }
            else 
                rtValue = rtvMin;          
            break;
        
        case keys::kcUp:
        case keys::kcDown:  
            if (k.mode == keys::kmLong)
                delta = -5;
            else 
                if (k.code != last_key_code)
                    delta = -1;
            if (k.code == keys::kcUp) // invert delta if UP pressed 
                delta *= -1;
            if (rtValue == rtvMin) {
                dmin += delta;
                dmin = normalize(dmin, DELAY_MIN_DEFAULT, DELAY_MAX_DEFAULT);
                updated = true;
            }
            else {
                dmax += delta;
                dmax = normalize(dmax, DELAY_MIN_DEFAULT, DELAY_MAX_DEFAULT);
                updated = true;
            }
            // Align tmin and tmax according to timer mode
            if ((dmode == tmRandom && dmin > dmax) || dmode == tmFixed)
                dmax = dmin;
            break;

        default:
            break;
    }
    last_key_code = k.code;
    //save config to EEPROM
    if (updated)
        save();
  
    return true;
}


//------------------------------------------------------------------------------------------
bool rtimer::RTimer::set_repeat_run(keys::Key k) {

    String fStr("SET RPT"),
           sStr("");

    switch (trmode) {
        case trmForever:
            fStr += ":FRV ";
            break;
            
        case trmTLimit:
            fStr += ":TIME ";
            sStr += trlimit;
            break;
            
        case trmRounds:
            fStr += ":RND ";
            sStr += trlimit;
            break;
    }
    lcd.showLine(fStr, 0);
    lcd.showLine(sStr, 1);

    int delta = 0;
    bool updated = false;
  
    switch (k.code) {
        case keys::kcSelect:
            if (k.code == last_key_code)
                return true;
            if (trmode == trmRounds)       trmode = trmForever;
            else if (trmode == trmForever) trmode = trmTLimit;
            else if (trmode == trmTLimit)  trmode = trmRounds;
            updated = true;
            break;
      
        case keys::kcUp:
        case keys::kcDown:
            if (trmode == trmForever)
                break;  
            if (k.mode == keys::kmLong)
                delta = -5;
            else 
                if (k.code != last_key_code)
                    delta = -1;
            if (k.code == keys::kcUp) // invert delta if UP pressed 
                delta *= -1;
            trlimit += delta;
            trlimit = normalize(trlimit, 
                                trmode == trmTLimit ? 2 * TIMER_MIN_DEFAULT : 1,
                                trmode == trmTLimit ? 2 * TIMER_MAX_DEFAULT : 50);
            if (delta != 0)
                updated = true;
            break;

        default:
            break;
    }
    last_key_code = k.code;
    // save updated config to EEPROM
    if (updated)
        save();

  return true;
}


//------------------------------------------------------------------------------------------
bool rtimer::RTimer::set_beep_run(keys::Key k) {

    String fStr("SET CNTDWN BEEP"),
           sStr("");

    if (rtValue == rtvMin) {
        sStr += "START:";
        if (tstart_cntdwn)
            sStr += "ON";
        else
            sStr += "OFF";
    }
    else {
        sStr += "END:";
        if (tend_cntdwn)
            sStr += "ON";
        else
            sStr += "OFF";
    }
    lcd.showLine(fStr, 0);
    lcd.showLine(sStr, 1);

    bool updated = false;
    switch (k.code) {
        case keys::kcRight:
            if (k.code == last_key_code)
                return true;
            if (rtValue == rtvMin)
                rtValue = rtvMax;
            else
                rtValue = rtvMin;
            break;

        case keys::kcUp:
        case keys::kcDown:
            if (k.code == last_key_code)
                return true;
            if (rtValue == rtvMin)
                tstart_cntdwn = !tstart_cntdwn;
            else
                tend_cntdwn = !tend_cntdwn;      
                
            updated = true;
            break;

        default:
            break;
    }
    last_key_code = k.code;
    //save updated config to EEPROM
    if (updated)
        save();

    return true;
}


//------------------------------------------------------------------------------------------
bool rtimer::RTimer::set_reset_run(keys::Key k) {

    String fStr("RESET?"),
           sStr(reset_flag ? "YES" : "NO");
    lcd.showLine(fStr, 0);
    lcd.showLine(sStr, 1);
  
    if (k.code == last_key_code)
        return true;
    switch (k.code) {
        case keys::kcSelect:
            if (reset_flag) {
                reset();
                reset_flag = false;
                return false;
            }
            break;
  
        case keys::kcUp:
        case keys::kcDown:
            reset_flag = !reset_flag;      
            break;
    }
    last_key_code = k.code;
  
    return true;
}


//------------------------------------------------------------------------------------------

