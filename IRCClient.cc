#include <stdio.h>
#include <gtk/gtk.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

GtkListStore * list_rooms;
GtkListStore * list_users;
GtkListStore * list_messages;
GtkWidget *tree_view;
GtkTreeSelection *selection;

const char * host = "localhost";
const char * user = (const char *) malloc (20 * sizeof(char));
char * password = (char *) malloc (20 * sizeof(char));
char * message = (char *) malloc (300 * sizeof(char));
char * tmessage = (char *) malloc (200 * sizeof(char));
char * room = (char *) malloc (50 * sizeof(char));
char * sport = (char *) malloc (20 * sizeof(char));
char * args = (char *) malloc (300 * sizeof(char));
int port ;
int roomCount = 0;
int user_exist = 0;
int room_exist = 0;
int messages_exist = 0;
char * pre_value = NULL;
int lastMsgNum = 0;

int open_client_socket(const char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}

#define MAX_RESPONSE (10 * 1024)
int sendCommand(const char * host, int port, const char * command, const char * user,
		char * password, const char * args, char * response) {
	int sock = open_client_socket( host, port);

	// Send command
	write(sock, command, strlen(command));
	write(sock, " ", 1);
	write(sock, user, strlen(user));
	write(sock, " ", 1);
	write(sock, password, strlen(password));
	write(sock, " ", 1);
	write(sock, args, strlen(args));
	write(sock, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}

	printf("response:%s\n", response);

	close(sock);
	return 1;
}

void send_message() {
	char response[ MAX_RESPONSE ] = {0};
	response[0] = '\0';
	args = strdup(room);
	strcat(args, " ");
	strcat(args, message);
	sendCommand(host, port, "SEND-MESSAGE", user, password, args, response);
	if (!strcmp(response,"OK\r\n")) {
		//messages_exist = 1;
		printf("Message %s sent\n", args);
	}
}

void update_list_rooms() {
	GtkTreeIter iter;
	int i;
	char * temp = (char *) malloc (30 * sizeof(char));
	gchar *msg;

	// Clear rooms list
	gtk_list_store_clear(GTK_LIST_STORE (list_rooms));

	/* Add some messages to the window */
	if (user_exist == 1) {
		char responserooms[ MAX_RESPONSE ] = {0};
		responserooms[0] = '\0';
		sendCommand(host, port, "LIST-ROOMS", user, password, "default", responserooms);
		printf("response: %s\n", responserooms);

		temp = strtok(responserooms, "\r\n");
		if(temp != NULL){
			msg = g_strdup_printf ("%s", temp);
    			gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
    			gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, msg, -1);
			roomCount++;
		}
		
		while ((temp = strtok(NULL, "\r\n")) != NULL) {

			msg = g_strdup_printf ("%s", temp);
			printf("ROOMS %s\n", msg);	        
			gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
	        	gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, msg, -1);
			roomCount++;
		}

	}
}

void update_list_users() {
	GtkTreeIter iterator;
	int i;
	char * temp = (char *) malloc (30 * sizeof(char));
	gchar *msg;

	//Add some messages to the window
	if (user_exist == 1) {    	
		char responseuser[ MAX_RESPONSE ] = {0};
		responseuser[0] = '\0';

		if (room != NULL) {
			sendCommand(host, port, "GET-USERS-IN-ROOM", user, password, room, responseuser);
			temp = strdup(responseuser);
			temp = strtok(temp, "\r\n");
			if( temp != NULL){
				msg = g_strdup_printf ("%s", temp);
    				gtk_list_store_append (GTK_LIST_STORE (list_users), &iterator);
    				gtk_list_store_set (GTK_LIST_STORE (list_users), &iterator, 0, msg, -1);
			}

			while ((temp = strtok(NULL, "\r\n")) != NULL) {
				msg = g_strdup_printf ("%s", temp);
	       			gtk_list_store_append (GTK_LIST_STORE (list_users), &iterator);
	        		gtk_list_store_set (GTK_LIST_STORE (list_users), &iterator, 0, msg, -1);
			}
		}

	}
}

