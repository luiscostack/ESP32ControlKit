// src/plant.h

#ifndef PLANT_H
#define PLANT_H

void plant_init();
double plant_read_voltage(int plant_id);
void plant_write_control(int plant_id, double control_signal_u);

#endif // PLANT_H