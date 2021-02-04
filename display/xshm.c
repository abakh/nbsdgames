#include <Python.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

typedef struct {
  XImage* m_shm_image;
  XShmSegmentInfo m_shminfo;
  int m_width, m_height;
} XImage_Shm;

typedef struct {
  PyObject_HEAD
  Display* dpy;
  int default_scr;
  Window root, win;
  int width, height;
  XVisualInfo visual_info;
  GC gc, gc_and, gc_or;
  XImage_Shm plane;
  Pixmap backpixmap;
  int shmmode;
  int selectinput;
  PyObject* keyevents;
  PyObject* mouseevents;
  PyObject* motionevent;
} DisplayObject;

typedef struct {
  PyObject_HEAD
  DisplayObject* dpy;
  int width, height;
  Pixmap mask;
  Pixmap handle;
} XPixmapObject;


#define DisplayObject_Check(v)	((v)->ob_type == &Display_Type)
staticforward PyTypeObject Display_Type;
staticforward PyTypeObject XPixmap_Type;


static void pixmap_dealloc(XPixmapObject* pm)
{
  if (pm->dpy->dpy)
    {
      if (pm->mask != (Pixmap) -1)
        XFreePixmap(pm->dpy->dpy, pm->mask);
      XFreePixmap(pm->dpy->dpy, pm->handle);
    }
  Py_DECREF(pm->dpy);
  PyObject_Del(pm);
}

static XPixmapObject* new_pixmap(DisplayObject* self, int w, int h, int withmask)
{
  XPixmapObject* pm = PyObject_New(XPixmapObject, &XPixmap_Type);
  if (pm != NULL)
    {
      Py_INCREF(self);
      pm->dpy = self;
      pm->width = w;
      pm->height = h;
      pm->handle = XCreatePixmap(self->dpy, self->win, w, h,
                                 self->visual_info.depth);
      if (withmask)
        pm->mask = XCreatePixmap(self->dpy, self->win, w, h,
                                 self->visual_info.depth);
      else
        pm->mask = (Pixmap) -1;
    }
  return pm;
}


static void flush(DisplayObject* self)
{
  XSync(self->dpy, False);
}

static int create_shm_image(DisplayObject* self, XImage_Shm* img,
                            int width, int height)
{
  int image_size = 4*width*height;
	
  if (XShmQueryExtension(self->dpy) == False)
    /* does we have the extension at all? */
    return 0;
	
  img->m_shm_image = XShmCreateImage(
		self->dpy,
		self->visual_info.visual,
		self->visual_info.depth,
		ZPixmap,
		NULL,
		&img->m_shminfo,
		width,
		height);
  if (img->m_shm_image == NULL)
    return 0;
  img->m_width = width;
  img->m_height = height;

  /* Create shared memory segment: */
  img->m_shminfo.shmid = shmget(IPC_PRIVATE, image_size, IPC_CREAT|0777);
  if (img->m_shminfo.shmid < 0)
    return 0;
	
  /* Get memory address to segment: */
  img->m_shminfo.shmaddr = (char *) shmat(img->m_shminfo.shmid, 0, 0);

  /* Mark the segment as destroyable (it will be destroyed when this
     process terminates) */
  shmctl(img->m_shminfo.shmid, IPC_RMID, NULL);

  /* Tell XServer that it may only read from it and attach to display: */
  img->m_shminfo.readOnly = True;
  XShmAttach (self->dpy, &img->m_shminfo);

  /* Fill the XImage struct: */
  img->m_shm_image->data = img->m_shminfo.shmaddr;
  return 1;
}

