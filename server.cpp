#include <gtk/gtk.h>
#include <unistd.h>
#include <pthread.h>
#include <gdk/gdkkeysyms.h>
#include <iostream>
#include <math.h>
#include <list>
#include <vector>

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 


#include <errno.h>  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include "safeQueue.cpp"

//SafeQueue<string>* commandsFromClient;

#include <thread>         // std::thread


#define PORT 8088

#define TRUE   1  
#define FALSE  0  


#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400

using namespace std;

double degreeToRad(int degree){
    return (2*3.14159 * degree)/(360.0);
}


//the global pixmap that will serve as our buffer
static GdkPixmap *pixmap = NULL;

//Global variables are life
int boxWidth = 10;
int bulletSpeedMultiplier = 5;
#include "Tank.cpp"

bool aDown = false;
bool dDown = false;
bool wDown = false;
bool sDown = false;
bool spaceDown = false;

bool rightDown = false;
bool leftDown = false;
bool upDown = false;
bool downDown = false;
bool pDown = false;

GtkWidget *window = NULL;

int warmupCount = 0;



vector<Tank*> tanks;

class Bullet{
    public:
    double dx;
    double dy;
    double x;
    double y;
    int angle;
    Bullet(int tankIndexThatFired){
        this->angle = tanks[tankIndexThatFired]->rotationAngle;
        dx = -(bulletSpeedMultiplier * cos(degreeToRad(angle) + 3.14159/2));
        dy = -(bulletSpeedMultiplier * sin(degreeToRad(angle) + 3.14159/2));

        this->x = tanks[tankIndexThatFired]->x;
        this->y = tanks[tankIndexThatFired]->y;
    
    }
    void update(){
        x += dx;
        y += dy;
    }
};


list<Bullet*> bullets;


void fireBullet(int tankIndexThatFired){
    Bullet* b = new Bullet(tankIndexThatFired);
    bullets.push_back(b);
}

gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if(event->type == GDK_KEY_PRESS){
        switch (event->keyval)
        {
            case GDK_a:
                aDown = true; 
                break;
            case GDK_d:
                dDown = true; 
                break;

            case GDK_w:
                wDown = true; 
                break;
            case GDK_s:
                sDown = true; 
                break;
            case GDK_Up:
                upDown = true; 
                break;
            case GDK_Down:
                downDown = true; 
                break;
            case GDK_Left:
                leftDown = true; 
                break;
            case GDK_Right:
                rightDown = true; 
                break;
            case GDK_space:
                if(spaceDown) return FALSE;
                spaceDown = true;
                fireBullet(0);
                break;
            case GDK_p:
                if(pDown) return FALSE;
                pDown = true;
                fireBullet(1);
                break;
            default:
                return FALSE; 
        }
    }else if(event->type == GDK_KEY_RELEASE){
        switch (event->keyval)
        {
            case GDK_a:
                aDown = false; //Stop move left
                break;
            case GDK_d:
                dDown = false; //Stop move right
                break;
            case GDK_w:
                wDown = false; //Stop move left
                break;
            case GDK_s:
                sDown = false; //Stop move right
                break;
            case GDK_space:
                spaceDown = false;
                break;
            case GDK_p:
                pDown = false;
                break;

            case GDK_Up:
                upDown = false; 
                break;
            case GDK_Down:
                downDown = false; 
                break;
            case GDK_Left:
                leftDown = false; 
                break;
            case GDK_Right:
                rightDown = false; 
                break;

            default:
                return FALSE; 
        }
    }
   return FALSE; 
}

gboolean on_window_configure_event(GtkWidget * da, GdkEventConfigure * event, gpointer user_data){
    static int oldw = 0;
    static int oldh = 0;
    //make our selves a properly sized pixmap if our window has been resized
    if (oldw != event->width || oldh != event->height){
        //create our new pixmap with the correct size.
        GdkPixmap *tmppixmap = gdk_pixmap_new(da->window, event->width,  event->height, -1);
        //copy the contents of the old pixmap to the new pixmap.  This keeps ugly uninitialized
        //pixmaps from being painted upon resize
        int minw = oldw, minh = oldh;
        if( event->width < minw ){ minw =  event->width; }
        if( event->height < minh ){ minh =  event->height; }
        gdk_draw_drawable(tmppixmap, da->style->fg_gc[GTK_WIDGET_STATE(da)], pixmap, 0, 0, 0, 0, minw, minh);
        //we're done with our old pixmap, so we can get rid of it and replace it with our properly-sized one.
        g_object_unref(pixmap);
        pixmap = tmppixmap;
    }
    oldw = event->width;
    oldh = event->height;
    return TRUE;
}

gboolean on_window_expose_event(GtkWidget * da, GdkEventExpose * event, gpointer user_data){
    gdk_draw_drawable(da->window,
        da->style->fg_gc[GTK_WIDGET_STATE(da)], pixmap,
        // Only copy the area that was exposed.
        event->area.x, event->area.y,
        event->area.x, event->area.y,
        event->area.width, event->area.height);
    return TRUE;
}

