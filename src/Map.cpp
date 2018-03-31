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
  EmscriptenWebGLContextAttributes attributes;
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context{0};
  std::string canvasID;

  glm::ivec2 mousePos{0,0};
  bool pointerLock{false};

  bool updateRequested{true};

  //################################################################################################
  Private(Map* q_, std::string canvasID_):
    q(q_),
    canvasID(canvasID_)
  {

  }

  //################################################################################################
  void update()
  {
    q->makeCurrent();
    q->paintGL();
  }

  //################################################################################################
  static EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* event, void *userData)
  {
    Private* d = static_cast<Private*>(userData);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_CLICK: //-----------------------------------------------------------------
    {
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEDOWN: //-------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Press);
      d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      emscripten_request_pointerlock(d->canvasID.data(), false);
      d->pointerLock = true;

      //0 : Left button
      //1 : Middle button (if present)
      //2 : Right button
      switch (event->button)
      {
      case 0:  e.button = tp_maps::Button::LeftButton;  break;
      case 2:  e.button = tp_maps::Button::RightButton; break;
      default: e.button = tp_maps::Button::NoButton;    break;
      }
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP: //---------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
      d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      emscripten_exit_pointerlock();
      d->pointerLock = false;

      //0 : Left button
      //1 : Middle button (if present)
      //2 : Right button
      switch (event->button)
      {
      case 0:  e.button = tp_maps::Button::LeftButton;  break;
      case 2:  e.button = tp_maps::Button::RightButton; break;
      default: e.button = tp_maps::Button::NoButton;    break;
      }
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_DBLCLICK: //--------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::DoubleClick);
      //d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      //0 : Left button
      //1 : Middle button (if present)
      //2 : Right button
      switch (event->button)
      {
      case 0:  e.button = tp_maps::Button::LeftButton;  break;
      case 2:  e.button = tp_maps::Button::RightButton; break;
      default: e.button = tp_maps::Button::NoButton;    break;
      }
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE: //-------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Move);

      if(d->pointerLock)
      {
        EmscriptenPointerlockChangeEvent pointerlockStatus;
        if(emscripten_get_pointerlock_status(&pointerlockStatus)==EMSCRIPTEN_RESULT_SUCCESS &&
           pointerlockStatus.isActive == EM_TRUE)
        {
          d->mousePos += glm::ivec2(tpBound(-10, int(event->movementX), 10),
                                    tpBound(-10, int(event->movementY), 10));
        }
      }
      else
        d->mousePos = glm::ivec2(event->targetX, event->targetY);

      e.pos = d->mousePos;
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEENTER: //------------------------------------------------------------
    {
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSELEAVE: //------------------------------------------------------------
    {
      break;
    }

    default: //-------------------------------------------------------------------------------------
    {
      break;
    }
    }
    return EM_TRUE;
  }

  //################################################################################################
  static EM_BOOL wheelCallback(int eventType, const EmscriptenWheelEvent* event, void* userData)
  {
    Private* d = static_cast<Private*>(userData);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_WHEEL: //-----------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Wheel);
      e.pos = d->mousePos;
      e.delta = -event->deltaY;
      d->q->mouseEvent(e);
      break;
    }
    default:
    {
      break;
    }
    }
    return EM_TRUE;
  }
};

//##################################################################################################
Map::Map(const char* canvasID, bool enableDepthBuffer):
  tp_maps::Map(enableDepthBuffer),
  d(new Private(this, canvasID))
{
  emscripten_webgl_init_context_attributes(&d->attributes);
  d->attributes.alpha                           = EM_FALSE;
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

  for(auto callback : {
      emscripten_set_click_callback     ,
      emscripten_set_mousedown_callback ,
      emscripten_set_mouseup_callback   ,
      emscripten_set_dblclick_callback  ,
      emscripten_set_mousemove_callback ,
      emscripten_set_mouseenter_callback,
      emscripten_set_mouseleave_callback})
  {
    if(callback(canvasID,
                d,
                EM_TRUE,
                Private::mouseCallback) != EMSCRIPTEN_RESULT_SUCCESS)
    {
      d->error = true;
      tpWarning() << "Failed to install mouse callback for: " << canvasID;
      return;
    }
  }

  if(emscripten_set_wheel_callback(canvasID,
                                   d,
                                   EM_TRUE,
                                   Private::wheelCallback) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    d->error = true;
    tpWarning() << "Failed to install wheel callback for: " << canvasID;
    return;
  }

  initializeGL();

  resize();
}

//##################################################################################################
Map::~Map()
{
  preDelete();
  if(emscripten_webgl_destroy_context(d->context) != EMSCRIPTEN_RESULT_SUCCESS)
    tpWarning() << "Failed to delete context: " << d->context;
  delete d;
}

//##################################################################################################
void Map::processEvents()
{
  if(!d->updateRequested)
    return;

  d->update();
  d->updateRequested = false;
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
  d->updateRequested = true;
}

//##################################################################################################
void Map::resize()
{
  makeCurrent();
  double width{0};
  double height{0};
  emscripten_get_element_css_size(d->canvasID.data(), &width, &height);

  int w = int(width);
  int h = int(height);

  emscripten_set_canvas_element_size(d->canvasID.data(), w, h);

  resizeGL(w, h);
}

}
