#include <gtk/gtk.h>
#include <unistd.h>
#include <pthread.h>
#include <gdk/gdkkeysyms.h>
#include <iostream>
#include <math.h>
#include <list>
#include <vector>

#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>

#include <thread>         // std::thread
#include <mutex>
#include "safeQueue.cpp"

#include <string.h>

#define PORT 8088 

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400

using namespace std;

//TODO fix lag when updateinng bullets and tanks at same time - combine bulletupdate and tankupdate messages into one send() call
//TODO bug where 2nd client seees 1st client's tank alway in top left corner and can take control of wrong tank - playerIDs gettign messed up with tank updates -- seems to only happen when a client quits and rejoins a match - this is because the server isn't informing the clients of when a tank leaves the game

mutex bulletMtx;
mutex tankMtx;
mutex playerMtx;

int playerID = -1;

SafeQueue<string> *commandQueue;

double degreeToRad(int degree){
    return (2*3.14159 * degree)/(360.0);
}

string ipAddress = "127.0.0.1";
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

bool connected = false;

vector<Tank*> tanks;

class Bullet{
    public:
    double dx = 0.0;
    double dy = 0.0;
    double x= 0;
    double y = 0;
    int angle = 0;
    bool local = true;
    int rsquared = 10;
    bool valid = true;
    Bullet(int tankIndexThatFired){
        this->angle = tanks[tankIndexThatFired]->rotationAngle;
        dx = -(bulletSpeedMultiplier * cos(degreeToRad(angle) + 3.14159/2));
        dy = -(bulletSpeedMultiplier * sin(degreeToRad(angle) + 3.14159/2));

        this->x = tanks[tankIndexThatFired]->x;
        this->y = tanks[tankIndexThatFired]->y;
    }

    Bullet(){
    }

    void update(){
        x += dx;
        y += dy;
        
        if( x < 0 || y < 0 || x > 400 || y > 400){
            valid = false;
        }
    }
};

bool is_bullet_valid (const Bullet* value) { 
    return (!value->valid); 
}

bool is_bullet_remote (const Bullet* value) { 
    return (!value->local); 
}



list<Bullet*> bullets;


