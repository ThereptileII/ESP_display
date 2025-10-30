#pragma once
#include <Arduino.h>
struct CanFrame { uint32_t id=0; uint8_t len=0; uint8_t data[8]={0}; bool valid=false; };
void canbridge_begin(Stream& serial);
bool canbridge_read(CanFrame& out);
uint32_t n2k_pgn(uint32_t id);