cairo_surface_t *tankImage;
cairo_surface_t *bulletImage;

static int currently_drawing = 0;

//the drawObj
void  drawObj(cairo_surface_t *image, int x, int  y,int angle, cairo_surface_t * surface){
    cairo_t *cr = cairo_create(surface);
    int tankWidth = cairo_image_surface_get_width (image);
    int tankHeight = cairo_image_surface_get_height (image);

    cairo_translate (cr, x, y);
    cairo_rotate (cr, angle  * 3.14159/180);
    cairo_scale  (cr, 0.15, 0.15);
    cairo_translate (cr, -tankWidth/2, -tankHeight/2);

    cairo_set_source_surface (cr, image, 0, 0 );
    cairo_paint (cr);
    cairo_destroy(cr);

}



//gives pixel value in bgra
void getPixelValue(int x, int y, cairo_surface_t *cst, unsigned char* rgb){
    cairo_surface_flush (cst);
    int index = (y* WINDOW_WIDTH*4 + 4*x);
    unsigned char* data = cairo_image_surface_get_data(cst);
    if (data != NULL){
        rgb = data + index;
    }else{
        cout << "Error 243212";
    }

    cairo_surface_mark_dirty(cst);
}

//do_draw will be executed in a separate thread whenever we would like to update
//our animation
void* do_draw(void *ptr){

    warmupCount++;
    if (warmupCount < 10){
        return NULL;
    }

    currently_drawing = 1;

    int width, height;
    gdk_threads_enter();
    gdk_drawable_get_size(pixmap, &width, &height);


    gdk_threads_leave();

    //create a gtk-independant surface to draw on
    cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(cst);

    cairo_set_source_rgb (cr, 0, 0, 0.0); //Background color
    cairo_paint(cr);// Makes things work right
    cairo_destroy(cr);

    for(int i = 0; i < tanks.size(); i++){
        if(tanks[i]->inGame){
            drawObj(tankImage, tanks[i]->x, tanks[i]->y, tanks[i]->rotationAngle, cst);
        }
    }

    //carrots
    //Draw all the bullets:
    for (auto b : bullets) {
        drawObj(bulletImage, b->x, b->y, b->angle, cst);
    }

    //box2

    //Do collision detection
    unsigned char* rgb;
    getPixelValue(0, 0, cst, rgb);
    


    //When dealing with gdkPixmap's, we need to make sure not to
    //access them from outside gtk_main().
    gdk_threads_enter();

    cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
    cairo_set_source_surface (cr_pixmap, cst, 0, 0);
    cairo_paint(cr_pixmap);
    cairo_destroy(cr_pixmap);

    gdk_threads_leave();
    currently_drawing = 0;

    cairo_surface_destroy(cst);

    return NULL;
}

gboolean timer_exe(GtkWidget * window){

    static gboolean first_execution = TRUE;

    //Update vehicle position and stuff
    /*
    //For tank 0
    if(aDown){
        tanks[0]->rotateLeft();
    }else if(dDown){
        tanks[0]->rotateRight();
    }
    if(wDown){
        tanks[0]->moveForward();
    }else if(sDown){
        tanks[0]->moveBackward();
    }

    //For tank 1
    if(leftDown){
        tanks[1]->rotateLeft();
    }else if(rightDown){
        tanks[1]->rotateRight();
    }
    if(upDown){
        tanks[1]->moveForward();
    }else if(downDown){
        tanks[1]->moveBackward();
    }
    */


    for (auto b : bullets) {
        b->update();
    }


    //use a safe function to get the value of currently_drawing so
    //we don't run into the usual multithreading issues
    int drawing_status = g_atomic_int_get(&currently_drawing);

    //if we are not currently drawing anything, launch a thread to
    //update our pixmap
    if(drawing_status == 0){
        static pthread_t thread_info;
        int  iret;
        if(first_execution != TRUE){
            pthread_join(thread_info, NULL);
        }
        iret = pthread_create( &thread_info, NULL, do_draw, NULL);
    }

    //tell our window it is time to draw our animation.
    int width, height;
    gdk_drawable_get_size(pixmap, &width, &height);
    gtk_widget_queue_draw_area(window, 0, 0, width, height);

    first_execution = FALSE;

    return TRUE;

}

#define COMMAND 0
#define TANKID 1
#define MOVETYPE 2

#define ROTATIONINDEX 4
#define XINDEX 2
#define YINDEX 3

void handleCommand(char* command, int clientID){
    string s(command);
    vector<string> parts;
    string delimiter = ":";
    size_t pos = 0;
    while ((pos = s.find(delimiter)) != string::npos) {
        string token = s.substr(0, pos);
        parts.push_back(token);
        s.erase(0, pos + delimiter.length());
    }

    if (parts.size() < 1){
        return;
    }
    /*
    if (parts.at(COMMAND) == "TankMoveCommand"){
        int tankIndex = parts.at(TANKID)[0] - '0';
        string action = parts.at(MOVETYPE);
        Tank* t = tanks.at(tankIndex);
        if(action == "rl") t->rotateLeft();
        if(action == "rr") t->rotateRight();
        if(action == "mf") t->moveForward();
        if(action == "mb") t->moveBackward();

    }
    */
    if (parts.at(COMMAND) == "TankPositionUpdate"){
        int tankIndex = clientID;//parts.at(TANKID)[0] - '0';
        Tank* t = tanks.at(tankIndex);

        int angle = stoi(parts.at(ROTATIONINDEX));
        int x = stoi(parts.at(XINDEX));
        int y = stoi(parts.at(YINDEX));

        t->x = x;
        t->y = y;
        t->rotationAngle = angle;

        
    }
    
}

