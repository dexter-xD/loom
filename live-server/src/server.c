#include "mongoose.h"
#include "log.h"
#include <sys/inotify.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#define DEFAULT_DIR "./public"
#define DEFAULT_PORT "8080"
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define MAX_FILES 1000

static char watch_dir[512] = DEFAULT_DIR;
static char port[16] = DEFAULT_PORT;

typedef struct {
    char path[512];
    time_t mtime;
} FileInfo;

static int inotify_fd = -1;
static int watch_descriptor = -1;
static FileInfo tracked_files[MAX_FILES];
static int tracked_count = 0;

static char* stristr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;

    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);

    if (needle_len > haystack_len) return NULL;

    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        size_t j;
        for (j = 0; j < needle_len; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                break;
            }
        }
        if (j == needle_len) {
            return (char*)(haystack + i);
        }
    }
    return NULL;
}

static int uri_matches(struct mg_str uri, const char *pattern) {
    return mg_strcmp(uri, mg_str(pattern)) == 0;
}

static void add_tracked_file(const char* filepath) {
    if (tracked_count >= MAX_FILES) return;

    for (int i = 0; i < tracked_count; i++) {
        if (strcmp(tracked_files[i].path, filepath) == 0) {
            return;
        }
    }

    struct stat st;
    if (stat(filepath, &st) == 0) {
        strncpy(tracked_files[tracked_count].path, filepath, sizeof(tracked_files[0].path) - 1);
        tracked_files[tracked_count].mtime = st.st_mtime;
        tracked_count++;
        LOG_FILE_WATCHER("Tracking: %s", filepath);
    }
}

static void scan_directory(const char* dirpath) {
    DIR* dir = opendir(dirpath);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(fullpath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                scan_directory(fullpath);
            } else if (S_ISREG(st.st_mode)) {

                char* ext = strrchr(entry->d_name, '.');
                if (ext && (strcmp(ext, ".html") == 0 || 
                           strcmp(ext, ".css") == 0 || 
                           strcmp(ext, ".js") == 0)) {
                    add_tracked_file(fullpath);
                }
            }
        }
    }

    closedir(dir);
}

static int check_tracked_files() {
    int changed = 0;
    struct stat st;

    for (int i = 0; i < tracked_count; i++) {
        if (stat(tracked_files[i].path, &st) == 0) {
            if (st.st_mtime > tracked_files[i].mtime) {
                LOG_FILE_WATCHER("Modified: %s", tracked_files[i].path);
                tracked_files[i].mtime = st.st_mtime;
                changed = 1;
            }
        }
    }

    return changed;
}

static void notify_clients(struct mg_mgr *mgr) {
    struct mg_connection *c;
    int count = 0;
    for (c = mgr->conns; c != NULL; c = c->next) {
        if (c->is_websocket) {
            mg_ws_send(c, "reload", 6, WEBSOCKET_OP_TEXT);
            count++;
        }
    }
    if (count > 0) {
        LOG_INFO("Sent reload to %d client(s)", count);
    }
}

static int init_file_watcher(const char *directory) {
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        LOG_ERROR("inotify_init failed: %s", strerror(errno));
        return -1;
    }

    watch_descriptor = inotify_add_watch(inotify_fd, directory, 
                                         IN_MODIFY | IN_CREATE | IN_CLOSE_WRITE | 
                                         IN_MOVED_TO | IN_ATTRIB | IN_DELETE);
    if (watch_descriptor < 0) {
        LOG_ERROR("inotify_add_watch failed: %s", strerror(errno));
        close(inotify_fd);
        return -1;
    }

    scan_directory(directory);

    LOG_FILE_WATCHER("Watching directory: %s (%d files)", directory, tracked_count);
    return 0;
}

