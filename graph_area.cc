#include "graph_area.h"
#include "func.h"

using namespace std;
using namespace Gtk;

void
GraphArea::init (double center_x, double center_y, double scale)
{
  this->center_x = center_x;
  this->center_y = center_y;
  this->scale    = scale;

  null_func = true;
  grid_active = false;
}

void
GraphArea::on_realize (void)
{
  DrawingArea::on_realize();
  
  img = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8,
                             get_width(), get_height());

  set_double_buffered (false);  // we always blt from a pixbuf
  modify_bg (Gtk::STATE_NORMAL, Gdk::Color()); // bg -> black
}

bool
GraphArea::on_configure_event (GdkEventConfigure *ev)
{
  //TODO: do this the smart way
  img = Gdk::Pixbuf::create (Gdk::COLORSPACE_RGB, false, 8,
                             ev->width, ev->height);
  change_graph (scale, center_x, center_y);

  return true;
}

void
GraphArea::save_img (const string& fn, const string& type, bool save_grid)
{
  if (!is_null_func())
  {
    if (!save_grid)
      img->save (fn, type);
    else
    {
      Glib::RefPtr< Gdk::Pixmap > pmap = Gdk::Pixmap::create
        (get_window(), (const char*) img->get_pixels(), img->get_width(),
	 img->get_height(), img->get_bits_per_sample() * img->get_n_channels(),
	 Gdk::Color(), Gdk::Color());

      draw_grid (pmap);

      Glib::RefPtr< Gdk::Pixbuf > pbuf = Gdk::Pixbuf::create
	((Glib::RefPtr< Gdk::Drawable >)pmap, pmap->get_colormap(), 0, 0, 0, 0,
	 img->get_width(), img->get_height());

      pbuf->save (fn, type);
    }
  }
}

void
GraphArea::draw_grid (Glib::RefPtr< Gdk::Drawable > canvas)
{
  int width  = img->get_width();
  int height = img->get_height();
    
  double start_x = ceil (horiz_px_to_pt (0));
  double start_y = ceil (vert_px_to_pt  (height));

  double end_x = horiz_px_to_pt (width);
  double end_y = vert_px_to_pt  (0);

  Glib::RefPtr< Gdk::GC > gc = Gdk::GC::create (canvas);

  Gdk::Color line_color;
  line_color.set_rgb_p (0.5, 0.5, 0.5);

  Gdk::Screen::get_default()->get_default_colormap()->alloc_color(line_color);
  gc->set_foreground (line_color);

  for (double x = start_x; x < end_x; x++)
  {
    int i = horiz_pt_to_px (x);
    gc->set_line_attributes (1, 
	                     (x == 0.0) ? 
		               Gdk::LINE_SOLID : Gdk::LINE_ON_OFF_DASH,
			       Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER);
    canvas->draw_line (gc, i, 0, i, height);
  }

  for (double y = start_y; y < end_y; y++)
  {
    int i = vert_pt_to_px (y);
    gc->set_line_attributes (1, 
                             (y == 0.0) ? 
                               Gdk::LINE_SOLID : Gdk::LINE_ON_OFF_DASH, 
			       Gdk::CAP_NOT_LAST, Gdk::JOIN_MITER);
    canvas->draw_line (gc, 0, i, width, i);
  }
}

void
GraphArea::toggle_grid (void)
{
  grid_active = !grid_active;
  if (grid_active)
    draw_grid();
  else
    queue_draw();
}

bool
GraphArea::on_expose_event (GdkEventExpose *ev)
{
  Gdk::Region region = Glib::wrap (ev->region, true);
  Glib::ArrayHandle< Gdk::Rectangle > rectangles = region.get_rectangles();
  Glib::ArrayHandle< Gdk::Rectangle >::const_iterator it = rectangles.begin();
  
  Glib::RefPtr< Gdk::Window > graph_area_win = get_window();

  Glib::RefPtr< Gdk::GC > gc = Gdk::GC::create (graph_area_win);
  
  for (; it != rectangles.end(); it++)
  {
    int x = (*it).get_x();
    int y = (*it).get_y();
    int width = (*it).get_width();
    int height = (*it).get_height();
    
    if (null_func)
      graph_area_win->draw_rectangle (gc, true, x, y, width, height);
    else
      img->render_to_drawable (graph_area_win, gc,
			       x, y, x, y, width, height,
			       Gdk::RGB_DITHER_NORMAL, 0, 0);
  }

  if (grid_active)
    draw_grid();
  
  return true; // stop further emission of the signal
}

void
GraphArea::change_graph (double scale, double center_x, double center_y)
{
  this->scale = scale;
  this->center_x = center_x;
  this->center_y = center_y;
  
  if (!null_func)
  {
    draw_graph (0, 0, img->get_width(), img->get_height());
    queue_draw();
  }
}

void
GraphArea::move_graph (double center_x, double center_y)
{
  //TODO: partial redraw
  change_graph (scale, center_x, center_y);
}

