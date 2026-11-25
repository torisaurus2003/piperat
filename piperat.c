/*INCLUDES*/

#include <pipewire/pipewire.h>	//For PipeWire stuff
#include <gtk/gtk.h>		//For GTK stuff
#include <gtk4-layer-shell.h>	//For layer shell stuff
#include <spa/param/props.h>	//For SPA POD
#include <spa/pod/parser.h>	//For parsing SPA POD structures
#include <spa/debug/pod.h>	//For debugging SPA POD structures
#include <spa/pod/builder.h>	//To build SPA POD structures
#include <math.h>		//For cubic root and third power operations
#include <dirent.h>		//To open/close the desktop file directory
#include <string.h>		//For strcmp
#include <limits.h>		//For PATH_MAX
#include <stdio.h>		//To open/close the desktop file
#include <stdlib.h>		//For memory management (malloc, free, etc...)

/*VARIABLES*/

//Configuration
typedef struct {
	int width;		//Window width
	int height;		//Window height
	bool has_position;	//Flag to check if there's x_pos or y_pos
	int spacing;		//Space between icon and label
	char *title;		//Window title
	bool set_as_layer;	//Flag to check the window as layer
	char *namespace;	//Layer namespace
	int x_pos;		//Horizontal window position
	int y_pos;		//Vertical window position
	double volume_step;	//Step size for the volume slider
	bool set_tooltip;	//Sets the tooltip
} Config;


//App prototype
typedef struct App App;

//Stream
typedef struct Stream {
	uint32_t id;			//Stream ID
	struct pw_node *node;		//Stream node
	struct spa_hook listener;	//Stream listener
	char *name;			//Stream name
	char *media_name;		//Stream media name
	char *icon;			//Stream icon
	char *binary;			//Stream binary name
	float volume;			//Stream volume
	GtkWidget *row;			//Stream row
	GtkWidget *slider;		//Stream slider
	GtkWidget *image;		//Stream image
	struct Stream *next;		//Next stream
	App *app;			//App structure
	bool updating_slider;		//Flag for updating slider
	uint32_t count;			//Stream channel count
} Stream;

//App
typedef struct App {
	struct pw_loop *loop;		//PipeWire loop
	struct pw_context *context;	//PipeWire context
	struct pw_core *core;		//PipeWire core
	struct pw_registry *registry;	//PipeWire registry
	struct spa_hook listener;	//PipeWire event listener
	GtkApplication *application;	//Application
	GtkWindow *window;		//Window
	GtkListBox *listbox;		//Window listbox
	Stream *streams;		//Stream list
	char *config_file;		//Configuration file path
	char *style_file;		//Style file path
	GtkCssProvider *css;		//CSS provider
	GFile *file;			//Style file object
	GFileMonitor *monitor;		//Style file monitor
	Config config;			//App configuration
	bool has_streams;		//Flag for streams
} App;

//GSource wrapping for PipeWire's loop
typedef struct {
	GSource base;
	struct pw_loop *loop;
} PipeWireSource;


/*FUNCTIONS*/