static PyObject* new_display(PyObject* dummy, PyObject* args)
{
  DisplayObject* self;
  XSetWindowAttributes attr;
  int width, height, use_shm=1;
  if (!PyArg_ParseTuple(args, "ii|i", &width, &height, &use_shm))
    return NULL;

  self = PyObject_New(DisplayObject, &Display_Type);
  if (self == NULL)
    return NULL;

  self->dpy = XOpenDisplay(NULL);
  if (self->dpy == NULL) goto err;
  self->default_scr = DefaultScreen(self->dpy);
  self->root = RootWindow(self->dpy, self->default_scr);
  self->width = width;
  self->height = height;

  if (!XMatchVisualInfo(self->dpy, self->default_scr,
                        DefaultDepth(self->dpy,self->default_scr), TrueColor,
                        &self->visual_info)) goto err2;

  /* set window attributes */
  memset(&attr, 0, sizeof(attr));
  attr.override_redirect = False;
  attr.background_pixel = BlackPixel(self->dpy, self->default_scr);
  attr.backing_store = NotUseful;

  /* Create the window */
  self->win = XCreateWindow(
		self->dpy,
		self->root,
		0,
		0,
		width,
		height,
		0,
		CopyFromParent,
		CopyFromParent,
		self->visual_info.visual,
		CWOverrideRedirect | CWBackPixel | CWBackingStore,
		&attr);
  if (self->win == (Window) -1) goto err2;

  XMapRaised(self->dpy, self->win);
  
  self->shmmode = use_shm &&
    create_shm_image(self, &self->plane, width, height);
  
  self->gc = XCreateGC(self->dpy, self->win, 0, 0);
  if (!self->shmmode)
    {
      self->backpixmap = XCreatePixmap(self->dpy, self->root,
                                       width, height, self->visual_info.depth);
      if (self->backpixmap == (Pixmap) -1) goto err2;

      self->gc_and = XCreateGC(self->dpy, self->win, 0, 0);
      self->gc_or = XCreateGC(self->dpy, self->win, 0, 0);
      XSetForeground(self->dpy, self->gc, attr.background_pixel);
      XSetFunction(self->dpy, self->gc_and, GXand);
      XSetFunction(self->dpy, self->gc_or, GXor);
    }
  
  self->selectinput = 0;
  self->keyevents = NULL;
  self->mouseevents = NULL;
  self->motionevent = NULL;

  flush(self);
  return (PyObject*) self;

 err2:
  XCloseDisplay(self->dpy);
 err:
  Py_DECREF(self);
  PyErr_SetString(PyExc_IOError, "cannot open X11 display");
  return NULL;
}

static void display_close(DisplayObject* self)
{
  if (self->dpy)
    {
      XCloseDisplay(self->dpy);
      self->dpy = NULL;
    }
}

static void display_dealloc(DisplayObject* self)
{
  display_close(self);
  Py_XDECREF(self->keyevents);
  Py_XDECREF(self->mouseevents);
  Py_XDECREF(self->motionevent);
  PyObject_Del(self);
}

static PyObject* display_close1(DisplayObject* self, PyObject* args)
{
  display_close(self);
  Py_INCREF(Py_None);
  return Py_None;
}

static int checkopen(DisplayObject* self)
{
  if (self->dpy)
    return 1;
  PyErr_SetString(PyExc_IOError, "X11 connexion already closed");
  return 0;
}

static unsigned char* get_dpy_data(DisplayObject* self)
{
  unsigned char* result;
  if (!checkopen(self))
    return NULL;
  result = (unsigned char*)(self->plane.m_shminfo.shmaddr);
  if (!result)
    PyErr_SetString(PyExc_IOError, "X11 SHM failed");
  return result;
}

static PyObject* display_clear1(DisplayObject* self, PyObject* args)
{
  if (self->shmmode)
    {
      unsigned char* data = get_dpy_data(self);
      if (data == NULL)
        return NULL;
      memset(data, 0,
	     ( self->plane.m_shm_image->bits_per_pixel/8
	       *self->width*self->height ) );
    }
  else
    {
      if (!checkopen(self))
        return NULL;
      XFillRectangle(self->dpy, self->backpixmap, self->gc,
                     0, 0, self->width, self->height);
    }
  Py_INCREF(Py_None);
  return Py_None;
}

inline void pack_pixel(unsigned char *data, int r, int g, int b,
		       int depth, int bytes_per_pixel)
{
  unsigned short pixel = 0;
  switch( depth )
    {
      /* No True color below 15 bits per pixel */
    case 15:
      pixel = ((r<<7) & 0x7c00) | ((g<<2) & 0x03e0) | ((b>>3) & 0x001f);
      data[0] = (pixel) & 0xff;
      data[1] = (pixel>>8) & 0xff;
      break;
    case 16:
      /* assumes 5,6,5 model. */
      pixel = ((r<<8) & 0xf800) | ((g<<3) & 0x07e0) | ((b>>3) & 0x001f);
      data[0] = (pixel) & 0xff;
      data[1] = (pixel>>8) & 0xff;
      break;
    case 24:
      if( bytes_per_pixel == 3 )
	{
	  data[0] = b;
	  data[1] = g;
	  data[2] = r;
	  break;
	}
      /* else it's on 32 bits. Drop into depth of 32. */
    case 32:
      *((long *)data) = (r<<16) | (g<<8) | b;
      break;
    }
}

