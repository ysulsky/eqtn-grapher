/*
 * BUGS: "y =" doesn't give error
 *       
 */



#include <gtkmm.h>
#include <libglademm/xml.h>
#include <math.h>
#include <stdlib.h>
#include <list>
#include <assert.h>

#include "func.h"
#include "graph_area.h"

using namespace std;
using namespace Math;
using namespace Gnome;

typedef Glib::RefPtr< Glade::Xml > xmlptr;
typedef Glib::RefPtr< Gdk::Pixbuf > pixbufptr;

#define DEFAULT_SCALE      100.0
#define DEFAULT_SCALE_STR "100.0"

struct win_info
{
  win_info (void)
  {
    // create file selection dialog
    filesel = new Gtk::FileSelection ("Save Graph Image");
  }

  ~win_info (void)
  {
    delete filesel;
    delete graph_area;
  }

  Gtk::Window *graph_window;
  Gtk::Entry  *eqtn_entry, *scale_entry, *center_x_entry, *center_y_entry;
  Gtk::ToggleButton *draw_grid_btn;
  GraphArea  *graph_area;
  Gtk::MenuItem *new_window, *save_as, *quit, *about;
  Gtk::Ruler *hruler, *vruler;
  
  Gtk::FileSelection *filesel;
};

list< win_info* > g_windows;

// window
static bool on_graph_window_delete  (GdkEventAny* any, win_info* wi);

// menu
static void on_new_window_activate  (win_info *wi);
static void on_save_as_activate     (win_info *wi);
static void on_save_ok_clicked      (win_info *wi);
static void on_quit_activate        (win_info *wi);
static void on_about_activate       (win_info *wi);

// entries
static void eqtn_changed            (win_info *wi);
static void modify_graph_params     (win_info *wi);

// other
static void on_draw_grid_btn_toggled     (win_info *wi);
static bool on_graph_area_motion_notify  (GdkEventMotion *ev, win_info *wi);

static void set_rulers (win_info *wi, int x = -1, int y = -1);

static win_info*
create_new_window (void)
{
  win_info *wi = NULL;
  
  xmlptr new_win;
  try
  {
    new_win = Glade::Xml::create ("grapher.glade", "graph_window");
  }
  catch (Glade::XmlError e)
  {
    fprintf (stderr, "ERROR: %s\n", e.what().c_str());
    return wi;
  }
  
  // get the widgets
  wi = new win_info;
  
  new_win->get_widget ("graph_window", wi->graph_window);
  new_win->get_widget ("eqtn_entry", wi->eqtn_entry);
  new_win->get_widget ("scale_entry", wi->scale_entry);
  new_win->get_widget ("center_x_entry", wi->center_x_entry);
  new_win->get_widget ("center_y_entry", wi->center_y_entry);
  new_win->get_widget ("draw_grid_btn", wi->draw_grid_btn);
  new_win->get_widget ("new_window", wi->new_window);
  new_win->get_widget ("save_as", wi->save_as);
  new_win->get_widget ("quit", wi->quit);
  new_win->get_widget ("about", wi->about);
  new_win->get_widget ("hruler", wi->hruler);
  new_win->get_widget ("vruler", wi->vruler);
  

  wi->graph_area = new GraphArea(75);
  Gtk::Table* layout_table;
  layout_table = new_win->get_widget ("layout_table", layout_table);
  layout_table->attach (*wi->graph_area, 1, 2, 1, 2);
  
  // window signals
  wi->graph_window->signal_delete_event().connect (SigC::bind< win_info* > 
    (SigC::slot (on_graph_window_delete), wi));
  
  // graph_area signals
  wi->graph_area->signal_motion_notify_event().connect (SigC::bind< win_info* >
    (SigC::slot (on_graph_area_motion_notify), wi));
  
  // entry signals
  wi->eqtn_entry->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (eqtn_changed), wi));
  
  wi->scale_entry->signal_activate().connect (SigC::bind< win_info* >
    (SigC::slot (modify_graph_params), wi));
  wi->center_x_entry->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (modify_graph_params), wi));
  wi->center_y_entry->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (modify_graph_params), wi));

  // toggle grid signal
  wi->draw_grid_btn->signal_toggled().connect (SigC::bind< win_info* > 
    (SigC::slot (on_draw_grid_btn_toggled), wi));
      
  // menu signals
  wi->new_window->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (on_new_window_activate), wi));
  wi->save_as->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (on_save_as_activate), wi));
  wi->quit->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (on_quit_activate), wi));
  wi->about->signal_activate().connect (SigC::bind< win_info* > 
    (SigC::slot (on_about_activate), wi));

  // file selection signals
  wi->filesel->get_ok_button()->signal_clicked().connect (SigC::bind<win_info*>
    (SigC::slot (on_save_ok_clicked), wi));
  wi->filesel->get_cancel_button()->signal_clicked().connect (
    SigC::slot (*wi->filesel, &Gtk::Widget::hide));
 
  // set the rulers
  //TODO: set_rulers (wi); (do this after realization)

  // set the text for the scale_entry
  wi->scale_entry->set_text (DEFAULT_SCALE_STR);
  
  // select all of the text in the eqtn_entry
  wi->eqtn_entry->select_region (0, -1);
  
  // disable the save_as, there is no function yet
  wi->save_as->set_sensitive (false);
  
  // set file selection defaults
  wi->filesel->set_transient_for (*wi->graph_window);
  wi->filesel->set_filename ("graph.png");
  
  g_windows.push_back (wi);

  wi->graph_window->show_all();

  return wi;
}

