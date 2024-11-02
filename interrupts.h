/*
Name: Max Bui
Student Number: 101261646
Professor: Gabriel Wainer
Due Date: October 4 2024
*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdio.h>

// Define the structure for the events in the trace file
typedef struct Event {
    char event_type[50];  // Could be CPU, SYSCALL, or END_IO
    int interrupt_id;     // Interrupt request number (only for SYSCALL and END_IO)
    int duration;         // Time duration of the event
    struct Event *next_event; // Pointer to the next event in the linked list
} event_t;

// Function prototypes

// Function to parse a single line of the trace file into an event
event_t *parse_event(char *line);

// Function to append an event to the event linked list
event_t *append_event(event_t *new_event, event_t *event_list);

// Function to read the trace file and return the linked list of events
event_t *parse_trace_file(const char *filename);

// Function to simulate the event-based interrupt handling system
void run_simulation(event_t *events, const char **vector_table, char *output_file);

// Function to handle saving the CPU context during an interrupt
int save_context(FILE *output, int time_elapsed);

// Function to load the program counter from the interrupt vector table
int load_pc_from_vector(const char **vector_table, event_t *event, FILE *output, int time_elapsed);

// Function to process the ISR (Interrupt Service Routine)
int process_isr(event_t *event, int time_elapsed, FILE *output);

// Function to validate that the interrupt vector is within valid bounds
void validate_vector(event_t *event, const char **vector_table, FILE *output);

// Function to handle the END_IO event during the interrupt simulation
int handle_io_end(FILE *output, int time_elapsed, const char **vector_table, event_t *event);

// Function to free the linked list of events
void free_event_list(event_t *head);

#endif