static PyObject* display_pixmap1(DisplayObject* self, PyObject* args)
{
  int w,h;
  unsigned char* input = NULL;
  int length;
  long keycol = -1;

  if (!checkopen(self))
    return NULL;
  if (!PyArg_ParseTuple(args, "ii|s#l", &w, &h, &input, &length, &keycol))
    return NULL;

  if (self->shmmode)
    {
      int x, y;
      int bytes_per_pixel = self->plane.m_shm_image->bits_per_pixel/8;
      int countblocks, countpixels;
      PyObject* result;
      PyObject* strblocks;
      PyObject* strpixels;
      unsigned int* pblocks;
      unsigned char* ppixels;
      unsigned char* input1;

      if (input == NULL)
        {
          Py_INCREF(Py_None);
          return Py_None;
        }
      if (3*w*h != length)
	{
	  PyErr_SetString(PyExc_ValueError, "bad string length");
          return NULL;
	}

      /* Convert the image to our internal format.
         See display_putppm1() for a description of the format.
      */

      countblocks = 0;
      countpixels = 0;
      input1 = input;
      for (y=0; y<h; y++)
        {
          int opaque = 0;
          for (x=0; x<w; x++)
            {
              unsigned int r = input1[0];
              unsigned int g = input1[1];
              unsigned int b = input1[2];
              input1 += 3;
              if (((r<<16)|(g<<8)|b) == keycol)
                opaque = 0;
              else
                {
                  if (!opaque)
                    {
                      countblocks++;  /* start a new block */
                      opaque = 1;
                    }
                  countpixels++;
                }
            }
          countblocks++;  /* end-of-line marker block */
        }

      /* allocate memory */
      strblocks = PyString_FromStringAndSize(NULL,
                                             countblocks*sizeof(int));
      if (strblocks == NULL)
        return NULL;
      strpixels = PyString_FromStringAndSize(NULL,
                                             countpixels*bytes_per_pixel);
      if (strpixels == NULL)
        {
          Py_DECREF(strblocks);
          return NULL;
        }

      /* write data */
      pblocks = (unsigned int*) PyString_AS_STRING(strblocks);
      ppixels = (unsigned char*) PyString_AS_STRING(strpixels);
      for (y=0; y<h; y++)
        {
          int opaque = 0;
          for (x=0; x<w; x++)
            {
              unsigned int r = input[0];
              unsigned int g = input[1];
              unsigned int b = input[2];
              input += 3;
              if (((r<<16)|(g<<8)|b) == keycol)
                opaque = 0;
              else
                {
                  if (!opaque)
                    {
                      *pblocks++ = x*bytes_per_pixel;  /* start a new block */
                      opaque = 1;
                    }
                  pblocks[-1] += bytes_per_pixel<<16;  /* add pixel to block */
                  pack_pixel(ppixels, r, g, b,
                             self->visual_info.depth, bytes_per_pixel);
                  ppixels += bytes_per_pixel;
                }
            }
          *pblocks++ = 0;  /* end-of-line marker block */
        }
      
      result = Py_BuildValue("iiOO", w, h, strblocks, strpixels);
      Py_DECREF(strblocks);
      Py_DECREF(strpixels);
      return result;
    }
  else
    {
      XImage* image;
      long extent;
      unsigned char* data = NULL;
      unsigned char* maskdata = NULL;
      int scanline, bitmap_pad;
      XPixmapObject* pm;

      pm = new_pixmap(self, w, h, keycol>=0);
      if (pm == NULL)
        return NULL;

      if (input == NULL)
        return (PyObject*) pm;   /* uninitialized pixmap */
  
      extent = w*h;
      if (3*extent != length)
        {
          PyErr_SetString(PyExc_ValueError, "bad string length");
          goto err;
        }

      bitmap_pad = self->visual_info.depth >= 24 ? 32 : 16;
      scanline = ((w+bitmap_pad-1) & ~(bitmap_pad-1)) / 8;
      /*while (scanline&3) scanline++;*/
      data = malloc(self->visual_info.depth*scanline*h);
      if (data == NULL)
        {
          PyErr_NoMemory();
          goto err;
        }
      memset(data, 0, self->visual_info.depth*scanline*h);
      maskdata = malloc(self->visual_info.depth*scanline*h);
      if (maskdata == NULL)
        {
          PyErr_NoMemory();
          goto err;
        }
      memset(maskdata, 0, self->visual_info.depth*scanline*h);

      {
        int key_r = keycol>>16;
        unsigned char key_g = keycol>>8;
        unsigned char key_b = keycol>>0;
        unsigned char* target = data;
        unsigned char* masktarget = maskdata;
        int plane, color;

        unsigned int p_size[3];
        switch( self->visual_info.depth )
          {
          case 15:
            p_size[0] = p_size[1] = p_size[2] = 5;
            break;
          case 16:
            p_size[0] = p_size[2] = 5;
            p_size[1] = 6;
            break;
          case 24:
          case 32:
            p_size[0] = p_size[1] = p_size[2] = 8;
            break;
          }

        for (color=0; color<3; color++)
          for (plane=128; plane>=(1<<(8-p_size[color])); plane/=2)
            {
              unsigned char* src = input;
              int x, y;
              for (y=0; y<h; y++, target+=scanline, masktarget+=scanline)
                for (x=0; x<w; x++, src+=3)
                  {
                    if (src[0] == key_r && src[1] == key_g && src[2] == key_b)
                      {
                        /* transparent */
                        masktarget[x/8] |= (1<<(x&7));
                      }
                    else
                      if (src[color] & plane)
                        target[x/8] |= (1<<(x&7));
                  }
            }
      }

      if (keycol < 0)
        free(maskdata);
      else
        {
          image = XCreateImage(self->dpy, self->visual_info.visual,
                               self->visual_info.depth, XYPixmap, 0,
                               maskdata, w, h,
                               bitmap_pad, scanline);
          if (image == NULL || image == (XImage*) -1)
            {
              PyErr_SetString(PyExc_IOError, "XCreateImage failed (2)");
              goto err;
            }
          image->byte_order = LSBFirst;
          image->bitmap_bit_order = LSBFirst;
          maskdata = NULL;
          XPutImage(self->dpy, pm->mask, self->gc, image, 0, 0, 0, 0, w, h);
          XDestroyImage(image);
        }
  
      image = XCreateImage(self->dpy, self->visual_info.visual,
                           self->visual_info.depth, XYPixmap, 0,
                           data, w, h,
                           bitmap_pad, scanline);
      if (image == NULL || image == (XImage*) -1)
        {
          PyErr_SetString(PyExc_IOError, "XCreateImage failed");
          goto err;
        }
      image->byte_order = LSBFirst;
      image->bitmap_bit_order = LSBFirst;
      data = NULL;
      XPutImage(self->dpy, pm->handle, self->gc, image, 0, 0, 0, 0, w, h);
      XDestroyImage(image);

      return (PyObject*) pm;

    err:
      free(maskdata);
      free(data);
      Py_DECREF(pm);
      return NULL;
    }
}