int
main (int argc, char **argv)
{
  Gtk::Main app(argc, argv);

  if (!create_new_window())
    return -1;
  
  app.run();
  
  return 0;
}

static void 
on_new_window_activate (win_info* wi)
{
  win_info *new_wi = create_new_window();
  if (new_wi)
  {
    // copy the window state
    new_wi->eqtn_entry->set_text (wi->eqtn_entry->get_text());
    new_wi->scale_entry->set_text (wi->scale_entry->get_text());
    new_wi->center_x_entry->set_text (wi->center_x_entry->get_text());
    new_wi->center_y_entry->set_text (wi->center_y_entry->get_text());
    new_wi->draw_grid_btn->set_sensitive (wi->draw_grid_btn->sensitive());
    new_wi->draw_grid_btn->set_active (wi->draw_grid_btn->get_active());  
    new_wi->save_as->set_sensitive (wi->save_as->sensitive());

    *(new_wi->graph_area) = *(wi->graph_area);
  }
}

static void 
on_save_as_activate (win_info* wi)
{
  wi->filesel->show();
}

static void
on_save_ok_clicked (win_info *wi)
{
  wi->graph_area->save_img (wi->filesel->get_filename(), "png");
  wi->filesel->hide();
}

static void
on_quit_activate (win_info* wi)
{
  exit (0);
}

static void
on_about_activate (win_info* wi)
{
  printf ("Grapher 0.1 - Yury Sulsky <yury@nyu.edu>\n");
}

static void
on_draw_grid_btn_toggled (win_info* wi)
{
  wi->graph_area->toggle_grid();
}

void
GraphArea::draw_graph (int x, int y, int width, int height)
{
  int half_width  = img->get_width() / 2;
  int half_height = img->get_height() / 2;
  
  guchar* buf = img->get_pixels();
  int stride  = img->get_rowstride();
  int pixel_size = img->get_n_channels() *
                   img->get_bits_per_sample() / 8;

  Glib::Timer timer;

  double x_y[2];
  
  for (int i = x; i < x + width; i++ )
  {
    for (int j = y; j < y + height; j++ )
    {
      x_y[0] = ((double) (i - half_width))  / scale + center_x;
      x_y[1] = ((double)-(j - half_height)) / scale - center_y;
      
      int bytepos = j*stride + i*pixel_size;
      double diff = fabs (F (x_y));

      if( diff < 1.0 )
      {
        // steepen the error curve
        diff = ::pow (diff, 0.3f); // not float std::pow (float, float)
        buf[ bytepos ] = (guchar) ((1.0f-diff) * 0xFF);
      }
      else
        buf[ bytepos ] = 0;
    }
  }

  timer.stop();
  printf ("time elapsed: %f\n", timer.elapsed());
}

#if 0
static void
redraw_graph_full (win_info *wi)
{
  if (wi->null_func)
  {
    wi->save_as->set_sensitive (false);
    return;
  }
  
  redraw_graph (wi, 0, 0, wi->img->get_width(), wi->img->get_height());
  wi->graph_area->queue_draw();
}