static void update_list_messages() {
	GtkTreeIter iterators;
	int i;
	char * buffer = (char *) malloc (300 * sizeof(char));
	gchar *msg;

	//Add some messages to the window
	if (messages_exist == 1) {    
		char responsemessage[ MAX_RESPONSE ] = {0};
		responsemessage[0] = '\0';
		sprintf(args,"%d %s", lastMsgNum, room);
		sendCommand(host, port, "GET-MESSAGES", user, password, args, responsemessage);
		buffer = strdup(responsemessage);
		buffer = strtok(buffer, "\r\n");
		if(buffer != NULL){
			msg = g_strdup_printf ("%s", buffer);
			gtk_list_store_append (GTK_LIST_STORE (list_messages), &iterators);
			gtk_list_store_set (GTK_LIST_STORE (list_messages), &iterators, 0, msg, -1);
		}

		while ((buffer = strtok(NULL, "\r\n")) != NULL) {
			msg = g_strdup_printf ("%s", buffer);
			gtk_list_store_append (GTK_LIST_STORE (list_messages), &iterators);
			gtk_list_store_set (GTK_LIST_STORE (list_messages), &iterators, 0, msg, -1);
		}

	}
//	free (buffer);
}

static void get_messages() {
	gtk_list_store_clear(GTK_LIST_STORE(list_messages));
	messages_exist = 1;
	update_list_messages();
}

void create_room() {
	char response[ MAX_RESPONSE ] = {0};
	response[0] = '\0';
	sendCommand(host, port, "CREATE-ROOM", user, password, room, response);
	if (!strcmp(response,"DENIED\r\n")) {
		printf("Room couldn't be added because password error\n");
		room_exist = 1;
		return;	
	} else if (!strcmp(response, "OK\r\n")) {
		room_exist = 1;
		printf("Room %s added\n", room);
		user_exist = 1;
		gtk_list_store_clear(GTK_LIST_STORE(list_rooms));
		update_list_rooms();
	}
}

void enter_room() {
	char response[ MAX_RESPONSE ] = {0};
	response[0] = '\0';
	sendCommand(host, port, "ENTER-ROOM", user, password, room, response);
	if (!strcmp(response, "OK\r\n")) {
		sprintf(message, "%s %s",  "entered room:", room);
		send_message();
		update_list_rooms();
		update_list_users();
//		get_messages();
		printf("User %s added\n", user);
	}
}

void leave_room(char * troom) {
	char response[ MAX_RESPONSE ] = {0};
	response[0] = '\0';
	strcpy(args, troom);

	sprintf(message, "%s %s", "leaves room:", room);
	send_message();

	sendCommand(host, port, "LEAVE-ROOM", user, password, args, response);
	if (!strcmp(response,"OK\r\n")) {
		update_list_rooms();
		printf("User %s removed\n", user);
	}
}

void add_user() {
	// Try first to add user in case it does not exist.
	char responseadduser[ MAX_RESPONSE ] = {0};
	responseadduser[0] = '\0';
	sendCommand(host, port, "ADD-USER", user, password, "default", responseadduser);
	if (!strcmp(responseadduser, "OK\r\n")) {
		printf("User %s added\n", user);
		user_exist = 1;
		update_list_rooms();
	} else if (!strcmp(responseadduser, "DENIED\r\n")) {
		printf("User %s logged in\n", user);
		user_exist = 1;
	}
}

/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model ) {
	GtkWidget *scrolled_window;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;

	int i;

	/* Create a new scrolled window, with scrollbars only if needed */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	tree_view = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
	gtk_widget_show (tree_view);

	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (titleColumn, cell, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), GTK_TREE_VIEW_COLUMN (column));

	return scrolled_window;
}

static void enter_user(GtkWidget *entry) {
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
	user = strdup(entry_text);	
	printf("Entry contents: %s\n", entry_text);
}

static void enter_password(GtkWidget *entry) {
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
	password = strdup(entry_text);
	printf("Entry contents: %s\n", entry_text);
}

static void enter_room_name(GtkWidget *entry) {
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
	printf("Entry contents: %s\n", entry_text);
	room = strdup(entry_text);	
	create_room();	
}

static void click_changed(GtkWidget * widget, GtkWidget * Widgetone) {
	//get the new room name and leave user from previous room
	GtkTreeIter iter;
 	GtkTreeModel *model;
	char * value;
	
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), &model, &iter)) {
	        gtk_tree_model_get(model, &iter, 0, &value,  -1);
		printf("%s\n", value);
		room = strdup(value);

		if (pre_value == NULL) {
			printf("pre_value is null\n");
			enter_room();
		} else {
			printf("pre_room = %s\n", pre_value);
			leave_room(pre_value);
			enter_room();
			gtk_list_store_clear(GTK_LIST_STORE(list_users));
			update_list_users();
		}

		messages_exist = 1;
		pre_value = strdup(value);
 	}
}

