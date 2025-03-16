#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Original functions
static void* (*real_qt_start_timer)(void* instance, int interval, void* timerType) = NULL;
static int (*real_nanosleep)(const struct timespec *req, struct timespec *rem) = NULL;
static int (*real_usleep)(unsigned int usec) = NULL;

// Factor to reduce timer intervals (increase frame rate)
static float rate_factor = 10.0;
static int debug_mode = 0;

// Initialize the library
__attribute__((constructor))
static void init(void) {
    // Get environment variable for configuration
    char* env_factor = getenv("QT_RATE_FACTOR");
    if (env_factor) {
        float factor = atof(env_factor);
        if (factor > 0) {
            rate_factor = factor;
        }
    }

    // Debug mode
    if (getenv("QT_FRAME_UNLOCK_DEBUG")) {
        debug_mode = 1;
        printf("Frame Unlock: Initialized with rate factor %.2fx\n", rate_factor);
    }

    // Get original function pointers
    real_nanosleep = dlsym(RTLD_NEXT, "nanosleep");
    real_usleep = dlsym(RTLD_NEXT, "usleep");
}

// Function to intercept QTimer::start in Qt6
void* _ZN6QTimer5startEi(void* instance, int interval) {
    // Load the original function if not already loaded
    if (!real_qt_start_timer) {
        real_qt_start_timer = dlsym(RTLD_NEXT, "_ZN6QTimer5startEi");
        if (!real_qt_start_timer) {
            fprintf(stderr, "Error: Could not find QTimer::start function\n");
            exit(1);
        }
    }

    int original_interval = interval;

    // Only modify intervals that seem to be for frame limiting
    // Typically 16ms (60 FPS), 33ms (30 FPS) or similar round values
    if (interval >= 16 && interval <= 100) {
        interval /= rate_factor;
        if (interval < 1) interval = 1;
    }

    if (debug_mode && original_interval != interval) {
        printf("QTimer interval adjusted: %d -> %d ms\n", original_interval, interval);
    }

    // Call original function with modified interval
    return real_qt_start_timer(instance, interval, NULL);
}

// Hook for nanosleep - reduce sleep times that look like frame limiters
int nanosleep(const struct timespec *req, struct timespec *rem) {
    struct timespec modified_req;

    // Copy the original request
    if (req) {
        modified_req = *req;

        // Calculate the time in milliseconds
        long ms = (req->tv_sec * 1000) + (req->tv_nsec / 1000000);

        // Only modify sleeps that look like frame limiting (15ms-100ms)
        if (ms >= 15 && ms <= 100) {
            // Reduce the sleep time by the rate factor
            double new_time_sec = req->tv_sec + (req->tv_nsec / 1000000000.0);
            new_time_sec /= rate_factor;

            modified_req.tv_sec = (time_t)new_time_sec;
            modified_req.tv_nsec = (long)((new_time_sec - modified_req.tv_sec) * 1000000000.0);

            if (debug_mode) {
                printf("Sleep reduced: %ld.%09ld -> %ld.%09ld seconds\n",
                       req->tv_sec, req->tv_nsec, modified_req.tv_sec, modified_req.tv_nsec);
            }
        }
    }

    return real_nanosleep(req ? &modified_req : NULL, rem);
}

// Hook for usleep - reduce sleep times that look like frame limiters
int usleep(unsigned int usec) {
    unsigned int modified_usec = usec;

    // Only modify sleeps that look like frame limiting (15ms-100ms)
    if (usec >= 15000 && usec <= 100000) {
        modified_usec = usec / rate_factor;
        if (debug_mode) {
            printf("usleep reduced: %u -> %u microseconds\n", usec, modified_usec);
        }
    }

    return real_usleep(modified_usec);
}
