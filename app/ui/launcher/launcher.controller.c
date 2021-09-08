#include "app.h"
#include "launcher.controller.h"
#include "apps.controller.h"

#include "util/logging.h"

static void launcher_controller(struct lv_obj_controller_t *self, void *args);

static void launcher_view_init(lv_obj_controller_t *self, lv_obj_t *view);

static void launcher_view_destroy(lv_obj_controller_t *self, lv_obj_t *view);

static void launcher_handle_server_updated(void *userdata, PPCMANAGER_RESP resp);

static void update_pclist(launcher_controller_t *controller);

static void cb_pc_selected(lv_event_t *event);

static void cb_nav_focused(lv_event_t *event);

static void cb_detail_focused(lv_event_t *event);


const lv_obj_controller_class_t launcher_controller_class = {
        .constructor_cb = launcher_controller,
        .create_obj_cb = launcher_win_create,
        .obj_created_cb = launcher_view_init,
        .obj_deleted_cb = launcher_view_destroy,
        .instance_size = sizeof(launcher_controller_t),
};

static void launcher_controller(struct lv_obj_controller_t *self, void *args) {
    (void) args;
    launcher_controller_t *controller = (launcher_controller_t *) self;
    controller->_pcmanager_callbacks.added = launcher_handle_server_updated;
    controller->_pcmanager_callbacks.updated = launcher_handle_server_updated;
    controller->_pcmanager_callbacks.userdata = controller;
    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (cur->selected) {
            controller->selected_server = cur;
            break;
        }
    }
    static const lv_style_prop_t props[] = {LV_STYLE_OPA, LV_STYLE_TRANSLATE_X, LV_STYLE_TRANSLATE_Y, 0};
    lv_style_transition_dsc_init(&controller->tr_detail, props, lv_anim_path_ease_out, 200, 5, NULL);
    lv_style_transition_dsc_init(&controller->tr_nav, props, lv_anim_path_ease_out, 250, 5, NULL);
}


static void launcher_view_init(lv_obj_controller_t *self, lv_obj_t *view) {
    launcher_controller_t *controller = (launcher_controller_t *) self;
    pcmanager_register_listener(&controller->_pcmanager_callbacks);
    controller->pane_manager = lv_controller_manager_create(controller->detail);
    lv_obj_add_event_cb(controller->nav, cb_nav_focused, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(controller->detail, cb_detail_focused, LV_EVENT_FOCUSED, controller);
    lv_obj_add_event_cb(controller->pclist, cb_pc_selected, LV_EVENT_CLICKED, controller);
    update_pclist(controller);

    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        if (!cur->selected) continue;
        lv_controller_manager_replace(controller->pane_manager, &apps_controller_class, cur);
        break;
    }
    pcmanager_auto_discovery_start(pcmanager);
}

static void launcher_view_destroy(lv_obj_controller_t *self, lv_obj_t *view) {
    pcmanager_auto_discovery_stop(pcmanager);

    launcher_controller_t *controller = (launcher_controller_t *) self;
    lv_controller_manager_del(controller->pane_manager);
    pcmanager_unregister_listener(&controller->_pcmanager_callbacks);
}

void launcher_handle_server_updated(void *userdata, PPCMANAGER_RESP resp) {
    launcher_controller_t *controller = userdata;
    update_pclist(controller);
}

static void cb_pc_selected(lv_event_t *event) {
    struct _lv_obj_t *target = lv_event_get_target(event);
    if (lv_obj_get_parent(target) != lv_event_get_current_target(event)) return;
    launcher_controller_t *controller = lv_event_get_user_data(event);
    PSERVER_LIST selected = lv_obj_get_user_data(target);
    lv_controller_manager_replace(controller->pane_manager, &apps_controller_class, selected);
    uint32_t pclen = lv_obj_get_child_cnt(controller->pclist);
    for (int i = 0; i < pclen; i++) {
        lv_obj_t *pcitem = lv_obj_get_child(controller->pclist, i);
        PSERVER_LIST cur = (PSERVER_LIST) lv_obj_get_user_data(pcitem);
        cur->selected = cur == selected;
        if (!cur->selected) {
            lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        }
    }
}

static void update_pclist(launcher_controller_t *controller) {
    lv_obj_clean(controller->pclist);
    for (PSERVER_LIST cur = pcmanager_servers(pcmanager); cur != NULL; cur = cur->next) {
        lv_obj_t *pcitem = lv_list_add_btn(controller->pclist, LV_SYMBOL_DUMMY, cur->server->hostname);
        lv_obj_add_flag(pcitem, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_style_bg_color(pcitem, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
        lv_obj_set_user_data(pcitem, cur);

        if (!cur->selected) {
            lv_obj_clear_state(pcitem, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(pcitem, LV_STATE_CHECKED);
        }
    }
}


static void cb_detail_focused(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    if (lv_obj_get_parent(event->target) != controller->detail) return;
    lv_obj_add_state(controller->detail, LV_STATE_USER_1);
    applog_i("Launcher", "Focused content");
}

static void cb_nav_focused(lv_event_t *event) {
    launcher_controller_t *controller = lv_event_get_user_data(event);
    lv_obj_t *target = event->target;
    while (target && target != controller->nav) {
        target = lv_obj_get_parent(target);
    }
    if (!target) return;
    lv_obj_clear_state(controller->detail, LV_STATE_USER_1);
    applog_i("Launcher", "Focused navigation");
}