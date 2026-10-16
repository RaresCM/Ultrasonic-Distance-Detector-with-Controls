## Concepts learnt

This project has taught me:
* Using a UART protocol to output to a console/PC and how it functions in terms of code
* How a servo motor works and is based on angle rather than speed
* Operation of an ultrasonic sensor and how timing the output and reflected input can be used for accurate distance measurements
* The importance and use of ISR routines to capture or execute things during code operation
* PWM oepration for the servo
* Use of volatile variables to ensure the position of the servo isn't overwritten or assumed

## Issues Encountered

* Jitter in the servo motor during many and very fast encoder inputs
* Sensor doesn't read or the distance output breaks post ~250 cm and I've narrowed this down to my uint for my distanceread variable
