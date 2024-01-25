Note: For all assignments and Energy Profiler measurements you’ll be taking this semester,  Peak measurements are instantaneous measurements taken at a specific point in time. In the Energy Profiler, this is accomplished by left-clicking at a location along the time axis.
Average measurements are measurements that are taken over a time-span. In the Energy Profiler, this is accomplished by left-clicking and dragging a region along the time axis.

Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
   Answer:5.31mA of instantaneous current drawn by a single LED with GPIO set to StrongAlternateStrong.


**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
   Answer:5.45mA of instantaneous current drawn by a single LED with GPIO set to WeakAlternateWeak.


**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, referencing the main board schematic, WSTK-Main-BRD4001A-A01-schematic.pdf or WSTK-Main-BRD4002A-A06-schematic.pdf, and AEM Accuracy in the ug279-brd4104a-user-guide.pdf. Both of these PDF files are available in the ECEN 5823 Student Public Folder in Google drive at: https://drive.google.com/drive/folders/1ACI8sUKakgpOLzwsGZkns3CQtc7r35bB?usp=sharing . Extra credit is available for this question and depends on your answer.**
   Answer: There is no significant difference between the two measurements. By observing current mesurements in both modes, the system draws almost the same current where there is a difference at 0.1msec level. As per the Gecko board reference manual, Strong drive strength can drive upto 10mA of current where as weak drive strength can drive 1mA of current. This measn that in weak drive strength the port is forced to provide sufficient current to drive LED which is 5.04mA, as per the schematics the UIF LED0 nad LED1 are in series with 3kohm resistors results in this minimum required current for driving LED's. Driving a port by forcing it to utilize more current than its rated for eventually heat up the part and damage it.

   But we are able to see the difference in both currents at 0.1mA accuracy. This is becuase A in Advanced Energy Monitor (AEM) mode, the board is capable of measuring changes from 0.1uA to 95mA. For currents above 250uA, accuracy of measurement is within 0.1mA. Hence we were able to see the change at 0.1mA level.


**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer: 5.01mA of avg current for one complete on=off cycle with one LED on in weak drive strength.


**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer:5.37mA of avg current for one complete on=off cycle with LED's on in weak drive strength.

