main:	
  sertxd("Hello world",13,10)
  pause 2000
	
repeat:	
	serrxd b0
	inc b0
	sertxd (b0)
	goto repeat
	
