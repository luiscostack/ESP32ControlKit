// src/mux.h

#ifndef MUX_H
#define MUX_H

void mux_init();
void mux_select_plant(int plant_id, int combination);
void mux_report_selection(int plant_id, int combination);

#endif // MUX_H