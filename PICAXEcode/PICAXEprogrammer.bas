' This program is used to add a serial break before transmission
' It is used to compensate for a USB to serial cable that does not
'   properly send a serial break
'  (The PICAXE programming requires a serial break to initiate programming)
' How it works:
' The TX from the PC is connected to hserin C.1.
' hserout C.0 is connected to the serin of the PICAXE to be programmed.
' The serout of the PICAXE to be programmed is connected directly to the RX to the PC
' 
' Step by step:
' 1) Connect PC to PICAXEprogrammer and PICAXEprogrammer to the PICAXE to be programmed
' 2) Power up the PICAXEprogrammer.  It will start sending a serial break
' 3) Power up the PICAXE to be programmed.
' 4) Start the program download.  The PICAXEprogrammer will stop sending the break and will pass through
'    the transmission to the PICAXE.
' 5) Once no transmission has been received for 5 seconds, the PICAXEprogrammer will start sending break
'     again
symbol HSERIN_N_POLARITY = %00100
symbol HSEROUT_OFF = %01000
symbol HSERIN_ON = %00000

symbol ENDBREAK_DELAY_MS = 1 '4800 baud -> 1ms ~ 5 bits
symbol IDLE_TIME_BEFORE_BREAK_S = 5 ' the serial break will be reestablished after this length of idle time

symbol hseroutPin = C.0

symbol sendingbreak = b0
symbol getbyte = w1
symbol lasttime = w2

serialbreak:
  HSERSETUP B4800_4, 4 'HSEROUT_N_POLARITY | HSERIN_N_POLARITY | HSEROUT_OFF 
  high hseroutPin

	do 
	  w1 = $FFFF            ; set up a non-valid value
	  hserin w1             ; receive 1 byte into w1
	while w1 = $FFFF
		
receiving:		
	low hseroutPin
	pause ENDBREAK_DELAY_MS
  HSERSETUP B4800_4, %00110 'HSEROUT_N_POLARITY | HSERIN_N_POLARITY | HSEROUT_ON

passingthrough:
  hserout 0, w1 & $ff
	lasttime = time
	
waitnext:
  w1 = $FFFF            ; set up a non-valid value
	hserin w1             ; receive 1 byte into w1
	if w1 = $FFFF then
		if time - lasttime > IDLE_TIME_BEFORE_BREAK_S then
			goto serialbreak		
		end if	
	  goto waitnext
  end if	

	goto passingthrough

	
  	
	
	
 