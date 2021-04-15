#include <stdio.h>

#include "app.h"
#define RES_IMPL
#include "res.h"
#undef RES_IMPL

#include "ui/config.h"

#include "nuklear/config.h"
#include "nuklear.h"
#include "nuklear/ext_functions.h"
#include "nuklear/ext_image.h"
#include "nuklear/ext_sprites.h"
#include "nuklear/ext_styling.h"
#include "nuklear/platform.h"

#include "debughelper.h"
#include "backend/backend_root.h"
#include "stream/session.h"
#include "stream/platform.h"
#include "ui/root.h"
#include "ui/fonts.h"
#include "util/bus.h"

bool running = true;

int main(int argc, char *argv[])
{
#if TARGET_WEBOS || TARGET_LGNC
    if (getenv("MOONLIGHT_OUTPUT_NOREDIR") == NULL)
        REDIR_STDOUT(APPID);
#endif
#if TARGET_RASPI
    setenv("SDL_VIDEODRIVER", "rpi", 1);
#endif
    bus_init();

    int ret = app_init(argc, argv);
    if (ret != 0)
    {
        return ret;
    }
    platform_init(app_configuration->platform, argc, argv);
    printf("Decoder platform: %s\n", platform_definitions[platform_current].name);

    /* GUI */
    struct nk_context *ctx;
    APP_WINDOW_CONTEXT win = app_window_create();

    backend_init();

    ctx = nk_platform_init(win);
    ui_display_size(app_window_width, app_window_height);
    streaming_display_size(app_window_width, app_window_height);
    {
        struct nk_font_atlas *atlas;
        nk_platform_font_stash_begin(&atlas);
        struct nk_font *noto20 = nk_font_atlas_add_from_memory_s(atlas, (void *)res_notosans_regular_data, res_notosans_regular_size, 20, NULL);
        fonts_init(atlas);
        nk_platform_font_stash_end();
        nk_style_set_font(ctx, &noto20->handle);
#if DEBUG && TARGET_DESKTOP
        nk_style_load_all_cursors(ctx, atlas->cursors);
#endif
    }
    nk_ext_sprites_init();
    nk_ext_apply_style(ctx);

    ui_root_init(ctx);

    while (running)
    {
        app_main_loop((void *)ctx);
    }

    settings_save(app_configuration);

    ui_root_destroy();

    nk_ext_sprites_destroy();

    nk_platform_shutdown();
    backend_destroy();
    app_destroy();
    bus_destroy();

#if DEBUG
    printf("Quitted gracefully :)\n");
#endif
    return 0;
}

void app_request_exit()
{
    running = false;
}