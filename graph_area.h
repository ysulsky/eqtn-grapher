#ifndef _GRAPH_AREA_H_
#define _GRAPH_AREA_H_

#include <gtkmm.h>
#include <string>
#include "func.h"

class GraphArea : public Gtk::DrawingArea
{
  bool null_func;
  bool grid_active;
  double center_x, center_y;
  double scale;

  void init (double center_x, double center_y, double scale);
  
public:
  Math::Function F;
  Glib::RefPtr< Gdk::Pixbuf > img;

  GraphArea (double scale)
  {
    init (0.0, 0.0, scale);
  }

  GraphArea (const GraphArea& other)
  {
    init (other.center_x, other.center_y, other.scale);
    
    null_func   = other.null_func;
    grid_active = other.grid_active;
    
    F = other.F;
  }

  GraphArea& operator= (const GraphArea& other)
  {
    if (this == &other) return *this;
    
    center_x  = other.center_x;
    center_y  = other.center_y;
    scale     = other.scale;
    null_func = other.null_func;
    grid_active = other.grid_active;
    F = other.F;
    
    return *this;
  }

  bool           is_null_func (void) const { return null_func; }
  Math::Function get_func     (void) const { return F; }
  double         get_center_x (void) const { return center_x; }
  double         get_center_y (void) const { return center_y; }
  double         get_scale    (void) const { return scale; }
  bool           has_grid     (void) const { return grid_active; }
  
  void toggle_grid();
  void set_null_func()
  {
    null_func = true;
    queue_draw();
  }
  
  void change_graph (double scale, double center_x, double center_y);
  void change_graph (double scale, double center_x, double center_y,
                     Math::Function& F)
  {
    null_func = false;
    this->F = F;
    
    change_graph (scale, center_x, center_y);
  }

  void move_graph (double center_x, double center_y);

  void save_img (const std::string& filename, const std::string& type,
                 bool save_grid = false);

  // pixels to logical points
  double horiz_px_to_pt (int x)
  { return ((double)(x - img->get_width()/2)) / scale + center_x; }
  
  double  vert_px_to_pt (int y)
  { return ((double)(img->get_height()/2 - y)) / scale + center_y; }
  
  // logical points to pixels
  int horiz_pt_to_px (double x)
  { return ((int)(scale * (x - center_x))) + img->get_width()/2; }
  
  int  vert_pt_to_px (double y)
  { return img->get_height()/2 - ((int)(scale * (y - center_y))); }

protected:
  virtual void on_realize         (void);
  virtual bool on_configure_event (GdkEventConfigure *ev);
  virtual bool on_expose_event    (GdkEventExpose *ev);

  void draw_grid (Glib::RefPtr< Gdk::Drawable > canvas);
  void draw_grid (void)
  { draw_grid (get_window()); }
  
  void draw_graph (int x, int y, int width, int height);
};

#endif

