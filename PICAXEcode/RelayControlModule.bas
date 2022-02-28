' The RelayControlModule has two major functions:
' 1) Communicate with the master over RS485
' 2) Manipulate the outputs on the attached Relay Control Module (up to 8)
'
' the unique byte identifier for this device - change it to unique value before download!
symbol MY_BYTEID = "A"

' hardware connections:
' C.0 = dual purpose: serial out (to RS485 chip), and data for Relay module
' C.5 = serial in (from RS485 chip)
' C.4 = serial direction (RS485 chip selection): high = send, low = receive
'    C.5 is valid when C.4 is low; C.5 is high impedance when C.4 is high.
'
' C.1 = clock for Relay module
' C.2 = latch for Relay module
'
' C.3 = reprogram mode.  if power up with grounded REPROGRAM_MODE_PIN, tristate the RS485 and wait for reprogramming 

symbol RS485_IN = C.5		
symbol RS485_DIR = C.4
symbol RS485_OUT = C.0

symbol RELAY_DATA = C.0
symbol RELAY_CLOCK = C.1
symbol RELAY_LATCH = C.2

symbol REPROGRAM_MODE_PIN = pinC.3
'
' The relay module functions as follows:
' It has three inputs- data, clock, and latch.
' The data and clock are used to control 8-bit shift register.
' Each time there is a falling clock edge, it shifts the data bit into the register.
' On a falling latch edge, the relay module outputs are all turned off
' On a rising latch edge, the relay module outputs are latched from the shift register.
' NB the PICAXE outputs are inverted when passed to the Relay Module so the PICAXE pinouts are:
'  C.0 = data inverse
'  C.1 = rising clock edge to clock data in
'  C.2 = falling latch edge to latch the output
' Due to limited pin count, the serout and Relay module Data are combined:
'   Ensure to disable serial out (C.4) before manipulating Data for the Relay module
' To change the relay value, briefly (10 ms) set C.2 to 0.
'
' b0 = target relay states (1 bit per relay)
' b1 = current relay states
' b2 = reserved for sendbyte and sendbit
' b3 = reserved for sendbyte
' b4 = reserved for sendbyte
' b5 = reserved for changestates
' b6 = reserved for changestates
' b7 = RS485 send/receive mode (0 = receive, 1 = send)
' b8/b9 (w4) = crc calculation temp

symbol relayTargetStates = b0
symbol relayCurrentStates = b1
symbol rs485Mode = b7
symbol relayByteToSend = b3
symbol crc16value = w4					' crc16 value calculation
symbol crc16valueLo = b8
symbol crc16valueHi = b9

symbol inputAttentionByte = b16
symbol IO_BUFFER_BPTR = 18
symbol ioByteId = b18
symbol ioByteCommand = b19
symbol ioParameterB0 = b20
symbol ioParameterB1 = b21
symbol ioParameterB2 = b22
symbol ioParameterB3 = b23
symbol ioCRCb0 = b24
symbol ioCRCb1 = b25
symbol IO_BUFFER_BASE_LENGTH = 6 ' number of bytes in the input output buffer for crc16 calculation

symbol errorcount1 = b10  '  (number of timeouts during serial receive)
symbol errorcount2 = b11  ' (number of CRC16 errors during serial receive)
symbol errorcount3 = b12  ' unused 
symbol i = b13	'used by crc16
symbol x = b14  'used by crc16
symbol x2 = b15 'used by crc16


main:
	pullup ON
  low RELAY_CLOCK
  low RELAY_LATCH
	low RS485_DIR				'receive
	pause 50

  goto rs485DebugWriteTest2

' if power up with grounded REPROGRAM_MODE_PIN, tristate the RS485
waitwhilereprogramming:
  if REPROGRAM_MODE_PIN = 0 then 
		high RS485_DIR
		goto waitwhilereprogramming
	endif	 

' set system to idle, set all relays to off, and wait for RS485 comms
	disconnect
  relayTargetStates = 0
  relayCurrentStates = 0
  rs485Mode = 0 'receive
  errorcount1 = 0
  errorcount2 = 0
  errorcount3 = 0
	
  relayByteToSend = 0
  gosub sendrelaysbyte

	goto waitforfirst
	