class Player{
    public:
    int id = 0;
    int tankIndex = 0;
    bool inGame = true;
    Player(int index){
        this->tankIndex = index;
        this->id = index;
    }
};

vector<Player*> players;

void serverThread(){
    int opt = TRUE;   
    int master_socket , addrlen , new_socket , client_socket[30] ,  
          max_clients = 30 , activity, i , valread , sd;   
    int max_sd;   
    struct sockaddr_in address;   
         
    char buffer[1025];  //data buffer of 1K  
         
    //set of socket descriptors  
    fd_set readfds;   
         
    //a message  
    char *message = "ECHO Daemon v1.0 \r\n";   
     
    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }   
         
    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   
         
    //bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT);   
         
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    //accept the incoming connection  
    addrlen = sizeof(address);   
    puts("Waiting for connections ...");   
         
    while(TRUE)   
    {   
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(master_socket, &readfds);   
        max_sd = master_socket;   
             
        //add child sockets to set  
        for ( i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }   
             
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(master_socket, &readfds))   
        {   
            if ((new_socket = accept(master_socket,  
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
             
            //inform user of socket number - used in send and receive commands  
            //printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs 
                  //(address.sin_port));

 
            //add new socket to array of sockets  
            for (i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    printf("Adding to list of sockets as %d\n" , i);    


                    cout << "new player with id=" << i << "\n";
                    //goto new player
                    //If new player
                    if(i >= players.size()){   
                        Player* newPlayer = new Player(i);
                        players.push_back(newPlayer);

                        Tank *newTank = new Tank(200.0, 200.0);
                        tanks.push_back(newTank);
                    }else{
                        //It's a player adding back in
                        players.at(i)->inGame = true;
                        tanks.at(i)->addBackIntoGame();
                    }


  
                    break;   
                }   
            }   
        }   
             
        //else its some IO operation on some other socket 
        for (i = 0; i < max_clients; i++)   
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = read( sd , buffer, 1024)) == 0)   
                {   
                    //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&address , \ 
                        (socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    client_socket[i] = 0;   
                    
                    //Delete players tank
                    tanks.at(i)->removeFromGame();
                }   
                     
                //Read incomming message
                else 
                {   
                    //i is the client number
                    //set the string terminating NULL byte on the end  - should be set already by client
                    //of the data read  
                    buffer[valread] = '\0';
                    handleCommand(buffer, i);
                }   
            }   
        }   
    }   
         
}

int main (int argc, char *argv[]){
    std::thread first (serverThread);     // spawn new thread for host listening

    tankImage = cairo_image_surface_create_from_png ("tank.png");
    bulletImage = cairo_image_surface_create_from_png ("bullet.png");
    //XKeyboardControl control; 
    //control.auto_repeat_mode = 0; 

    //gdk_error_trap_push (); 
    //XChangeKeyboardControl (GDK_DISPLAY (), KBAutoRepeatMode, &control); 
    //gdk_error_trap_pop(); 

    //we need to initialize all these functions so that gtk knows
    //to be thread-aware
    if (!g_thread_supported ()){ g_thread_init(NULL); }
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);

    //GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    //GdkWindow *gtk_window = gtk_widget_get_parent_window(window);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);  
    gtk_window_resize(GTK_WINDOW(window), WINDOW_WIDTH, WINDOW_HEIGHT);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(window), "expose_event", G_CALLBACK(on_window_expose_event), NULL);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(on_window_configure_event), NULL);
    g_signal_connect(G_OBJECT (window), "key_press_event", G_CALLBACK (on_key_press), NULL);
    g_signal_connect(G_OBJECT (window), "key_release_event", G_CALLBACK (on_key_press), NULL);

    //gtk_window_set_resizable (GTK_WINDOW(window), FALSE);

    //done initializing
    //this must be done before we define our pixmap so that it can reference
    //the colour depth and such
    gtk_widget_show_all(window);




int new_width, new_height;
gtk_window_get_size (GTK_WINDOW(window), &new_width, &new_height);

    //cout << "Size is " <<  new_width << " " << new_height << "\n" ;

    //set up our pixmap so it is ready for drawing
    pixmap = gdk_pixmap_new(window->window,10,10,-1);
    //because we will be painting our pixmap manually during expose events
    //we can turn off gtk's automatic painting and double buffering routines.
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_widget_set_double_buffered(window, FALSE);

    (void)g_timeout_add(16, (GSourceFunc)timer_exe, window);


    gtk_main();
    gdk_threads_leave();



    return 0;
}

