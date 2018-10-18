#ifndef MOTOR_H_INCLUDED_
#define MOTOR_H_INCLUDED_

    enum motor_coil {
        MOTOR_COIL_A = 0,
        MOTOR_COIL_B = 1
    };

    enum motor_polar {
        MOTOR_POLAR_POS = 0,
        MOTOR_POLAR_NEG = 1
    };

    enum motor_channel {
        MOTOR_CHANNEL_P = 0,
        MOTOR_CHANNEL_N = 1
    };

    enum motor_pulse {
      MOTOR_PULSE_POS,
      MOTOR_PULSE_NEG,
      MOTOR_PULSE_NTL
    };

#endif /* MOTOR_H_INCLUDED_ */