static int check_file_changes() {
    char buffer[BUF_LEN];
    int length = read(inotify_fd, buffer, BUF_LEN);

    if (length < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Error reading inotify events: %s", strerror(errno));
        }
        return 0;
    }

    int i = 0;
    int event_detected = 0;

    while (i < length) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];

        if (event->len > 0) {
            char *ext = strrchr(event->name, '.');
            if (ext && (strcmp(ext, ".html") == 0 || 
                       strcmp(ext, ".css") == 0 || 
                       strcmp(ext, ".js") == 0)) {

                if (event->mask & IN_CREATE) {
                    char fullpath[512];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", watch_dir, event->name);
                    add_tracked_file(fullpath);
                }

                event_detected = 1;
            }
        }

        i += EVENT_SIZE + event->len;
    }

    if (check_tracked_files()) {
        event_detected = 1;
    }

    return event_detected;
}

static void inject_hot_reload(struct mg_connection *c, struct mg_http_message *hm) {
    const char *hot_reload_js = 
        "<script>\n"
        "(function() {\n"
        "    function connect() {\n"
        "        console.log('[Hot Reload] Connecting...');\n"
        "        const ws = new WebSocket('ws://' + location.host + '/ws');\n"
        "        ws.onopen = () => console.log('[Hot Reload] Connected');\n"
        "        ws.onmessage = (e) => {\n"
        "            console.log('[Hot Reload] Reloading...', e.data);\n"
        "            location.reload();\n"
        "        };\n"
        "        ws.onclose = () => {\n"
        "            console.log('[Hot Reload] Disconnected, reconnecting...');\n"
        "            setTimeout(connect, 2000);\n"
        "        };\n"
        "        ws.onerror = (e) => console.error('[Hot Reload] Error:', e);\n"
        "        setInterval(() => {\n"
        "            if (ws.readyState === WebSocket.OPEN) ws.send('ping');\n"
        "        }, 30000);\n"
        "    }\n"
        "    if (document.readyState === 'complete') connect();\n"
        "    else window.addEventListener('load', connect);\n"
        "})();\n"
        "</script>\n";

    char path[512];
    char uri_str[256] = {0};
    snprintf(uri_str, sizeof(uri_str), "%.*s", (int)hm->uri.len, hm->uri.buf);

    if (strcmp(uri_str, "/") == 0) {
        snprintf(path, sizeof(path), "%s/index.html", watch_dir);
    } else {
        snprintf(path, sizeof(path), "%s%s", watch_dir, uri_str);
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        LOG_ERROR("Could not open file: %s", path);
        mg_http_reply(c, 404, "", "File not found");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        mg_http_reply(c, 500, "", "Memory allocation failed");
        return;
    }

    size_t read_bytes = fread(content, 1, size, f);
    content[read_bytes] = '\0';
    fclose(f);

    char *body_end = stristr(content, "</body>");

    if (body_end) {
        size_t before_len = body_end - content;
        size_t after_len = strlen(body_end);
        size_t script_len = strlen(hot_reload_js);
        size_t total_len = before_len + script_len + after_len;

        mg_printf(c, "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %lu\r\n"
                     "Connection: close\r\n\r\n", 
                     (unsigned long)total_len);
        mg_send(c, content, before_len);
        mg_send(c, hot_reload_js, script_len);
        mg_send(c, body_end, after_len);
    } else {

        mg_http_reply(c, 200, "Content-Type: text/html\r\n", "%s", content);
    }

    free(content);
}

static void event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_WS_OPEN) {
        LOG_WEBSOCKET("Client connected");
    } else if (ev == MG_EV_WS_MSG) {

        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        if (mg_strcmp(wm->data, mg_str("ping")) == 0) {
            mg_ws_send(c, "pong", 4, WEBSOCKET_OP_TEXT);
        }
    } else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        if (uri_matches(hm->uri, "/ws")) {
            mg_ws_upgrade(c, hm, NULL);
            return;
        }

        struct mg_str *content_type = mg_http_get_header(hm, "Accept");
        char uri_str[256] = {0};
        snprintf(uri_str, sizeof(uri_str), "%.*s", (int)hm->uri.len, hm->uri.buf);

        int is_html_request = (content_type && 
                              strstr(content_type->buf, "text/html") != NULL) ||
                              (hm->uri.len == 1 && hm->uri.buf[0] == '/') ||
                              (hm->uri.len > 5 && strstr(uri_str, ".html") != NULL);

        if (is_html_request) {
            inject_hot_reload(c, hm);
            return;
        }

        struct mg_http_serve_opts opts = {
            .root_dir = watch_dir,
        };
        mg_http_serve_dir(c, hm, &opts);

    } else if (ev == MG_EV_CLOSE) {
        if (c->is_websocket) {
            LOG_WEBSOCKET("Client disconnected");
        }
    }
}

