#include <mpd/client.h>
#include <sys/inotify.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define WATCH_DIR "/dev/input"
#define JACK_EVENT_FILE "event9"

static struct mpd_connection * mpd_init_connection();

static bool mpd_pause();
static bool mpd_play();

static bool watch_jack();

static struct mpd_connection * mpd_init_connection() {

    /*struct mpd_connection * conn = mpd_connection_new("127.0.0.1", 8600, 0);*/
    struct mpd_connection * conn = mpd_connection_new(NULL, 0, 0);

    if (conn == NULL) {
        fprintf(stderr, "Out of memory\n");
    }
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "%s\n", mpd_connection_get_error_message(conn));
        mpd_response_finish(conn);
        conn = NULL;
    }

    return conn;
}

/*
 * Currently mpd_pause and mpd_play establish seperate connections as they
 * been called each time, and closes the connection at the end. It is implemented
 * this way because, libmpdclient or maybe mpd seem to drop the connection
 * if there is a long pause between each request. Or it could be something wrong
 * with this phone itself (Samsung J5 2015). Otherwise we could have
 * just starta single connection, and simply send mpd_send_pause instead
 * of creating seperate connections each time.
 * */
static bool mpd_pause() {

    struct mpd_connection * conn = mpd_init_connection();

    if (conn == NULL) {
        return false;
    }

    if (mpd_send_pause(conn, true)) {
        mpd_response_finish(conn);
    } else {
        fprintf(stderr, "%s\n", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        return false;
    }

    mpd_connection_free(conn);

    return true;
}

static bool mpd_play() {

    struct mpd_connection * conn = mpd_init_connection();

    if (conn == NULL) {
        return false;
    }

    if (mpd_send_pause(conn, false)) {
        mpd_response_finish(conn);
    } else {
        fprintf(stderr, "%s\n", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        return false;
    }

    mpd_connection_free(conn);

    return true;
}

static bool watch_jack() {

    int ifd, wfd;
    char ev_buf[sizeof(struct inotify_event) + NAME_MAX + 1];

    struct inotify_event * ev = (struct inotify_event *) ev_buf;

    ifd = inotify_init();

    if (ifd == -1) {
        fprintf(stderr, "inotify_init(): failed");
        return false;
    }

    wfd = inotify_add_watch(ifd, WATCH_DIR, IN_CREATE | IN_DELETE);

    if (wfd == -1) {
        fprintf(stderr, "inotify_add_watch(): failed");
        return false;
    }

    /*
     * Now we can listen to earphone jack events
     * */

    while(read(ifd, ev_buf, sizeof(ev_buf))) {

        /*
         * Making sure the event is event9, the earphone jack event
         * */

        if (strncmp(JACK_EVENT_FILE, ev->name, sizeof(JACK_EVENT_FILE)) == 0) {

            if (ev->mask & IN_CREATE) {
                /* Earphones plugged in */
                if (mpd_play()) {
                    printf("Earphones plugged in: Audio Resumed\n");
                } else {
                    printf("Earphones plugged in: Failed to Resume Audio\n");
                }
            } else if (ev->mask & IN_DELETE) {
                /* Earphones plugged out */
                if (mpd_pause()) {
                    printf("Earphones plugged out: Audio Paused\n");
                } else {
                    printf("Earphones plugged out: Failed to Pause Audio\n");
                }
            }
        }

    }

    return true;

}

int main() {

    printf("Listening for Earphone Jack Events.\n");

    bool status = watch_jack();

    if (status) {
        printf("W\n");
        return 0;
    } else {
        printf("L\n");
        return 1;
    }

}
