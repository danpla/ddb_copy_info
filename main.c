
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>

// Formatting v2 requires API >= 1.8
#define DDB_API_LEVEL 8
#define DDB_WARN_DEPRECATED 1
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>


#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 1


static DB_functions_t *deadbeef;
static ddb_gtkui_t *gtkui_plugin;

static char* tf;


#define CFG_FORMAT "copy_info.format"
#define DEFAULT_FORMAT "%tracknumber%. %artist% - %title% (%length%)"


static void recompile_tf(void)
{
    if (tf)
        deadbeef->tf_free(tf);

    deadbeef->conf_lock();
    tf = deadbeef->tf_compile(
        deadbeef->conf_get_str_fast(CFG_FORMAT, DEFAULT_FORMAT));
    deadbeef->conf_unlock();
}


static int start(void)
{
    recompile_tf();
    return 0;
}


static int stop(void)
{
    if (tf)
        deadbeef->tf_free(tf);
    tf = NULL;

    return 0;
}


static int connect(void)
{
    gtkui_plugin = (
        (ddb_gtkui_t *)deadbeef->plug_get_for_id(DDB_GTKUI_PLUGIN_ID));
    return gtkui_plugin ? 0 : -1;
}


static void copy_to_clipboard(const char *text, size_t text_length)
{
    assert(text);

    GdkDisplay *display = gtk_widget_get_display(gtkui_plugin->get_mainwin());
    GtkClipboard *clipboard = gtk_clipboard_get_for_display(
        display, GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, text, text_length);
}


static void copy_tf_error_msg_to_clipboard(void)
{
    assert(!tf);

    const char *msg = "DeaDBeeF copy info wrong format: %s\n";

    deadbeef->conf_lock();
    const char* fmt = deadbeef->conf_get_str_fast(CFG_FORMAT, DEFAULT_FORMAT);

    int ret = snprintf(NULL, 0, msg, fmt);
    if (ret >= 0) {
        const int buf_size = ret + 1;
        char* buf = (char *)malloc(buf_size);
        if (buf) {
            ret = snprintf(buf, buf_size, msg, fmt);
            if (ret >= 0)
                copy_to_clipboard(buf, ret);
            free(buf);
        }
    }

    deadbeef->conf_unlock();
}