static PyObject* display_get(DisplayObject* self, int x, int y, int w, int h)
{
  if (self->shmmode)
    {
      int clipx=0, clipy=0, clipw=self->width, cliph=self->height;
      int original_w, original_h;
      int firstline=0, firstcol=0;
      unsigned int bytes_per_pixel = self->plane.m_shm_image->bits_per_pixel/8;
      unsigned char* data = get_dpy_data(self);
      if (!data)
        return NULL;

      original_w = w;
      original_h = h;
      if (x<clipx) { firstcol=clipx-x; w+=x-clipx; x=clipx; }
      if (y<clipy) { firstline=clipy-y; h+=y-clipy; y=clipy; }
      if (x+w > clipw) w = clipw-x;
      if (y+h > cliph) h = cliph-y;

      {
        int countblocks = original_h + ((w>0 && h>0) ? h : 0);
        /* end blocks + real blocks */
        int countpixels = (w>0 && h>0) ? w * h : 0;
        PyObject* result;
        PyObject* strblocks;
        PyObject* strpixels;
        unsigned int* pblocks;
        unsigned char* ppixels;
        int wbytes = w * bytes_per_pixel;
        int block = (firstcol * bytes_per_pixel) | (wbytes << 16);
        int data_scanline = bytes_per_pixel*self->width;

        /* allocate memory */
        strblocks = PyString_FromStringAndSize(NULL,
                                               countblocks*sizeof(int));
        if (strblocks == NULL)
          return NULL;
        strpixels = PyString_FromStringAndSize(NULL,
                                               countpixels*bytes_per_pixel);
        if (strpixels == NULL)
          {
            Py_DECREF(strblocks);
            return NULL;
          }
          
        /* write data */
        pblocks = (unsigned int*) PyString_AS_STRING(strblocks);
        ppixels = (unsigned char*) PyString_AS_STRING(strpixels);
        data += bytes_per_pixel*(x+y*self->width);
        for (y=0; y<original_h; y++)
          {
            if (y >= firstline && y < firstline+h && w > 0)
              {
                *pblocks++ = block;
                memcpy(ppixels, data, wbytes);
                ppixels += wbytes;
                data += data_scanline;
              }
            *pblocks++ = 0;
          }
          
        result = Py_BuildValue("iiOO", original_w, original_h,
                               strblocks, strpixels);
        Py_DECREF(strblocks);
        Py_DECREF(strpixels);
        return result;
      }
    }
  else
    {
      XPixmapObject* pm = new_pixmap(self, w, h, 0);
      if (pm != NULL)
        XCopyArea(self->dpy, self->backpixmap, pm->handle, self->gc,
                  x, y, w, h, 0, 0);
      return (PyObject*) pm;
    }
}

