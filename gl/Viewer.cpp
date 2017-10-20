//File: Viewer.h
//Brief: Creates an area for opengl drawing in a Gtk Widget.  Expects to be given constructor information for Drawables by 
//       external code, then draws those Drawables.  User can remove Drawables by VisID.  Drawables are managed by GLScenes.
//Author: Andrew Olivier aolivier@ur.rochester.edu

//c++ includes
#include <memory> //for std::unique_ptr
#include <giomm/resource.h> //for Glib::RefPointer?
#include <type_traits> //for std::true_type and std::false_type

//local includes
#include "gl/Viewer.h"
#include "gl/camera/Camera.h"

//gtk includes
#include <gtkmm.h>

//c++ includes
#include <iostream>
#include <sstream>
#include <utility> //For std::piecewise_construct shenanigans
#include <tuple> //For std::forward_as_tuple

//GLEW includes
#define GLEW_STATIC
#include "GL/glew.h"

namespace mygl
{
  Viewer::Viewer(std::shared_ptr<Camera> cam, const float xPerPixel, const float yPerPixel, const float zPerPixel): //TODO: Camera mode GUI
                Gtk::Box(Gtk::ORIENTATION_VERTICAL), fArea(), fCamera(cam), fSceneMap(), fXPerPixel(xPerPixel), fYPerPixel(yPerPixel), fZPerPixel(zPerPixel)
  {
    //Setup GLArea
    add(fArea);        

    //TODO: Key press/release is still not working in GLArea
    add_events(Gdk::SCROLL_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK
                     | Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
    fArea.add_events(Gdk::SCROLL_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK 
                     | Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
    fArea.set_can_focus(true); //This allows fArea to get keyboard events until another widget grabs focus
    fArea.grab_focus();
    
    fArea.set_required_version(3, 3);

    //Looks like window layout hints (ROOT's TGLayoutHints?)
    //TODO: Should the Window including this object call these?
    //fArea.set_hexpand(true);
    fArea.set_vexpand(true);
    fArea.set_auto_render(true);
    
    //GLArea signals
    //TODO: Confirm that these work.  I am now overriding these methods directly for Gtk::GLArea
    fArea.signal_realize().connect(sigc::mem_fun(*this, &Viewer::realize), false);
    fArea.signal_unrealize().connect(sigc::mem_fun(*this, &Viewer::unrealize), false);
    fArea.signal_render().connect(sigc::mem_fun(*this, &Viewer::render), false);

    //Configure opengl
    fArea.set_has_depth_buffer(true);

    fCamera->ConnectSignals(fArea);
    show_all();
  }

  Viewer::~Viewer() {}

  void Viewer::MakeScene(const std::string& name, const std::string& fragSrc, const std::string& vertSrc)
  {
    auto found = fSceneMap.find(name);
    if(found != fSceneMap.end())
    {
      throw util::GenException("Duplicate Scene Name") << "In mygl::Viewer::MakeScene(), requested scene name " << name << " is already in use.\n";
      return;      
    }

    //fSceneMap.emplace(name, Scene(name, fragSrc, vertSrc));
    fArea.throw_if_error();
    fArea.make_current();
    fSceneMap.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, fragSrc, vertSrc)); //lol
  }

  void Viewer::realize()
  {
    fArea.make_current();
    try
    {
      fArea.throw_if_error();
      glewExperimental = GL_TRUE;
      if(glewInit() != GLEW_OK)
      {
        std::cerr << "Failed to initialize GLEW\n";
      }

      std::cout << "In mygl::Viewer::realize(), got viewer width of " << fArea.get_allocated_width() << " and viewer "
                << "height of " << fArea.get_allocated_height() << "\n";
      glViewport(0, 0, fArea.get_allocated_width(), fArea.get_allocated_height());

      //enable depth testing
      //glEnable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    catch(const Gdk::GLError& err)
    {
      std::cerr << "Caught exception in Viewer::realize():\n"
                << err.domain() << "-" << err.code() << "-" << err.what() << "\n";
      //TODO: Should I rethrow?
    }
  }

  void Viewer::unrealize()
  {
    fArea.make_current();
    try
    {
      fArea.throw_if_error();
    }
    catch(const Gdk::GLError& err)
    {
      std::cerr << "Caught exception in Viewer::unrealize():\n"
                << err.domain() << "-" << err.code() << "-" << err.what() << "\n";
      //TODO: Should I rethrow?
    }
  }

  bool Viewer::render(const Glib::RefPtr<Gdk::GLContext>& /*context*/) 
  {
    fArea.make_current();
    try
    {
      fArea.throw_if_error();

      glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //TODO: Allow user to set background color
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      for(auto& scenePair: fSceneMap)
      {
        const auto view = glm::scale(fCamera->GetView(), glm::vec3(1.f/fXPerPixel, 1.f/fYPerPixel, 1.f/fZPerPixel));

        scenePair.second.Render(view, fCamera->GetPerspective(fArea.get_allocated_width(), fArea.get_allocated_height()));
      }
      glFlush();

      //Force continuous rendering.  
      fArea.queue_render(); 

      return false; 
    }
    catch(const Gdk::GLError& err)
    {
      std::cerr << "Caught exception in Viewer::render():\n"
                << err.domain() << "-" << err.code() << "-" << err.what() << "\n";
      return false;
    }
  }
  
  bool Viewer::on_motion_notify_event(GdkEventMotion* /*evt*/)
  {
    fArea.grab_focus();
    return false;
  }
}