//Goes through the configuration file
static int load_config(const char *config_file, App *app) {
	
	//Open file
	FILE *f = fopen(config_file, "r");

	//If the file doesn't exist return the fail flag
	if (f == NULL) {
    		return 1;
	}
	
	//Line buffer
	char line[512];

	//Go through each line in the file and assign
	//its value (if there is one)
	while (fgets(line, sizeof(line), f) != NULL) {
	
		//Change the string terminator from new line to null
		line[strcspn(line, "\n")] = '\0';		
		
		//Check if we are on the width line
		if (strncmp(line, "width=", 6) == 0) {

			//Clean the line and store width as an int
			char *width = line + 6;
			app->config.width = atoi(width);

		}

		//Check if we are on the height line
		if (strncmp(line, "height=", 7) == 0) {

			//Clean the line and store height as an int
			char *height = line + 7;
			app->config.height = atoi(height);

		}

		//Check if we are on the label_spacing line
		if (strncmp(line, "spacing=", 8) == 0) {

			//Clean the line and pass ownership
			//of the string to app
			char *spacing = line + 8;
			app->config.spacing = atoi(spacing);

		}

		//Check if we are on the title line
		if (strncmp(line, "title=", 6) == 0) {

			//Clean the line and pass ownership
			//of the string to app
			char *title = line + 6;
			app->config.title = strdup(title);

		}

		//Check if we are on the set_as_layer line
		if (strncmp(line, "set_as_layer=", 13) == 0) {

			//Clean the line and pass ownership
			//of the string to app
			char *set_as_layer = line + 13;

			//Set the layer shell flag
			if (strcmp(set_as_layer, "true") == 0) {
				app->config.set_as_layer = true;
			}

			if (strcmp(set_as_layer, "false") == 0) {
				app->config.set_as_layer = false;
			}

		}

		//Check if we are on the namespace line
		if (strncmp(line, "namespace=", 10) == 0) {

			//Clean the line and pass ownership
			//of the string to app
			char *namespace = line + 10;
			app->config.namespace = strdup(namespace);

		}

		//Check if we are on the x_pos line
		if (strncmp(line, "x_pos=", 6) == 0) {

			//Clean the line and store x_pos as an int
			char *x_pos = line + 6;
			app->config.x_pos = atoi(x_pos);

			//Enable the has_position flag
			app->config.has_position = true;

		}

		//Check if we are on the y_pos line
		if (strncmp(line, "y_pos=", 6) == 0) {

			//Clean the line and store y_pos as an int
			char *y_pos = line + 6;
			app->config.y_pos = atoi(y_pos);

			//Enable the has_position flag
			app->config.has_position = true;

		}
		
		//Check if we are on the volume_step line
		if (strncmp(line, "volume_step=", 12) == 0) {

			//Clean the line and store width as an int
			char *volume_step = line + 12;
			app->config.volume_step = strtod(volume_step, NULL);

		}
	
		//Check if we are on the set_tooltip line
		if (strncmp(line, "set_tooltip=", 12) == 0) {

			//Clean the line and pass ownership
			//of the string to app
			char *set_tooltip = line + 12;

			//Set the tooltip flag
			if (strcmp(set_tooltip, "true") == 0) {
				app->config.set_tooltip = true;
			}

			if (strcmp(set_tooltip, "false") == 0) {
				app->config.set_tooltip = false;
			}

		}

	}
	
	//Close the file and return the success flag
	fclose(f);
	return 0;

}

//Gets the path to a desktop file using a binary name
static char *get_desktop_file(const char *binary) {

	//If binary is NULL: return NULL
	if (binary == NULL) {
		return NULL;
	}
	
	//Home folder
	const char *home = getenv("HOME");
	
	//Local applications folder
	char applications[PATH_MAX];
	snprintf(applications, PATH_MAX, "%s/.local/share/applications", home);

	//Character array of paths to search in
	const char *paths[] = {
		applications,
		"/usr/share/applications/"
	};

	//Number of elements in paths
	int count = sizeof(paths) / sizeof(paths[0]);

	//Iterates over the files in paths
	for (int i = 0; i < count; i++) {

		//Current directory
		const char *path = paths[i];

		//Open the directory
		DIR *dir = opendir(path);

		//If dir is NULL: search in the next path
		if (dir == NULL) {
    			continue;
		}

		//Directory entry
		struct dirent *entry;

		//Iterates over each entry in the directory
		while ((entry = readdir(dir)) != NULL) {

			//Checks if the current entry's name
			//is equal to binary
			if (strstr(entry->d_name, binary)) {

				//Desktop file
				char *desktop_file = malloc(PATH_MAX);

				//Create path to the desktop file
				snprintf(desktop_file, PATH_MAX, "%s/%s", path, entry->d_name);

				//Close the directory
				closedir(dir);

				return desktop_file;

			}

		}

		//If nothing is found, close the directory
		closedir(dir);

	}

	//If nothing is found, return null
	return NULL;

}

//Gets icon name from a desktop file
static char *get_icon(const char *desktop_file) {

	//Fallback when no icon is found
	const char *fallback = "audio-x-generic";

	//If desktop_file is empty: return the fallback
	if (desktop_file == NULL) {
    		return strdup(fallback);
	}

	//Open the desktop file
	FILE *f = fopen(desktop_file, "r");

	//If f is NULL: return the fallback
	if (f == NULL) {
    		return strdup(fallback);
	}

	//Line
	char line[512];

	//Iterates over each line of f
	while (fgets(line, sizeof(line), f) != NULL) {

		//Checks if line starts with 'Icon='
		if (strncmp(line, "Icon=", 5) == 0) {
			
			//Truncates the first five characters of line
			char *icon = line + 5;

			//Changes the new line character in line for
			//a string terminator
			icon[strcspn(icon, "\n")] = '\0';

			//Icon name
			char *result = strdup(icon);

			//Close the desktop file
			fclose(f);

			return result;

		}

	}

	//If nothing is found, close the desktop file
	//and return the fallback
	fclose(f);
	return strdup(fallback);

}