'  change from the current states (b1) to the target states (b0), one bit at a time, waiting 500ms between bit switches
changestates:
  if relayCurrentStates = relayTargetStates then 
	  return
  endif
  b5 = 128
  do	
		b6 = relayCurrentStates xor relayTargetStates
		b6 = b6 and b5
		if b6 <> 0 then
	  	b3 = relayCurrentStates xor b5 	
	  	gosub sendrelaysbyte
			pause 50
			gosub latchrelaysstate
			pause 450
		endif	
		b5 = b5 / 2  
  loop until b5 = 0
  return
  
' byte to send is in b3
sendrelaysbyte:
  gosub rs485modeSetToRead

  b1 = relayByteToSend
  for b4 = 0 to 7
    b2 = relayByteToSend and 1
    gosub sendbit
    relayByteToSend = relayByteToSend / 2
  next b4
  return
  
' bit to send is in b2
sendbit:
  if b2 = 1 then
		low RELAY_DATA
  else
		high RELAY_DATA
  endif	
  pause 5
  high RELAY_CLOCK
  pause 5
  low RELAY_CLOCK
  return
  
latchrelaysstate:
  low RELAY_LATCH
	pause 10
	high RELAY_LATCH
	pause 10
	return

'/*
' * Protocol for communicating with slave device is:
' * 
' * 1) Master sends !{BYTEID}{BYTECOMMAND}{DWORDCOMMANDPARAM}{CRC16} then releases bus and waits for reply
' * 2) Slave waits at least 100 ms then responds ${BYTEID}{BYTECOMMAND}{DWORDSTATUS}{CRC16} then releases bus
' * 
' * Commands:
' * 100 = are you alive?  Response = device status; byte0 = status (0=good), bytes 1, 2, 3 = errorcounts (debug)
' * 
' * For solenoids:
' * 101 = what is your current output?  Response = output (bits 0->7 = current states, bits 8->15 = target states)
' * 102 = change output (bits 0->31).  Response = repeat target output
' *       After sending a reply immediately, the code will then change the solenoids to match the target states
' *       It won't be responsive to further commands during this time
' * 
' * If slave responds with bytecommand = 255 it indicates parsing error / invalid command
' */

waitforfirst:
	gosub rs485modeSetToRead
  serrxd inputAttentionByte 
		
	if inputAttentionByte <> "$" and inputAttentionByte <> "!" then goto waitforfirst
  bptr = IO_BUFFER_BPTR
  serrxd [1000, timeout],@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc 
	' ioByteId  = {BYTEID}
	' ioByteCommand  = {BYTECOMMAND}
	' ioParameterB0 -> ioParameterB3  = {DWORDCOMMANDPARAM}
	' ioCRCb0 -> ioCRCb1  = {CRC16}
	if inputAttentionByte <> "!" or ioByteId <> MY_BYTEID then goto waitforfirst
	gosub checkcrc16
	if crc16value <> 0 then
		errorcount2 = errorcount2 + 1 MAX 250
		goto waitforfirst
	end if

	pause 100
	if ioByteCommand = 100 then 
		gosub cmd100
	else if ioByteCommand = 101 then
		gosub cmd101
	else if ioByteCommand = 102 then
		gosub cmd102
	else
		gosub cmdinvalid
	end if
	gosub calculatecrc16
	gosub rs485modeSetToWrite
	bptr = IO_BUFFER_BPTR
	sertxd ("$",@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc,@bptrinc)
	gosub rs485modeSetToRead
	if ioByteCommand = 102 then
		relayTargetStates = ioParameterB0
		gosub changestates
	end if
	
	goto waitforfirst	
	
timeout:
  errorcount1 = errorcount1 + 1 MAX 250
	goto waitforfirst
  
	' calculate crc16 of the bytes in inputByteId,inputByteCommand, inputParameterB0  - inputParameterB3
	' compare with inputCRCb0 - inputCRCb1
	' if match: set crc16value to 0, otherwise non-zero
