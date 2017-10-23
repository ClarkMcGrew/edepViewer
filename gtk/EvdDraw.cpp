//File: EvdDraw.cpp
//Brief: Runs a EvdWindow to show how a ROOT geometry can be viewed with GTKMM and OpenGL.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//evd includes
#include "gtk/EvdWindow.h"

//gtkmm includes
#include "gtkmm/application.h"

int main(int argc, char** argv)
{
  auto app = Gtk::Application::create(argc, argv, "test.EvdWindow");

  mygl::EvdWindow window("/home/aolivier/ND_Studies/100_in_KLOE_tracker_g4.root"); //, darkColors); 
  
  return app->run(window);
}