//GLib callback to read PipeWire's file descriptor
static gboolean pipewire_source_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {

	//Unused arguments
	(void)callback;
	(void)user_data;

	//GSource that contains PipeWire's loop
    	PipeWireSource *pw_src = (PipeWireSource *) source;

	//Process PipeWire's events
    	pw_loop_iterate(pw_src->loop, 0);

    	return G_SOURCE_CONTINUE;

}

//Creates a Glib source for the PipeWire loop
static GSource *create_pipewire_source(struct pw_loop *loop) {

	//Assigns the pipewire_source_dispatch callback
	//as the PipeWire source function
	static GSourceFuncs pipewire_source_funcs = {
		.dispatch = pipewire_source_dispatch
	};

	//PipeWire source
	PipeWireSource *pw_src = (PipeWireSource *) g_source_new(&pipewire_source_funcs, sizeof(PipeWireSource));

	//Pipewire loop
	pw_src->loop = loop;

	//For GLib to monitor PipeWire's incoming data
	g_source_add_unix_fd(&pw_src->base, pw_loop_get_fd(loop), G_IO_IN | G_IO_ERR);

	return (GSource *) pw_src;

}

//Changes stream volume
void stream_set_volume(Stream *stream, float linear_volume) {

	//Buffer
	uint8_t buffer[1024];
	struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

	//Stream volumes array
	float volumes[stream->count];

	//Fills the values in volumes
	for (uint32_t i = 0; i < stream->count; i++) {
    		volumes[i] = linear_volume;
	}

	//spa_pod that contains the volume array
	const struct spa_pod *param = spa_pod_builder_add_object(&b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_channelVolumes, SPA_POD_Array(sizeof(float), SPA_TYPE_Float, stream->count, volumes));

	//Sets the channelVolumes parameter
	//in the stream node with the param spa_pod
	pw_node_set_param(stream->node, SPA_PARAM_Props, 0, param);

}

//Changes volume when a slider is moved
static void on_slider_changed(GtkRange *range, gpointer user_data) {

	//Stream
    	Stream *stream = user_data;

	//Slider volume
    	float volume = gtk_range_get_value(range);

	//If the slider is being updated, return
	if (stream->updating_slider)
    		return;

	//Convert the slider value into linear volume
    	float linear_volume = pow(volume, 3);

	//Change the stream volume
    	stream_set_volume(stream, linear_volume);

}

//Key handler
gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data) {

	//Unused arguments
	(void)controller;
	(void)keycode;
	(void)state;

	//Local variables
	App *app = user_data;							//App
	GtkListBoxRow *row = gtk_list_box_get_selected_row(app->listbox);	//Current row
	Stream *stream = g_object_get_data(G_OBJECT(row), "stream");		//Row stream
	double value = gtk_range_get_value(GTK_RANGE(stream->slider));		//Stream slider

	//If the right key is pressed
	if (keyval == GDK_KEY_Right) {

		//Increase the slider volume
    		gtk_range_set_value(GTK_RANGE(stream->slider), value + app->config.volume_step);
		
		return TRUE;
	
	}
	
	//If the left key is pressed
	if (keyval == GDK_KEY_Left) {

		//Decrease the slider volume
    		gtk_range_set_value(GTK_RANGE(stream->slider), value - app->config.volume_step);
		
		return TRUE;

	}

	//If the up key is pressed
	if (keyval == GDK_KEY_Up) {

		//Get the previous row
        	GtkListBoxRow *prev = gtk_list_box_get_row_at_index(app->listbox, gtk_list_box_row_get_index(row) - 1);

		//Check if prev exists
		if (prev) {

			//Select the previous row
            		gtk_list_box_select_row(app->listbox, prev);

    		}

    		return TRUE;

	}
	
	//If the down key is pressed
	if (keyval == GDK_KEY_Down) {

		//Get the next row
        	GtkListBoxRow *next = gtk_list_box_get_row_at_index(app->listbox, gtk_list_box_row_get_index(row) + 1);

		//Check if next exists
        	if (next) {

			//Select the next row
            		gtk_list_box_select_row(app->listbox, next);

    		}

    		return TRUE;

	}
	//If the escape is pressed
	if (keyval == GDK_KEY_Escape) {

		//Destroy the window
		gtk_window_destroy(app->window);

		return TRUE;

	}

	return FALSE;

}

