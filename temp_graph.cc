#include <gtkmm.h>
#include <math.h>
#include "graph_area.h"

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

int
main (int argc, char **argv)
{
  Gtk::Main app (argc, argv);

  GraphArea ga (75.0);
  Gtk::Window win;
  win.add (ga);

  win.show_all();
  
  app.run (win);

  return 0;  
}
