#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <hildon/hildon.h>
#include <string.h>
#include <pthread.h>

#include "../frontend/plugin_lib.h"
#include "../frontend/main.h"
#include "../libpcsxcore/misc.h"
#include "../include/psemu_plugin_defs.h"
#include "../libpcsxcore/cdrom.h"
#include "../libpcsxcore/cdriso.h"
#include "../frontend/libpicofe/readpng.h"
#include "maemo_common.h"
#include <libosso.h>
#include <dbus/dbus.h>

#define X_RES           800
#define Y_RES           480
#define D_WIDTH			640
#define D_HEIGHT		480

#define CALL_SIGNAL_IF "com.nokia.csd.Call"
#define CALL_SIGNAL_PATH "/com/nokia/csd/call"
#define CALL_INCOMING_SIG "Coming"

#define DBUS_RULE_CALL_INCOMING "type='signal',interface='" CALL_SIGNAL_IF \
                                "',path='" CALL_SIGNAL_PATH \
                                "',member='" CALL_INCOMING_SIG "'"

osso_context_t* osso = NULL;
int bRunning = TRUE;
extern int bKeepDisplayOn;
extern int bAutosaveOnExit;
extern int cornerActions[4];
extern char keys_config_file[MAXPATHLEN];
static pthread_t display_thread = (pthread_t)0;
int g_layer_x = (X_RES - D_WIDTH) / 2;
int g_layer_y = (Y_RES - D_HEIGHT) / 2;
int g_layer_w = D_WIDTH, g_layer_h = D_HEIGHT;

static GdkImage *image;
static HildonAnimationActor *actor;
static GtkWidget *window, *drawing = NULL;

static int pl_buf_w, pl_buf_h;
int keymap[65536];
int direction_keys[4];

// map psx4m compatible keymap to PSX keys
static const unsigned char keymap2[14] = {
	DKEY_LEFT,   // 0
	DKEY_RIGHT,
	DKEY_UP,
	DKEY_DOWN,
	DKEY_CIRCLE,
	DKEY_CROSS,  // 5
	DKEY_TRIANGLE,
	DKEY_SQUARE,
	DKEY_SELECT,
	DKEY_START,
	DKEY_L1,     // 10
	DKEY_R1,
	DKEY_L2,
	DKEY_R2,
};

void hildon_quit()
{
	maemo_finish();
	gtk_main_quit();
	exit(0);
}

gdouble press_x = -1;
gdouble press_y = -1;

int maemo_x11_update_keys();
void show_notification(char* text);

void change_slot(int delta)
{
	state_slot += delta;
	if (state_slot > 9)
		state_slot = 0;
	else if (state_slot < 0)
		state_slot = 9;
	char message[50];
	sprintf(message,"Savestate slot: %i",state_slot + 1);
	show_notification(message);
}

void save(int state_slot)
{
	emu_save_state(state_slot);
	char buf[MAXPATHLEN];
	if (image && image->mem){
		sprintf (buf,"/opt/maemo/usr/games/screenshots%s.%3.3d",file_name,state_slot);
		writepng(buf, image->mem, pl_buf_w,pl_buf_h);
	}
	char message[50];
	sprintf(message,"Saved savestate slot: %i",state_slot + 1);
	show_notification(message);
}

void quit()
{
	if (bAutosaveOnExit){
		show_notification("Autosaving");
		emu_save_state(99);
		char buf[MAXPATHLEN];
		if (image && image->mem){
			sprintf (buf,"/opt/maemo/usr/games/screenshots%s.%3.3d",file_name,99);
			writepng(buf, image->mem, pl_buf_w,pl_buf_h);
		}
	}
	hildon_quit();
}

int show_confirmbox(char* text)
{
	if (!window)
		return TRUE;

	GtkWidget *dialog;
	dialog = gtk_message_dialog_new (GTK_WINDOW(window),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO,
									 text);
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	if (result == GTK_RESPONSE_YES)
		return TRUE;
	return FALSE;
}

