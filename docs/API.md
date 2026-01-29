# JS8Call API Documentation v2.5.0

This document provides documention of the API to control JS8Call through version 2.5.0.

# Methods
The API is normally located on localhost port 2242, but can be changed at ``Settings -> Reporting Tab -> API``
If you have access to ``telnet`` you can connect to the API with

```
telnet 127.0.0.1 2242
```
If you do nothing, the API will print information as it has it available, or you can submit one of the ``End Points`` below.


# API Format
All API calls are via JSON. JS8Call spits out a JSON packet of the form (for a heartbeat in this case):

```
{"params":{"PTT":true,"UTC":1768760160665,"_ID":-1},"type":"RIG.PTT","value":"on"}
{"params":{"TONES":[4,2,5,6,1,3,0,7,1,5,7,6,0,2,2,3,7,4,2,7,6,4,5,1,7,1,7,3,6,6,4,1,0,4,1,7,4,2,5,6,1,3,0,0,2,4,2,1,1,4,1,6,3,4,4,4,6,2,0,7,0,5,6,2,3,1,0,3,7,4,6,4,4,2,5,6,1,3,0],"_ID":-1},"type":"TX.FRAME","value":""}
{"params":{"TONES":[4,2,5,6,1,3,0,7,1,5,7,6,0,2,2,3,7,4,2,7,6,4,5,1,7,1,7,3,6,6,4,1,0,4,1,7,4,2,5,6,1,3,0,0,2,4,2,1,1,4,1,6,3,4,4,4,6,2,0,7,0,5,6,2,3,1,0,3,7,4,6,4,4,2,5,6,1,3,0],"_ID":-1},"type":"TX.FRAME","value":""}
{"params":{"PTT":false,"UTC":1768760173403,"_ID":-1},"type":"RIG.PTT","value":"off"}
```
The steps above are
1) PTT on
2) Send HB MSG as TONES (2 transmission frames)
3) PTT off

``{"params":xxx}`` is **required** in both directions and forms the overall packet


