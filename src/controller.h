// src/controller.h

#ifndef CONTROLLER_H
#define CONTROLLER_H

void controller_init(double kP, double kI, double kD);
void controller_set_tunings(double kP, double kI, double kD);
void controller_compute();

#endif // CONTROLLER_H