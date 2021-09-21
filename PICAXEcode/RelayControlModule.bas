' hardware connections:
' C.0 = dual purpose: serial out (to RS485 chip), and data for Relay module
' C.5 = serial in (from RS485 chip)
' C.4 = serial direction (RS485 chip selection): high = send, low = receive
'
' C.1 = clock for Relay module
' C.2 = latch for Relay module
symbol RS485_IN = C.5
symbol RS485_DIR = C.4
symbol RS485_OUT = C.0

symbol RELAY_DATA = C.0
symbol RELAY_CLOCK = C.1
symbol RELAY_LATCH = C.2
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
' To change the relay value, briefly (ZZ ms) set C.2 to 0.
'
' b0 = target relay states (1 bit per relay)
' b1 = current relay states
' b2 = reserved for sendbyte and sendbit
' b3 = reserved for sendbyte
' b4 = reserved for sendbyte
' b5 = reserved for changestates
' b6 = reserved for changestates
' b7 = send/receive mode (0 = receive, 1 = send)

symbol relayTargetStates = b0
symbol relayCurrentStates = b1
symbol rs485Mode = b7
symbol rs485ByteToSend = b3

  b0 = 0
  b1 = 0
  rs485Mode = 0
  
  low RELAY_CLOCK
  high RELAY_LATCH
  
  rs485ByteToSend = 0
  gosub sendrelaysbyte
  
	b8 = $55
	
main:
  relayTargetStates = b8
	gosub changestates
  sleep 1000
	gosub latchrelaysstate
	sleep 3000
	b8 = b8 + 1
  goto main
  
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
			sleep 500
		endif	
		b5 = b5 / 2  
  loop until b5 = 0
  return
  
' byte to send is in b3
sendrelaysbyte:
  if rs485Mode = 1 then
		rs485Mode = 0
		low RS485_OUT
		sleep 5
	endif

  b1 = rs485ByteToSend
  for b4 = 0 to 7
    b2 = rs485ByteToSend and 1
    gosub sendbit
    rs485ByteToSend = rs485ByteToSend / 2
  next b4
  return
  
' bit to send is in b2
sendbit:
  if b2 = 1 then
		high RELAY_DATA
  else
		low RELAY_DATA
  endif	
  sleep 5
  high RELAY_CLOCK
  sleep 5
  low RELAY_CLOCK
  return
  
latchrelaysstate:
  high RELAY_LATCH
	sleep 10
	low RELAY_LATCH
	return


  
  
  
  
  
  
  
  