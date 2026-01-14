# 2x2-axis-tilt-maze
2x2 axis tilt maze design with an embedded controller - using stepper motors and drivers. 


The main components, as numbered in the figure are:
1. The maze table. The center section is sized to hold the maze baseplate. It has a total of 3 motors, one controls
tilting about the X axis and the others control tilting about the Y axis.
Note: There are two motors connected to the Y axis in order to provide enough torque to tilt the maze, the
inner frame, and the X motor. Both motors are controlled by the driver board (Section 2.3). You can treat
them as if they were a single motor, just like the X axis motor.
2. The driver board. This board contains all the circuitry needed to drive the motors. It also has a connection
for your 4 digit display to be included.
3. The controller shield. This is a shield which fits on the Nucleo. It has, among other things, a joystick you will
use to control the maze.



<img width="653" height="851" alt="image" src="https://github.com/user-attachments/assets/930cae84-2899-4953-b3a1-df6fd63521ad" />


The driver board connects the maze motors to your Nucleo and provides power for the maze. The main connectors
are described here and are identified in Figure 2.
1. Maze Connections: There are a number of signals between the maze and the driver board. They include a
total of 12 phase controls (4 for each motor), a high current 5V supply for the motors, and two general data
signals to allow you to monitor the starting line and finish line of your maze.
2. I2C Connection: This 4 pin header is used to attach your 4 digit display to the driver board.
3. Ribbon Cable: The 10 pin ribbon cable connects the necessary signals from the Nucleo to the rest of the system.
This is your interface to the maze. It is described further below.
4. Barrel Jack: The barrel jack provides power for the motors. Whenever you attempt to move the motors,
you must connect power here first!


<img width="848" height="317" alt="image" src="https://github.com/user-attachments/assets/a8aa9276-35f2-4122-ad7f-a83b642d9478" />


The controller shield (with the joystick) is how you will communicate between your Nucleo and the rest of the system.
The ribbon cable (Table 1) connects various pins on your Nucleo to the system. Those connections are described in
Table 2.
Please note, these pins are permanently connected as described in the table, so your code will need to interface
with the specified pins.

<img width="888" height="384" alt="image" src="https://github.com/user-attachments/assets/b7d86ee9-8fa2-4a72-a0d0-34c679a36b46" />


The driver board has a micro-controller on it that interfaces with your Nucleo. The driver board produces
the pulse train required to actually move the motors and
provides a Step and Direction interface for you to control. Using this interface, the driver board will move the
motor one full step for each rising edge on the “Step”
control. See the timing diagram in Figure 3 for details.
Note: The Step signal must have a frequency of no more
than 1KHz. In practice, you should probably be making
very small adjustments, so this is unlikely to be a significant problem. Table 3 shows the effect of the DIRX
and DIRY pins.

A note on nomenclature: Throughout this document,
references to X and Y refer to the direction the ball will
travel, so the StepX signal will tilt the table so that the
ball rolls in the X direction.

<img width="469" height="463" alt="image" src="https://github.com/user-attachments/assets/647cdc17-583e-4175-a8e6-090d1818587d" />


