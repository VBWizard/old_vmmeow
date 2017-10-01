// screen.h -- Defines the interface for monitor.h
// From JamesM's kernel development tutorials.

#ifndef SCREEN_H
#define SCREEN_H

// Write a single character out to the screen.
void monitor_put(char c);

// Clear the screen to all black.
void monitor_clear();

// Output a null-terminated ASCII string to the monitor.
void monitor_write(char *c);

void monitor_write_at(char *c, int locX, int locY);

#endif // SCREEN_H