//CSS monitor callback
static void css_changed(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event, gpointer user_data) {

	//Unused arguments
	(void)monitor;
	(void)other_file;

	//Store the app structure
	App *app = user_data;

	//Reload the style file 
	if (event == G_FILE_MONITOR_EVENT_CHANGED || event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
	
		gtk_css_provider_load_from_path(app->css, g_file_peek_path(file));
	
	}

}

//Creates the window
void create_window(App *app) {

	//Local variables
	char *config;		//Config file path
	char *style;		//Style file path
	
	//Check if a config file path was set by the user
	if (app->config_file) {
	
		//if so, pass the ownership of the path
		//to config and destroy the old pointer
		config = app->config_file;
		app->config_file = NULL;
	
	}
	else {

		//If the user didn't set one, use the default path
		config = g_build_filename(g_get_user_config_dir(), "piperat", "config", NULL);

	}

	//Check if the config file exists
	if (! g_file_test(config, G_FILE_TEST_EXISTS)) {
		printf("config file not found at %s\n", config);
	}
	
	//Check if the config file exists
	if (g_file_test(config, G_FILE_TEST_EXISTS)) {	


		//Load the config file and check the return
		//status in case something goes wrong
		if (load_config(config, app) != 0) {
			exit(1);
		}

	}

	//check if a style file was set by the user
	if (app->style_file) {

		//if so, pass the ownership to style
		//and destroy the pointer variable
		style = app->style_file;
		app->style_file = NULL;
	
	}
	else {

		//If the user didn't set one, use the default path
		style = g_build_filename(g_get_user_config_dir(), "piperat", "style.css", NULL);
	
	}

	//Creates window
	app->window = GTK_WINDOW(gtk_application_window_new(app->application));

	//Check if the style file is there
	if (g_file_test(style, G_FILE_TEST_EXISTS)) {

		//Default CSS style provider
		app->css = gtk_css_provider_new();
	
		//Load the style.css for the provider
		gtk_css_provider_load_from_path(app->css, style);

		//Add style provider for display (whatever that means)
		gtk_style_context_add_provider_for_display(gtk_widget_get_display(GTK_WIDGET(app->window)), GTK_STYLE_PROVIDER(app->css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		
		//CSS file
		app->file = g_file_new_for_path(style);

		//Create monitor for CSS file
		app->monitor = g_file_monitor_file(app->file, G_FILE_MONITOR_NONE, NULL, NULL);

		//Connect the monitor to callback
		g_signal_connect(app->monitor, "changed", G_CALLBACK(css_changed), app);

	}
	else {
		printf("style file not found at %s\n", style);
	}

	//Window title
	gtk_window_set_title(app->window, app->config.title);
	gtk_widget_add_css_class(GTK_WIDGET(app->window), "window");

	//Window size
	gtk_window_set_default_size(app->window, app->config.width, app->config.height);

	//Check if the window should follow the layer shell protocol
	if (app->config.set_as_layer) {	
	
		//Set window as layer shell
		gtk_layer_init_for_window(app->window);

		//Set the window's namespace
		gtk_layer_set_namespace(app->window, app->config.namespace);
	
		//Set the window as overlay	
		gtk_layer_set_layer(app->window, GTK_LAYER_SHELL_LAYER_OVERLAY);

		//Focus the keyboard on the window
		gtk_layer_set_keyboard_mode(app->window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
	
		//Window position
		if (app->config.has_position) {
			//Set the anchor point
			gtk_layer_set_anchor(app->window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
			gtk_layer_set_anchor(app->window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);	

			//Set the position
			gtk_layer_set_margin(app->window, GTK_LAYER_SHELL_EDGE_TOP, app->config.y_pos);
			gtk_layer_set_margin(app->window, GTK_LAYER_SHELL_EDGE_LEFT, app->config.x_pos);
		}
	
	}
	
	//Listbox for streams
	app->listbox = GTK_LIST_BOX(gtk_list_box_new());
	gtk_widget_set_hexpand(GTK_WIDGET(app->listbox), TRUE);
	gtk_window_set_child(app->window, GTK_WIDGET(app->listbox));
	gtk_widget_add_css_class(GTK_WIDGET(app->listbox), "listbox");

	//Create the key controller
	GtkEventController *controller = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(controller), GTK_PHASE_CAPTURE);

	//Connect the controller to the key handler
	g_signal_connect(controller,"key-pressed", G_CALLBACK(on_key_pressed), app);
	
	//Add the controller to the window
	gtk_widget_add_controller(GTK_WIDGET(app->window), controller);

	//Cleanup
	if (config) {
		g_free(config);
	}

	if (style) {
		g_free(style);
	}

}

//Creates a row to put inside the window
void create_row(App *app, Stream *stream) {

	//Box to store the widgets
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, app->config.spacing);

	//Stream icon
	GtkWidget *icon = gtk_image_new_from_icon_name(stream->icon);
	gtk_widget_add_css_class(icon, "icon");


	//Check if the tooltip was set
	if (app->config.set_tooltip) {

		//Stream tooltip
		char tooltip[256];
		snprintf(tooltip, sizeof(tooltip), "ID: %u\nApplication: %s\nName: %s", stream->id, stream->name, stream->media_name);
		gtk_widget_set_tooltip_text(box, tooltip);

	}
	
	//Stream slider
	GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, app->config.volume_step);
	gtk_widget_set_hexpand(slider, TRUE);
	g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_changed), stream);
	gtk_widget_add_css_class(slider, "slider");

	//Appending to box
	gtk_box_append(GTK_BOX(box), icon);
	gtk_box_append(GTK_BOX(box), slider);

	//Create a row
	GtkWidget *row = gtk_list_box_row_new();
	gtk_widget_add_css_class(row, "row");

	//Set the box as the row's child
	gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
	
	//Store things in the stream
	stream->image = icon;
	stream->slider = slider;
	stream->row = row;

	//Store the stream in the row
	g_object_set_data(G_OBJECT(row), "stream", stream);

	//Append the row to listbox
	gtk_list_box_append(app->listbox, row);
	
	//Check if a row wasn't selected
	if (gtk_list_box_get_selected_row(app->listbox) == NULL) {

		//First row
    		GtkListBoxRow *first = gtk_list_box_get_row_at_index(app->listbox, 0);

		//Checks if the first row exists
    		if (first) {

			//Select the first row
        		gtk_list_box_select_row(app->listbox, first);

		}

	}

}

