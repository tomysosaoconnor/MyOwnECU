#define IGN 6// Ignition pin in the PORTB.
#define CANCEL 7 // This pin turns off the HV generator in the CDI module.

#define TABLE_X_SIZE 20//table size on X axis.

#define N_CYL 4//number of cylinders.
#define STROKES 4//number of strokes.
#define DEF_ADV 900//default advance given by the sensor location, in tenths of a degree.
#define THY_IGN 100// Time the thirystor will be on in uS.

//macros.
#define bit_set(x,reg) (reg|=(1<<x))
#define bit_clear(x,reg) (reg&=~(1<<x))