static void message_callback(GtkWidget *entry) {
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
	printf("Entry contents: %s\n", entry_text);
	message = strdup(entry_text);
	messages_exist = 1;
	send_message();
	gtk_entry_set_text (GTK_ENTRY (entry), "");
	get_messages();
}

static gboolean time_handler(GtkWidget *widget) {
	gtk_list_store_clear(GTK_LIST_STORE(list_rooms));
	update_list_rooms();
	if(strlen(user) != 0 && strlen(room) != 0){
		gtk_list_store_clear(GTK_LIST_STORE(list_users));
		update_list_users();
		get_messages();
	}
	return TRUE;
}

/*Callback function in which reacts to the "clicked" signal*/
static void show_create_account_dialog (GtkButton *button, GtkWindow *window) {
	GtkWidget *dialog;
	GtkWidget *table;  
	GtkWidget *user_name;
	GtkWidget *user_password;
	GtkWidget *name_label;
	GtkWidget *password_label;
	gint result;  

	/*Create the dialog window. Modal windows prevent interaction with other 
		windows in the same application*/
	dialog = gtk_dialog_new_with_buttons ("Input Account Information", 
						window, GTK_DIALOG_MODAL,  
						GTK_STOCK_OK,GTK_RESPONSE_OK,  
						GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL, NULL);  

	gtk_dialog_set_default_response(GTK_DIALOG(dialog),GTK_RESPONSE_OK); 
	name_label = gtk_label_new("User Name");
	password_label = gtk_label_new("User Password");
	user_name = gtk_entry_new();
	user_password = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (user_name), 50);
	gtk_entry_set_max_length (GTK_ENTRY (user_password), 50);
	gtk_entry_set_visibility (GTK_ENTRY (user_password), FALSE);

	table = gtk_table_new(2,2,FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table),name_label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table),user_name,1,2,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table),password_label,0,1,1,2);
	gtk_table_attach_defaults(GTK_TABLE(table),user_password,1,2,1,2);

	gtk_table_set_row_spacings(GTK_TABLE(table),5);  
	gtk_table_set_col_spacings(GTK_TABLE(table),5);  
	gtk_container_set_border_width(GTK_CONTAINER(table),5);  
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),table);  
	gtk_widget_show_all(dialog);  

	result = gtk_dialog_run(GTK_DIALOG(dialog));  
 
	switch(result){  
		case GTK_RESPONSE_OK:  
			enter_user(user_name); 
			enter_password(user_password);
			add_user();
			break;  

		case GTK_RESPONSE_CANCEL:  
			g_print("cancel is press!\n");  
			break;  

		case GTK_RESPONSE_CLOSE:  
			break;  

		default:  
			g_print("something wrong!\n");  
 
		break;  
	}  
 
	gtk_widget_destroy(dialog);  
}

/*Callback function in which reacts to the "clicked" signal*/
static void show_create_room_dialog (GtkButton *button, GtkWindow *window) {
	GtkWidget *dialog;
	GtkWidget *table;  
	GtkWidget *room_name;
	GtkWidget *room_label;
	gint result;  

	/*Create the dialog window. Modal windows prevent interaction with other 
		windows in the same application*/
	dialog = gtk_dialog_new_with_buttons ("Input Room Name", 
                                    window, GTK_DIALOG_MODAL,  
					 GTK_STOCK_OK,GTK_RESPONSE_OK,  
                                     GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL, NULL);  

	gtk_dialog_set_default_response(GTK_DIALOG(dialog),GTK_RESPONSE_OK); 
	room_label = gtk_label_new("Room Name");
	room_name = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY (room_name), 50);

	table = gtk_table_new(1,2,FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table),room_label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table),room_name,1,2,0,1);

	gtk_table_set_row_spacings(GTK_TABLE(table),5);  
	gtk_table_set_col_spacings(GTK_TABLE(table),5);  
	gtk_container_set_border_width(GTK_CONTAINER(table),5);  
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),table);  
	gtk_widget_show_all(dialog);  

	result = gtk_dialog_run(GTK_DIALOG(dialog));  
 
	switch(result){  
		case GTK_RESPONSE_OK:  
			enter_room_name(room_name);
			break;  

		case GTK_RESPONSE_CANCEL:  
			g_print("cancel is press!\n");  
			break;  

		case GTK_RESPONSE_CLOSE:  
			break;  

		default:  
			g_print("something wrong!\n");  
			break;  
	}  

	gtk_widget_destroy(dialog);  
}


