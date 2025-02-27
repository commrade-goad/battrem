/* TODO:
    - add sound support using sdl
    - make it more modular with couple of .c and .h
*/

#include <assert.h>
#include <libnotify/notification.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <libnotify/notify.h>

#define BATT_LEVEL_FILE "/sys/class/power_supply/BAT1/capacity"
#define BATT_LEVEL_BUFFER_SIZE 8

#define BATT_STATUS_FILE "/sys/class/power_supply/BAT1/status"
#define BATT_STATUS_BUFFER_SIZE 32

#define LONG_SLEEP_TIME 60 * 5
#define NORMAL_SLEEP_TIME 15
#define FAST_SLEEP_TIME 10

#define CHARGING "Charging"
#define DISCHARGING "Discharging"
#define NOTCHARGING "Not Charging"
#define FULL "Full"

#define BATT_WARNING 30
#define BATT_PREWARNING 45

#define LOCK_FILE_PATH "/tmp/battrem.lock"

bool running = true;
pthread_t loop_thread;

void read_file(char *fpath, char *buffer, size_t buff_size) {
    FILE *f = fopen(fpath, "r");
    if (f == NULL) return;
    size_t bytes_read = fread(buffer, sizeof(char), buff_size - 1, f);
    buffer[bytes_read] = '\0';
    fclose(f);
}

uint8_t get_batt_level(void) {
    char buf[BATT_LEVEL_BUFFER_SIZE] = {0};
    read_file(BATT_LEVEL_FILE, buf, BATT_LEVEL_BUFFER_SIZE - 1);
    uint8_t res = atol(buf);
    return res;
}

void get_batt_status(char *buffer, size_t buf_size) {
    char buf[BATT_STATUS_BUFFER_SIZE] = {0};
    read_file(BATT_STATUS_FILE, buf, BATT_STATUS_BUFFER_SIZE - 1);
    size_t strsize = strlen(buf) + 1;
    assert(strsize <= buf_size);
    memcpy(buffer, buf, strsize);
}

void send_notif(NotifyNotification *notif, char *header, char *body) {
    notif = notify_notification_new(header, body, NULL);
    notify_notification_set_urgency(notif, NOTIFY_URGENCY_CRITICAL);
    notify_notification_show(notif, NULL);
}

void *main_loop(void *notif_ptr) {
    NotifyNotification *notif = (NotifyNotification *) notif_ptr;
    while (running) {
        uint8_t level = get_batt_level();
        char status[BATT_STATUS_BUFFER_SIZE] = {0};
        get_batt_status(status, BATT_STATUS_BUFFER_SIZE);

        // handle discharging
        if (strncmp(DISCHARGING, status, strlen(status))) {
            if (level <= BATT_WARNING) {
                char head_buf[24] = {0};
                char *head_fmt_string = "%u%% Remaining";
                char *body = "please plug-in the charger.";
                assert(sizeof(head_buf) >= strlen(head_fmt_string) + 2);
                snprintf(
                    head_buf,
                    strlen(head_fmt_string),
                    head_fmt_string, level
                );
                send_notif(notif, head_buf, body);
                sleep(LONG_SLEEP_TIME);
            }
            else if (level <= BATT_PREWARNING)
                sleep(FAST_SLEEP_TIME);
            else 
                sleep(NORMAL_SLEEP_TIME);
        }
        // handle charging
        else if (strncmp(CHARGING, status, strlen(status))) {
            sleep(LONG_SLEEP_TIME);
        }
        // handle notcharging
        else if (strncmp(NOTCHARGING, status, strlen(status))) {
            sleep(NORMAL_SLEEP_TIME);
        }
        // handle full
        else if (strncmp(FULL, status, strlen(status))) {
            sleep(LONG_SLEEP_TIME);
        }
        else {
            sleep(NORMAL_SLEEP_TIME);
        }

    }
    return NULL;
}

void handle_sigint_n_sigterm(int sig) {
    fprintf(stderr, "\nINFO: get signal `%d` terminating...\n", sig);
    running = false;
    pthread_kill(loop_thread, SIGALRM);
}

void handle_sigalrm(int _) {
    fprintf(stderr,
            "INFO: get signal SIGALRM exiting the"
            "worker thread...\n"
            );
    return;
}

// will return true if the file didnt exist
// which mean there is no lock file
bool check_lock(void) {
    FILE *f = fopen(LOCK_FILE_PATH, "r");
    if (f == NULL) return true;

    // read the file and convert
    // to int.
    char buf[16] = {0};
    fgets(buf, sizeof(buf), f);
    int pid = atoi(buf);
    fclose(f);

    // cant convert it to int so
    // valid.
    if (pid == 0) return true;

    // which mean the process exist
    // so exit.
    int res = kill(pid, 0);
    if (res == 0) return false;

    return true;
}

void create_lock(void) {
    int pid = getpid();
    FILE *f = fopen(LOCK_FILE_PATH, "w");
    fprintf(f, "%d\n", pid);
    fclose(f);
}

int main(void) {

    bool ok = check_lock();
    if (!ok) {
        fprintf(stderr,
                "ERROR: there is other instance of this program "
                "running please kill them first.\n"
                );
        exit(EXIT_FAILURE);
    }
    create_lock();

    signal(SIGINT, handle_sigint_n_sigterm);
    signal(SIGTERM, handle_sigint_n_sigterm);
    signal(SIGALRM, handle_sigalrm);

    NotifyNotification *notif = NULL;
    gboolean status = notify_init("battrem");
    if (status != 1) {
        fprintf(stderr, "ERROR: failed to init libnotify!\n");
        exit(EXIT_FAILURE);
    }

    if (
        pthread_create(
            &loop_thread,
            NULL,
            main_loop,
            (void *)notif
        ) != 0)
    {
        fprintf(stderr, "ERROR: failed to create worker thread!\n");
        exit(EXIT_FAILURE);
    }

    pthread_join(loop_thread, NULL);
    notify_uninit();
    return 0;
}