| Commands                     | Version |
|------------------------------|---------|
| [PING ](#ping)                               | 2.5 |
| [RIG.GET_FREQ](#rigget_freq)                 | 2.5 |
| [RIG.SET_FREQ](#rigset_freq)                 | 2.5 |
| [STATION.GET_CALLSIGN](#stationget_callsign) | 2.5 |
| [STATION.GET_GRID](#stationget_grid)         | 2.5 |
| [STATION.SET_GRID](#stationset_grid)         | 2.5 |
| [STATION.GET_INFO](#stationget_info)         | 2.5 |
| [STATION.SET_INFO](#stationset_info)         | 2.5 |
| [STATION.GET_STATUS](#stationget_status)     | 2.5 |
| [STATION.SET_STATUS](#stationset_status)     | 2.5 |
| [STATION.GET_PTT](#stationget_ptt)           | 2.6 |
| [STATION.VERSION](#stationversion)           | 2.6 |
| [STATION.GET_OS](#stationget_os)             | 2.6 |
| [STATION.SET_STATUS](#stationset_status)     | 2.5 |
| [STATION.GET_SPOT](#stationget_spot)         | 2.6 |
| [STATION.SET_SPOT](#stationset_spot)         | 2.6 |
| [RX.GET_CALL_ACTIVITY](#rxget_call_activity) | 2.5 |
| [RX.GET_CALL_SELECTED](#rxget_call_selected) | 2.5 |
| [RX.GET_BAND_ACTIVITY](#rxget_band_activity) | 2.5 |
| [RX.GET_TEXT](#rxget_text)                   | 2.5 |
| [TX.GET_TEXT](#txget_text)                   | 2.5 |
| [TX.SET_TEXT](#txset_text)                   | 2.5 |
| [TX.SEND_MESSAGE](#txsend_message)           | 2.5 |
| [MODE.GET_SPEED](#modeget_speed)             | 2.5 |
| [MODE.SET_SPEED](#modeset_speed)             | 2.5 |
| [INBOX.GET_MESSAGES](#inboxget_messages)     | 2.5 |
| [INBOX.STORE_MESSAGE](#inboxstore_message)   | 2.5 |
| [WINDOW.RAISE](#windowraise)                 | 2.5 |

## _ID Number
The ID number is the epoch time of 1499299200000 (July 6, 2017) plus current epoch time.

## Error messages
| Response |
|----------|
|{"params":{"_ID":"269558031750"},"type":"API.ERROR","value":"unterminated object: json parsing error"}|

``value`` contains the error


# PING
Wakes up the API if need be.

| End Point |
|-----------|
|{"params":{},"type":"PING","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
| None     |

# RIG.GET_FREQ
Gets radio dial and offset frequencies

| End Point |
|-----------|
|{"params":{},"type":"RIG.GET_FREQ","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"DIAL":7078000,"FREQ":7079950,"OFFSET":1950,"_ID":269554125481},"type":"RIG.FREQ","value":""}|
``FREQ`` is the ``DIAL`` frequency plus ``OFFSET``


# RIG.SET_FREQ
Sets radio dial and offset frequencies

| End Point |
|-----------|
|{"params":{"DIAL":dial,"OFFSET":offset},"type":"RIG.SET_FREQ","value":""}|

| Requirements | |
|--------------|-|
| dial         | Frequency in Hz |
| offset       | Offset in Hz    |
| value        | empty string    |

| Response |
|----------|
| {"params":{"DIAL":7078000,"FREQ":7079950,"OFFSET":1950,"SELECTED":"","SPEED":0,"_ID":"269554221645"},"type":"STATION.STATUS","value":""} |

``FREQ`` is the ``DIAL`` frequency plus ``OFFSET``
``SPEED`` is one of the ``MODE`` speeds


# STATION.GET_CALLSIGN
Gets station callsign

| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_CALLSIGN","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269553944755},"type":"STATION.CALLSIGN","value":"callsign"}|
Callsign is returned in `value`


# STATION.GET_GRID
Gets station grid square

| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_GRID","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269558175403},"type":"STATION.GRID","value":"EM85"}|
Grid square is returned in `value`

# STATION.SET_GRID
Sets station grid square

| End Point |
|-----------|
|{"params":{},"type":"STATION.SET_GRID","value":grid}|

| Requirements | |
|--------------|-|
| grid         | 4 or 6 character grid square enclosed in ""|

| Response |
|----------|
|{"params":{"_ID":269558371794},"type":"STATION.GRID","value":"EM85"}|

Grid square is returned in `value`

# STATION.GET_INFO
Gets station info

| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_INFO","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269559398161},"type":"STATION.INFO","value":"JS8-IMPROVED VER <MYVERSION>"}|

Station info is returned in `value`

# STATION.SET_INFO
Sets station info

| End Point |
|-----------|
|{"params":{},"type":"STATION.SET_INFO","value":info}|

| Requirements | |
|--------------|-|
| value        | Station info enclosed in "" |

| Response |
|----------|
|{"params":{"_ID":269559620289},"type":"STATION.INFO","value":"JS8-IMPROVED VER <MYVERSION>"}|

Station info is returned in `value`


# STATION.GET_STATUS
Gets station status message
| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_STATUS","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269559773383},"type":"STATION.STATUS","value":"IDLE <MYIDLE> JS8CALL-IMPORVED VERSION <MYVERSION>"}|

Station status is returned in `value`


# STATION.SET_STATUS
Sets station status message

| End Point |
|-----------|
|{"params":{},"type":"STATION.SET_STATUS","value":""}|

| Requirements | |
|--------------|-|
| value        | Station status enclosed in "" |

| Response |
|----------|
|{"params":{"_ID":269559773383},"type":"STATION.STATUS","value":"IDLE <MYIDLE> JS8CALL-IMPORVED VERSION <MYVERSION>"}|

Station status is returned in `value`


# STATION.GET_PTT
Gets station PTT status

| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_PTT","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"MESSAGE":"","PTT":false,"_ID":269908447335},"type":"STATION.PTT_STATUS","value":""}|
|{"params":{"MESSAGE":"SLbuEVAt7YC0","PTT":true,"_ID":269908640060},"type":"STATION.PTT_STATUS","value":""}|

`MESSAGE` will be empty string or a message if more is being transmitted.

`PTT` will be true if transmitting.


# STATION.VERSION
Gets JS8Call version. Use to check for API changes or compatiblity.

| End Point |
|-----------|
|{"params":{},"type":"STATION.VERSION","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"VERSION":"2.6.0-NOT_FOR_RELEASE","_ID":269908381596},"type":"STATION.VERSION","value":""}|

# STATION.GET_OS
Gets OS info including name, kernel version, and type

| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_OS","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"OS_KERNEL":"linux","OS_KERNEL_VERSION":"6.14.0-37-generic","OS_NAME":"Ubuntu 24.04.3 LTS","_ID":269977840701},"type":"STATION.GET_OS","value":""}|


# STATION.GET_SPOT
Gets status of SPOT setting

| End Point |
|-----------|
|{"params":{},"type":"STATION.GET_SPOT","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":270409823907,"value":true},"type":"STATION.SPOT","value":""}|


# STATION.SET_SPOT
Sets status of SPOT setting

| End Point |
|-----------|
|{"params":{},"type":"STATION.SET_SPOT","value":"true"}|

| Requirements | |
|--------------|-|
| value        | true/false |

| Response |
|----------|
|{"params":{"_ID":270409737648,"value":true},"type":"STATION.SPOT","value":""}|


# RX.GET_CALL_ACTIVITY
Returns the recent call activity

| End Point |
|-----------|
|{"params":{},"type":"RX.GET_CALL_ACTIVITY","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"":{"GRID":"","SNR":0,"UTC":0},"AB4WV":{"GRID":"","SNR":-18,"UTC":1768858992167},"K4EXA":{"GRID":"EM63","SNR":-18,"UTC":1768858242147},"type":"RX.CALL_ACTIVITY","value":""}|


# RX.GET_CALL_SELECTED
Returns the callsign of a station that has been selected in the UI.

| End Point |
|-----------|
|{"params":{},"type":"RX.GET_CALL_SELECTED","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269560590229},"type":"RX.CALL_SELECTED","value":""}|
|{"params":{"_ID":269560649847},"type":"RX.CALL_SELECTED","value":"KM4PVB"}|

1) First form is if no callsign is currently selected in the UI.

2) Second form is the selected callsign in the UI.


# RX.GET_BAND_ACTIVITY
Gets current band activity

| End Point |
|-----------|
|{"params":{},"type":"RX.GET_BAND_ACTIVITY","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"1067":{"DIAL":7078000,"FREQ":7079067,"OFFSET":1067,"SNR":-7,"TEXT":"W6OEM: VE3SOY HEARTBEAT SNR -20 ","UTC":1768860611907},"616":{"DIAL":7078000,"FREQ":7078616,"OFFSET":616,"SNR":-18,"TEXT":"KM4BOF: VE3SOY HEARTBEAT SNR -09 ","UTC":1768860611918},"type":"RX.BAND_ACTIVITY","value":""}|


# RX.GET_TEXT
Gets the contents of the directed message window

| End Point |
|-----------|
|{"params":{},"type":"RX.GET_TEXT","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269562514193},"type":"RX.TEXT","value":"\n22:06:10 - (1950) - KJ4CTD: W4CAT SNR?  ♢ \n\n22:06:52 - (1950) - KJ4CTD: KB4DSF SNR?  ♢ \n\n22:14:29 - (800) - KJ4CTD: N0AAS HEARTBEAT SNR -18  ♢ \n\n22:25:31 - (1950) - KJ4CTD: @SITREP MSG F!104 100 ST[SC] GR[EM85] #ASRM KWC ♢ "}|


# TX.GET_TEXT
Gets the text to be transmitted

| End Point |
|-----------|
|{"params":{},"type":"TX.GET_TEXT","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"_ID":269562593590},"type":"TX.TEXT","value":""}|
|{"params":{"_ID":269562644567},"type":"TX.TEXT","value":"KJ4CTD: KJ4YQK  SOME RANDOM TEXT"}|

1) If text box is empty
2) If text box has something in it

# TX.SET_TEXT
Sets the text to be transmitted

| End Point |
|-----------|
|{"params":{},"type":"TX.SET_TEXT","value":""}|

| Requirements | |
|--------------|-|
| value        | text to be transmitted |

| Response |
|----------|
|{"params":{"_ID":269563119535},"type":"TX.TEXT","value":"TEST"}|


# TX.SEND_MESSAGE
Sends the value in the next transmit cycle

[NOTE] If the message window already has text displayed, this will **NOT** transmit your new message!

| End Point |
|-----------|
|{"params":{},"type":"TX.SEND_MESSAGE","value":""}|

| Requirements | |
|--------------|-|
| value        | text to be transmitted |

| Response |
|----------|
|{"params":{"PTT":true,"UTC":1768862879801,"_ID":-1},"type":"RIG.PTT","value":"on"}|
|{"params":{"TONES":[4,2,5,6,1,3,0,1,0,2,6,6,3,1,6,6,4,0,1,7,0,7,2,6,2,6,0,4,3,4,5,2,3,5,2,0,4,2,5,6,1,3,0,3,4,2,5,4,5,7,0,1,6,3,6,7,0,2,3,5,6,4,5,7,4,0,0,1,7,3,6,4,4,2,5,6,1,3,0],"_ID":-1},"type":"TX.FRAME","value":""}|
|{"params":{"PTT":false,"UTC":1768862893402,"_ID":-1},"type":"RIG.PTT","value":"off"}|
|{"params":{"PTT":true,"UTC":1768862894801,"_ID":-1},"type":"RIG.PTT","value":"on"}|
|{"params":{"TONES":[4,2,5,6,1,3,0,2,3,7,1,3,7,7,5,2,2,4,1,1,2,2,1,1,5,3,5,4,0,7,0,5,2,1,6,5,4,2,5,6,1,3,0,7,1,6,4,4,3,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,2,6,7,1,2,4,2,5,6,1,3,0],"_ID":-1},"type":"TX.FRAME","value":""}|
|{"params":{"PTT":false,"UTC":1768862908402,"_ID":-1},"type":"RIG.PTT","value":"off"}|

1) Trigger PTT on
2) Send the message tones
3) Trigger PTT off
4) Trigger PTT on
5) Send the message tones
6) Trigger PTT off


# MODE.GET_SPEED
Gets the currently set transmit mode speed

| End Point |
|-----------|
|{"params":{},"type":"MODE.GET_SPEED","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"SPEED":0,"_ID":269564224294},"type":"MODE.SPEED","value":""}|
|{"params":{"SPEED":1,"_ID":269564372867},"type":"MODE.SPEED","value":""}|
|{"params":{"SPEED":2,"_ID":269564383363},"type":"MODE.SPEED","value":""}|
|{"params":{"SPEED":4,"_ID":269564510080},"type":"MODE.SPEED","value":""}|
|{"params":{"SPEED":8,"_ID":269564511182},"type":"MODE.SPEED","value":""}|

# MODE.SET_SPEED
Sets the currently set transmit mode speed

| End Point |
|-----------|
|{"params":{"SPEED":speed},"type":"MODE.SET_SPEED","value":""}|

| Requirements | |
|--------------|-|
| speed        | mode speed [number](#mode-speeds) |
| value        | empty string |

| Response |
|----------|
|{"params":{"DIAL":7078000,"FREQ":7079950,"OFFSET":1950,"SELECTED":"","SPEED":0,"_ID":"269564663038"},"type":"STATION.STATUS","value":""}|


## MODE Speeds
| Mode  | Number |
|-------|--------|
| Normal|   0    |
| Fast  |   1    |
| Turbo |   2    |
| Slow  |   4    |
| Ultra |   8    |
Please note, Ultra speed is an *experimental* and unreliable speed.

# INBOX.GET_MESSAGES
Fetches all Inbox messages, warning, this could be very large

| End Point |
|-----------|
|{"params":{},"type":"INBOX.GET_MESSAGES","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
|{"params":{"MESSAGES":[{"params":{"CMD":" MSG ","DIAL":7078000,"FREQ":7080318,"FROM":"KJ5MIW","GRID":" EM15","OFFSET":2318,"PATH":"KJ5MIW","SNR":-15,"SUBMODE":0,"TDRIFT":0.13999998569488525,"TEXT":"F!104 100 ST[OK] GR[EM15] #ATTV","TO":"@SITREP","UTC":"2026-01-21 01:44:26","_ID":"269660742003"},"type":"READ","value":""}],"_ID":269699597005},"type":"INBOX.MESSAGES","value":""}|

``MESSAGES`` is a comma seperated list of message records, each record is contained within brackets.


# INBOX.STORE_MESSAGE
Stores a message in YOUR Inbox

| End Point |
|-----------|
|{"params":{"CALLSIGN":callsign,"TEXT":text},"type":"INBOX.STORE_MESSAGE","value":""}}|

| Requirements | |
|--------------|-|
| callsign | "TO" callsign in ""|
| text     | Text to store in ""|
| value    | empty string |

| Response |
|----------|
| {"params":{"ID":228,"_ID":269569847892},"type":"INBOX.MESSAGE","value":""} |


# WINDOW.RAISE
If allowed by OS. brings the program to the front

| End Point |
|-----------|
|{"params":{},"type":"WINDOW.RAISE","value":""}|

| Requirements | |
|--------------|-|
| value        | empty string |

| Response |
|----------|
| None     |
