/*!
\copyright  Copyright (c) 2008 - 2018 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       av_headset_ui.c
\brief      Application User Interface
*/

#include <panic.h>
#include <ps.h>
#include <boot.h>
#include <input_event_manager.h>

#include "av_headset.h"
#include "av_headset_ui.h"
#include "av_headset_sm.h"
#include "av_headset_hfp.h"
#include "av_headset_power.h"
#include "av_headset_log.h"

/*! Include the correct button header based on the number of buttons available to the UI */
#if defined(HAVE_9_BUTTONS)
#include "9_buttons.h"
#elif defined(HAVE_6_BUTTONS)
#include "6_buttons.h"
#elif defined(HAVE_1_BUTTON)
#include "1_button.h"
#else
#error "No buttons define found"
#endif

#ifdef SINGLE_BDADDR
uint8 reconnect_num = 0;
#endif

#ifdef BM_MUL_CLICK

uint8 bm_press_count = 0;

#define MUL_CLICK_INTERVAL (500)
#define APP_MFB_BUTTON_SHORT_PRESS	1021
#define APP_MFB_BUTTON_DOUBLE_PRESS 1022
#define APP_MFB_BUTTON_TRIPLE_PRESS 1023
#define APP_MFB_BUTTON_QUARTIC_PRESS 1024
#define APP_MFB_BUTTON_QUINTIC_PRESS 1025
#define APP_MFB_BUTTON_SEXTIC_PRESS 1026

#define APP_BUTTON_RELEASE_TIME_OUT 1027

#endif

#define PWOER_KEY_IS_PRESSING()                     ((PioGet32Bank(0) & (1UL <<  0)) == 1)

#ifdef POWER_MANAGER
const char StartBatteryLevelIphone[] = "AT+XAPL=05AC-1702-0100,7\r";

const char BatteryLevel1[] = "AT+IPHONEACCEV=2,1,0,2,0\r";
const char BatteryLevel2[] = "AT+IPHONEACCEV=2,1,1,2,0\r";
const char BatteryLevel3[] = "AT+IPHONEACCEV=2,1,2,2,0\r";
const char BatteryLevel4[] = "AT+IPHONEACCEV=2,1,3,2,0\r";
const char BatteryLevel5[] = "AT+IPHONEACCEV=2,1,4,2,0\r";
const char BatteryLevel6[] = "AT+IPHONEACCEV=2,1,5,2,0\r";
const char BatteryLevel7[] = "AT+IPHONEACCEV=2,1,6,2,0\r";
const char BatteryLevel8[] = "AT+IPHONEACCEV=2,1,7,2,0\r";
const char BatteryLevel9[] = "AT+IPHONEACCEV=2,1,8,2,0\r";
const char BatteryLevel10[] = "AT+IPHONEACCEV=2,1,9,2,0\r";

#endif



/*! User interface internal messasges */
enum ui_internal_messages
{
    /*! Message sent later when a prompt is played. Until this message is delivered
        repeat prompts will not be played */
    UI_INTERNAL_CLEAR_LAST_PROMPT,
};


//------------------ ------------
/*! At the end of every tone, add a short rest to make sure tone mxing in the DSP doens't truncate the tone */
#define RINGTONE_STOP  RINGTONE_NOTE(REST, HEMIDEMISEMIQUAVER), RINGTONE_END

/*!@{ \name Definition of LEDs, and basic colour combinations

    The basic handling for LEDs is similar, whether there are
    3 separate LEDs, a tri-color LED, or just a single LED.
 */

#if (appConfigNumberOfLeds() == 3)
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 1)
#define LED_2_STATE  (1 << 2)
#elif (appConfigNumberOfLeds() == 2)
/* We only have 2 LED so map all control to the same LED */
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 1)
#define LED_2_STATE  (1 << 2)
#else
/* We only have 1 LED so map all control to the same LED */
#define LED_0_STATE  (1 << 0)
#define LED_1_STATE  (1 << 0)
#define LED_2_STATE  (1 << 0)
#endif

#define LED_RED     (LED_0_STATE)
#define LED_GREEN   (LED_1_STATE)
#define LED_BLUE    (LED_1_STATE | LED_2_STATE)//(LED_2_STATE)
#define LED_WHITE   (LED_1_STATE | LED_2_STATE)
#define LED_YELLOW  (LED_RED | LED_GREEN)

/*!@} */

/*! \brief An LED filter used for battery low

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_battery_low(uint16 led_state)
{
    return (led_state) ? LED_RED : 0;
}

/*! \brief An LED filter used for low charging level

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_low(uint16 led_state)
{
    UNUSED(led_state);
    return LED_RED;
}

/*! \brief An LED filter used for charging level OK

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_ok(uint16 led_state)
{
    UNUSED(led_state);
    return LED_RED;
}

/*! \brief An LED filter used for charging complete 

    \param led_state    State of LEDs prior to filter

    \returns The new, filtered, state
*/
uint16 app_led_filter_charging_complete(uint16 led_state)
{
    UNUSED(led_state);
    return 0;
}

/*! \cond led_patterns_well_named
    No need to document these. The public interface is
    from public functions such as appUiPowerOn()
 */

const ledPattern app_led_pattern_power_on[] = 
{
    LED_ON(LED_RED),LED_WAIT(400),LED_OFF(LED_RED),LED_WAIT(400),
    LED_END
};

const ledPattern app_led_pattern_power_off[] = 
{
    LED_ON(LED_RED),LED_WAIT(400),LED_OFF(LED_RED),LED_WAIT(400),
    LED_END

};

const ledPattern app_led_pattern_error[] = 
{
    LED_END
};