static PyObject* save_background(DisplayObject* self, int x, int y,
                                 int w, int h, int save_bkgnd)
{
  if (save_bkgnd)
    {
      PyObject* pm = display_get(self, x, y, w, h);
      PyObject* result;
      if (pm == NULL)
        return NULL;
      result = Py_BuildValue("iiO", x, y, pm);
      Py_DECREF(pm);
      return result;
    }
  else
    {
      Py_INCREF(Py_None);
      return Py_None;
    }
}

#define ALPHAFACTOR 2
#define ALPHABLEND(maximum, x, y)   ((maximum-y)*x/(maximum*ALPHAFACTOR) + y)

static void memcpy_alpha_32(unsigned int* dst, unsigned int* src, int count)
{
  int i;
  for (i=0; i<count/4; i++)
    {
      int x = dst[i];
      int y = src[i];

      int xr = x >> 16;
      int xg = x & 0xff00;
      int xb = x & 0xff;

      int yr = y >> 16;
      int yg = y & 0xff00;
      int yb = y & 0xff;

      int zr = ALPHABLEND(0xff,   xr, yr);
      int zg = ALPHABLEND(0xff00, xg, yg);
      int zb = ALPHABLEND(0xff,   xb, yb);
      
      dst[i] = (zr << 16) | (zg & 0xff00) | zb;
    }
}

static void memcpy_alpha_24(unsigned char* dst, unsigned char* src, int count)
{
  int i;
  for (i=0; i<count; i++)
    {
      int x = dst[i];
      int y = src[i];
      dst[i] = ALPHABLEND(255, x, y);
    }
}

static void memcpy_alpha_15(unsigned short* dst, unsigned short* src, int count)
{
  int i;
  for (i=0; i<count/2; i++)
    {
      unsigned short x = dst[i];
      unsigned short y = src[i];

      int xr = x >> 10;
      int xg = x & 0x03e0;
      int xb = x & 0x001f;

      int yr = y >> 10;
      int yg = y & 0x03e0;
      int yb = y & 0x001f;

      int zr = ALPHABLEND(31, xr, yr);
      int zg = ALPHABLEND(0x3e0, xg, yg);
      int zb = ALPHABLEND(31, xb, yb);

      dst[i] = (zr << 10) | (zg & 0x03e0) | zb;
    }
}

static void memcpy_alpha_16(unsigned short* dst, unsigned short* src, int count)
{
  int i;
  for (i=0; i<count/2; i++)
    {
      unsigned short x = dst[i];
      unsigned short y = src[i];

      int xr = x >> 11;
      int xg = x & 0x07e0;
      int xb = x & 0x001f;

      int yr = y >> 11;
      int yg = y & 0x07e0;
      int yb = y & 0x001f;

      int zr = ALPHABLEND(31, xr, yr);
      int zg = ALPHABLEND(0x7e0, xg, yg);
      int zb = ALPHABLEND(31, xb, yb);

      dst[i] = (zr << 11) | (zg & 0x07e0) | zb;
    }
}

typedef void (*memcpy_alpha_fn) (unsigned char*, unsigned char*, int);

