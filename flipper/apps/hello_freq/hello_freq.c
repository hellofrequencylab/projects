#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

#define TAG "HelloFreq"

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* input_queue;
    bool running;
} HelloFreqApp;

/* Runs on the GUI thread. No allocation, no blocking, no I/O. */
static void hello_freq_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Hello, Flipper");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 28, "Press Back to exit.");
}

static void hello_freq_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    FuriMessageQueue* input_queue = context;
    furi_message_queue_put(input_queue, event, FuriWaitForever);
}

static HelloFreqApp* hello_freq_app_alloc(void) {
    HelloFreqApp* app = malloc(sizeof(HelloFreqApp));
    app->running = true;
    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    app->view_port = view_port_alloc();

    view_port_draw_callback_set(app->view_port, hello_freq_draw_callback, app);
    view_port_input_callback_set(app->view_port, hello_freq_input_callback, app->input_queue);

    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    return app;
}

/* Must undo hello_freq_app_alloc in reverse order, on every exit path. */
static void hello_freq_app_free(HelloFreqApp* app) {
    furi_assert(app);
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->input_queue);
    free(app);
}

int32_t hello_freq_app(void* p) {
    UNUSED(p);
    HelloFreqApp* app = hello_freq_app_alloc();
    FURI_LOG_I(TAG, "started");

    InputEvent event;
    while(app->running) {
        if(furi_message_queue_get(app->input_queue, &event, 100) != FuriStatusOk) {
            continue;
        }
        if(event.type == InputTypeShort && event.key == InputKeyBack) {
            app->running = false;
        }
        view_port_update(app->view_port);
    }

    FURI_LOG_I(TAG, "stopping");
    hello_freq_app_free(app);
    return 0;
}