void fireBullet(int tankIndexThatFired){
    bulletMtx.lock();
    Bullet* b = new Bullet(tankIndexThatFired);
    b->local = true;
    bullets.push_back(b);
    bulletMtx.unlock();
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
                fireBullet(playerID);
                break;
            case GDK_p:
                if(pDown) return FALSE;
                pDown = true;
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
                upDown = false; //Move right
                break;
            case GDK_Down:
                downDown = false; //Move right
                break;
            case GDK_Left:
                leftDown = false; //Move right
                break;
            case GDK_Right:
                rightDown = false; //Move right
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
    bulletMtx.lock();
    for (auto b : bullets) {
        drawObj(bulletImage, b->x, b->y, b->angle, cst);
    }
    bulletMtx.unlock();
    //box2

   

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


    while(tanks.size() <= playerID){   
        Tank* newTank = new Tank();
        tanks.push_back(newTank); 
    }

    //fireBullet(playerID); for constant fire
    //Tell server about where we think the bullets are
    
    //TankUpdate:len:id:x:y:angle:id:x:y:angle:etc
    string tankAndBulletUpdate = "";
    bulletMtx.lock();
    int count = 0;
    string message = "";
    std::list<Bullet*>::iterator it;
    for (it = bullets.begin(); it != bullets.end(); ++it){
        if((*it)->local){
            message += to_string(playerID) + ":" + to_string((int)(*it)->x) + ":" + to_string((int)(*it)->y) + ":" + to_string((int)(*it)->angle) + ":";
            count++;
        }
    }
    string messageHeader = "BulletUpdate:" + to_string(count) + ":";
    bulletMtx.unlock();
    if (count > 0){
        tankAndBulletUpdate += (messageHeader + message + "~");
    }

    //Update Absolute Tank Positions
    string commandToSend = "TankPositionUpdate:0:" + to_string((int)(tanks[playerID]->x)) + ":" + to_string((int)(tanks[playerID]->y)) + ":" + to_string(tanks[playerID]->rotationAngle) + ":~";
    tankAndBulletUpdate += commandToSend;
    commandQueue->enqueue(tankAndBulletUpdate);

    static gboolean first_execution = TRUE;

    //Update vehicle position and stuff
    
    //For tank 0
    if(aDown){
        tanks[playerID]->rotateLeft();
    }else if(dDown){
        tanks[playerID]->rotateRight();
    }
    if(wDown){
        tanks[playerID]->moveForward();
    }else if(sDown){
        tanks[playerID]->moveBackward();
    }

    bulletMtx.lock();
    for (auto b : bullets) {
        b->update();
    }

    //Remove invalid bullets (wentoff screen)
    bullets.remove_if (is_bullet_valid);
    bulletMtx.unlock();



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


int sock = -1;
void clientThread(){
    while(1){
        struct sockaddr_in address; 
        int valread; 
        struct sockaddr_in serv_addr; 
        char *hello = "Hello from client"; 
        char buffer[1024] = {0}; 
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        { 
            printf("\n Socket creation error \n"); 
            exit(0); 
        } 
       
        memset(&serv_addr, '0', sizeof(serv_addr)); 
       
        serv_addr.sin_family = AF_INET; 
        serv_addr.sin_port = htons(PORT); 
           
        // Convert IPv4 and IPv6 addresses from text to binary form 
        if(inet_pton(AF_INET, ipAddress.c_str() , &serv_addr.sin_addr)<=0)  
        { 
            printf("\nInvalid address/ Address not supported \n"); 
            exit(0); 
        } 
       
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
        { 
            printf("\Could not connect, trying again in 1 second\n"); 
            unsigned microsec = 1000000;
            usleep(microsec);
        }else{
            cout << "Connected to server!\n";
            connected = true;
            while(1){
                string command = commandQueue->dequeue();
                const char* Cstr = command.c_str();
                send(sock , Cstr , strlen(Cstr), 0 );
            }
        }
    }
}

vector<string> parseCommand(string message, string delimiter){

    vector<string> parts;
    size_t pos = 0;
    while ((pos = message.find(delimiter)) != string::npos) {
        string token = message.substr(0, pos);
        parts.push_back(token);
        message.erase(0, pos + delimiter.length());
    }
    return parts;
}

//Handle messages from server
void handleMessage(string allMessages){

    vector<string> messages = parseCommand(allMessages, "~");
    for (auto message : messages){

        vector<string> parts = parseCommand(message, ":");
        if (parts.size() < 1){
            return;
        }
        if(parts.at(0) == "Welcome player "){
            playerID = stoi(parts.at(1));
        }
        //TankUpdate:len:id:x:y:angle:id:x:y:angle:etc
        if(parts.at(0) == "TankUpdate"){
            int numTanks = stoi(parts.at(1));
            for(int i = 0 ; i < numTanks; i++){
                if (i == playerID) continue;

                int tankId = stoi(parts.at(2 + i*4));
                int x = stoi(parts.at(2 + i*4 + 1));
                int y = stoi(parts.at(2 + i*4 + 2));
                int angle = stoi(parts.at(2 + i*4 + 3));
        
                while(tanks.size() <= tankId){   
                    Tank* newTank = new Tank();

                    tanks.push_back(newTank); 

                }
                if(tankId < tanks.size()){
                    Tank* tank = tanks.at(tankId);
                    tank->x = x;
                    tank->y = y;
                    tank->rotationAngle = angle;

                }else{
                    cout << "client does not have enough tanks" << "\n";
                    exit(0);
                }

            }
        }

        if(parts.at(0) == "BulletUpdate"){
            bulletMtx.lock();
            int numBullets = stoi(parts.at(1));
            if(parts.size() <= (numBullets-1)*4 + 2 + 3){
                cout << "Bad command from server \n";
                bulletMtx.unlock();
                return;
            }

            bullets.remove_if (is_bullet_remote);
            for(int i = 0 ; i < numBullets; i++){

                int owner = stoi(parts.at(2 + i*4)); //Ignoring this for now - will use later to know which player bullet came from to know who get's the point
                int x = stoi(parts.at(2 + i*4 + 1));
                int y = stoi(parts.at(2 + i*4 + 2));
                int angle = stoi(parts.at(2 + i*4 + 3));
                if (owner == playerID){cout << "skipping bullet\n";continue;}//Don't get bullet updates for client's own bullets
                
                Bullet* newBullet = new Bullet();
                newBullet->x = (double)x;
                newBullet->y = (double)y;
                newBullet->angle = angle;
                newBullet->local = false;

                
                bullets.push_back(newBullet);
               
            }
            bulletMtx.unlock();
        }
    }
}



void listenThread(){
    char buffer[1024] = {0}; 
    while(1){
        if(connected){
            int len = read( sock , buffer, 1024); 
            if(len > 0){
                buffer[len] = '\0';
                string message(buffer);
                //cout << "Got message from server:" << message << "\n";
                handleMessage(buffer);
            }
        }
    
    }   
}

int main (int argc, char *argv[]){

    string serverIPInput = "";
    cout << "Enter Server IP or just press enter for localhost\n";
    getline (cin, serverIPInput);

    if (serverIPInput != ""){ 
        ipAddress = serverIPInput;
    }
    
    cout << "Connecting to " << ipAddress << "\n";
    
    
    
    commandQueue = new SafeQueue<string>();
    std::thread first (clientThread);     // spawn new thread for host listening
    std::thread second (listenThread);     // spawn new thread for host listening

    while(!connected){}
    while(playerID == -1){}



    cout << "Assigned player " << playerID << "\n";


    tankImage = cairo_image_surface_create_from_png ("tank.png");
    bulletImage = cairo_image_surface_create_from_png ("bullet.png");

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

    cout << "Size is " <<  new_width << " " << new_height << "\n" ;

    //set up our pixmap so it is ready for drawing
    pixmap = gdk_pixmap_new(window->window,WINDOW_WIDTH,WINDOW_HEIGHT,-1);
    //because we will be painting our pixmap manually during expose events
    //we can turn off gtk's automatic painting and double buffering routines.
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_widget_set_double_buffered(window, FALSE);

    //fps
    (void)g_timeout_add(16, (GSourceFunc)timer_exe, window);


    gtk_main();
    gdk_threads_leave();



    return 0;
}