static void
window_button_proxy(GtkWidget *widget,
  				    GdkEventButton *event,
				    gpointer user_data)
{
	int corner = -1;
	int sens = 100;

	switch (event->type){
	case GDK_BUTTON_PRESS:
		//printf("GDK_BUTTON_PRESS: x=%f y=%f\n", event->x, event->y);
		press_x = event->x;
		press_y = event->y;
		break;
	case GDK_BUTTON_RELEASE:
		//printf("GDK_BUTTON_RELEASE: x=%f y=%f\n", event->x, event->y);
		if (press_x < sens && press_y < sens && event->x < sens && event->y < sens)
			corner = 0;
		else if (press_x > 800 - sens && press_y < sens && event->x > 800 - sens && event->y < sens)
			corner = 1;
		else if (press_x > 800 - sens && press_y > 480 - sens && event->x > 800 - sens && event->y > 480 - sens)
			corner = 2;
		else if (press_x < sens && press_y > 480 - sens && event->x < sens && event->y > 480 - sens)
			corner = 3;

		press_x = -1;
		press_y = -1;
		break;
	default:
		break;
	}

	if (corner >= 0){
		switch (cornerActions[corner]){
			case 1:
				if (show_confirmbox("Save savestate?"))
					save(state_slot);
				break;
			case 2:
				if (show_confirmbox("Load savestate?"))
					emu_load_state(state_slot);
				break;
			case 3:
				change_slot(1);
				break;
			case 4:
				change_slot(-1);
				break;
			case 5:
				if (show_confirmbox("Quit?"))
					quit();
				break;
		}
	}
}

static void *displayThread(void *arg)
{
	DBusConnection* system_bus = (DBusConnection*)osso_get_sys_dbus_connection(osso);
	DBusMessage* msg = dbus_message_new_method_call("com.nokia.mce",
												    "/com/nokia/mce/request",
												    "com.nokia.mce.request",
												    "req_display_blanking_pause");
	if (msg && system_bus) {
		bRunning = TRUE;
		while (bRunning) {
			dbus_connection_send(system_bus, msg, NULL);
			dbus_connection_flush(system_bus);
			int i = 0;
			for (i=0; i<8; i++){
				usleep(500000);
				if (!bRunning)
					break;
			}
		}
		dbus_message_unref(msg);
	}

	pthread_exit(0);
	return NULL;
}

void show_notification(char* text)
{
	if (window){
		GtkWidget* banner = hildon_banner_show_information(GTK_WIDGET(window), NULL, text);
		hildon_banner_set_timeout(HILDON_BANNER(banner), 3000);
	}else{
		DBusConnection* session_bus = (DBusConnection*)osso_get_dbus_connection(osso);
		DBusMessageIter args;
		DBusMessage*msg = dbus_message_new_method_call("org.freedesktop.Notifications",
													   "/org/freedesktop/Notifications",
													   "org.freedesktop.Notifications",
													   "SystemNoteInfoprint");
		if (msg) {
			dbus_message_iter_init_append(msg, &args);
			char* param = text;
			if (dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param)) {
				dbus_connection_send(session_bus, msg, NULL);
				dbus_connection_flush(session_bus);
			}
			dbus_message_unref(msg);
		}
	}
}

void show_messagebox(char* text)
{
	if (!window)
		return;

	GtkWidget *dialog;
	dialog = gtk_message_dialog_new (GTK_WINDOW(window),
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_OK,
									 text);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

#include <hildon/hildon-file-chooser-dialog.h>
void change_disc()
{
	GtkWidget *dialog;
	dialog = hildon_file_chooser_dialog_new (GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_window_set_title (GTK_WINDOW (dialog), "Change disc");

	char currentFile[MAXPATHLEN];
	strcpy(currentFile, GetIsoFile());
	if (strlen(currentFile))
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(dialog), currentFile);
	else
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), "/home/user/MyDocs/");

	GtkFileFilter *filter=gtk_file_filter_new();
	gtk_file_filter_add_pattern (filter,"*.bin");
	gtk_file_filter_add_pattern (filter,"*.BIN");
	gtk_file_filter_add_pattern (filter,"*.iso");
	gtk_file_filter_add_pattern (filter,"*.ISO");
	gtk_file_filter_add_pattern (filter,"*.img");
	gtk_file_filter_add_pattern (filter,"*.IMG");
	gtk_file_filter_add_pattern (filter,"*.z");
	gtk_file_filter_add_pattern (filter,"*.Z");
	gtk_file_filter_add_pattern (filter,"*.znx");
	gtk_file_filter_add_pattern (filter,"*.ZNX");
	gtk_file_filter_add_pattern (filter,"*.pbp");
	gtk_file_filter_add_pattern (filter,"*.PBP");
	gtk_file_filter_add_pattern (filter,"*.mdf");
	gtk_file_filter_add_pattern (filter,"*.MDF");
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog),filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		//if (strcmp(filename, currentFile)) {
			CdromId[0] = '\0';
			CdromLabel[0] = '\0';

			set_cd_image(filename);
			if (ReloadCdromPlugin() < 0)
				printf("Failed to load cdr plugin\n");

			if (CDR_open() < 0)
				printf("Failed to open cdr plugin\n");

			strcpy(file_name, strrchr(filename,'/'));

			SetCdOpenCaseTime(time(NULL) + 3);
			LidInterrupt();
		//}
		g_free (filename);
	}

	gtk_widget_destroy (dialog);
}