//Gets the linear volume from the stream node
static void node_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param) {

	//Unused arguments
	(void)seq;
	(void)id;
	(void)index;
	(void)next;

	//Local variables
	struct spa_pod_object *obj = (struct spa_pod_object *)param;
	struct spa_pod_prop *prop;
	Stream *stream = data;

	//Iterates over each key in params.Props
	SPA_POD_OBJECT_FOREACH(obj, prop) {

		//Compares the key values
    		switch (prop->key) {

			//In the case the key is channelVolumes
			case SPA_PROP_channelVolumes: {

				//Volumes array
    				struct spa_pod_array *arr = (struct spa_pod_array *)&prop->value;

				//Number of elements in arr
				uint32_t n = SPA_POD_ARRAY_N_VALUES(arr);

				//Volume values in arr
    				float *values = SPA_POD_ARRAY_VALUES(arr);

				//Linear volume
				float linear_volume = values[0];

				//Storing
				stream->count = n;
				stream->volume = cbrt(linear_volume);

				//Checks if there's a slider
				if (stream->slider) {

					//Update the slider
					stream->updating_slider = true;
    					gtk_range_set_value(GTK_RANGE(stream->slider), stream->volume);
					stream->updating_slider = false;

				}

				//Break the iteration
    				break;

			}

    		}

	}

}

