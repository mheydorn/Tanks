#include <gtk/gtk.h>
#include <unistd.h>
#include <pthread.h>
#include <gdk/gdkkeysyms.h>
#include <iostream>
#include <math.h>

using namespace std;
//the global pixmap that will serve as our buffer
static GdkPixmap *pixmap = NULL;

//Global variables are life
int boxWidth = 10;
double tankX,tankY = 0;//tankLocation
int rotationAngle = 0; // tank rotation

bool movingLeft = false;
bool movingRight = false;
int speed = 0;


bool aDown = false;
bool dDown = false;
bool wDown = false;
bool sDown = false;


gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if(event->type == GDK_KEY_PRESS){
        switch (event->keyval)
        {
            case GDK_a:
                aDown = true; //Move left
                break;
            case GDK_d:
                dDown = true; //Move right
                break;

            case GDK_w:
                wDown = true; //Move left
                break;
            case GDK_s:
                sDown = true; //Move right
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


static int currently_drawing = 0;
//do_draw will be executed in a separate thread whenever we would like to update
//our animation
void *do_draw(void *ptr){

    currently_drawing = 1;
    //carrots
    int width, height;
    gdk_threads_enter();
    gdk_drawable_get_size(pixmap, &width, &height);
    gdk_threads_leave();

    //create a gtk-independant surface to draw on
    cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(cst);

    cairo_set_source_rgb (cr, 0, 0, 0.0); //Background color
    cairo_paint(cr);// Makes things work right




    int tankWidth = cairo_image_surface_get_width (tankImage);
    int tankHeight = cairo_image_surface_get_height (tankImage);

    cairo_translate (cr, tankX, tankY);
    cairo_rotate (cr, rotationAngle  * 3.14159/180);
    cairo_scale  (cr, 0.15, 0.15);
    cairo_translate (cr, -tankWidth/2, -tankHeight/2);
    //cairo_translate (cr, (int)tankX, (int)tankY);

    //cairo_set_source_surface (cr, tankImage, x - tankWidth/2, y - tankHeight/2);
    //cairo_set_source_surface (cr, tankImage,  - tankWidth/2*cos(rotationAngle  * 3.14159/180) , -tankHeight/2*sin(rotationAngle*3.14159/180));
    cairo_set_source_surface (cr, tankImage, 0, 0 );
    cairo_paint (cr);

    cairo_destroy(cr);


    //When dealing with gdkPixmap's, we need to make sure not to
    //access them from outside gtk_main().
    gdk_threads_enter();

    cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
    cairo_set_source_surface (cr_pixmap, cst, 0, 0);
    cairo_paint(cr_pixmap);
    cairo_destroy(cr_pixmap);

    gdk_threads_leave();

    cairo_surface_destroy(cst);

    currently_drawing = 0;


  // Draw the image at 110, 90, except for the outermost 10 pixels.
    //Gdk::Cairo::set_source_pixbuf(cr, image, 100, 80);
    //cr->rectangle(110, 90, image->get_width()-20, image->get_height()-20);
    //cr->fill();
    //return true;


    return NULL;
}

double degreeToRad(int degree){
    return (2*3.14159 * degree)/(360.0);

}



gboolean timer_exe(GtkWidget * window){

    static gboolean first_execution = TRUE;
    int rotationAngleMul = 1;
    int speedMultiplier = 1;
//cars
    //Update vehicle position and stuff
    if(aDown){
        rotationAngle -= rotationAngleMul;
        cout << "angle = " << rotationAngle << "\n";
    }else if(dDown){
        rotationAngle += rotationAngleMul;
        cout << "angle = " << rotationAngle << "\n";
    }
    rotationAngle = rotationAngle % 360;

    if(wDown){
        tankX += -(speedMultiplier * cos(degreeToRad(rotationAngle) + 3.14159/2));
        tankY += -(speedMultiplier * sin(degreeToRad(rotationAngle) + 3.14159/2));

        cout << "xPortion = " << -(speedMultiplier * cos(degreeToRad(rotationAngle) + 3.14159/2));
        cout << "yPortion = " << -(speedMultiplier * sin(degreeToRad(rotationAngle) + 3.14159/2));

        cout << "tankX = " << tankX << "\n";
        cout << "tankY = " << tankY << "\n";
    }else if(sDown){
        tankX += (speedMultiplier * cos(degreeToRad(rotationAngle) + 3.14159/2));
        tankY += (speedMultiplier * sin(degreeToRad(rotationAngle) + 3.14159/2));

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


int main (int argc, char *argv[]){
    tankImage = cairo_image_surface_create_from_png ("tank.png");
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

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(window), "expose_event", G_CALLBACK(on_window_expose_event), NULL);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(on_window_configure_event), NULL);
    g_signal_connect(G_OBJECT (window), "key_press_event", G_CALLBACK (on_key_press), NULL);
    g_signal_connect(G_OBJECT (window), "key_release_event", G_CALLBACK (on_key_press), NULL);

    //this must be done before we define our pixmap so that it can reference
    //the colour depth and such
    gtk_widget_show_all(window);

    //set up our pixmap so it is ready for drawing
    pixmap = gdk_pixmap_new(window->window,500,500,-1);
    //because we will be painting our pixmap manually during expose events
    //we can turn off gtk's automatic painting and double buffering routines.
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_widget_set_double_buffered(window, FALSE);

    (void)g_timeout_add(8, (GSourceFunc)timer_exe, window);


    gtk_main();
    gdk_threads_leave();



    return 0;
}