void change_multi_disc()
{
    HildonDialog* window = HILDON_DIALOG(hildon_dialog_new());
    gtk_window_set_title (GTK_WINDOW (window), "Change disc");
    gtk_window_set_default_size(GTK_WINDOW (window), 480, 300);

    GtkWidget* sw = hildon_pannable_area_new ();
    gtk_box_pack_start (GTK_BOX(GTK_DIALOG(window)->vbox), sw, TRUE, TRUE, 0);

    GtkWidget* tree_view = hildon_gtk_tree_view_new (HILDON_UI_MODE_EDIT);
    gtk_widget_set_name (tree_view, "fremantle-widget");

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);

    int i;
    GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    for (i = 0; i < cdrIsoMultidiskCount; i++) {
        gchar *str;

        str = g_strdup_printf ("Disc %d", i+1);
        gtk_list_store_insert_with_values (store, NULL, i, 0, str, -1);
        g_free (str);
    }
    GtkTreeModel* model =  GTK_TREE_MODEL (store);

    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), model);
    g_object_unref (model);

    GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    GtkCellRenderer* renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer,
                  "xalign", 0.5,
                  "weight", PANGO_WEIGHT_NORMAL,
                  NULL);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view),
                                                 0, "Column 0",
                                                 renderer,
                                                 "text", 0,
                                                 NULL);

    char current[5];
    sprintf(current, "%i", cdrIsoMultidiskSelect);
    GtkTreePath* path = gtk_tree_path_new_from_string(current);
    gtk_tree_selection_select_path (selection, path);
    gtk_tree_path_free(path);

    gtk_widget_set_size_request (tree_view, 480, 800);
    gtk_container_add (GTK_CONTAINER (sw), tree_view);

    hildon_dialog_add_button (HILDON_DIALOG(window), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);

    gtk_widget_show_all (GTK_WIDGET(window));
    gint result = gtk_dialog_run (GTK_DIALOG (window));
    if (result == GTK_RESPONSE_ACCEPT) {
      GtkTreeModel* model;
      GtkTreeIter iter;
      GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
      if (gtk_tree_selection_get_selected(selection, &model, &iter)){
	    GtkTreePath* path = gtk_tree_model_get_path(model , &iter);
		int* i = gtk_tree_path_get_indices(path) ;

		cdrIsoMultidiskSelect = *i;
		CdromId[0] = '\0';
		CdromLabel[0] = '\0';

		CDR_close();
		if (CDR_open() < 0) {
			printf("Failed to load cdr plugin\n");
			return;
		}

		SetCdOpenCaseTime(time(NULL) + 3);
		LidInterrupt();
      }
    }
	gtk_widget_destroy(GTK_WIDGET(window));
}