static PyObject* display_overlay(DisplayObject* self, PyObject* args,
                                 int save_bkgnd)
{
  PyObject* result;
  
  if (self->shmmode)
    {
      int x,y,w,h, original_x, original_y, original_w, original_h;
      int data_scanline;
      int clipx=0, clipy=0, clipw=65536, cliph=65536, alpha=255;
      unsigned int* src;
      unsigned char* srcdata;
      unsigned char* original_srcdata;
      int length1, length2, firstline=0, firstcol=0;
      unsigned int bytes_per_pixel = self->plane.m_shm_image->bits_per_pixel/8;
      memcpy_alpha_fn memcpy_alpha;
      unsigned char* data = get_dpy_data(self);
      if (!PyArg_ParseTuple(args, "ii(iis#s#)|(iiii)i",
                            &x, &y, &w, &h, &src, &length1, &srcdata, &length2,
                            &clipx, &clipy, &clipw, &cliph, &alpha) || !data)
        return NULL;

      original_x = x;
      original_y = y;
      original_w = w;
      original_h = h;
      original_srcdata = srcdata;
      x -= clipx;
      y -= clipy;
      clipx += x;
      clipy += y;
      clipw += clipx;
      cliph += clipy;
      if (clipx<0) clipx=0;
      if (clipy<0) clipy=0;
      if (clipw>self->width) clipw=self->width;
      if (cliph>self->height) cliph=self->height;
      if (x<clipx) { firstcol = clipx-x; w+=x-clipx; x=clipx; }
      if (y<clipy) { firstline = clipy-y; h+=y-clipy; y=clipy; }
      if (x+w > clipw) w = clipw-x;
      if (y+h > cliph) h = cliph-y;
      if (w > 0 && h > 0)
        {
          int dstoffset, blocksize;
          unsigned int block;
          data += bytes_per_pixel*(x+y*self->width);
          data_scanline = bytes_per_pixel*self->width;

          memcpy_alpha = (memcpy_alpha_fn) memcpy;
          if (alpha < 255)
            switch (self->visual_info.depth) {
            case 15: memcpy_alpha = (memcpy_alpha_fn) memcpy_alpha_15; break;
            case 16: memcpy_alpha = (memcpy_alpha_fn) memcpy_alpha_16; break;
            case 24: memcpy_alpha = (memcpy_alpha_fn) memcpy_alpha_24; break;
            case 32: memcpy_alpha = (memcpy_alpha_fn) memcpy_alpha_32; break;
            }

          /* 'structure' points to a sequence of int-sized blocks with the
             following meaning:

             n & 0xFFFF  -- byte offset within the line
             n >> 16     -- number of opaque bytes to copy there

             n == 0 means end of line.
          */
          
          /* read and ignore 'firstline' complete lines */
          while (firstline--)
            {
              while ((block = *src++) != 0)
                {
                  blocksize = block >> 16;
                  srcdata += blocksize;
                }
            }

          if (w == original_w)
            {
              if (!save_bkgnd)
                {
                  /* common fast case: copy the whole width of the image */
                  do
                    {
                      while ((block = *src++) != 0)
                        {
                          dstoffset = block & 0xFFFF;
                          blocksize = block >> 16;
                          memcpy(data + dstoffset, srcdata, blocksize);
                          srcdata += blocksize;
                        }
                      data += data_scanline;
                    }
                  while (--h);
                  result = Py_None;
                  Py_INCREF(result);
                }
              else
                {
                  /* copy and save the background */
                  PyObject* cliprect;
                  PyObject* strblocks;
                  PyObject* strpixels;
                  unsigned char* ppixels;
                  
                  strpixels = PyString_FromStringAndSize(NULL, length2);
                  if (strpixels == NULL)
                    return NULL;
                  ppixels = (unsigned char*) PyString_AS_STRING(strpixels);
                  ppixels += srcdata - original_srcdata;
                  
                  do
                    {
                      while ((block = *src++) != 0)
                        {
                          dstoffset = block & 0xFFFF;
                          blocksize = block >> 16;
                          memcpy(ppixels, data + dstoffset, blocksize);
                          ppixels += blocksize;
                          memcpy_alpha(data + dstoffset, srcdata, blocksize);
                          srcdata += blocksize;
                        }
                      data += data_scanline;
                    }
                  while (--h);

                  strblocks = PyTuple_GET_ITEM(PyTuple_GET_ITEM(args, 2), 2);
                  if (PyTuple_GET_SIZE(args) > 3)
                    {
                      cliprect = PyTuple_GET_ITEM(args, 3);
                      result = Py_BuildValue("ii(iiOO)O",
                                             original_x,
                                             original_y,
                                             original_w,
                                             original_h,
                                             strblocks,
                                             strpixels,
                                             cliprect);
                    }
                  else
                    {
                      result = Py_BuildValue("ii(iiOO)",
                                             original_x,
                                             original_y,
                                             original_w,
                                             original_h,
                                             strblocks,
                                             strpixels);
                    }
                  Py_DECREF(strpixels);
                }
            }
          else
            {
              /* byte offsets within a line */
              unsigned char* blocksrc;
              int skip, lastcol;

              result = save_background(self, x, y, w, h, save_bkgnd);

              lastcol = (firstcol + w) * bytes_per_pixel;
              firstcol *= bytes_per_pixel;
              
              /* slow case: only copy a portion of the width of the image */
              data -= firstcol;
              do
                {
                  while ((block = *src++) != 0)
                    {
                      dstoffset = block & 0xFFFF;
                      blocksize = block >> 16;
                      blocksrc = srcdata;
                      srcdata += blocksize;
                      skip = firstcol - dstoffset;
                      if (skip < 0)
                        skip = 0;
                      if (blocksize > lastcol - dstoffset)
                        blocksize = lastcol - dstoffset;
                      if (blocksize > skip)
                        memcpy_alpha(data + dstoffset + skip, blocksrc + skip,
                                     blocksize - skip);
                    }
                  data += data_scanline;
                }
              while (--h);
            }
        }
      else
        {
          result = args;
          Py_INCREF(result);
        }
    }
  else
    {
      int x,y, x1=0,y1=0,w1=-1,h1=-1,alpha;
      XPixmapObject* pm;

      if (!checkopen(self))
        return NULL;
      if (!PyArg_ParseTuple(args, "iiO!|(iiii)i", &x, &y, &XPixmap_Type, &pm,
                            &x1, &y1, &w1, &h1, &alpha))
        return NULL;

      if (w1 < 0)
        w1 = pm->width;
      if (h1 < 0)
        h1 = pm->height;

      result = save_background(self, x, y, w1, h1, save_bkgnd);

      if (pm->mask == (Pixmap) -1)
        {
          XCopyArea(self->dpy, pm->handle, self->backpixmap, self->gc,
                    x1, y1, w1, h1, x, y);
        }
      else
        {
          XCopyArea(self->dpy, pm->mask, self->backpixmap, self->gc_and,
                    x1, y1, w1, h1, x, y);
          XCopyArea(self->dpy, pm->handle, self->backpixmap, self->gc_or,
                    x1, y1, w1, h1, x, y);
        }
    }
  return result;
}