//Gets information from the stream node
static void node_info(void *data, const struct pw_node_info *info) {

	//Stream
	Stream *stream = data;

	//Stream name
	const char *name = spa_dict_lookup(info->props, "application.name");

	//Frees name
	if (name) {
		free(stream->name);
		stream->name = strdup(name);
	}

	//Media name
	const char *media_name = spa_dict_lookup(info->props, "media.name");
	
	//Frees media name
	if (media_name) {
		free(stream->media_name);
		stream->media_name = strdup(media_name);
	}

	//Stream icon
	const char *icon = spa_dict_lookup(info->props, "application.icon-name");

	//Stream binary
	const char *binary = spa_dict_lookup(info->props, "application.process.binary");

	//If the icon is NULL, searches for one
	if (icon == NULL) {
    		char *desktop_file=get_desktop_file(binary);
		icon=get_icon(desktop_file);
		free(desktop_file);
	}

	//Frees icon
	free(stream->icon);
	stream->icon = strdup(icon);

	//Frees binary
	if (binary) {
		free(stream->binary);
		stream->binary = strdup(binary);
	}

	//Call the SPA_PARAM_Props object, which calls node_param
	pw_node_enum_params(stream->node, 0, SPA_PARAM_Props, 0, UINT32_MAX, NULL);
	
	//Check if the stream already has a row
	//if not, call create_row()	
	if (!stream->row) {
        	create_row(stream->app, stream);
	}

}

//Callback for removing streams
static void registry_global_remove(void *data, uint32_t id) {

	//App
	App *app = data;

	//Pointer to stream list
	Stream **current = &app->streams;

	//Loops over stream list
	while (*current) {

		//Get the stream current points to
		Stream *stream = *current;

		//Checks if stream id is equal to the removed id
		if (stream->id == id) {

			//Make the pointer of the current stream
			//point to the next stream
			*current = stream->next;

			//If there's a listbox or row for the removed id, remove them
			if (stream->row)
                		gtk_list_box_remove(app->listbox, stream->row);

			//Free pointers
			free(stream->name);
			free(stream->icon);
			free(stream->binary);

			//If there's a node, unlink it
			if (stream->node)
                		pw_proxy_destroy((struct pw_proxy *)stream->node);

			//Free stream
			free(stream);

			//Break loop
			break;

		}
		
		//If the id doesn't match, move current to
		//the pointer of the next stream
    		current = &stream->next;

	}
		
	//If the streams are gone, close the window
	if (app->streams == NULL && app->window != NULL) {
		gtk_window_close(app->window);
	}

}

//Acts as a callback for changes in the PipeWire registry
static void registry_global(void *_data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props) {
	
	(void)permissions;

	//Local variables
	const char *media_class;
	App *app = _data;

	//Gets the media.class attribute of the node
	media_class = spa_dict_lookup(props, "media.class");
	
	//Checks if the interface is a node, if it has a media.class and if the media class is Stream/Output/Audio
	if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0 && media_class && strcmp(media_class, "Stream/Output/Audio") == 0) {
		
		/*if (!app->has_streams) {
			app->has_streams = true;
		}*/
			
		//Preallocates the stream object
		Stream *stream = calloc(1, sizeof(Stream));

		//Stream ID
		stream->id = id;

		//Stream node
		stream->node = pw_registry_bind(app->registry, stream->id, PW_TYPE_INTERFACE_Node, version, 0);

		stream->app = app;

		//Stores node_info and node_param as a callbaks for changes in the node
		static const struct pw_node_events node_events = {
			PW_VERSION_NODE_EVENTS,
			.info = node_info,
			.param = node_param
		};
		
		//Starts to listen for node changes
		pw_node_add_listener(stream->node, &stream->listener, &node_events, stream);

		stream->next = app->streams;
		app->streams = stream;

	}
	
	/*if (!app->has_streams) {
		gtk_window_destroy(app->window);
	}*/

}

//Cleanup
void cleanup(App *app) {

	//Stream
	Stream *stream = app->streams;

	//Loops over each stream
    	while (stream) {

		//Next stream
        	Stream *next = stream->next;
		
		//Unlink listener
        	if (stream->listener.link.next != NULL)
            		spa_hook_remove(&stream->listener);

		//Unlink node
        	if (stream->node)
            		pw_proxy_destroy((struct pw_proxy *)stream->node);

		//Free storage
        	free(stream->name);
        	free(stream->media_name);
        	free(stream->icon);
        	free(stream->binary);

		//Free stream
        	free(stream);

		//Move to next stream
        	stream = next;
    	}

	//Unlink registry
    	if (app->registry)
        	pw_proxy_destroy((struct pw_proxy *)app->registry);

	//Disconnect core
    	if (app->core)
        	pw_core_disconnect(app->core);

	//Destroy context
    	if (app->context)
        	pw_context_destroy(app->context);

	//Destroy app loop
    	if (app->loop)
        	pw_loop_destroy(app->loop);
	
	if (app->css) {
		g_object_unref(app->css);
	}

	if (app->file) {
		g_object_unref(app->file);
	}

	if (app->monitor) {
		g_object_unref(app->monitor);
	}
	
	free(app->config.title);
	free(app->config.namespace);

}

