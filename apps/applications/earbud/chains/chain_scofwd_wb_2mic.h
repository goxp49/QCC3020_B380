/*!
    \copyright Copyright (c) 2020 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version 
    \file chain_scofwd_wb_2mic.h
    \brief The chain_scofwd_wb_2mic chain. This file is generated by C:/qtil/ADK_QCC512X_QCC302X_WIN_6.4.0.41/tools/chaingen/chaingen.py.
*/

#ifndef _CHAIN_SCOFWD_WB_2MIC_H__
#define _CHAIN_SCOFWD_WB_2MIC_H__

/*!
    @startuml
        object OPR_SCO_RECEIVE
        OPR_SCO_RECEIVE : id = CAP_ID_WBS_DEC
        object OPR_SCO_SEND
        OPR_SCO_SEND : id = CAP_ID_WBS_ENC
        object OPR_CVC_RECEIVE
        OPR_CVC_RECEIVE : id = CAP_ID_CVC_RECEIVE_WB
        object OPR_SCOFWD_SPLITTER
        OPR_SCOFWD_SPLITTER : id = CAP_ID_SPLITTER
        object OPR_SCOFWD_BASIC_PASS
        OPR_SCOFWD_BASIC_PASS : id = CAP_ID_BASIC_PASS
        object OPR_SWITCHED_PASSTHROUGH_CONSUMER
        OPR_SWITCHED_PASSTHROUGH_CONSUMER : id = EB_CAP_ID_SWITCHED_PASSTHROUGH
        object OPR_SCOFWD_SEND
        OPR_SCOFWD_SEND : id = EB_CAP_ID_ASYNC_WBS_ENC
        object OPR_CVC_SEND
        OPR_CVC_SEND : id = CAP_ID_CVCHS2MIC_MONO_SEND_WB
        object OPR_SOURCE_SYNC
        OPR_SOURCE_SYNC : id = CAP_ID_SOURCE_SYNC
        object OPR_VOLUME_CONTROL
        OPR_VOLUME_CONTROL : id = CAP_ID_VOL_CTRL_VOL
        object OPR_SCO_AEC
        OPR_SCO_AEC : id = EB_CAP_ID_AEC_REF
        OPR_CVC_RECEIVE "IN(0)"<-- "OUT(0)" OPR_SCO_RECEIVE
        OPR_SOURCE_SYNC "IN(0)"<-- "OUT(0)" OPR_CVC_RECEIVE
        OPR_SCOFWD_SPLITTER "IN(0)"<-- "OUT(0)" OPR_SOURCE_SYNC
        OPR_SCO_AEC "INPUT1(0)"<-- "OUT(0)" OPR_VOLUME_CONTROL
        OPR_VOLUME_CONTROL "MAIN_IN(0)"<-- "OUT1(0)" OPR_SCOFWD_SPLITTER
        OPR_SCOFWD_BASIC_PASS "IN(0)"<-- "OUT2(1)" OPR_SCOFWD_SPLITTER
        OPR_SCOFWD_SEND "IN(0)"<-- "OUT(0)" OPR_SCOFWD_BASIC_PASS
        OPR_SWITCHED_PASSTHROUGH_CONSUMER "IN(0)"<-- "OUT(0)" OPR_SCOFWD_SEND
        OPR_CVC_SEND "IN1(1)"<-- "OUTPUT1(3)" OPR_SCO_AEC
        OPR_CVC_SEND "IN2(2)"<-- "OUTPUT2(4)" OPR_SCO_AEC
        OPR_SCO_SEND "IN(0)"<-- "OUT(0)" OPR_CVC_SEND
        OPR_CVC_SEND "REFERENCE(0)"<-- "REFERENCE(0)" OPR_SCO_AEC
        object EPR_SCO_FROM_AIR #lightgreen
        OPR_SCO_RECEIVE "SCO_IN(0)" <-- EPR_SCO_FROM_AIR
        object EPR_SCO_MIC1 #lightgreen
        OPR_SCO_AEC "MIC1(2)" <-- EPR_SCO_MIC1
        object EPR_SCO_MIC2 #lightgreen
        OPR_SCO_AEC "MIC2(3)" <-- EPR_SCO_MIC2
        object EPR_VOLUME_AUX #lightgreen
        OPR_VOLUME_CONTROL "AUX_IN(1)" <-- EPR_VOLUME_AUX
        object EPR_SCO_SPEAKER #lightblue
        EPR_SCO_SPEAKER <-- "SPEAKER1(1)" OPR_SCO_AEC
        object EPR_SCO_TO_AIR #lightblue
        EPR_SCO_TO_AIR <-- "SCO_OUT(0)" OPR_SCO_SEND
        object EPR_SCOFWD_TX_OTA #lightblue
        EPR_SCOFWD_TX_OTA <-- "FORWARD(0)" OPR_SWITCHED_PASSTHROUGH_CONSUMER
    @enduml
*/

#include <chain.h>

extern const chain_config_t chain_scofwd_wb_2mic_config;

#endif /* _CHAIN_SCOFWD_WB_2MIC_H__ */