const ledPattern app_led_pattern_idle[] = 
{
    LED_OFF(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(4000),LED_WAIT(4000),
    LED_REPEAT(0, 0),
};

const ledPattern app_led_pattern_idle_connected[] = 
{
    LED_OFF(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(4000),LED_WAIT(4000),
    LED_REPEAT(0, 0),
};


const ledPattern app_led_pattern_pairing[] = 
{
    LED_DIM_ON(LED_RED, 40),LED_DIM_OFF(LED_RED, 40),
    LED_REPEAT(0, 0)
};

const ledPattern app_led_pattern_pair_success[] = 
{
    LED_END
};

const ledPattern app_led_pattern_pairing_deleted[] = 
{
    LED_END
};

const ledPattern app_led_pattern_peer_pairing[] =
{
    LED_ON(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(300),LED_ON(LED_BLUE),LED_OFF(LED_RED),LED_WAIT(300),
    LED_REPEAT(0, 0)

};

const ledPattern app_led_pattern_peer_pair_success[] =
{
    LED_END
};

#ifdef INCLUDE_DFU
const ledPattern app_led_pattern_dfu[] = 
{
    LED_LOCK,
    LED_ON(LED_RED),LED_ON(LED_BLUE),LED_WAIT(4000), LED_WAIT(1000),
    LED_UNLOCK,
    LED_REPEAT(0, 0)
};
#endif

#ifdef INCLUDE_AV
const ledPattern app_led_pattern_streaming[] =
{
    LED_OFF(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(4000), LED_WAIT(1000),
    LED_REPEAT(0, 0),

};
#endif

#ifdef INCLUDE_AV
const ledPattern app_led_pattern_streaming_aptx[] =
{
    LED_OFF(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(4000), LED_WAIT(1000),
    LED_REPEAT(0, 0),

};
#endif

const ledPattern app_led_pattern_sco[] = 
{
    LED_OFF(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(4000), LED_WAIT(1000),
    LED_REPEAT(0, 0),

};

const ledPattern app_led_pattern_call_incoming[] = 
{
    LED_OFF(LED_RED),LED_OFF(LED_BLUE),LED_WAIT(4000), LED_WAIT(1000),
    LED_REPEAT(0, 0),

};

const ledPattern app_led_pattern_battery_empty[] = 
{
    LED_END
};

const ledPattern app_led_pattern_enter_dut_mode[] = 
{
    LED_LOCK,
    LED_ON(LED_RED),
    LED_ON(LED_BLUE), 
    LED_WAIT(4000), LED_WAIT(1000),
    LED_UNLOCK,
    LED_REPEAT(0, 0),
};


    
const ledPattern app_led_pattern_linkback_handset[] =
{
    LED_ON(LED_BLUE),LED_WAIT(200),LED_OFF(LED_BLUE), LED_WAIT(200),
    LED_REPEAT(0, 0)
};

const ledPattern app_led_pattern_factory_reset[] = 
{
    LED_LOCK,
    LED_ON(LED_RED), LED_WAIT(300),LED_OFF(LED_RED), LED_WAIT(300),
    LED_UNLOCK,
    LED_REPEAT(1, 2),
    LED_END

};



/*! \endcond led_patterns_well_named
 */


/*! \cond constant_well_named_tones 
    No Need to document these tones. Their access through functions such as
    appUiIdleActive() is the public interface.
 */
 
const ringtone_note app_tone_button[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_2[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_3[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_button_4[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

#ifdef INCLUDE_DFU
const ringtone_note app_tone_button_dfu[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};
#endif

const ringtone_note app_tone_button_factory_reset[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(C7, SEMIQUAVER),
    RINGTONE_STOP
};

#ifdef INCLUDE_AV
const ringtone_note app_tone_av_connect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_disconnect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_remote_control[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_av_link_loss[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};
#endif

const ringtone_note app_tone_hfp_connect[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6,  SEMIQUAVER),
    RINGTONE_NOTE(D6,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_link_loss[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};
        
const ringtone_note app_tone_hfp_sco_connected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(AS5, DEMISEMIQUAVER),
    RINGTONE_NOTE(DS6, DEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_disconnected[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(DS6, DEMISEMIQUAVER),
    RINGTONE_NOTE(AS5, DEMISEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_sco_unencrypted_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_ring[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),
    RINGTONE_NOTE(REST, SEMIQUAVER),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_NOTE(D7,   SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_ring_caller_id[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6,   SEMIQUAVER),
    RINGTONE_NOTE(G6,   SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_voice_dial[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_voice_dial_disable[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_answer[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_hangup[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_active[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS7, SEMIQUAVER),
    RINGTONE_NOTE(DS7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_mute_inactive[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(DS7, SEMIQUAVER),
    RINGTONE_NOTE(CS7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_hfp_talk_long_press[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_pairing[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_paired[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_pairing_deleted[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(A6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_volume[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_volume_limit[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_error[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_battery_empty[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_battery_low[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_NOTE(B6, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_power_on[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS5, SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_power_off[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(CS5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_paging_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(D7, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing_error[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_NOTE(B5, SEMIQUAVER),
    RINGTONE_STOP
};

const ringtone_note app_tone_peer_pairing_reminder[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_NOTE(A5,  SEMIQUAVER),
    RINGTONE_STOP
};

#ifdef INCLUDE_DFU
const ringtone_note app_tone_dfu[] =
{
    RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
    RINGTONE_NOTE(CS5, SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_NOTE(D5,  SEMIQUAVER),
    RINGTONE_STOP
};
#endif

//----------user define--------------
const ringtone_note app_tone_dut_mode[] =
{
	RINGTONE_TIMBRE(sine), RINGTONE_DECAY(16),
	RINGTONE_NOTE(G5, SEMIQUAVER),
	RINGTONE_NOTE(G5, SEMIQUAVER),
	RINGTONE_STOP
};

/*! \endcond constant_well_named_tones */

/*! \brief Play tone.
    \param tone The tone to play.
    \param interruptible If TRUE, always play to completion, if FALSE, the tone may be
    interrupted before completion.
    \param queueable If TRUE, tone can be queued behind already playing tone, if FALSE, the tone will
    only play if no other tone playing or queued.
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
    in client_lock when the tone finishes - either on completion, when interrupted,
    or if the tone is not played at all, because the UI is not currently playing tones.
    \param client_lock_mask A mask of bits to clear in the client_lock.
*/
void appUiPlayToneCore(const ringtone_note *tone, bool interruptible, bool queueable,
                       uint16 *client_lock, uint16 client_lock_mask)
{
#ifndef INCLUDE_TONES
    UNUSED(tone);
    UNUSED(interruptible);
    UNUSED(queueable);
#else
    /* Only play tone if it can be heard */
    if ((PHY_STATE_IN_EAR == appPhyStateGetState()) && appUserIsHeadsetPowerOn())
    {
        if (queueable || !appKymeraIsTonePlaying())
            appKymeraTonePlay(tone, interruptible, client_lock, client_lock_mask);
    }
    else
#endif
    {
        if (client_lock)
        {
            *client_lock &= ~client_lock_mask;
        }
    }
}

/*! \brief Play prompt.
    \param prompt The prompt to play.
    \param interruptible If TRUE, always play to completion, if FALSE, the prompt may be
    interrupted before completion.
    \param client_lock If not NULL, bits set in client_lock_mask will be cleared
    in client_lock when the prompt finishes - either on completion, when interrupted,
    or if the prompt is not played at all, because the UI is not currently playing prompts.
    \param client_lock_mask A mask of bits to clear in the client_lock.
*/
void appUiPlayPromptCore(voicePromptName prompt, bool interruptible, bool queueable,
                         uint16 *client_lock, uint16 client_lock_mask)
{
#ifndef INCLUDE_PROMPTS
    UNUSED(prompt);
    UNUSED(interruptible);
    UNUSED(queueable);
#else


    uiTaskData *theUi = appGetUi();
    PanicFalse(prompt < NUMBER_OF_PROMPTS);
#ifdef BM_POWER_ON
	/*! Do not play voice prompt if we in off state*/
	if((!appUserIsHeadsetPowerOn()) && (prompt != PROMPT_POWER_OFF))
	{
		UNUSED(prompt);
    	UNUSED(interruptible);
    	UNUSED(queueable);    
        if (client_lock)
        {
            *client_lock &= ~client_lock_mask;
        }
		return;
    }
#endif	
    /* Only play prompt if it can be heard */
    if ((PHY_STATE_IN_EAR == appPhyStateGetState()) && (prompt != theUi->prompt_last) &&
        (queueable || !appKymeraIsTonePlaying()))
    {
        const promptConfig *config = appConfigGetPromptConfig(prompt);
        FILE_INDEX *index = theUi->prompt_file_indexes + prompt;
        if (*index == FILE_NONE)
        {
            const char* name = config->filename;
            *index = FileFind(FILE_ROOT, name, strlen(name));
            /* Prompt not found */
            PanicFalse(*index != FILE_NONE);
        }
        appKymeraPromptPlay(*index, config->format, config->rate,
                            interruptible, client_lock, client_lock_mask);

        if (appConfigPromptNoRepeatDelay())
        {
            MessageCancelFirst(&theUi->task, UI_INTERNAL_CLEAR_LAST_PROMPT);
            MessageSendLater(&theUi->task, UI_INTERNAL_CLEAR_LAST_PROMPT, NULL,
                             appConfigPromptNoRepeatDelay());
            theUi->prompt_last = prompt;
        }
    }
    else
#endif
    {
        if (client_lock)
        {
            *client_lock &= ~client_lock_mask;
        }
    }
}

/*! \brief Report a generic error on LEDs and play tone */
void appUiError(void)
{
    //appUiPlayTone(app_tone_error);
    //appLedSetPattern(app_led_pattern_error, LED_PRI_EVENT);
}

/*! \brief Play HFP error tone and set LED error pattern.
    \param silent If TRUE the error is not presented on the UI.
*/
void appUiHfpError(bool silent)
{
	#if 0
    if (!silent)
    {
        appUiPlayTone(app_tone_error);
        appLedSetPattern(app_led_pattern_error, LED_PRI_EVENT);
    }
	#endif
	UNUSED(silent);
}

/*! \brief Play AV error tone and set LED error pattern.
    \param silent If TRUE the error is not presented on the UI.
*/
void appUiAvError(bool silent)
{
    if (!silent)
    {
//        appUiPlayTone(app_tone_error);
//        appLedSetPattern(app_led_pattern_error, LED_PRI_EVENT);
    }
}

/*! \brief Play power on prompt and LED pattern */
void appUiPowerOn(void)
{
    /* Enable LEDs */
    appLedEnable(TRUE);

    //init led mode
    appUiIdleActive();
    //set pair led
    appLedSetPattern(app_led_pattern_power_on, LED_PRI_EVENT);
    appUiPlayPrompt(PROMPT_POWER_ON);
}

/*! \brief Play power off prompt and LED pattern.
    \param lock The caller's lock, may be NULL.
    \param lock_mask Set bits in lock_mask will be cleared in lock when the UI completes.
 */
void appUiPowerOff(uint16 *lock, uint16 lock_mask)
{
	UNUSED(lock);
	UNUSED(lock_mask);
    appLedStopPattern(LED_PRI_LOW);
    appLedStopPattern(LED_PRI_MED);
    appLedStopPattern(LED_PRI_HIGH);
    appLedSetPattern(app_led_pattern_power_off, LED_PRI_EVENT);
    appUiPlayPrompt(PROMPT_POWER_OFF);
	MessageSendLater(appGetUiTask(), APP_LED_DISABLE,0,D_SEC(2));

}

/*! \brief Prepare UI for sleep.
    \note If in future, this function is modified to play a tone, it should
    be modified to resemble #appUiPowerOff, so the caller's lock is cleared when
    the tone is completed. */
void appUiSleep(void)
{
    appLedSetPattern(app_led_pattern_power_off, LED_PRI_EVENT);
    appLedEnable(FALSE);
}

/*change volume headset volume level*/
void appUserMainOutVolumeChange(volume_direction direction)
{	
	uint16 num_steps =	1;
	int16 hfp_change = (appConfigGetHfpVolumeStep() * num_steps);
	int16 av_change = (appConfigGetAvVolumeStep() * num_steps); 	

	if(direction == same_volume)
	{
		return;
	}
	else if(direction == decrease_volume)
	{
		av_change = -av_change;
		hfp_change = -hfp_change;
	}
	/*change volume*/
	if (appSmIsOutOfCase())
	{
		if (appHfpIsScoActive())
		{			 
		   appHfpVolumeStart(hfp_change);
		   appHfpVolumeStop(hfp_change);
		}
		else if (appScoFwdIsReceiving())
		{
			appScoFwdVolumeStart(hfp_change);
			appScoFwdVolumeStop(hfp_change);
		}
#ifdef INCLUDE_AV
		else if (appAvIsStreaming())
		{
			DEBUG_LOGF("Av streaming volume %x",av_change);
			appAvVolumeStart(av_change);
			appAvVolumeStop(av_change);
		}
#endif
		else if (appHfpIsConnected())
		{
			appHfpVolumeStart(hfp_change);
			appHfpVolumeStop(hfp_change);
		}
		else if (appScoFwdIsConnected())
		{
			appScoFwdVolumeStart(hfp_change);
			appScoFwdVolumeStop(hfp_change);
		}
		else
		{
			//appUiHfpError(FALSE);
		}
	}
}

/*! \brief Message Handler

    This function is the main message handler for the UI module, all user button
    presses are handled by this function.

    NOTE - only a single button config is currently defined for both earbuds.
    The following defines could be used to split config amongst the buttons on
    two earbuds.

        APP_RIGHT_CONFIG
        APP_SINGLE_CONFIG
        APP_LEFT_CONFIG
*/    
static void appUiHandleMessage(Task task, MessageId id, Message message)
{
    uiTaskData *theUi = (uiTaskData *)task;
    UNUSED(message);

    switch (id)
    {
        case UI_INTERNAL_CLEAR_LAST_PROMPT:
            theUi->prompt_last = PROMPT_NONE;
            break;


        /* HFP call/reject & A2DP play/pause */
        case APP_MFB_BUTTON_SINGLE_RELEASE:
            DEBUG_LOG("APP_MFB_BUTTON_PRESS");
            if (appChargerIsDisconnected())
            {
				MessageCancelAll(appGetUiTask(), APP_BUTTON_RELEASE_TIME_OUT);
				bm_press_count += 1;
				MessageSendLater(appGetUiTask(), APP_BUTTON_RELEASE_TIME_OUT,0,MUL_CLICK_INTERVAL);
            }
            break;

        
#ifdef BM_MUL_CLICK
		case APP_MFB_BUTTON_SHORT_PRESS:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_SHORT_PRESS");
			if(appUserIsHeadsetPowerOn() && (appSmIsOutOfCase()))
			{
                if (appHfpIsCallIncoming())
                    appHfpCallAccept();
                else if (appScoFwdIsCallIncoming())
                    appScoFwdCallAccept();
                /* If voice call active, hangup */
                else if (appHfpIsCallActive())
                {
                    appHfpCallHangup();
                }
                /* Sco Forward can be streaming a ring tone */
                else if (appScoFwdIsReceiving() && !appScoFwdIsCallIncoming())
                {
                    appScoFwdCallHangup();
                }
                /* If outgoing voice call, hangup */
                else if (appHfpIsCallOutgoing())
                {
                    appHfpCallHangup();
                }
                /* If AVRCP to handset connected, send play or pause */
                else if (appDeviceIsHandsetAvrcpConnected())
                    appAvPlayToggle(FALSE);
                /* If AVRCP is peer is connected and peer is connected to handset, send play or pause */
                else if (appDeviceIsPeerAvrcpConnectedForAv() && appPeerSyncIsComplete() && appPeerSyncIsPeerHandsetAvrcpConnected())
                    appAvPlayToggle(FALSE);
			}
		    break;
        
		case APP_MFB_BUTTON_DOUBLE_PRESS:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_DOUBLE_PRESS");
			if(appUserIsHeadsetPowerOn() && (appChargerIsDisconnected()))
            {
                if(appAvPlayStatus() == avrcp_play_status_playing)
                {
                    if(appConfigIsRealLeft())
                    {                   
                        appAvBackward();
                    }
                    else
                    {
                        appAvForward();
                    }
                }
            }         
		    break;
            
		case APP_MFB_BUTTON_TRIPLE_PRESS:
            DEBUG_LOG("UI_INTERNAL--------UI_MESSAGE_TRIPLE_BUTTON_TRIGGER");
			if(appUserIsHeadsetPowerOn() && (appChargerIsDisconnected()))
            {
            
                if(appConfigIsRealLeft())
                {
                    appUserMainOutVolumeChange(decrease_volume);
                }
                else
                {
                    appUserMainOutVolumeChange(increase_volume);
                }
            
            }         
		    break;
            
		case APP_MFB_BUTTON_QUARTIC_PRESS:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_QUARTIC_PRESS");
		    break;
            
		case APP_MFB_BUTTON_QUINTIC_PRESS:
            DEBUG_LOG("UI_INTERNAL--------UI_MESSAGE_QUINTUPLE_BUTTON_TRIGGER");            
			if(appUserIsHeadsetPowerOn() && (appSmIsOutOfCase()))
			{
				if(appGetState() == APP_STATE_HANDSET_PAIRING)
				{
				    DEBUG_LOG("------------EnterDutMode--------------");
                    
					appUiDutMode();
                    appUserSetDutState(TRUE);
					ConnectionEnterDutMode();
                    /*need to reset some system message*/
                    appUiClearMessageWhenPowerOff();
				}
#if 0
                else if(appGetState() == APP_STATE_PEER_PAIRING)
                {
                    DEBUG_LOG("------------appUserEnterPairingMode--------------");
    			    if(appConfigIsLeft() || (appConfigIsRight() && !appDeviceGetPeerBdAddr(NULL)))
                    {
                        DEBUG_LOG("APP_MFB_BUTTON_VVLONG_PRESS enter pairing mode");
                        appUserEnterPairingMode();
                    }         
                }
#endif                
			}
		    break;
            
		case APP_MFB_BUTTON_SEXTIC_PRESS:
		    break;
            
		case APP_BUTTON_RELEASE_TIME_OUT:
			{
				if(bm_press_count == 1)
				{
					MessageSend(appGetUiTask(), APP_MFB_BUTTON_SHORT_PRESS,0);
				}
				if(bm_press_count == 2)
				{
					MessageSend(appGetUiTask(), APP_MFB_BUTTON_DOUBLE_PRESS,0);
				}
				if(bm_press_count == 3)
				{
					MessageSend(appGetUiTask(), APP_MFB_BUTTON_TRIPLE_PRESS,0);
				}
				if(bm_press_count == 4)
				{
					MessageSend(appGetUiTask(), APP_MFB_BUTTON_QUARTIC_PRESS,0);
				}
				if(bm_press_count == 5)
				{
					MessageSend(appGetUiTask(), APP_MFB_BUTTON_QUINTIC_PRESS,0);
				}
				if(bm_press_count == 6)
				{
					MessageSend(appGetUiTask(), APP_MFB_BUTTON_SEXTIC_PRESS,0);
				}
				bm_press_count = 0;
			}
		    break;
#endif

#ifdef BM_POWER_ON
		case APP_POWER_ON_TIME_OUT:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_POWER_ON_TIME_OUT");
				appPowerEnterOffState();
			}
		    break;
        
		case APP_POWER_REBOOT:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_POWER_REBOOT");
				appPowerReboot();
			}
		    break;
#endif

		case APP_MFB_BUTTON_HELD_POWER_ON:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_HELD_POWER_ON");
			{
				/*device is not in case*/
				if(!appChargerIsConnected())
				{
					if(!appUserIsHeadsetPowerOn())
					{
						appUserPowerOn();
					}
				}
			}
		    break;

        case APP_MFB_BUTTON_HELD_POWER_OFF:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_HELD_POWER_OFF");
			/*headset power off*/
			if(appChargerIsDisconnected() && (appGetState() > APP_STATE_DFU_CHECK) )
			{								
				if(appUserGetPowerOffEnable())
				{
					appUserPowerOff();
				}		
			}
            break;
                                
        case APP_MFB_BUTTON_HELD_LONG_2S:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_HELD_LONG_1_5S");
			if(appUserIsHeadsetPowerOn() && (appSmIsOutOfCase()))
            {

                if (appHfpIsCallIncoming())
                {
                    appHfpCallReject();
                }
                else if (appScoFwdIsCallIncoming())
                {
                    appScoFwdCallReject();
                }
                else if((appGetState() != APP_STATE_HANDSET_PAIRING)&&(appGetState() != APP_STATE_PEER_PAIRING))
                {                   
                    if (appHfpIsConnected() && (!appHfpIsCall())) 
                    {
                        if(appHfpIsVoiceRecognitionActive())
                        {
                            appHfpCallVoiceDisable();
                        }
                        else
                        {
                            appHfpCallVoice();
                        }
                    }
                    else if(appConfigIsRight())
                    {
                        appScoFwdSyncVoice();
                    }
                }
            }
            break;

		case APP_MFB_BUTTON_HELD_PAIR_HANDSET:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_HELD_PAIR_HANDSET");
    		/*headset power off*/
    		if(appChargerIsDisconnected() && (appGetState() > APP_STATE_DFU_CHECK) && (appGetState() != APP_STATE_HANDSET_PAIRING))
    		{								
    			if(!appUserGetPowerOffEnable())
    			{
                    bdaddr peer_bd_addr;
                    //condition:left or right with no peer record 
    			    if(appConfigIsLeft() || (appConfigIsRight() && !appDeviceGetPeerBdAddr(&peer_bd_addr)))
                    {
                        DEBUG_LOG("APP_MFB_BUTTON_VVLONG_PRESS enter pairing mode");
                        appUserEnterPairingMode();
                    }         
    			}
    	
    		}
		    break;


        case APP_MFB_BUTTON_HELD_RESET:
            DEBUG_LOG("UI_INTERNAL--------APP_MFB_BUTTON_HELD_RESET");
            if(appChargerIsConnected())
            {
                //Stop Charge Led
                appLedCancelFilter(1);
                //Stop Dormant Timer
				MessageCancelAll(&appGetCharger()->task,CHARGER_COMPLETE_POWER_OFF);
                
                appUserSetResetState(TRUE);
                appUiButtonFactoryReset();
                appSmFactoryReset();
            }
            break;
        
            
#ifdef POWER_MANAGER
		case APP_BATTERY_UPDATE_DETECT:
			{
				/*battery update detect*/
				if(appUserIsHeadsetPowerOn())
				{
					appUserBatteryLevelUpdateDetect();
					MessageSendLater(appGetUiTask(),APP_BATTERY_UPDATE_DETECT, 0, D_SEC(2));
				}
				
			}
		    break;

		case APP_IND_BAT_LEVEL_TO_PHONE:
			{
				MessageCancelAll(appGetUiTask(),APP_IND_BAT_LEVEL_TO_PHONE);
				if(appDeviceIsHandsetConnected() && (!appUserIsPageScanDisable()))
					appUserIndBatteryLevelToPhone();
			}
		    break;
        
		case APP_LOW_BAT_WARNING:
			{                
                DEBUG_LOG("UI_INTERNAL--------APP_LOW_BAT_WARNING");
				MessageCancelAll(appGetUiTask(),APP_LOW_BAT_WARNING);
				/*play low battery prompt*/
				appUiBatteryLow();
				MessageSendLater(appGetUiTask(),APP_LOW_BAT_WARNING,0,LOW_BAT_WARNING_INTERVAL);
			}
		    break;
        
#endif

		case APP_POWER_ON_RECONNECT:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_POWER_ON_RECONNECT");
				if(appChargerIsDisconnected())
				{
					if(appConfigIsLeft() && (!appSmIsPairing()))
					{
						if((!appDeviceIsHandsetA2dpConnected()) && (!appDeviceIsHandsetHfpConnected()))
						{
                            DEBUG_LOG("DO !");
                            appLedSetPattern(app_led_pattern_linkback_handset, LED_PRI_MED);
                            //sync led status
                            appScoFwdSyncLedLinkBack();
                            
							appConnRulesResetEvent(RULE_EVENT_USER_CONNECT);
							appConnRulesSetRuleComplete(CONN_RULES_CONNECT_HANDSET);
							appConManagerAllowHandsetConnect(TRUE);
							appPeerSyncSend(FALSE); 			
							appHfpConnectHandset(); 								
							appAvConnectHandset(RULE_POST_HANDSET_CONNECT_ACTION_PLAY_MEDIA);
							MessageSendLater(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING,0,(POWER_ON_LINKBACK_TIMEOUT - POWER_ON_LINKBACK_HANDSET_TIME));
						}
					}
				}
			}
		    break;

		case APP_LED_DISABLE:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_LED_DISABLE");
				if(appChargerIsDisconnected())
				{
					appLedEnable(FALSE);
					appGetPower()->lock &= 0x00;
				}
			}
		    break;

		case APP_POWER_OFF_ENABLE:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_POWER_OFF_ENABLE");
				appUserEnablePowerOff();
			}
		    break;
        
		case APP_USER_TRANFER_PEER_TDL:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_USER_TRANFER_PEER_TDL");
				MessageCancelAll(appGetUiTask(),APP_USER_TRANFER_PEER_TDL);
				if(appPeerSyncIsComplete())
					appPeerSigTranferTdlMessage();
			}
		    break;
        
		case APP_USER_START_ADV_BDADDR:
			{	
                DEBUG_LOG("UI_INTERNAL--------APP_USER_START_ADV_BDADDR");
				appScanBleStopScanning();
				appAdvStartAdvCurrentUseBdaddr();
			}
		    break;
        
		case APP_USER_STOP_BLE_SCAN:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_USER_STOP_BLE_SCAN");
				appScanBleStopScanning();
				if(appConfigIsRight() && appPeerSyncIsChargerConnected() && (!appPeerSyncIsPeerHandsetA2dpConnected())
					&& (!appPeerSyncIsPeerHandsetHfpConnected()) && appUserGetCanChangeTdl())
				{
					/*need to change to left addr*/
					appSmDisconnectAllLink();
					appPeerSigExchangeTdlRecord();
					appUserDisableAllRfActivity();							
					MessageSendLater(appGetUiTask(),APP_CHANGE_LOCAL_ADDR,0,500);
					
				}
				else if((!appPeerSyncIsComplete()) && appConfigIsRight() && appDeviceGetPeerBdAddr(NULL) && (!appUiIsMasterAddrInUsed()) && appUserGetCanChangeTdl())
				{
					appSmDisconnectAllLink();
					appPeerSigExchangeTdlRecord();
					appUserDisableAllRfActivity();	
					MessageSendLater(appGetUiTask(),APP_CHANGE_LOCAL_ADDR,0,500);
				}
				else
				{			
					appAdvStartAdvCurrentUseBdaddr();					
					
				}
			}
		    break;
        
		case APP_CHANGE_LOCAL_ADDR:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_CHANGE_LOCAL_ADDR");
				MessageCancelAll(appGetUiTask(),APP_CHANGE_LOCAL_ADDR);
				if(appChargerIsDisconnected())
				{
					appUserDisableAllRfActivity();				
					if(appUserChangeLocalBdaddrAttemp())
					{
//						appUiPlayTone(app_tone_button);	 
						
						appA2dpDestoryAvInstance();
						/*need to change connection lib*/				
						ConnectionInitTrustDevice();				
		                appDeviceReinit();							
						appUserSetPageScanDisableFlag(FALSE);							
						appAdvStartAdvCurrentUseBdaddr();											
						appConManagerSetDeviceStateDisconnected();
						appConManagerAllowHandsetConnect(TRUE);		
						reconnect_num = 2;
						
						if(appConfigIsLeft())
						{
							if(appDeviceGetHandsetBdAddr(NULL))
							{		
							    //show link back led
                                appLedSetPattern(app_led_pattern_linkback_handset, LED_PRI_MED);
                                //sync led status
                                appScoFwdSyncLedLinkBack();
                                
								appPairingReinit();						
								MessageSendLater(appGetUiTask(),APP_USER_RECONNECT_HANDSET,0,300);								
								MessageSendLater(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING,0,POWER_ON_LINKBACK_TIMEOUT);
							}
							else
							{
								appUserEnterPairingMode();
							}
						}
						else 
						{						
							MessageSendLater(appGetUiTask(),APP_USER_RECONNECT_PEER,0,500); 
						}
					
					}
					else
					{
						MessageSendLater(appGetUiTask(),APP_CHANGE_LOCAL_ADDR,0,20);
					}
				}
			}
		    break;
		
		case APP_USER_RECONNECT_PEER:
			{
				DEBUG_LOG("UI_INTERNAL--------APP_USER_RECONNECT_PEER");
				if( (!appChargerIsConnected()))
				{
					if(appPeerSyncIsComplete() && appDeviceIsPeerA2dpConnected())
					{
						MessageCancelAll(appGetUiTask(),APP_USER_RECONNECT_PEER);
					}
					else
					{
						reconnect_num -= 1;
						appPeerSyncSend(FALSE);
						appScoFwdConnectPeer();
						appAvConnectPeer();
						if(reconnect_num)
						{
							MessageSendLater(appGetUiTask(),APP_USER_RECONNECT_PEER,0,(POWER_ON_LINKBACK_TIMEOUT/2));
						}
                        else
                        {
							MessageSendLater(appGetUiTask(),APP_PEER_CONNECT_STATE_CHECK,0,500);
                        }
					}
				}
			}
		    break;
            
            
		case APP_PEER_CONNECT_STATE_CHECK:
            DEBUG_LOG("UI_INTERNAL--------APP_PEER_CONNECT_STATE_CHECK");
            if(appConfigIsRight())
            {
                if((!appPeerSyncIsComplete() || appPeerSyncIsChargerConnected()) && !appDeviceIsHandsetConnected())
                {
                    appGetUi()->master_addr_in_used = FALSE;
                    appScanBleStartScanning();
                    MessageSendLater(appGetUiTask(),APP_USER_STOP_BLE_SCAN,0,D_SEC(1));
                }
            }
            break;

            
		case APP_USER_RECONNECT_HANDSET:
			{			
                DEBUG_LOG("UI_INTERNAL--------APP_USER_RECONNECT_HANDSET");
				/*work after change addr successed, reconnect to handset device*/
				if((!appDeviceIsHandsetA2dpConnected()) && (!appDeviceIsHandsetHfpConnected()) && (!appChargerIsConnected()))
				{
					reconnect_num -= 1;
					DEBUG_LOG("APP_USER_RECONNECT_HANDSET");				
					/* Connect AVRCP and A2DP to handset */
					appAvConnectHandset(RULE_POST_HANDSET_CONNECT_ACTION_PLAY_MEDIA);	
					appHfpConnectHandset(); 
					if(reconnect_num)
					{
						MessageSendLater(appGetUiTask(),APP_USER_RECONNECT_HANDSET,0,D_SEC(3));
					}
																				
				}
			}
		    break;

		case APP_CHARGER_STATE_CONFIRM:
			{
				appGetUi()->usb_attached = TRUE;
			}
		    break;

		case APP_LINKBACK_FAILED_ENTER_PAIRING:
			{
                DEBUG_LOG("UI_INTERNAL--------APP_LINKBACK_FAILED_ENTER_PAIRING");
                MessageCancelAll(appGetUiTask(), APP_LINKBACK_FAILED_ENTER_PAIRING);
				if(appDeviceGetHandsetBdAddr(NULL))
				{
					if(appConfigIsLeft() && (!appDeviceIsHandsetA2dpConnected()) && (!appDeviceIsHandsetHfpConnected()))
					{
						DEBUG_LOG("APP_LINKBACK_FAILED_ENTER_PAIRING enter pairing mode");
						appUserEnterPairingMode();
					}
				}
                else if(!appDeviceGetHandsetBdAddr(NULL) && (appDeviceGetPeerBdAddr(NULL)) && \
                    !appDeviceIsPeerConnected() && !appDeviceIsHandsetAnyProfileConnected() && \
                    (appGetState() != APP_STATE_HANDSET_PAIRING))
                {
                    if(appConfigIsLeft())
                    {
						DEBUG_LOG("APP_LINKBACK_FAILED_ENTER_PAIRING enter pairing mode - 2");
						appUserEnterPairingMode();
                    }
                }
			}
		    break;

        
    case APP_LINK_LOSS_LINKBACK_PEER_PROCESS:
        DEBUG_LOG("UI_INTERNAL--------APP_LINK_LOSS_LINKBACK_PEER_PROCESS");
        
        DEBUG_LOG("#appDeviceIsHandsetConnected : %d #",appDeviceIsHandsetConnected());            
        DEBUG_LOG("#appDeviceIsHandsetHfpConnected : %d #",appDeviceIsHandsetHfpConnected());
        DEBUG_LOG("#appDeviceIsHandsetAnyProfileConnected : %d #",appDeviceIsHandsetAnyProfileConnected());
        DEBUG_LOG("#appDeviceIsHandsetAvrcpConnected : %d #",appDeviceIsHandsetAvrcpConnected());
        DEBUG_LOG("#appDeviceIsHandsetA2dpConnected : %d #",appDeviceIsHandsetA2dpConnected());
        DEBUG_LOG("#appDeviceIsPeerConnected : %d #",appDeviceIsPeerConnected());
        DEBUG_LOG("#appDeviceIsPeerA2dpConnected : %d #",appDeviceIsPeerA2dpConnected());
        DEBUG_LOG("#appDeviceIsPeerScoFwdConnected : %d #",appDeviceIsPeerScoFwdConnected());
        DEBUG_LOG("#appDeviceIsPeerAvrcpConnected : %d #",appDeviceIsPeerAvrcpConnected());
        DEBUG_LOG("#appPeerSyncIsPeerPairing : %d #",appPeerSyncIsPeerPairing());
        DEBUG_LOG("#appPeerSyncIsComplete : %d #",appPeerSyncIsComplete());
        DEBUG_LOG("#appPeerSyncIsPeerHandsetA2dpConnected : %d #",appPeerSyncIsPeerHandsetA2dpConnected());
        DEBUG_LOG("#appPeerSyncIsPeerHandsetHfpConnected : %d #",appPeerSyncIsPeerHandsetHfpConnected());
        if(appSmIsOutOfCase())
        {
            if(!appPeerSyncIsComplete() && !appDeviceIsPeerConnected())
            {
                appConnRulesSetEvent(appGetSmTask(), RULE_EVENT_PEER_LINK_LOSS);
                MessageSendLater(appGetUiTask(), APP_LINK_LOSS_LINKBACK_PEER_PROCESS, 0, D_SEC(5));
            }
            else
            {
                //nothing
            }
        }
        break;

    case APP_POWER_ON_PRESS_CHECK:
        DEBUG_LOG("BUTTON--------APP_POWER_ON_PRESS_CHECK");
        if(appSmIsOutOfCase())
        {
            if(!PWOER_KEY_IS_PRESSING())
            {
                appGetUi()->power_on_key_check_num ++;
    
                //we need 300ms high level period for debounce
                if((appGetState() > APP_STATE_FACTORY_RESET)&&(appGetUi()->power_on_key_check_num >= 2))
                {                        
                    MessageSend(appGetUiTask(), APP_POWER_OFF_ENABLE,0);
                    appGetUi()->power_on_key_check_num = 0;
                    MessageCancelAll(appGetUiTask(), APP_POWER_ON_PRESS_CHECK);
    
                }
                else
                {
                    MessageSendLater(appGetUiTask(), APP_POWER_ON_PRESS_CHECK,0, 100);
                }
    
            }
            else
            {
                appGetUi()->power_on_key_check_num = 0;
                MessageSendLater(appGetUiTask(), APP_POWER_ON_PRESS_CHECK,0, 100);
            }
    
        }
        break;

    case APP_LINK_LOSS:
        DEBUG_LOG("BUTTON--------APP_LINK_LOSS");
        if(appSmIsOutOfCase())
        {
            appScanBleStopScanning();
            appConnRulesSetEvent(appGetSmTask(), RULE_EVENT_HANDSET_LINK_LOSS);
        }
        break;

    case APP_RESET_LED_STOP:
        DEBUG_LOG("BUTTON--------APP_RESET_LED_STOP");
        appUserSetResetState(FALSE);
        if(ChargerStatus() == STANDBY)
        {
            DEBUG_LOG("Restart timer");
            MessageSendLater(&appGetCharger()->task,CHARGER_COMPLETE_POWER_OFF,0,D_SEC(60));
        }
        break;

       
        
    }
}

/*! brief Initialise UI module */
void appUiInit(void)
{
    uiTaskData *theUi = appGetUi();
    
    /* Set up task handler */
    theUi->task.handler = appUiHandleMessage;
    
    /* Initialise input event manager with auto-generated tables for
     * the target platform */
    theUi->input_event_task = InputEventManagerInit(appGetUiTask(), InputEventActions,
                                                    sizeof(InputEventActions),
                                                    &InputEventConfig);

    memset(theUi->prompt_file_indexes, FILE_NONE, sizeof(theUi->prompt_file_indexes));

    theUi->prompt_last = PROMPT_NONE;
	theUi->power_on_flag = FALSE;
	theUi->power_off_enable = FALSE;
    theUi->reset_flag = FALSE;
    theUi->dut_flag = FALSE;
    theUi->handset_link_loss_flag = FALSE;
#ifdef SINGLE_BDADDR
	theUi->page_scan_disable = FALSE;
	theUi->master_addr_in_used = FALSE;

#endif
#ifdef POWER_MANAGER
	theUi->bat_last_percent = 0;
	theUi->bat_last_state = battery_level_unknown;
	theUi->auto_switch_off_timer = 0;
#endif
    theUi->power_on_key_check_num = 0;
    theUi->receive_peer_package_status = RECEIVE_PACKAGE_ALL;    

}


#ifdef BM_POWER_ON

void appUserSetHeadsetPowerState(bool OnOff)
{
	uiTaskData *theUi = appGetUi();
	theUi->power_on_flag = OnOff;
}

bool appUserIsHeadsetPowerOn(void)
{
	uiTaskData *theUi = appGetUi();	
	return theUi->power_on_flag ? TRUE:FALSE;

}

static void appUserRegisterPhyStateTask(void)
{
	smTaskData *sm = appGetSm();

	/* Register with physical state manager to get notification of state
 	* changes such as 'in-case', 'in-ear' etc */
	appPhyStateRegisterClient(&sm->task);
}

void appUserPowerOn(void)
{	
	/*only allow user to power if not in case */
	if(appChargerIsDisconnected() && (!appUserIsHeadsetPowerOn()))
	{
        
		MessageCancelAll(appGetUiTask(),APP_POWER_ON_TIME_OUT);
		appSetState(APP_STATE_STARTUP);				
		appUserSetHeadsetPowerState(TRUE);
		appUiPowerOn();
		appUserRegisterPhyStateTask();
		MessageSendLater(appGetUiTask(), APP_BATTERY_UPDATE_DETECT,0, D_SEC(20));
                
//		MessageSendLater(appGetUiTask(), APP_POWER_OFF_ENABLE,0,DISABLE_POWER_OFF_TIMEOUT);
	#ifdef SINGLE_BDADDR
		if(appDeviceGetPeerBdAddr(NULL))
		{			
			appScanBleStartScanning();
			MessageSendLater(appGetUiTask(),APP_USER_STOP_BLE_SCAN,0,D_SEC(3));	
            if(appConfigIsLeft())
            {
                appAdvStartAdvCurrentUseBdaddr();
            }
		}
		else
		{
			MessageSendLater(appGetUiTask(),APP_USER_START_ADV_BDADDR,0,D_SEC(3));
		}
	#endif
        appTaskListMessageSendId(appGetCharger()->client_tasks, CHARGER_MESSAGE_DETACHED);
		if(appDeviceGetHandsetBdAddr(NULL) && (!appDeviceIsHandsetA2dpConnected())
			&& (appDeviceGetPeerBdAddr(NULL)) && (!appSmIsInDfuMode()))
		{
			if(appConfigIsLeft())
			{
                DEBUG_LOG("appUserPowerOn - case 1");
				if(appPeerSyncIsComplete())
					MessageSendLater(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING, 0, POWER_ON_LINKBACK_TIMEOUT);
				else
					MessageSendLater(appGetUiTask(), APP_POWER_ON_RECONNECT,0,POWER_ON_LINKBACK_HANDSET_TIME);
			}
				
		}
        //To fix the problem of the left not automatically enter handset pair(NO.21)
        else if(!appDeviceGetHandsetBdAddr(NULL) && (appDeviceGetPeerBdAddr(NULL)) && appConfigIsLeft() && (!appSmIsInDfuMode()))
        {
            DEBUG_LOG("appUserPowerOn - case 2");
            MessageSendLater(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING, 0, POWER_ON_LEFT_SINGLE_TIMEOUT);
        }
        //To fix the problem of left has a phone record, the pair of ears will not automatically enter handset pair(NO.22)
        else if(appDeviceGetHandsetBdAddr(NULL) && !appDeviceGetPeerBdAddr(NULL) && appConfigIsLeft() && (!appSmIsInDfuMode()))
        {
            DEBUG_LOG("appUserPowerOn - case 3");
            MessageSendLater(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING, 0, POWER_ON_LINKBACK_TIMEOUT);
        }
	}
}


void appUserPowerOff(void)
{
	appPowerOffRequest();
	if(appDeviceIsPeerConnected())
		appScoFwdUserPowerOff();
	/*need to reset some system message*/
	appUiClearMessageWhenPowerOff();

	appUserSetHeadsetPowerState(FALSE);
}

void appUserEnablePowerOff(void)
{
	 uiTaskData *theUi = appGetUi();	 
	 theUi->power_off_enable = TRUE;
}

void appUserDisablePowerOff(void)
{
	uiTaskData *theUi = appGetUi();
	 theUi->power_off_enable = FALSE;
	
}

bool appUserGetPowerOffEnable(void)
{
	uiTaskData *theUi = appGetUi(); 	
	return theUi->power_off_enable ? TRUE:FALSE;

}

void appUserCancelLinkBackToHandset(void)
{
	MessageCancelAll(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING);
	MessageCancelAll(appGetUiTask(),APP_POWER_ON_RECONNECT);
}


#endif


#ifdef POWER_MANAGER
void appUserIndBatteryLevelToPhone(void)
{
	uint8 temp = appBatteryGetPercent();
							
	if(temp > 90)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel10);	
	}
	else if(temp > 80)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel9);
	}
	else if(temp > 71)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel8);
	}
	else if(temp > 65)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel7);
	}
	else if(temp > 60)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel6);
	}
	else if(temp > 55)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel5);
	}
	else if(temp > 45)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel4);
	}
	else if(temp > 30)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel3);
	}
	else if(temp > 15)
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel2);
	}
	else
	{
		HfpAtCmdRequest(hfp_primary_link, StartBatteryLevelIphone);
		HfpAtCmdRequest(hfp_primary_link, BatteryLevel1);
	}
	return;

					
}


void appUserBatteryLevelUpdateDetect(void)
{	
	battery_level_state new_state = appBatteryGetState();
	uint8 percent = appBatteryGetPercent();
	uiTaskData *theUi = appGetUi(); 

	DEBUG_LOG("appUserBatteryIndicateDetection");
    DEBUG_LOG("appBatteryGetState : %d", appBatteryGetState());
    DEBUG_LOG("appBatteryGetPercent : %d", appBatteryGetPercent());
    DEBUG_LOG("appBatteryGetVoltage : %d", appBatteryGetVoltage());

	/*battery percent level change */						
	if(appChargerIsDisconnected())
	{
		if (percent != theUi->bat_last_percent || (theUi->bat_last_percent == battery_level_unknown))
		{
			if(percent < theUi->bat_last_percent)
				MessageSend(appGetUiTask(),APP_IND_BAT_LEVEL_TO_PHONE,0);	
			
			theUi->bat_last_percent = percent;

			if(theUi->bat_last_state != new_state) 
			{
				if(new_state == battery_level_too_low)
				{
					/*need to power off now*/
					if(appUserIsHeadsetPowerOn())
					{
						DEBUG_LOG("battery too low to power off");						
						appUserPowerOff();

					}
				}
				else if(new_state == battery_level_critical)
				{
					DEBUG_LOG("low battery warning");
					MessageSend(appGetUiTask(),APP_LOW_BAT_WARNING,0);
					
				}
                else if(new_state >= battery_level_low)
				{
					DEBUG_LOG("low battery high");
					MessageCancelAll(appGetUiTask(),APP_LOW_BAT_WARNING);
					
				}
				theUi->bat_last_state = new_state;
			}
			
		}
		else if(percent == 0)
		{
			if(appUserIsHeadsetPowerOn() && (new_state == battery_level_too_low))
			{	
				appUserPowerOff();
			}
			theUi->bat_last_percent = percent;
			theUi->bat_last_state = new_state;
		}

	}
	else
	{
		theUi->bat_last_percent = percent;
	}
	
	return;

}

#endif


#ifdef BM_ENTER_HANDSET_PAIRING
/*enter handset pairing mode*/
void appUserEnterPairingMode(void)
{
	if (appChargerIsDisconnected())
	{
		/*only work if user not in pairing and dfu mode*/
		if((!appSmIsPairing()) && (!appSmIsInDfuMode()) && (!appUserIsShutingDown()))
		{
		    DEBUG_LOG("###do#####");
			if(appGetState() == APP_STATE_PEER_PAIRING ) 
			{				
				/*user button press to enter headset Pairing Mode*/
				appPairingPeerPairCancel();
				appConnRulesResetEvent(RULE_EVENT_STARTUP);
			}
			MessageCancelAll(appGetUiTask(),APP_POWER_ON_RECONNECT);
            MessageCancelAll(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING);

            //clear link loss led flag
            appUserSetLinkLossState(FALSE);
            
            //Avoid the third phone connection failure 
            appDestroyAvInstance();
            
//			appSetState(APP_STATE_OUT_OF_CASE_IDLE);			
			appSetState(APP_STATE_HANDSET_PAIRING);
		 	appConManagerAllowHandsetConnect(TRUE);
		}
	}


}
#endif


bool appUserIsShutingDown(void)
{
	return appGetPower()->user_initiated_shutdown ? TRUE:FALSE;
}


#ifdef SINGLE_BDADDR

/*
	@breif: read page scan flag which is control page scan rule working
	@param: void
	@return bool , TRUE for disable page scan, FALSE page scan can work
*/
bool appUserIsPageScanDisable(void)
{
	uiTaskData *theUi = appGetUi();	
	return theUi->page_scan_disable ? TRUE:FALSE;
}

/*
	@breif: set page scan flag which is control page scan rule working
	@param: bool , TRUE for disable page scan, FALSE page scan can work
	@return void
*/
void appUserSetPageScanDisableFlag(bool OnOff)
{
	uiTaskData *theUi = appGetUi();	
	theUi->page_scan_disable = OnOff;
}

void appUserDisableAllRfActivity(void)
{
	bdaddr addr;
	appUserSetPageScanDisableFlag(TRUE);
	if(appDeviceGetPeerBdAddr(&addr))
	{
		appConManagerSendCloseAclRequest(&addr,TRUE);
	}
	MessageCancelAll(appGetUiTask(),APP_USER_START_ADV_BDADDR);
	MessageCancelAll(appGetUiTask(),APP_USER_STOP_BLE_SCAN);
	MessageCancelAll(appGetUiTask(),APP_POWER_ON_RECONNECT);
	MessageCancelAll(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING);
    MessageCancelAll(appGetUiTask(),APP_USER_TRANFER_PEER_TDL);
    MessageCancelAll(appGetUiTask(),APP_LINK_LOSS);
	/*disable all work about rf*/
	ConnectionDmBleSetAdvertiseEnable(FALSE);
	ConnectionDmBleSetScanEnable(FALSE);
	ConnectionWriteScanEnable(hci_scan_enable_off);
	
}

/*
	@breif: attemp to change local bluetooth addr to peer device addr
	@param: void
	@return bool, operation is succeed or not
*/
bool appUserChangeLocalBdaddrAttemp(void)
{
	uint16 attempts_remaining = OVERRIDE_BD_ADDR_MAX_ATTEMPTS;
	bdaddr addr;
	bdaddr peer_addr;
	
	
	appUiGetStoreAddr(&addr, PSKEY_CURRENT_USE_ADDR);
	appUiGetStoreAddr(&peer_addr, PSKEY_PEER_ADDR_BACKUPS);
	
	DEBUG_LOGF("current, bdaddr %04x:%02x:%06x", addr.nap, addr.uap, addr.lap);
	DEBUG_LOGF("peer  bdaddr %04x:%02x:%06x", peer_addr.nap, peer_addr.uap, peer_addr.lap);
	/*decide which addr we need to change*/
	if(BdaddrIsSame(&addr,&peer_addr))
	{
		DEBUG_LOG("USE LOCAL ADDR");
		appUiGetStoreAddr(&addr,PSKEY_LOCAL_ADDR_BACKUPS);
	}
	else
	{
		DEBUG_LOG("USE PEER ADDR");
		appUiGetStoreAddr(&addr,PSKEY_PEER_ADDR_BACKUPS);
	}

	while (attempts_remaining && !VmOverrideBdaddr(&addr))
	{
		
	    attempts_remaining--;
	}
		
	if(attempts_remaining)
	{
		appGetInit()->appInitIsLeft = addr.lap & 0x01;
		appUiConvertBdaddrToPsStore(&addr, PSKEY_CURRENT_USE_ADDR);
		
        return  TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*
	@breif: convert bdaddr to data to store in pskey
	@param: addr, pointer to bdaddr 
	@param:	pskey, pksey which want to store in
	@return bool, operation is succeed or not
*/
bool appUiConvertBdaddrToPsStore(const bdaddr *addr, uint16 pskey )
{
	uint16 data[4];
	
	if(addr == NULL)
		return FALSE;

	data[0] = (addr->lap >> 16) & 0xff;
	data[1] = addr->lap & 0xffff;
	data[2] = addr->uap & 0xff;
	data[3] = addr->nap;

	PsStore(pskey, data, 4);
	return TRUE;
}


/*
	@breif: get bdaddr form specified pskey
	@param: addr, pointer to bdaddr 
	@param:	pskey, pksey which bdaddr store in
	@return bool, operation is succeed or not
*/
bool appUiGetStoreAddr(bdaddr *addr, uint16 pskey)
{
	uint16 data[4];
	memset(data,0,8);
	
	if(PsRetrieve(pskey, data, 4) == 4)
	{
		addr->lap = (uint32)(((uint32)data[0] << 16) | (data[1]));
        addr->uap = (uint8)(data[2] & 0xff);
		addr->nap = data[3];
		return TRUE;
	}
	return FALSE;
}

/*
	@breif: check if device is using raw device addr working now
	@param: void
	@return bool, TRUE for using raw device addr, FALSE is not
*/
bool appUiIsUseLocalAddrNow(void)
{
	bdaddr local_addr;
	bdaddr current_addr;
	
	appUiGetStoreAddr(&local_addr,PSKEY_LOCAL_ADDR_BACKUPS);
	if(!appUiGetStoreAddr(&current_addr,PSKEY_CURRENT_USE_ADDR))
	{
		return TRUE;
	}
	
	if(BdaddrIsSame(&local_addr, &current_addr))
	{
		return TRUE;
	}

	return FALSE;
	
}

/*
	@breif: check if master addr is using for peer device
	@param: void
	@return: master addr is already in use
*/
bool appUiIsMasterAddrInUsed(void)
{	
	uiTaskData *theUi = appGetUi();	
	return theUi->master_addr_in_used ? TRUE:FALSE;
}

/*
	@breif:check if the user baddr is match with tdl pskey
	@param: void
	@return: bool, TRUE is match, FALSE is not
*/
bool appUiIsBdaddrMatchWithTdl(void)
{
	uint16 data[3];
	uint8 i;
	bdaddr current_addr;
	bdaddr record_addr;
    
    DEBUG_LOG("appUiIsBdaddrMatchWithTdl");
        
	if(appUiGetStoreAddr(&current_addr,PSKEY_LOCAL_ADDR_BACKUPS))
	{
	
        DEBUG_LOG("appUiGetStoreAddr, bdaddr %04x:%02x:%06x",
                    current_addr.nap, current_addr.uap, current_addr.lap);
        
		for(i=0;i<8;i++)
		{
			PsRetrieve(110+i, data, 3);
			record_addr.nap = data[0];
			record_addr.uap = (uint8)((data[1]>>8) & 0xff);
			record_addr.lap = (uint32)((uint32)((data[1] & 0xff) << 16)| data[2]);			
			if(BdaddrIsSame(&current_addr, &record_addr))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

/*
	@breif:check if the receive package is match with current addr
	@param: void
	@return: bool, TRUE is match, FALSE is not
*/

bool appCheckReceiveMatchWithTdl(void)
{
	uint16 data[3];
	uint8 i, attr_verify = FALSE, base_verify = FALSE;
	bdaddr current_addr,backup_addr;
	bdaddr record_addr;
    
    DEBUG_LOG("appUiIsBdaddrMatchWithTdl");
        
	if(appUiGetStoreAddr(&current_addr,PSKEY_CURRENT_USE_ADDR))
	{
	
        DEBUG_LOG("appUiGetStoreAddr, bdaddr %04x:%02x:%06x",
                    current_addr.nap, current_addr.uap, current_addr.lap);

        //get unused addr
        if(appUiGetStoreAddr(&backup_addr,PSKEY_LOCAL_ADDR_BACKUPS))
        {
            if(BdaddrIsSame(&backup_addr, &current_addr))
            {
                appUiGetStoreAddr(&backup_addr,PSKEY_PEER_ADDR_BACKUPS);
            }
        }
        
		for(i=0;i<8;i++)
		{
			PsRetrieve(220+i, data, 3);
			record_addr.nap = data[0];
			record_addr.uap = (uint8)((data[1]>>8) & 0xff);
			record_addr.lap = (uint32)((uint32)((data[1] & 0xff) << 16)| data[2]);	
            //The address currently used should appear in the package received
			if(BdaddrIsSame(&current_addr, &record_addr))
			{
				attr_verify = TRUE;
			}
            //Unused addresses should not appear in received packets
            if(BdaddrIsSame(&backup_addr, &record_addr))
            {            
                DEBUG_LOG("Verify fail - 1");
                return FALSE;
            }

            PsRetrieve(242+i, data, 3);
			record_addr.nap = data[0];
			record_addr.uap = (uint8)((data[1]>>8) & 0xff);
			record_addr.lap = (uint32)((uint32)((data[1] & 0xff) << 16)| data[2]);		
            //The address currently used should appear in the package received
			if(BdaddrIsSame(&current_addr, &record_addr))
			{
				base_verify = TRUE;
			}            
            //Unused addresses should not appear in received packets
            if(BdaddrIsSame(&backup_addr, &record_addr))
            {
                DEBUG_LOG("Verify fail - 2");
                return FALSE;
            }
		}
	}

    if((attr_verify == TRUE) && (base_verify == TRUE))
    {
        return TRUE;
    }
    else
    {
        DEBUG_LOG("Verify fail - 3");
        return FALSE;
    }
    
}

/*
	@breif: clear all pskey which used to store tdl backups data
	@param: void
	@return void
*/
void appUiClearTdlBackups(void)
{
	uint16 ps_index;

	for(ps_index = PSKEY_LOCAL_ADDR_BACKUPS; ps_index<= 250; ps_index++)
	{
		PsStore(ps_index,NULL,0);
	}

    for(ps_index = 100; ps_index <= 150; ps_index++ )
	{
		PsStore(ps_index,NULL,0);
	}
}


void appUiClearMessageWhenPowerOff(void)
{
	MessageCancelAll(appGetUiTask(), APP_LOW_BAT_WARNING);
	MessageCancelAll(appGetUiTask(), APP_IND_BAT_LEVEL_TO_PHONE);
	MessageCancelAll(appGetUiTask(),APP_USER_STOP_BLE_SCAN);
	MessageCancelAll(appGetUiTask(),APP_USER_START_ADV_BDADDR);
	MessageCancelAll(appGetUiTask(),APP_CHANGE_LOCAL_ADDR);
	MessageCancelAll(appGetUiTask(),APP_LINKBACK_FAILED_ENTER_PAIRING);
	MessageCancelAll(appGetUiTask(),APP_POWER_ON_RECONNECT);
	appAdvStopAdvertising();
	appScanBleStopScanning();
	
}

#endif


void appUserSetAudiokey(uint32 key, const uint16 *data, uint16 length )
{

	DEBUG_LOG("data length %d", length);
    if(PsUpdateAudioKey(key,data,length,0,length))
	{

		DEBUG_LOG("sizeof %x",sizeof(data));
	}
	
}

void appDestroyAvInstance(void)
{
    bdaddr bd_addr;
    avInstanceTaskData *theInst;

    DEBUG_LOG("#appDestroyAvInstance#");

    /* Get handset device address */
    if (appDeviceGetHandsetBdAddr(&bd_addr))
    {
        DEBUG_LOG("#true#");
        /* Check if AV instance to this device already exists */
        theInst = appAvInstanceFindFromBdAddr(&bd_addr);

        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_A2DP_DESTROY_REQ, NULL, &appA2dpGetLock(theInst));
        
        MessageSendConditionally(&theInst->av_task, AV_INTERNAL_AVRCP_DESTROY_REQ, NULL, &appAvrcpGetLock(theInst));
    }


}


void appUserSetResetState(bool OnOff)
{
	uiTaskData *theUi = appGetUi();
	theUi->reset_flag = OnOff;
}

bool appUserIsResetState(void)
{
	uiTaskData *theUi = appGetUi();	
	return theUi->reset_flag ? TRUE:FALSE;

}

void appUserSetDutState(bool OnOff)
{
	uiTaskData *theUi = appGetUi();
	theUi->dut_flag = OnOff;
}

bool appUserIsDutState(void)
{
	uiTaskData *theUi = appGetUi();	
	return theUi->dut_flag ? TRUE:FALSE;

}

void appUserSetLinkLossState(bool OnOff)
{
	uiTaskData *theUi = appGetUi();
	theUi->handset_link_loss_flag = OnOff;
}

bool appUserIsLinkLossState(void)
{
	uiTaskData *theUi = appGetUi();	
	return theUi->handset_link_loss_flag ? TRUE:FALSE;

}

void appUserSetReceivePeerPackageStage(uint8 stage)
{
	uiTaskData *theUi = appGetUi();
    uint16 data = stage;
	theUi->receive_peer_package_status = stage;

    //
	PsStore(PSKEY_CAN_CHANGE_TDL_FLAG, &data, 1);
}

uint8 appUserGetReceivePeerPackageState(void)
{
	uiTaskData *theUi = appGetUi();	
    
    DEBUG_LOG("##appUserGetReceivePeerPackageState : %d##",theUi->receive_peer_package_status);
	return theUi->receive_peer_package_status;

}

bool appUserGetCanChangeTdl(void)
{
	uiTaskData *theUi = appGetUi();	
    
    DEBUG_LOG("##appUserGetCanChangeTdl : %d##",(theUi->receive_peer_package_status == RECEIVE_PACKAGE_ALL));
	return (theUi->receive_peer_package_status == RECEIVE_PACKAGE_ALL);

}

