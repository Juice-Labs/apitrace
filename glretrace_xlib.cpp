/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#include "glimports.hpp"
#include "glretrace.hpp"


namespace glretrace {


class XlibVisual : public Visual
{
public:
    XVisualInfo *visinfo;

    XlibVisual(XVisualInfo *vi) :
        visinfo(vi)
    {}

    ~XlibVisual() {
        XFree(visinfo);
    }
};


class XlibDrawable : public Drawable
{
public:
    Display *display;
    Window window;

    XlibDrawable(const Visual *vis, Display *dpy, Window win) :
        Drawable(vis),
        display(dpy), 
        window(win)
    {}

    ~XlibDrawable() {
        XDestroyWindow(display, window);
    }
    
    void
    resize(unsigned w, unsigned h) {
        Drawable::resize(w, h);
        XResizeWindow(display, window, w, h);
    }

    void swapBuffers(void) {
        glXSwapBuffers(display, window);
    }
};


class XlibContext : public Context
{
public:
    Display *display;
    GLXContext context;
    
    XlibContext(const Visual *vis, Display *dpy, GLXContext ctx) :
        Context(vis),
        display(dpy), 
        context(ctx)
    {}

    ~XlibContext() {
        glXDestroyContext(display, context);
    }
};


class XlibWindowSystem : public WindowSystem
{
private:
    Display *display;
    int screen;

public:
    Drawable *currentDrawable;
    Context *currentContext;

    XlibWindowSystem() {
       display = XOpenDisplay(NULL);
       screen = DefaultScreen(display);
    }

    ~XlibWindowSystem() {
        XCloseDisplay(display);
    }

    Visual *
    createVisual(bool doubleBuffer) {
        int single_attribs[] = {
            GLX_RGBA,
            GLX_RED_SIZE, 1,
            GLX_GREEN_SIZE, 1,
            GLX_BLUE_SIZE, 1,
            GLX_DEPTH_SIZE, 1,
            None
        };

        int double_attribs[] = {
            GLX_RGBA,
            GLX_RED_SIZE, 1,
            GLX_GREEN_SIZE, 1,
            GLX_BLUE_SIZE, 1,
            GLX_DOUBLEBUFFER,
            GLX_DEPTH_SIZE, 1,
            None
        };

        XVisualInfo *visinfo;
        
        visinfo = glXChooseVisual(display, screen, doubleBuffer ? double_attribs : single_attribs);

        return new XlibVisual(visinfo);
    }
    
    Drawable *
    createDrawable(const Visual *visual)
    {
        XVisualInfo *visinfo = dynamic_cast<const XlibVisual *>(visual)->visinfo;

        Window root = RootWindow(display, screen);

        /* window attributes */
        XSetWindowAttributes attr;
        attr.background_pixel = 0;
        attr.border_pixel = 0;
        attr.colormap = XCreateColormap(display, root, visinfo->visual, AllocNone);
        attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
        
        unsigned long mask;
        mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

        int x = 0, y = 0, width = 256, height = 256;

        Window window = XCreateWindow(
            display, root, 
            x, y, width, height,
            0, 
            visinfo->depth, 
            InputOutput,
            visinfo->visual, 
            mask, 
            &attr);

        XSizeHints sizehints;
        sizehints.x = x;
        sizehints.y = y;
        sizehints.width  = width;
        sizehints.height = height;
        sizehints.flags = USSize | USPosition;
        XSetNormalHints(display, window, &sizehints);
        
        const char *name = "glretrace";
        XSetStandardProperties(
            display, window, name, name,
            None, (char **)NULL, 0, &sizehints);

        XMapWindow(display, window);
        
        return new XlibDrawable(visual, display, window);
    }

    Context *
    createContext(const Visual *visual)
    {
        XVisualInfo *visinfo = dynamic_cast<const XlibVisual *>(visual)->visinfo;
        GLXContext context = glXCreateContext(display, visinfo, NULL, True);
        return new XlibContext(visual, display, context);
    }

    bool
    makeCurrent(Drawable *drawable, Context *context)
    {
        Window win = drawable ? dynamic_cast<XlibDrawable *>(drawable)->window : NULL;
        GLXContext ctx = context ? dynamic_cast<XlibContext *>(context)->context : NULL;

        bool ret = glXMakeCurrent(display, win, ctx);

        if (drawable && context && ret) {
            currentDrawable = drawable;
            currentContext = context;
        } else {
            currentDrawable = NULL;
            currentContext = NULL;
        }

        return ret;
    }

    bool
    processEvents(void) {
      while (XPending(display) > 0) {
         XEvent event;
         XNextEvent(display, &event);
         // TODO
      }
      return true;
    }
};


WindowSystem *createNativeWindowSystem(void) {
    return new XlibWindowSystem();
}


} /* namespace glretrace */
