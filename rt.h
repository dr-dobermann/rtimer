#ifndef __RT_H_
#define __RT_H_

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Keys.h>

#define __RTIMER_DBG_

namespace rtimer {
    // LED display ports
    //  rs, en, d4, d5, d6, d7
    const uint16_t lcp[6] {8, 9, 4, 5, 6, 7};

    const uint8_t
        P_DISPLAY_BKLIT = 10,
        DISPLAY_BKLIT = 90,
   
        P_BEEPER = 3;
    
    const uint16_t
        START_CNTDWN = 10,
        STEP_CNTDWN = 5,
       
        MIN_TOUT = 450,
        
        TIMER_MIN_DEFAULT = 30,
        TIMER_MAX_DEFAULT = 180,
        DELAY_MIN_DEFAULT = 1,
        DELAY_MAX_DEFAULT = 60;

    class RTimer {
        public:
          RTimer(const uint16_t lc_pins[6], const uint16_t keyboard_port, const uint8_t beep_port);
          
          void run();
          
        private:
            // Liquid display controller
            class LC {
                public:
                    LC(const uint16_t lc_pins[6]);
                    void showLine(String str, uint8_t line);
                    void changeBacklit(uint8_t new_bl);
                    uint8_t getBacklit() { return bklit; };
                    
                private:
                    LiquidCrystal _lcd;
                    uint64_t display_tout;
                    String lines[2];
                    uint64_t last_disp_time;
                    byte pos[2];
                    uint8_t bklit;
            };

            class Beeper {
                public:
                    // Beep type
                    typedef 
                        enum {
                            btStart,
                            btDelay,
                            btStartCntdwn,
                            btEndCntdwn,
                            btEnd
                        } TBeepType;
                        
                    Beeper(uint8_t bport);
                    void beep(TBeepType btype);
                    void check_beeper();
                 
                private:
                    uint8_t beeper_port;
                    uint64_t stop_beep_millis; // to prevent using delay(), each beep() call
                                               // sets a new time to call noTone() to stop beep
                    // Single beep info
                    struct Beep {
                            uint32_t freq;
                            uint64_t dur;
                        } beeps[5];                     
            };
            
            // Single step of timer IDs
            typedef 
                enum {
                    mRoot,
                    pTimer,
                    mSettings,
                    pTimerSet,
                    pDelaySet,
                    pRepeatSet,
                    pBeepSet,
                    pReSet,
                    pBklitSet,
                } StepID;

            // callback function type to process menuItem call
            typedef bool (RTimer::*RunProc)(keys::Key k);
      
            // Single step of the timer data
            typedef 
                struct {
                    StepID id;
                    String name;
                    String descr;
                    StepID prev;
                    StepID next[6];  
                    RunProc runner; // callback proc to proceess the step
                                    // if NULL, then it's just a menu item with
                                    // no defined processor
                } Step;

            // Interval type for timer of for delay
            typedef 
                enum {
                    tmFixed,
                    tmRandom
                } TimerMode;

            // Minimal or maximal part of random value to edit 
            typedef 
                enum {
                    rtvMin,
                    rtvMax
                } RndTimerValue;

            // Timer repeating mode
            typedef 
                enum {
                    trmForever,
                    trmTLimit,
                    trmRounds
                } TimerRepeatMode;

            // Timer states
            typedef 
                enum {
                    tsNotStarted,
                    tsStartCntdwn,
                    tsStarted,
                    tsDelayed,
                    tsTPaused,
                    tsDPaused
                } TimerState; 

            //------------------------------------------------------------
            // RTimer variables
            //------------------------------------------------------------
            keys::Keyboard kbd;
            LC lcd;

            // Timer core variables
            TimerState tstate;
            uint64_t last_millis;
            TimerMode tmode;  // timer mode
            uint8_t tmin;
            uint8_t tmax;
            TimerMode dmode; // delay mode
            uint8_t dmin;
            uint8_t dmax;
            TimerRepeatMode trmode;
            uint8_t  trlimit;
            uint8_t  trlim_left;
            RndTimerValue rtValue;
            uint8_t ttime;
            uint8_t tleft;
            uint8_t dtime;
            uint8_t dleft;

            bool reset_flag;

            // States map of the timer
            Step steps[9];
            // Timer steps' managing variables
            StepID curr_step;
            int curr_menu_item;
            uint8_t last_key_code;

            // Timer's beeper control
            Beeper beeper;
            // flags to enable/disable countdown beeps
            // Starts countdown could only be disabled for 
            // session after delay. Initial countdown is always presented
            bool tstart_cntdwn;
            bool tend_cntdwn;
            // beeps managing variables
            uint8_t start_cntdwn;

            // backlit value
            uint8_t lcd_bklit;

            // Timer step processing routines
            bool timer_run(keys::Key k);
            bool set_timer_run(keys::Key k);
            bool set_delay_run(keys::Key k);
            bool set_repeat_run(keys::Key k);
            bool set_beep_run(keys::Key k);
            bool set_reset_run(keys::Key k);
            bool set_bklit_run(keys::Key k);
            
            uint16_t normalize(uint16_t val, uint16_t minv, uint16_t maxv) {
                if (val < minv)
                    return minv;
                if (val > maxv)
                    return maxv;
          
                return val;
            }

            uint16_t get_ttime() {
                if (tmode == tmFixed)
                    return tmin;
              
                return random(tmin, tmax);
            }
      
            uint16_t get_dtime() {
                if (dmode == tmFixed)
                    return dmin;
              
                return random(dmin, dmax);
            }      

            Step *get_step(StepID id) {
                Step *step = NULL;
                for (uint16_t i = 0; i < 9; i++)
                    if (steps[i].id == id) {
                        step = &(steps[i]);
                        break;
                    }
                return step;
            }
            
            void set_defaults();
            void load();
            void save();
            void reset();
    };
}; // end of rtimer namespace

#endif // __RT_H_