static int copy_info(DB_plugin_action_t *action, int ctx)
{
    (void)(action);

    if (!tf) {
        copy_tf_error_msg_to_clipboard();
        return -1;
    }

    size_t text_capacity = 512;
    // text_size doesn't include the trailing null, so it's
    // always < text_capacity.
    size_t text_size = 0;
    char* text = (char *)malloc(text_capacity);
    if (!text)
        return -1;

    ddb_playlist_t *plt = deadbeef->action_get_playlist();
    DB_playItem_t *it = deadbeef->plt_get_first(plt, PL_MAIN);
    while (it) {
        if (ctx == DDB_ACTION_CTX_PLAYLIST || deadbeef->pl_is_selected(it)) {
            ddb_tf_context_t tf_ctx = {
                ._size = sizeof(ddb_tf_context_t),
                .flags = DDB_TF_CONTEXT_HAS_INDEX | DDB_TF_CONTEXT_NO_DYNAMIC,
                .it = it,
                .plt = plt,
                .idx = deadbeef->pl_get_idx_of(it),
                .iter = PL_MAIN,
            };

            while (1) {
                // We reserve a space for a question mark (in case of
                // errors), newline, and trailing null. The null is not
                // actually necessary, since gtk_clipboard_set_text()
                // accepts the string length, but let's keep it to make
                // the code future-proof.
                //
                // Keep in mind that tf_eval() terminates the output,
                // and therefore expects at least 1 char of available
                // space (passing 0 will give a segfault). The returned
                // length, however, doesn't include the null.
                #define MIN_AVAIL_SIZE 3
                assert(text_size < text_capacity);
                const size_t avail_size = text_capacity - text_size;
                int ret;
                if (avail_size < MIN_AVAIL_SIZE)
                    ret = 0;
                else
                    // We pass 1 less than avail_size, since 1 char is
                    // reserved for a newline, so at least 2 chars are
                    // available for tf_eval() (it will use the last
                    // one for null).
                    ret = deadbeef->tf_eval(
                        &tf_ctx, tf, text + text_size, avail_size - 1);

                if (ret < 0) {
                    // Skip errors.
                    assert(avail_size >= MIN_AVAIL_SIZE);
                    text[text_size++] = '?';
                    text[text_size++] = '\n';
                    break;
                } else if (avail_size < MIN_AVAIL_SIZE
                        // If the whole space is used, the string may be
                        // trimmed. Note that the length returned by
                        // tf_eval() doesn't include the trailing null.
                        || (size_t)ret == avail_size - 2) {
                    // Expand the capacity and try again.
                    text_capacity *= 2;
                    char* new_text = (char *)realloc(text, text_capacity);
                    if (!new_text) {
                        free(text);
                        deadbeef->pl_item_unref(it);
                        deadbeef->plt_unref(plt);
                        return -1;
                    }

                    text = new_text;
                    continue;
                } else {
                    text_size += ret;

                    assert(text_size < text_capacity);
                    assert(text_capacity - text_size >= 2);
                    text[text_size++] = '\n';
                    break;
                }
                #undef MIN_AVAIL_SIZE
            }
        }

        DB_playItem_t *next = deadbeef->pl_get_next(it, PL_MAIN);
        deadbeef->pl_item_unref(it);
        it = next;
    }

    assert(text_size < text_capacity);
    text[text_size] = 0;

    copy_to_clipboard(text, text_size);

    free(text);
    deadbeef->plt_unref(plt);
    return 0;
}


static DB_plugin_action_t copy_info_action = {
    .title = "Copy info",
    .name = "copy_info",
    .flags = (
        DB_ACTION_SINGLE_TRACK
        | DB_ACTION_MULTIPLE_TRACKS
        | DB_ACTION_ADD_MENU),
    .callback2 = copy_info,
    .next = NULL
};


static DB_plugin_action_t *get_actions(DB_playItem_t *it)
{
    (void)it;
    return &copy_info_action;
}


static int message(uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2)
{
    (void)ctx;
    (void)p1;
    (void)p2;

    if (id == DB_EV_CONFIGCHANGED)
        recompile_tf();

    return 0;
}


static const char configdialog[] =
    "property \"Format\" entry " CFG_FORMAT " \"" DEFAULT_FORMAT "\";";


static DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = DDB_API_LEVEL,
    .plugin.version_major = VERSION_MAJOR,
    .plugin.version_minor = VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3, 0, 0)
    .plugin.id = "ddb_copy_info_gtk3",
#else
    .plugin.id = "ddb_copy_info_gtk2",
#endif
    .plugin.name = "Copy info",
    .plugin.descr =
        "Copy information about selected tracks to the clipboard "
        "using a custom format",
    .plugin.copyright =
        "Copy info plugin for DeaDBeeF Player\n"
        "Copyright (c) 2017 Daniel Plakhotich\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty. In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        "   claim that you wrote the original software. If you use this software\n"
        "   in a product, an acknowledgement in the product documentation would be\n"
        "   appreciated but is not required.\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        "   misrepresented as being the original software.\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "https://github.com/danpla/ddb_copy_info",
    .plugin.start = start,
    .plugin.stop = stop,
    .plugin.connect = connect,
    .plugin.get_actions = get_actions,
    .plugin.configdialog = configdialog,
    .plugin.message = message,
};


#if GTK_CHECK_VERSION(3, 0, 0)
DB_plugin_t *ddb_copy_info_gtk3_load(DB_functions_t *api)
#else
DB_plugin_t *ddb_copy_info_gtk2_load(DB_functions_t *api)
#endif
{
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