void print_usage(const char* program_name) {

    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -d, --dir <directory>    Directory to serve (default: ./public)\n");
    printf("  -p, --port <port>        Port to listen on (default: 8080)\n");
    printf("  -h, --help               Show this help message\n");
    printf("  -v, --verbose            Enable verbose logging\n");
    printf("  -q, --quiet              Quiet mode (errors only)\n");
    printf("\nExamples:\n");
    printf("  %s                       # Serve ./public on port 8080\n", program_name);
    printf("  %s -d /var/www           # Serve /var/www on port 8080\n", program_name);
    printf("  %s -d ./dist -p 3000     # Serve ./dist on port 3000\n", program_name);
    printf("  %s index.html            # Serve current dir with index.html\n", program_name);
    printf("  %s -v                    # Verbose logging\n", program_name);
    printf("  %s -q                    # Quiet mode\n", program_name);
    printf("\n");
}

int main(int argc, char* argv[]) {
    struct mg_mgr mgr;
    struct mg_connection *c;

    log_init(LOG_LEVEL_INFO);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 < argc) {
                strncpy(watch_dir, argv[++i], sizeof(watch_dir) - 1);
            } else {
                LOG_ERROR("-d/--dir requires a directory argument");
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                strncpy(port, argv[++i], sizeof(port) - 1);
            } else {
                LOG_ERROR("-p/--port requires a port argument");
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            log_init(LOG_LEVEL_DEBUG);
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            log_init(LOG_LEVEL_ERROR);
        } else if (argv[i][0] != '-') {

            struct stat st;
            if (stat(argv[i], &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    strncpy(watch_dir, argv[i], sizeof(watch_dir) - 1);
                } else if (S_ISREG(st.st_mode)) {

                    char* last_slash = strrchr(argv[i], '/');
                    if (last_slash) {
                        size_t dir_len = last_slash - argv[i];
                        strncpy(watch_dir, argv[i], dir_len);
                        watch_dir[dir_len] = '\0';
                    } else {
                        strcpy(watch_dir, ".");
                    }
                }
            } else {
                LOG_ERROR("'%s' does not exist", argv[i]);
                return 1;
            }
        } else {
            LOG_ERROR("Unknown option '%s'", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (init_file_watcher(watch_dir) < 0) {
        return 1;
    }

    mg_mgr_init(&mgr);

    char listen_addr[256];
    snprintf(listen_addr, sizeof(listen_addr), "http://0.0.0.0:%s", port);
    c = mg_http_listen(&mgr, listen_addr, event_handler, NULL);
    if (c == NULL) {
        LOG_ERROR("Failed to create listener");
        return 1;
    }

    LOG_SERVER("===========================================");
    LOG_SERVER("Server started on http://localhost:%s", port);
    LOG_SERVER("Serving files from: %s", watch_dir);
    LOG_SERVER("Hot reload: ENABLED");
    LOG_SERVER("Press Ctrl+C to exit");
    LOG_SERVER("===========================================");

    time_t last_rescan = time(NULL);
    while (1) {
        mg_mgr_poll(&mgr, 100);

        if (check_file_changes()) {
            usleep(100000); 
            notify_clients(&mgr);
        }

        time_t now = time(NULL);
        if (difftime(now, last_rescan) > 10) {
            scan_directory(watch_dir);
            last_rescan = now;
        }
    }

    if (watch_descriptor >= 0) {
        inotify_rm_watch(inotify_fd, watch_descriptor);
    }
    if (inotify_fd >= 0) {
        close(inotify_fd);
    }
    mg_mgr_free(&mgr);

    return 0;
}