//Commandline callback
static void on_command_line(GApplication *application, GApplicationCommandLine *cmdline, gpointer user_data) {
	
	//App
	App *app = user_data;

	//Executable options
	GVariantDict *options = g_application_command_line_get_options_dict(cmdline);
	
	//Config file
	char *config_file;
	
	//Style file
	char *style_file;

	//Check if there's a config file
	//in the options and store it
    	if (g_variant_dict_lookup(options, "config", "&s", &config_file)) {
		app->config_file = g_strdup(config_file);
	}
	
	//Check if there's a style file
	//in the options and store it
    	if (g_variant_dict_lookup(options, "style", "&s", &style_file)) {
		app->style_file = g_strdup(style_file);
	}

	//Activate the application
	g_application_activate(application);

}

//Checks if there are any streams
bool has_streams(void) {

	//Runs pw-dump and saves the output
	FILE *output = popen("pw-dump", "r");

	//If the output is invalid, return false
	if (!output) {
        	return FALSE;
	}

    	//Output line
    	char line[4096];

	//Flag to check if a stream was found
	bool stream = FALSE;

	//Iterate over the lines of the output and
	//search for any containing Stream/Output/Audio
	while (fgets(line, sizeof(line), output)) {

		if (strstr(line, "Stream/Output/Audio")) {
			stream = TRUE;
			break;
		}
	
	}
	
	//Close the output	
	pclose(output);	
	
	//Return the stream status
	return stream;

}

//Activates the app
void activate(GtkApplication *application, gpointer user_data) {

	//Assign arguments to app structure
	App *app = user_data;
	app->application = application;
	
	if (has_streams()) {

		//Create window
		create_window(app);
	
		//Present window
		gtk_window_present(app->window);

	}

}

//Main loop
int main(int argc, char *argv[]) {

	//Initial app structure
	App app = {0};

	//Default app configuration
	app.config.width = 170;
	app.config.height = 1;
	app.config.has_position = false;
	app.config.spacing = 0;
	app.config.title = strdup("piperat");	
	app.config.set_as_layer = true;
	app.config.namespace = strdup("piperat");
	app.config.volume_step = 0.05;
	app.config.set_tooltip = true;
	app.has_streams = false;

	//Initializes the PipeWire library
	pw_init(&argc, &argv);

	//Creates an event loop for PipeWire
	app.loop =  pw_loop_new(NULL);

	//Creates a context for the application's state
	app.context = pw_context_new(app.loop, NULL, 0);

	//Connection with PipeWire's server
	app.core = pw_context_connect(app.context, NULL, 0);

	//PipeWire registry
	app.registry = pw_core_get_registry(app.core, PW_VERSION_REGISTRY, 0);

	//Stores registry_global and registry_global_remove
	//as a callbacks for changes in registry
	static const struct pw_registry_events registry_events = {
		PW_VERSION_REGISTRY_EVENTS,
		.global = registry_global,
		.global_remove = registry_global_remove
	};

	//Starts to listen for changes in registry
	pw_registry_add_listener(app.registry, &app.listener, &registry_events, &app);

	//Creates GSource wrapper for the PipeWire loop
	GSource *pw_source = create_pipewire_source(app.loop);
	g_source_attach(pw_source, NULL);

	//Create application
	app.application = gtk_application_new("com.torisaurus.piperat", G_APPLICATION_HANDLES_COMMAND_LINE);

	//Executable options
	g_application_add_main_option(G_APPLICATION(app.application), "config", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, "Path to configuration file", "FILE");
	g_application_add_main_option(G_APPLICATION(app.application), "style", 's', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, "Path to style file", "FILE");

	//Connect application to commandline callback
	g_signal_connect(app.application, "command-line", G_CALLBACK(on_command_line), &app);

	//Connect application to activate
	g_signal_connect(app.application, "activate", G_CALLBACK(activate), &app);

	//Initialize application
    	int status = g_application_run(G_APPLICATION(app.application), argc, argv);

	//Destroy the PipeWire source and unreference it
	g_source_destroy(pw_source);
	g_source_unref(pw_source);

	//Cleanup
	cleanup(&app);

	//Unreference application
	g_object_unref(app.application);

	//Return exit code from status
	return status;

}
