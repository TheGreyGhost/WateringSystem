' hardware connections:
' C.0 = serial out (to RS485 chip)
' C.5 = serial in (from RS485 chip)
' C.4 = serial direction (RS485 chip selection)
'
' C.1 = combined data and clock for Relay module
' C.2 = latch for Relay module
'
' The relay module functions as follows:
' It has three inputs- data, clock, and latch.
' The data and clock are used to control 8-bit shift register.
' Each time there is a rising clock edge, it shifts the data bit into the register.
' On a falling latch edge, the relay module outputs are all turned off
' On a rising latch edge, the relay module outputs are latched from the shift register.
' Due to limited pin count, the data and clock are combined as follows:
' A monostable is connected to the clock input.  On the falling edge of C.1, the monostable is triggered.  
'   After XX ms, the monostable resets, the clock input rises, and the current state of C.1 is shifted into the module.
' Hence the write operation is:
'   1) Generate a falling edge on C.1.  (If C.1 is already low, set it high for at least YY ms to allow the edge trigger capacitor to charge)
'   2) To write a 0, leave C.1 as zero.  To write a 1, change C.1 to one before the monostable resets (XX ms)
' To change the relay value, briefly (ZZ ms) set C.2 to 0.
'
' b0 = target relay states (1 bit per relay)
' b1 = current relay states
' b2 = reserved for sendbyte and sendbit
' b3 = reserved for sendbyte
' b4 = reserved for sendbyte
' b5 = reserved for changestates
' b6 = reserved for changestates

  b0 = 0
  b1 = 0
  
  high C.1
  high C.2
  
  b3 = 0
  gosub sendbyte
  
main:
  
  
  goto main
  
'  change from the current state (b1) to the target states (b0, one bit at a time  
changestates:
  if b1 = b0 then 
	  return
  endif
  b5 = 128
  do
	b6 = b1 xor b0
	b6 = b6 and b5
	if b6 <> 0 then
	  b3 = b1 xor b5 	
	  gosub sendbyte
	endif	
	b5 = b5 / 2  
  loop until b5 = 0
  return
  
' byte to send is in b3
sendbyte:
  b1 = b3
  for b4 = 0 to 7
    b2 = b3 and 1
    gosub sendbit
    b3 = b3 / 2
  next b4
  return
  
' bit to send is in b2
sendbit:
  if pinC.1 = 0 then
	high C.1  
	pause 10
  endif
  
  low C.1
  pause 10
  if b2 <> 0 then
	high C.1  
	pause 10
  else
	pause 10
	high C.1
	pause 10
  endif
  return
  
  
  
  
  
  
  
  
  
  