static DBusHandlerResult on_msg_recieved(DBusConnection* connection G_GNUC_UNUSED, DBusMessage* message, void* data)
{
	const char* path = dbus_message_get_path(message);
	if (path && g_str_equal(path, CALL_SIGNAL_PATH)){
		const char* mbr = dbus_message_get_member(message);
		if (mbr && g_str_equal(mbr, CALL_INCOMING_SIG))
			show_messagebox("Paused");
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
window_key_proxy(GtkWidget *widget,
		     GdkEventKey *event,
		     gpointer user_data)
{
	key_press_event(event->hardware_keycode, event->type == GDK_KEY_PRESS ? 1 : (event->type == GDK_KEY_RELEASE ? 2 : 0) );
}

int last_key_pressed = 0;
inline void key_press_event(int key2,int type)
{
	int psxkey1 = -1, psxkey2 = -1;
	int key=keymap[key2];

	if (key < 0)
		return;

	if (type == 1 && key2 == last_key_pressed)
		return;
	last_key_pressed = type == 1 ? key2 : 0;

	//printf("Key: %i %s\n", key2, type == 1 ? "Pressed" : (type == 2 ? "Released" : "Unknown"));
	if (key < ARRAY_SIZE(keymap2)){
		psxkey1 = keymap2[key];
	}else switch (key) {
		case 14:
			quit();
			break;
		case 15:
			psxkey1 = DKEY_UP;
			psxkey2 = DKEY_LEFT;
			break;
		case 16:
			psxkey1 = DKEY_UP;
			psxkey2 = DKEY_RIGHT;
			break;
		case 17:
			psxkey1 = DKEY_DOWN;
			psxkey2 = DKEY_LEFT;
			break;
		case 18:
			psxkey1 = DKEY_DOWN;
			psxkey2 = DKEY_RIGHT;
			break;
		case 19:
			if (type == 1)
				save(state_slot);
			return;
		case 20:
			if (type == 1)
				emu_load_state(state_slot);
			return;
		case 21:
			if (type == 1)
				change_slot(1);
			return;
		case 22:
			if (type == 1)
				change_slot(-1);
			return;
		case 23:
			if (type == 1){
				if (cdrIsoMultidiskCount > 1)
					change_multi_disc();
				else
					change_disc();
			}
			return;
	}

	if (in_type1 == PSE_PAD_TYPE_GUNCON){
		if (type == 1) {
			switch (psxkey1){
				case DKEY_CROSS:
					in_state_gun |= SACTION_GUN_A;
					break;		
				case DKEY_CIRCLE:
					in_state_gun |= SACTION_GUN_B;
					break;		
				case DKEY_TRIANGLE:
					in_state_gun |= SACTION_GUN_TRIGGER2;
					break;		
				case DKEY_SQUARE:
					in_state_gun |= SACTION_GUN_TRIGGER;
					break;		
			}
		}else if (type == 2) {
			switch (psxkey1){
				case DKEY_CROSS:
					in_state_gun &= ~SACTION_GUN_A;
					break;		
				case DKEY_CIRCLE:
					in_state_gun &= ~SACTION_GUN_B;
					break;		
				case DKEY_TRIANGLE:
					in_state_gun &= ~SACTION_GUN_TRIGGER2;
					break;		
				case DKEY_SQUARE:
					in_state_gun &= ~SACTION_GUN_TRIGGER;
					break;		
			}
		}
	}else{
		if (type == 1) {
		if (psxkey1 >= 0)
			in_keystate |= 1 << psxkey1;
		if (psxkey2 >= 0)
			in_keystate |= 1 << psxkey2;

			if (in_type1 == PSE_PAD_TYPE_ANALOGPAD){
				switch(psxkey1){
					case DKEY_LEFT:
						in_a1[0] = 0;
						break;
					case DKEY_RIGHT:
						in_a1[0] = 255;
						break;
					case DKEY_UP:
						in_a1[1] = 0;
						break;
					case DKEY_DOWN:
						in_a1[1] = 255;
						break;
				}
	}
		}
		else if (type == 2) {
		if (psxkey1 >= 0)
			in_keystate &= ~(1 << psxkey1);
		if (psxkey2 >= 0)
			in_keystate &= ~(1 << psxkey2);

			if (in_type1 == PSE_PAD_TYPE_ANALOGPAD){
				switch(psxkey1){
					case DKEY_LEFT:
					case DKEY_RIGHT:
						in_a1[0] = 127;
						break;
					case DKEY_UP:
					case DKEY_DOWN:
						in_a1[1] = 127;
						break;
				}
			}
		emu_set_action(SACTION_NONE);
	}
	}
}

void plat_finish()
{
	hildon_quit();
}

void set_accel_multipliers()
{
	accelOptions.xMultiplier = 255.0 / ( (accelOptions.maxValue - accelOptions.sens) * 2.0);
	accelOptions.yMultiplier = 255.0 / ( (accelOptions.maxValue - accelOptions.sens) * 2.0);
}

#include <gdk/gdkx.h>
int maemo_init(int *argc, char ***argv)
{
	osso = osso_initialize("pcsxrearmed", PACKAGE_VERSION, FALSE, NULL);

	DBusConnection* system_bus = (DBusConnection*)osso_get_sys_dbus_connection(osso);
    dbus_bus_add_match(system_bus, DBUS_RULE_CALL_INCOMING, NULL);
	dbus_connection_add_filter(system_bus, on_msg_recieved, NULL, NULL);

	FILE* pFile;
	pFile = fopen(keys_config_file, "r");
	if (pFile == NULL){
		fprintf(stderr, "Error opening keys config file %s\n", keys_config_file);
		return 1;
	}
	printf("Keys config read from %s\n", keys_config_file);

	int ch;
	int i=0;
	for (i=0;i<65536;i++)
		keymap[i]=-1;
	if (NULL != pFile) {
		for(i=0;i<24;i++){
			fscanf(pFile, "%i",&ch);
			keymap[ch]=i;
			if (i < 4)
				direction_keys[i] = ch;
		}
		fclose(pFile);
	}
	
	switch (in_type1){
		case PSE_PAD_TYPE_GUNCON:
			memset(cornerActions, 0, sizeof(cornerActions));
			printf("Controller set to GUNCON (SLPH-00034)\n");
			break;
		case PSE_PAD_TYPE_STANDARD:
			printf("Controller set to standard (SCPH-1080)\n");
			break;
		case PSE_PAD_TYPE_ANALOGPAD:
			printf("Controller set to analog (SCPH-1150)\n");
			break;	
	}

	if (in_enable_vibration)
		printf("Vibration enabled\n");

	if (!(g_maemo_opts&8)){
	gtk_init (argc, argv);

	window = hildon_stackable_window_new ();
	gtk_widget_realize (window);
	gtk_window_fullscreen (GTK_WINDOW(window));

		if (cornerActions[0] + cornerActions[1] + cornerActions[2] + cornerActions[3] > 0){
			g_signal_connect (G_OBJECT (window), "button_release_event",
						G_CALLBACK (window_button_proxy), NULL);
			g_signal_connect (G_OBJECT (window), "button_press_event",
						G_CALLBACK (window_button_proxy), NULL);
		}

	g_signal_connect (G_OBJECT (window), "key-press-event",
				G_CALLBACK (window_key_proxy), NULL);
	g_signal_connect (G_OBJECT (window), "key-release-event",
				G_CALLBACK (window_key_proxy), NULL);
	g_signal_connect (G_OBJECT (window), "delete_event",
				G_CALLBACK (hildon_quit), NULL);
	gtk_widget_add_events (window,
				GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

	actor = HILDON_ANIMATION_ACTOR (hildon_animation_actor_new());
	if (g_maemo_opts & 2)
		hildon_animation_actor_set_position (actor, 0, 0 );
	else
		hildon_animation_actor_set_position (actor, (X_RES - D_WIDTH)/2, (Y_RES - D_HEIGHT)/2 );
	hildon_animation_actor_set_parent (actor, GTK_WINDOW (window));

	drawing = gtk_image_new ();

	gtk_container_add (GTK_CONTAINER (actor), drawing);

	gtk_widget_show_all (GTK_WIDGET (actor));
	gtk_widget_show_all (GTK_WIDGET (window));
	}else{
		gtk_init (argc, argv);
		/*GdkScreen* scr = gdk_screen_get_default();
		window = GTK_WIDGET(gdk_screen_get_root_window(scr));
		if (!window)
			window = GTK_WIDGET(gdk_get_default_root_window());*/
	}

	set_accel_multipliers();

	if (bKeepDisplayOn){
		if (pthread_create(&display_thread, NULL, displayThread, NULL))
			printf("Failed to create display thread.\n");		
	}

	pl_rearmed_cbs.only_16bpp = 1;
	return 0;
}

void maemo_finish()
{
	if (display_thread > 0){
		bRunning = FALSE;
		pthread_join(display_thread, NULL);
	}

	if (osso){
		osso_deinitialize(osso);
		osso = NULL;
	}
	printf("Exiting\n");
}

void menu_loop(void)
{
}

void *plat_gvideo_set_mode(int *w_, int *h_, int *bpp_)
{
	int w = *w_, h = *h_;

	if (g_maemo_opts&8) return pl_vout_buf;
	//printf("Setting video mode %ix%i\n", w, h);

	if (w <= 0 || h <= 0)
		return pl_vout_buf;

	if (image) gdk_image_destroy(image);
	image = gdk_image_new( GDK_IMAGE_FASTEST, gdk_visual_get_system(), w, h );

	pl_vout_buf = (void *) image->mem;

	gtk_image_set_from_image (GTK_IMAGE(drawing), image, NULL);

	gtk_window_resize (GTK_WINDOW (actor), w, h);
	if (g_maemo_opts & 2)
		hildon_animation_actor_set_scale (actor,
				(gdouble)800 / (gdouble)w,
				(gdouble)480 / (gdouble)h
				);
	else
		hildon_animation_actor_set_scale (actor,
				(gdouble)D_WIDTH / (gdouble)w,
				(gdouble)D_HEIGHT / (gdouble)h
				);
	pl_buf_w=w;pl_buf_h=h;
	return pl_vout_buf;
}

void *plat_gvideo_flip(void)
{
	if (!(g_maemo_opts&8))
		gtk_widget_queue_draw(drawing);

	// process accelometer
	if (g_maemo_opts & 4) {
		float x, y, z;
		FILE* f = fopen( "/sys/class/i2c-adapter/i2c-3/3-001d/coord", "r" );
		if( !f ) {printf ("err in accel"); exit(1);}
		fscanf( f, "%f %f %f", &x, &y, &z );
		fclose( f );

		if (in_type1 == PSE_PAD_TYPE_ANALOGPAD){
			if (x > accelOptions.maxValue) x = accelOptions.maxValue;
			else if (x < -accelOptions.maxValue) x = -accelOptions.maxValue;

			const int maxValue = accelOptions.maxValue - accelOptions.sens;
			if(x > accelOptions.sens){
				x -= accelOptions.sens;
				in_a1[0] = (-x + maxValue ) *  accelOptions.xMultiplier;
			}else if (x < -accelOptions.sens){
				x += accelOptions.sens;
				in_a1[0] = (-x + maxValue ) *  accelOptions.xMultiplier;
			}else in_a1[0] = 127;

			y += accelOptions.y_def;
			if (y > accelOptions.maxValue) y = accelOptions.maxValue;
			else if (y < -accelOptions.maxValue) y = -accelOptions.maxValue;

			if(y > accelOptions.sens){
				y -= accelOptions.sens;
				in_a1[1] = (-y + maxValue ) *  accelOptions.yMultiplier;
			}else if (y < -accelOptions.sens){
				y += accelOptions.sens;
				in_a1[1] = (-y + maxValue ) *  accelOptions.yMultiplier;
			}else in_a1[1] = 127;

			//printf("x: %i y: %i\n", in_a1[0], in_a1[1]);
		}else{
			if( x > accelOptions.sens ) in_keystate |= 1 << DKEY_LEFT;
			else if( x < -accelOptions.sens ) in_keystate |= 1 << DKEY_RIGHT;
		else {in_keystate &= ~(1 << DKEY_LEFT);in_keystate &= ~(1 << DKEY_RIGHT);}

			y += accelOptions.y_def;
			if( y > accelOptions.sens )in_keystate |= 1 << DKEY_UP;
			else if( y < -accelOptions.sens ) in_keystate |= 1 << DKEY_DOWN;
		else {in_keystate &= ~(1 << DKEY_DOWN);in_keystate &= ~(1 << DKEY_UP);}
		}
	}

	return pl_vout_buf;
}

// for frontend/plugin_lib.c
void update_input(void)
{
	if (g_maemo_opts & 8)
		maemo_x11_update_keys();
	else {
		/* process GTK+ events */
		while (gtk_events_pending())
			gtk_main_iteration();
	}
}

int omap_enable_layer(int enabled)
{
	return 0;
}

void menu_notify_mode_change(int w, int h, int bpp)
{
}

void *plat_prepare_screenshot(int *w, int *h, int *bpp)
{
	return NULL;
}

void plat_step_volume(int is_up)
{
}

void plat_trigger_vibrate(int pad, int low, int high)
{
	const int vDuration = 10;

	DBusConnection* system_bus = (DBusConnection*)osso_get_sys_dbus_connection(osso);
	DBusMessageIter args;
	DBusMessage*msg = dbus_message_new_method_call("com.nokia.mce",
												   "/com/nokia/mce/request",
												   "com.nokia.mce.request",
												   "req_start_manual_vibration");
	if (msg) {
		dbus_message_iter_init_append(msg, &args);
		// FIXME: somebody with hardware should tune this
		int speed = high; // is_strong ? 200 : 150;
		int duration = vDuration;
		if (dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &speed)) {
			if (dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &duration)) {
				dbus_connection_send(system_bus, msg, NULL);
				//dbus_connection_flush(system_bus);
			}
		}
		dbus_message_unref(msg);
	}
}

void plat_minimize(void)
{
}

void plat_gvideo_close(void)
{
}

void plat_gvideo_open(int is_pal)
{
}