int main( int   argc, char *argv[] ) {
	GtkWidget *window;
	GtkWidget *list;
	GtkWidget *messages;
	GtkWidget *send_message;
	GtkWidget *label;

	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", "IRCClient <port>" );
		exit( -1 );
	}

	// Get the port from the arguments
	port = atoi (argv[1]);

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_title (GTK_WINDOW (window), "IRC Client");
	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	gtk_widget_set_size_request (GTK_WIDGET (window), 450, 400);

	// Create a table to place the widgets. Use a 8x6 Grid (8 rows x 6 columns)
	GtkWidget *table = gtk_table_new (8, 8, TRUE);
	gtk_container_add (GTK_CONTAINER (window), table);
	gtk_table_set_row_spacings(GTK_TABLE (table), 5);
	gtk_table_set_col_spacings(GTK_TABLE (table), 5);

	gtk_widget_show (table);

	// Add list of rooms. Use columns 0 to 3 (exclusive) and rows 0 to 2 (exclusive)
	list_rooms = gtk_list_store_new (1, G_TYPE_STRING);

	//update_list_rooms();
	list = create_list ("Rooms:", list_rooms);
	gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 4, 0, 2);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(click_changed), selection);

	gtk_widget_show (list);

	// Add list of users. Use columns 3 to 6 (exclusive) and rows 0 to 2 (exclusive)
	list_users = gtk_list_store_new (1, G_TYPE_STRING);
	//update_list_users();
	list = create_list ("Users:", list_users);
	gtk_table_attach_defaults (GTK_TABLE (table), list, 4, 8, 0, 2);
	gtk_widget_show (list);

	// Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
	list_messages = gtk_list_store_new (1, G_TYPE_STRING);
	list = create_list ("Messages:", list_messages);
	gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 8, 2, 5);
	gtk_widget_show (list);

	/* create a new label. */
	label = gtk_label_new ("Type a message:" );
	gtk_table_set_homogeneous(GTK_TABLE (table), TRUE);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 8, 5, 6);
	gtk_widget_show (label);

	// Add send button. Use columns 0 to 1 (exclusive) and rows 7 to 8 (exclusive)
	GtkWidget *send_button = gtk_button_new_with_label ("Send");
	gtk_table_attach_defaults(GTK_TABLE (table), send_button, 0, 2, 8, 9); 
	gtk_widget_show (send_button); 

	send_message = gtk_entry_new ();
	//gtk_entry_set_max_length (GTK_ENTRY (send_message), 300);
	gtk_widget_set_size_request(send_message, 80, 50);
	gtk_table_attach_defaults(GTK_TABLE (table), send_message, 0, 8, 6, 8);
	gtk_widget_show (send_message);	

	/*Connecting the clicked signal to the callback*/
	g_signal_connect_swapped (send_button, "clicked", G_CALLBACK (message_callback), send_message);

	// Add create account button. Use columns 3 to 4 (exclusive) and rows 7 to 8 (exclusive)
	GtkWidget *create_account_button = gtk_button_new_with_label ("Create/Login");
	gtk_table_attach_defaults(GTK_TABLE (table), create_account_button, 3, 5, 8, 9); 
	gtk_widget_show (create_account_button);

	/*Connecting the clicked signal to the callback*/ 
	g_signal_connect (GTK_BUTTON (create_account_button), 
		"clicked", G_CALLBACK (show_create_account_dialog), window);

	// Add create room button. Use columns 3 to 4 (exclusive) and rows 7 to 8 (exclusive)
	GtkWidget *create_room_button = gtk_button_new_with_label ("Create Room");
	gtk_table_attach_defaults(GTK_TABLE (table), create_room_button, 6, 8, 8, 9); 
	gtk_widget_show (create_room_button);

	/*Connecting the clicked signal to the callback*/
	g_signal_connect (GTK_BUTTON (create_room_button), 
		"clicked", G_CALLBACK (show_create_room_dialog), window);

	gtk_widget_show (table);
	gtk_widget_show (window);

	g_timeout_add(5000, (GSourceFunc) time_handler, (gpointer) window);

	gtk_main ();

	return 0;
}

