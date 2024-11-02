/*
Name: Max Bui
Student Number: 101261646
Professor: Gabriel Wainer
Due Date: October 4 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "interrupts.h"

int main(int arg_count, char **argv) {
    // A new set of interrupt vectors
    const char *vector_table[25] = {"0X1A45", "0X2B67", "0X3C89", "0X4D23", "0X5E12",
                                    "0X6F90", "0X7A32", "0X8B54", "0X9C76", "0XAD98",
                                    "0XBEBA", "0XCFCB", "0XDDAF", "0XEEF2", "0XFF01",
                                    "0X1023", "0X2134", "0X3245", "0X4356", "0X5467",
                                    "0X6578", "0X7689", "0X879A", "0X98AB", "0XA9BC"};

    int trace_id = -1;
    char output_filename[30];

    if (arg_count == 1) {
        printf("The trace file is not provided.\n");
        return EXIT_FAILURE;
    } else if (arg_count > 2) {
        printf("Too many arguments provided.\n");
        return EXIT_FAILURE;
    } else {
        for (int i = 0; argv[1][i] != '\0'; i++) {
            if (isdigit(argv[1][i])) {
                trace_id = strtol(&argv[1][i], NULL, 10);
                break;
            }
        }
        snprintf(output_filename, sizeof(output_filename), "execution%d.txt", trace_id);
    }

    // Parse the input trace file
    event_t *trace_data = parse_trace_file(argv[1]);

    // Run simulation
    if (trace_data != NULL) {
        run_simulation(trace_data, vector_table, output_filename);
        free_event_list(trace_data);
    } else {
        printf("Failed to read the trace file.\n");
    }

    return EXIT_SUCCESS;
}

// Parse an individual line from the trace file into a struct
event_t *parse_event(char *line) {
    event_t *event = (event_t *)malloc(sizeof(event_t));

    // Extract event type (CPU, SYSCALL, END_IO)
    strncpy(event->event_type, strtok(line, " "), sizeof(event->event_type) - 1);

    if (strcmp(event->event_type, "CPU,") == 0) {
        event->event_type[3] = '\0';  // Strip the comma for consistency
        event->interrupt_id = 0;
    } else {
        event->interrupt_id = atoi(strtok(NULL, ", "));
    }

    // Extract the duration
    event->duration = atoi(strtok(NULL, " "));
    event->next_event = NULL;

    return event;
}

// Append event to the linked list of events
event_t *append_event(event_t *new_event, event_t *event_list) {
    if (!event_list) {
        return new_event;
    }

    event_t *current = event_list;
    while (current->next_event) {
        current = current->next_event;
    }

    current->next_event = new_event;
    return event_list;
}

// Read the trace file and create a list of events
event_t *parse_trace_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open trace file '%s'\n", filename);
        return NULL;
    }

    char buffer[100];
    event_t *event_list = NULL;

    while (fgets(buffer, sizeof(buffer), file)) {
        event_t *new_event = parse_event(buffer);
        event_list = append_event(new_event, event_list);
    }

    fclose(file);
    return event_list;
}

// Simulate the events from the trace file
void run_simulation(event_t *event_head, const char **vec_table, char *output_file) {
    event_t *current_event = event_head;
    bool is_kernel_mode = false;
    int total_time = 0;
    int cpu_time = 0;
    int io_time = 0;
    int overhead_time = 0;
    int delay1, delay2, delay3, delay4;

    FILE *output = fopen(output_file, "w");

    if (output == NULL) {
        printf("Error opening output file.");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));  // Seed random number generator for delays

    while (current_event != NULL) {
        // Ensure valid time duration for the event
        if (current_event->duration <= 0 || current_event->duration > 400) {
            printf("Invalid event duration.");
            fprintf(output, "Invalid event duration. Simulation abort.\n");
            fclose(output);
            exit(EXIT_FAILURE);
        }

        // CPU Event Handling
        if (strcmp(current_event->event_type, "CPU") == 0) {
            printf("%d, %d, CPU execution\n", total_time, current_event->duration);
            fprintf(output, "%d, %d, CPU execution\n", total_time, current_event->duration);
            total_time += current_event->duration;
            cpu_time += current_event->duration;

        // SYSCALL Event Handling
        } else if (strcmp(current_event->event_type, "SYSCALL") == 0) {
            is_kernel_mode = false;

            printf("%d, %d, switch to kernel mode\n", total_time, 1);
            fprintf(output, "%d, %d, switch to kernel mode\n", total_time, 1);
            total_time++;
            overhead_time++;

            // Save context and add random delay
            delay1 = (rand() % 3) + 1;
            printf("%d, %d, context saved\n", total_time, delay1);
            fprintf(output, "%d, %d, context saved\n", total_time, delay1);
            total_time += delay1;
            overhead_time += delay1;

            // Validate the interrupt vector
            validate_vector(current_event, vec_table, output);
            total_time = load_pc_from_vector(vec_table, current_event, output, total_time);
            overhead_time += 2;

            // Random ISR execution times
            delay2 = rand() % ((current_event->duration / 2) - 9) + 10;
            delay3 = rand() % ((current_event->duration / 2) - 9) + 10;
            delay4 = current_event->duration - (delay2 + delay3);

            printf("%d, %d, %s: run the ISR\n", total_time, delay2, current_event->event_type);
            fprintf(output, "%d, %d, %s: run the ISR\n", total_time, delay2, current_event->event_type);
            total_time += delay2;

            printf("%d, %d, transfer data\n", total_time, delay3);
            fprintf(output, "%d, %d, transfer data\n", total_time, delay3);
            total_time += delay3;

            printf("%d, %d, check for errors\n", total_time, delay4);
            fprintf(output, "%d, %d, check for errors\n", total_time, delay4);
            total_time += delay4;

            printf("%d, %d, IRET\n", total_time, 1);
            fprintf(output, "%d, %d, IRET\n", total_time, 1);
            total_time++;
            overhead_time++;

            is_kernel_mode = true;

        // END_IO Event Handling
        } else if (strcmp(current_event->event_type, "END_IO") == 0) {
            printf("%d, %d, check priority of interrupt\n", total_time, 1);
            fprintf(output, "%d, %d, check priority of interrupt\n", total_time, 1);
            total_time++;
            overhead_time++;

            printf("%d, %d, check if masked\n", total_time, 1);
            fprintf(output, "%d, %d, check if masked\n", total_time, 1);
            total_time++;
            overhead_time++;

            is_kernel_mode = false;

            printf("%d, %d, switch to kernel mode\n", total_time, 1);
            fprintf(output, "%d, %d, switch to kernel mode\n", total_time, 1);
            total_time++;
            overhead_time++;

            delay1 = (rand() % 3) + 1;
            printf("%d, %d, context saved\n", total_time, delay1);
            fprintf(output, "%d, %d, context saved\n", total_time, delay1);
            total_time += delay1;
            overhead_time += delay1;

            validate_vector(current_event, vec_table, output);
            total_time = load_pc_from_vector(vec_table, current_event, output, total_time);

            printf("%d, %d, END_IO\n", total_time, current_event->duration);
            fprintf(output, "%d, %d, END_IO\n", total_time, current_event->duration);
            total_time += current_event->duration;
            io_time += current_event->duration;

            printf("%d, %d, IRET\n", total_time, 1);
            fprintf(output, "%d, %d, IRET\n", total_time, 1);
            total_time++;
            overhead_time++;

            is_kernel_mode = true;

        // Unexpected Command Handling
        } else {
            printf("Unknown event encountered. Exiting.\n");
            fprintf(output, "Unknown event encountered. Simulation exit.\n");
            fclose(output);
            exit(EXIT_FAILURE);
        }

        current_event = current_event->next_event;  // Move to the next event in the list
    }
    // Calculate counter time (CPU Time + I/O Time + Overhead Time)
    int counter_time = cpu_time + io_time + overhead_time;

    // Calculate Ratios
    double cpu_ratio = (double) cpu_time / counter_time;
    double io_ratio = (double) io_time / counter_time;
    double overhead_ratio = (double) overhead_time / counter_time;

    // Print Ratios
    printf("\nCPU Time Ratio: %2f%%\n", cpu_ratio * 100);
    printf("I/O Time Ratio: %2f%%\n", io_ratio * 100);
    printf("Overhead Time Ratio: %2f%%", overhead_ratio * 100);

    // Print Times
    printf("\n\nTotal System CPU Time: %dms\n", cpu_time);
    printf("Total System I/O Time: %dms\n", io_time);
    printf("Total System Overhead Time: %dms\n\n", overhead_time);
    fclose(output);
}

// Save context during a system call or interrupt
int save_context(FILE *output, int time_elapsed) {
    int context_time = (rand() % 3) + 1;
    printf("%d, %d, context saved\n", time_elapsed, context_time);
    fprintf(output, "%d, %d, context saved\n", time_elapsed, context_time);
    return context_time;
}

// Load the program counter based on the vector table
int load_pc_from_vector(const char **vector_table, event_t *event, FILE *output, int time_elapsed) {
    printf("%d, %d, find vector %d in memory position 0x%04x\n", time_elapsed, 1, event->interrupt_id, event->interrupt_id * 2);
    fprintf(output, "%d, %d, find vector %d in memory position 0x%04x\n", time_elapsed, 1, event->interrupt_id, event->interrupt_id * 2);
    time_elapsed++;

    printf("%d, %d, load address %s into PC\n", time_elapsed, 1, vector_table[event->interrupt_id]);
    fprintf(output, "%d, %d, load address %s into PC\n", time_elapsed, 1, vector_table[event->interrupt_id]);
    time_elapsed++;

    return time_elapsed;
}

// Validate the interrupt vector
void validate_vector(event_t *event, const char **vector_table, FILE *output) {
    if (event->interrupt_id < 0 || event->interrupt_id >= 25) {
        printf("Invalid interrupt ID.\n");
        fprintf(output, "Invalid interrupt ID. Exiting.\n");
        fclose(output);
        exit(EXIT_FAILURE);
    }
}

// Process the ISR (Interrupt Service Routine)
int process_isr(event_t *event, int time_elapsed, FILE *output) {
    int run_time = (rand() % (event->duration / 2 - 10)) + 10;
    int transfer_time = (rand() % (event->duration / 2 - 10)) + 10;
    int error_check_time = event->duration - (run_time + transfer_time);

    printf("%d, %d, run the ISR\n", time_elapsed, run_time);
    fprintf(output, "%d, %d, run the ISR\n", time_elapsed, run_time);
    time_elapsed += run_time;

    printf("%d, %d, transfer data\n", time_elapsed, transfer_time);
    fprintf(output, "%d, %d, transfer data\n", time_elapsed, transfer_time);
    time_elapsed += transfer_time;

    printf("%d, %d, check for errors\n", time_elapsed, error_check_time);
    fprintf(output, "%d, %d, check for errors\n", time_elapsed, error_check_time);
    time_elapsed += error_check_time;

    printf("%d, %d, IRET\n", time_elapsed, 1);
    fprintf(output, "%d, %d, IRET\n", time_elapsed, 1);
    time_elapsed++;

    return 2; // ISR overhead
}

// Handle the END_IO event
int handle_io_end(FILE *output, int time_elapsed, const char **vector_table, event_t *event) {
    printf("%d, %d, switch to kernel mode\n", time_elapsed, 1);
    fprintf(output, "%d, %d, switch to kernel mode\n", time_elapsed, 1);
    time_elapsed++;

    int context_time = (rand() % 3) + 1;
    printf("%d, %d, context saved\n", time_elapsed, context_time);
    fprintf(output, "%d, %d, context saved\n", time_elapsed, context_time);
    time_elapsed += context_time;

    validate_vector(event, vector_table, output);
    time_elapsed = load_pc_from_vector(vector_table, event, output, time_elapsed);

    printf("%d, %d, END_IO\n", time_elapsed, event->duration);
    fprintf(output, "%d, %d, END_IO\n", time_elapsed, event->duration);
    time_elapsed += event->duration;

    printf("%d, %d, IRET\n", time_elapsed, 1);
    fprintf(output, "%d, %d, IRET\n", time_elapsed, 1);
    time_elapsed++;

    return context_time + 2;  // END_IO overhead
}

// Free the allocated memory for the event list
void free_event_list(event_t *head) {
    event_t *temp;
    while (head != NULL) {
        temp = head;
        head = head->next_event;
        free(temp);
    }
}