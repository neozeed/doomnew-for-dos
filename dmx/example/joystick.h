#pragma aux ReadJoystick "_*" \
			parm [EDI] modify [EBX ECX EDX];

int
ReadJoystick(
	int		flags
	);

extern long	joyX1;
extern long	joyY1;
extern long	joyX2;
extern long	joyY2;
extern long joyButtons;
