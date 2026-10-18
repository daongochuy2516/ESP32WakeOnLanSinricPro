#pragma once

void wolExec();
// Relay on, red LED, beep, send the magic packet and arm the off timers.
// notifySinric pushes the "on" state to SinricPro before sending the packet.
void startWakeSequence(bool notifySinric);
// Called every loop: ends the beep and switches the relay/SinricPro state back off.
void handleWakeTimers();
