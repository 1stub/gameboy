 When comparing my 02-interrupts test with wheremyfoodat logs,
 ours match up. But the gameboy-doctor's does not. This is 
 almost 100% a fault (on my end) of the cpu's implementation
 not being cycle accurate. Here we interrupt (PC: 0x0050)
 which indicates a timer underflow. Since the instruction before takes
 12 cycles to finish execution, our timer just sees that the 
 number underflowed, even though it is not supposed to quite
 yet if we were modifying the cpu in increments of 4 T-cycles.
 I do think that the current implementation is mostly fine
 (especially given that it matches wheremyfoodat's logs), so 
 I will just continue onto the LCD and display and hope all is well.