static void
redraw_graph_partial (win_info *wi, double center_x, double center_y)
{
  if (wi->null_func)
  {
    wi->save_as->set_sensitive (false);
    return;
  }
  
  double old_val;
  
  int width  = wi->img->get_width();
  int height = wi->img->get_height();

  old_val = wi->center_x;
  wi->center_x = center_x;
  int delta_x_pix = wi->horiz_pt_to_pix (old_val) -
                    wi->horiz_pt_to_pix (center_x);
  
  old_val = wi->center_y;
  wi->center_y = center_y;
  int delta_y_pix = wi->vert_pt_to_pix  (old_val) - 
                    wi->vert_pt_to_pix  (center_y);
  
  if (abs(delta_x_pix) < width && abs(delta_y_pix) < height)
  {
    pixbufptr new_img = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8,
                                             width, height);
    
    int orig_x, orig_y, copy_width, copy_height, dest_x, dest_y;
    
    orig_x = max (0, -delta_x_pix);
    dest_x = max (0, delta_x_pix);
    copy_width = width - abs (delta_x_pix);
    
    orig_y = max (0, -delta_y_pix);
    dest_y = max (0, delta_y_pix);
    copy_height = height - abs (delta_y_pix);
    
printf("dx=%d dy=%d, x=%d, y=%d, w=%d, h=%d, destx=%d, desty=%d\n",    delta_x_pix, delta_y_pix, orig_x, orig_y, copy_width, copy_height, dest_x, dest_y);
    
    wi->img->copy_area (orig_x, orig_y, copy_width, copy_height,
	                new_img, dest_x, dest_y);
    wi->img = new_img;

    if (delta_x_pix != 0)
      redraw_graph (wi, (delta_x_pix > 0) ? 0 : width + delta_x_pix, 0,
	            width - copy_width, height);

    if (delta_y_pix != 0)
      redraw_graph (wi, dest_x, (delta_y_pix > 0) ? 0 : copy_height,
	            copy_width, abs (delta_y_pix));
    
    wi->graph_area->queue_draw();
  }
  else
    redraw_graph_full (wi);
}
#endif

static void
eqtn_changed (win_info *wi)
{
  string eqtn = wi->eqtn_entry->get_text();
  int equal_sign_pos = eqtn.find ('=');

  // make sure there's an equal sign
  if (equal_sign_pos == string::npos)
  {
    equal_sign_pos = 2;
    eqtn.insert (0, "y = ");
    wi->eqtn_entry->set_text (eqtn);
  }

  // create the 3-D explicit function "F"
  // f(x,y) = g(x,y)  -->  F(x,y) = f(x,y) - g(x,y)
  eqtn [equal_sign_pos] = '-';
  eqtn.insert (equal_sign_pos + 1, 1, '(');
  eqtn.append (1, ')');
  
  //TODO: give more detailed errors
  try
  {
     Function F = eqtn;
     wi->graph_area->change_graph (wi->graph_area->get_scale(),
	                           wi->graph_area->get_center_x(),
				   wi->graph_area->get_center_y(), F);
  }
  catch (SyntaxException e)
  {
    wi->graph_area->set_null_func();
  }
  catch (ArgumentException e)
  {
    wi->graph_area->set_null_func();
  }

  wi->save_as->set_sensitive (!wi->graph_area->is_null_func());
}

static void
set_rulers (win_info *wi, int x, int y)
{
  int width  = wi->graph_area->get_width();
  int height = wi->graph_area->get_height();

  x = (x == -1) ? width / 2  : x;
  y = (y == -1) ? height / 2 : y;
  
  wi->hruler->set_range (wi->graph_area->horiz_px_to_pt (0),
                         wi->graph_area->horiz_px_to_pt (width),
			 wi->graph_area->horiz_px_to_pt (x),
			 1000.0f);
  
  wi->vruler->set_range (wi->graph_area->vert_px_to_pt (0),
                         wi->graph_area->vert_px_to_pt (height),
			 wi->graph_area->vert_px_to_pt (y),
			 1000.0f);
}

static void
modify_graph_params (win_info* wi)
{
  double center_x = atof (wi->center_x_entry->get_text().c_str());
  if (center_x == 0.0) // in case of errors
    wi->center_x_entry->set_text ("0.0");

  double center_y = atof (wi->center_y_entry->get_text().c_str());
  if (center_y == 0.0) // in case of errors
    wi->center_y_entry->set_text ("0.0");

  double scale = atof (wi->scale_entry->get_text().c_str());
  if (scale == 0.0) // in case of errors
  {
    scale = DEFAULT_SCALE;
    wi->scale_entry->set_text (DEFAULT_SCALE_STR);
  }

  if (scale != wi->graph_area->get_scale())
  {
    wi->graph_area->change_graph (scale, center_x, center_y);
    
    if (scale < 5.0)
    {
      wi->draw_grid_btn->set_active (false);
      wi->draw_grid_btn->set_sensitive (false);
    }
    else
    {
      wi->draw_grid_btn->set_sensitive (true);
    }
  }
  else
    wi->graph_area->move_graph (center_x, center_y);

  set_rulers (wi);
}


static bool
on_graph_area_motion_notify (GdkEventMotion *ev, win_info *wi)
{
  set_rulers (wi, (int)ev->x, (int)ev->y);
  return false;
}

static bool
on_graph_window_delete (GdkEventAny* any, win_info* wi)
{
  list< win_info* >::iterator it;
  for (it = g_windows.begin(); it != g_windows.end(); it++)
  {
    if (*it == wi)
    {
      delete wi;
      g_windows.erase (it);
      if (g_windows.empty())
        exit (0);

      return false;
    }
  }

  assert (false);
  return false;
}


