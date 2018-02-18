#include "tp_maps_emcc/Map.h"

#include "tp_maps/MouseEvent.h"

#include "tp_utils/DebugUtils.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

namespace tp_maps_emcc
{
struct Map::Private
{
  Map* q;

  bool error{false};
  bool quitting{false};
  EmscriptenWebGLContextAttributes attributes;
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context{0};
  std::string canvasID;

  glm::ivec2 mousePos{0,0};

  //################################################################################################
  Private(Map* q_, std::string canvasID_):
    q(q_),
    canvasID(canvasID_)
  {

  }

  //################################################################################################
  void update()
  {
    //EMCC_Event event;
    //while(EMCC_PollEvent(&event))
    //{
    //  switch(event.type)
    //  {
    //
    //  case EMCC_QUIT: //-----------------------------------------------------------------------------
    //  {
    //    quitting = true;
    //    break;
    //  }
    //
    //  case EMCC_MOUSEBUTTONDOWN: //------------------------------------------------------------------
    //  {
    //    tp_maps::MouseEvent e(tp_maps::MouseEventType::Press);
    //    e.pos = {event.button.x, event.button.y};
    //    switch (event.button.button)
    //    {
    //    case EMCC_BUTTON_LEFT:  e.button = tp_maps::Button::LeftButton;  break;
    //    case EMCC_BUTTON_RIGHT: e.button = tp_maps::Button::RightButton; break;
    //    default:               e.button = tp_maps::Button::NoButton;    break;
    //    }
    //    q->mouseEvent(e);
    //    break;
    //  }
    //
    //  case EMCC_MOUSEBUTTONUP: //--------------------------------------------------------------------
    //  {
    //    tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
    //    e.pos = {event.button.x, event.button.y};
    //    switch (event.button.button)
    //    {
    //    case EMCC_BUTTON_LEFT:  e.button = tp_maps::Button::LeftButton;  break;
    //    case EMCC_BUTTON_RIGHT: e.button = tp_maps::Button::RightButton; break;
    //    default:               e.button = tp_maps::Button::NoButton;    break;
    //    }
    //    q->mouseEvent(e);
    //    break;
    //  }
    //
    //  case EMCC_MOUSEMOTION: //----------------------------------------------------------------------
    //  {
    //    tp_maps::MouseEvent e(tp_maps::MouseEventType::Move);
    //    mousePos = {event.motion.x, event.motion.y};
    //    e.pos = mousePos;
    //    q->mouseEvent(e);
    //    break;
    //  }
    //
    //  case EMCC_MOUSEWHEEL: //-----------------------------------------------------------------------
    //  {
    //    tp_maps::MouseEvent e(tp_maps::MouseEventType::Wheel);
    //    e.pos = mousePos;
    //    e.delta = event.wheel.y;
    //    q->mouseEvent(e);
    //    break;
    //  }
    //
    //  case EMCC_WINDOWEVENT: //----------------------------------------------------------------------
    //  {
    //    if (event.window.event == EMCC_WINDOWEVENT_RESIZED)
    //      q->resizeGL(event.window.data1, event.window.data2);
    //    break;
    //  }
    //
    //  default: //-----------------------------------------------------------------------------------
    //  {
    //    break;
    //  }
    //  }
    //}

    q->makeCurrent();
    resize();
    q->paintGL();
    //EMCC_GL_SwapWindow(window);
  }

  //################################################################################################
  static EM_BOOL resizeCallback(int eventType, const EmscriptenUiEvent* uiEvent, void* userData)
  {
    tpDebug() << "resizeCallback";
    TP_UNUSED(uiEvent);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_RESIZE:
    {
      static_cast<Private*>(userData)->resize();
      break;
    }
    default:
    {
      break;
    }
    }
    return EM_FALSE;
  }

  //################################################################################################
  void resize()
  {
    double width{0};
    double height{0};
    emscripten_get_element_css_size(canvasID.data(), &width, &height);

    int w = int(width);
    int h = int(height);

    emscripten_set_canvas_element_size(canvasID.data(), w, h);

    //emscripten_set_canvas_size(w, h);

    //emscripten_webgl_get_drawing_buffer_size(context, &w, &h);
    //emscripten_get_canvas_size(&w, &h, &isFullscreen);
    q->resizeGL(w, h);
  }
};

//##################################################################################################
Map::Map(const char* canvasID, bool enableDepthBuffer):
  tp_maps::Map(enableDepthBuffer),
  d(new Private(this, canvasID))
{
  emscripten_webgl_init_context_attributes(&d->attributes);
  d->attributes.alpha                           = EM_TRUE;
  d->attributes.depth                           = EM_TRUE;
  d->attributes.stencil                         = EM_TRUE;
  d->attributes.antialias                       = EM_TRUE;
  d->attributes.premultipliedAlpha              = EM_TRUE;
  d->attributes.preserveDrawingBuffer           = EM_FALSE;
  d->attributes.preferLowPowerToHighPerformance = EM_FALSE;
  d->attributes.failIfMajorPerformanceCaveat    = EM_FALSE;
  d->attributes.majorVersion                    = 2;
  d->attributes.minorVersion                    = 0;
  d->attributes.enableExtensionsByDefault       = EM_TRUE;

  d->context = emscripten_webgl_create_context(canvasID, &d->attributes);
  if(d->context == 0)
  {
    d->error = true;
    tpWarning() << "Failed to get OpenGL context for: " << canvasID;
    return;
  }

  if(emscripten_set_resize_callback(canvasID,
                                    d,
                                    EM_TRUE,
                                    Private::resizeCallback) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    d->error = true;
    tpWarning() << "Failed to install resize callback for: " << canvasID;
    return;
  }

  initializeGL();

  d->resize();
}

//##################################################################################################
Map::~Map()
{
  preDelete();
  delete d;
}

//##################################################################################################
void Map::processEvents()
{
  d->update();
}

//##################################################################################################
void Map::makeCurrent()
{
  if (emscripten_webgl_make_context_current(d->context) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    d->error = true;
    tpWarning() << "Failed to make current.";
  }
}

//##################################################################################################
void Map::update()
{
#warning implement me
}

}