static PyObject* display_putppm1(DisplayObject* self, PyObject* args)
{
  return display_overlay(self, args, 0);
}

static PyObject* display_overlayppm1(DisplayObject* self, PyObject* args)
{
  return display_overlay(self, args, 1);
}

static PyObject* display_getppm1(DisplayObject* self, PyObject* args)
{
  int x, y, w, h;
  if (!checkopen(self))
    return NULL;
  if (!PyArg_ParseTuple(args, "(iiii)", &x, &y, &w, &h))
    return NULL;
  return display_get(self, x, y, w, h);
}

static int readXevents(DisplayObject* self)
{
  while (XEventsQueued(self->dpy, QueuedAfterReading) > 0)
    {
      XEvent e;
      XNextEvent(self->dpy, &e);
      switch (e.type) {
      case KeyPress:
      case KeyRelease:
        {
	  KeySym sym;
	  PyObject* v;
          int err;
          if (self->keyevents == NULL)
            {
              self->keyevents = PyList_New(0);
              if (self->keyevents == NULL)
                return 0;
            }
	  sym = XLookupKeysym(&e.xkey,0);
	  v = Py_BuildValue("ii", sym, e.type);
          if (v == NULL)
            return 0;
	  err = PyList_Append(self->keyevents, v);
	  Py_DECREF(v);
	  if (err)
            return 0;
          break;
        }
      case ButtonPress:
        {
	  PyObject* v;
          int err;
          if (self->mouseevents == NULL)
            {
              self->mouseevents = PyList_New(0);
              if (self->mouseevents == NULL)
                return 0;
            }
	  v = Py_BuildValue("ii", e.xbutton.x, e.xbutton.y);
          if (v == NULL)
            return 0;
	  err = PyList_Append(self->mouseevents, v);
	  Py_DECREF(v);
	  if (err)
            return 0;
          break;
        }
      case MotionNotify:
        {
          Py_XDECREF(self->motionevent);
          self->motionevent = Py_BuildValue("ii", e.xmotion.x, e.xmotion.y);
          if (self->motionevent == NULL)
            return 0;
          break;
        }
      }
    }
  return 1;
}