checkcrc16:
	gosub calculatecrc16
	crc16valueHi = crc16valueHi ^ ioCRCb1 
	crc16valueLo = crc16valueLo ^ ioCRCb0
	return
	
	' calculate crc16 of the bytes in inputByteId,inputByteCommand, inputParameterB0  - inputParameterB3, store in crc16value
'	unsigned short crc16(const unsigned char* data_p, unsigned char length){
'    unsigned char x;
'    unsigned short crc = 0xFFFF;
'    while (length--){
'        x = crc >> 8 ^ *data_p++;
'        x ^= x>>4;
'        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
'    }
'    return crc;
'  }
calculatecrc16:
	crc16value = $ffff
	bptr = IO_BUFFER_BPTR
	for i = 1 to IO_BUFFER_BASE_LENGTH
		x = @bptrinc					'x = crc >> 8 ^ *data_p++;
		x = x ^ crc16valueHi  
		x2 = x / 16						'x ^= x>>4;
		x = x ^ x2
		crc16valueHi = crc16valueLo  'crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
		x2 = x * 16
		crc16valueHi = crc16valueHi ^ x2
		x2 = x / 8
		crc16valueHi = crc16valueHi ^ x2
		x2 = x * 32
		crc16valueLo = x2
		crc16valueLo = crc16valueLo ^ x
	next i	
	return 

' * 100 = are you alive?  Response = device status; byte0 = status (0=good), bytes 1, 2, 3 = errorcounts (debug)
cmd100:
	ioParameterB0 = 0
	ioParameterB1 = errorcount1
	ioParameterB2 = errorcount2
	ioParameterB3 = errorcount3
	return
	
' * 101 = what is your current output?  Response = output (bits 0->7 = current states, bits 8->15 = target states)
cmd101:
	ioParameterB0 = relayCurrentStates
	ioParameterB1 = relayTargetStates
	ioParameterB2 = 0
	ioParameterB3 = 0
	return

' * 102 = change output (bits 0->31).  Response = repeat target output
cmd102:
	return

cmdinvalid:
	ioByteCommand = 255
	return

rs485modeSetToRead:
  if rs485Mode = 1 then
		rs485Mode = 0
		low RS485_DIR
		pause 5
	endif
	return
  
rs485modeSetToWrite:
  if rs485Mode = 0 then
		rs485Mode = 1
		high RS485_DIR
		pause 5
	endif
	return
  
 rs485DebugWriteTest:
	gosub rs485modeSetToWrite

	sertxd ("0")
	pause 1000 
	sertxd ("1")
	pause 1000 

	sertxd ("2")
	pause 1000 

	sertxd ("3")
	pause 1000 

	sertxd ("4")
	pause 1000 

	sertxd ("5")
	pause 1000 

	sertxd ("6")
	pause 1000 

	sertxd ("7")
	pause 1000 

	sertxd ("8")
	pause 1000 

	sertxd ("9")
	pause 1000 

	sertxd ("a")
	pause 1000 

	sertxd ("A")
	pause 3000 
	goto rs485DebugWriteTest
	
rs485DebugWriteTest2:
	gosub rs485modeSetToWrite

	serout 1, T4800_4, ("0")
	pause 1000 
	
	serout 1, T4800_4, ("1")
	pause 1000 

	serout 1, T4800_4, ("2")
	pause 1000 

	serout 1, T4800_4, ("3")
	pause 1000 

	serout 1, T4800_4, ("4")
	pause 1000 

	serout 1, T4800_4, ("5")
	pause 1000 

	serout 1, T4800_4, ("6")
	pause 1000 

	serout 1, T4800_4, ("7")
	pause 1000 

	serout 1, T4800_4, ("8")
	pause 1000 

	serout 1, T4800_4, ("9")
	pause 1000 

	serout 1, T4800_4, ("a")
	pause 1000 

	serout 1, T4800_4, ("A")
	pause 3000 
	goto rs485DebugWriteTest2