#define ENABLE_EVENTS(mask)     do {                            \
  if (!(self->selectinput & (mask)))                            \
    {                                                           \
      self->selectinput |= (mask);                              \
      XSelectInput(self->dpy, self->win, self->selectinput);    \
    }                                                           \
} while (0)

static PyObject* display_keyevents1(DisplayObject* self, PyObject* args)
{
  PyObject* result;
  ENABLE_EVENTS(KeyPressMask|KeyReleaseMask);
  if (!readXevents(self))
    return NULL;
  result = self->keyevents;
  if (result == NULL)
    result = PyList_New(0);
  else
    self->keyevents = NULL;
  return result;
}

static PyObject* display_mouseevents1(DisplayObject* self, PyObject* args)
{
  PyObject* result;
  ENABLE_EVENTS(ButtonPressMask);
  result = self->mouseevents;
  if (result == NULL)
    result = PyList_New(0);
  else
    self->mouseevents = NULL;
  return result;
}

static PyObject* display_pointermotion1(DisplayObject* self, PyObject* args)
{
  PyObject* result;
  ENABLE_EVENTS(PointerMotionMask);
  result = self->motionevent;
  if (result == NULL)
    {
      Py_INCREF(Py_None);
      result = Py_None;
    }
  else
    self->motionevent = NULL;
  return result;
}

static PyObject* display_flip1(DisplayObject* self, PyObject* args)
{
  if (!checkopen(self))
    return NULL;

  if (self->shmmode)
    {
      XShmPutImage(self->dpy, self->win, self->gc,
                   self->plane.m_shm_image,
                   0, 0, 0, 0,
                   self->plane.m_width,
                   self->plane.m_height,
                   False);
    }
  else
    {
      XCopyArea(self->dpy, self->backpixmap, self->win, self->gc,
                0, 0, self->width, self->height, 0, 0);
    }
  flush(self);
  if (!readXevents(self))
    return NULL;
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* display_fd1(DisplayObject* self, PyObject *args)
{
  return PyInt_FromLong(ConnectionNumber(self->dpy));
}

static PyObject* display_shmmode(DisplayObject* self, PyObject *args)
{
  return PyInt_FromLong(self->shmmode);
}

static PyMethodDef display_methods[] = {
  {"close",    (PyCFunction)display_close1,    METH_VARARGS,  NULL},
  {"flip",     (PyCFunction)display_flip1,     METH_VARARGS,  NULL},
  {"clear",    (PyCFunction)display_clear1,    METH_VARARGS,  NULL},
  {"pixmap",   (PyCFunction)display_pixmap1,   METH_VARARGS,  NULL},
  {"putppm",   (PyCFunction)display_putppm1,   METH_VARARGS,  NULL},
  {"getppm",   (PyCFunction)display_getppm1,   METH_VARARGS,  NULL},
  {"overlayppm",(PyCFunction)display_overlayppm1, METH_VARARGS, NULL},
  {"keyevents",(PyCFunction)display_keyevents1,METH_VARARGS,  NULL},
  {"mouseevents",(PyCFunction)display_mouseevents1,METH_VARARGS,NULL},
  {"pointermotion",(PyCFunction)display_pointermotion1,METH_VARARGS,NULL},
  {"fd",       (PyCFunction)display_fd1,       METH_VARARGS,  NULL},
  {"shmmode",  (PyCFunction)display_shmmode,   METH_VARARGS,  NULL},
  {NULL,		NULL}		/* sentinel */
};

static PyObject* display_getattr(DisplayObject* self, char* name)
{
  return Py_FindMethod(display_methods, (PyObject*)self, name);
}


statichere PyTypeObject Display_Type = {
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"Display",		/*tp_name*/
	sizeof(DisplayObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)display_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)display_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
};

statichere PyTypeObject XPixmap_Type = {
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"Pixmap",		/*tp_name*/
	sizeof(XPixmapObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)pixmap_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
};


static PyMethodDef ShmMethods[] = {
           {"Display",  new_display,  METH_VARARGS},
           {NULL,       NULL}         /* Sentinel */
       };

void initxshm(void)
{
  Display_Type.ob_type = &PyType_Type;
  XPixmap_Type.ob_type = &PyType_Type;
  Py_InitModule("xshm", ShmMethods);
}
