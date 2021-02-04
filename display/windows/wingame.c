#include <Python.h>
#include <windows.h>
#include <mmsystem.h>


 /************************** DISPLAY PART ***************************/

typedef struct {
	BITMAPINFOHEADER bmiHeader;
	union {
		DWORD bmiMask[3];
		short bmiIndices[255];
	};
} screenbmp_t;

typedef struct {
  PyObject_HEAD
  HWND win;
  int width, height, bpp;
  HDC dc;
  HPALETTE hpal, hprevpal;
  screenbmp_t screenbmpinfo;
  unsigned char* screenbmpdata;
  unsigned char* screenfirstline;
  int screenscanline;   /* < 0 ! */
  PyObject* keyevents;
  PyObject* mouseevents;
  PyObject* motionevent;
} DisplayObject;

#define DisplayObject_Check(v)	((v)->ob_type == &Display_Type)
staticforward PyTypeObject Display_Type;


static void flush(DisplayObject* self)
{
  /*GdiFlush();*/
}

static void release_window_data(DisplayObject* self)
{
  if (self->hprevpal)
    {
      SelectPalette(self->dc, self->hprevpal, FALSE);
      self->hprevpal = (HPALETTE) NULL;
    }
  if (self->hpal)
    {
      DeleteObject(self->hpal);
      self->hpal = (HPALETTE) NULL;
    }
  if (self->dc && self->win)
    {
      ReleaseDC(self->win, self->dc);
      self->dc = (HDC) NULL;
    }
}

static void display_close(DisplayObject* self)
{
  release_window_data(self);
  if (self->win)
    {
      SetWindowLong(self->win, 0, 0);
      DestroyWindow(self->win);
      self->win = (HWND) NULL;
    }
}

static LRESULT CALLBACK display_proc(HWND hwnd, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam)
{
  DisplayObject* self;
  switch (uMsg) {

  case WM_KEYDOWN:
  case WM_KEYUP:
    self = (DisplayObject*) GetWindowLong(hwnd, 0);
    if (self)
      {
        PyObject* v;
        int etype;
        if (self->keyevents == NULL)
          {
            self->keyevents = PyList_New(0);
            if (self->keyevents == NULL)
              break;
          }
        etype = (uMsg == WM_KEYDOWN) ? 2 : 3;
        v = Py_BuildValue("ii", (int) wParam, etype);
        if (v == NULL)
          break;
        PyList_Append(self->keyevents, v);
        Py_DECREF(v);
      }
    break;

  case WM_LBUTTONDOWN:
    self = (DisplayObject*) GetWindowLong(hwnd, 0);
    if (self)
      {
        PyObject* v;
        if (self->mouseevents == NULL)
          {
            self->mouseevents = PyList_New(0);
            if (self->mouseevents == NULL)
              break;
          }
        v = Py_BuildValue("ii", LOWORD(lParam), HIWORD(lParam));
        if (v == NULL)
          break;
        PyList_Append(self->mouseevents, v);
        Py_DECREF(v);
      }
    break;

  case WM_MOUSEMOVE:
    self = (DisplayObject*) GetWindowLong(hwnd, 0);
    if (self)
      {
        Py_XDECREF(self->motionevent);
        self->motionevent = Py_BuildValue("ii", LOWORD(lParam), HIWORD(lParam));
      }
    break;

  case WM_DESTROY:
    self = (DisplayObject*) GetWindowLong(hwnd, 0);
    if (self)
      {
        release_window_data(self);
        self->win = (HWND) NULL;
      }
    break;

  default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }

  return 0;
}

static PyObject* new_display(PyObject* dummy, PyObject* args)
{
  char* CLASSNAME = "winxshm";
  WNDCLASS wcls;
  DisplayObject* self;
  int width, height, bytes, bpp, use_shm=0;
  if (!PyArg_ParseTuple(args, "ii|i", &width, &height, &use_shm))
    return NULL;

  self = PyObject_New(DisplayObject, &Display_Type);
  if (self == NULL)
    return NULL;

  memset(&self->win, 0, ((char*)(self+1)) - ((char*)(&self->win)));
  self->width = width;
  self->height = height;

  /* set window class */
  memset(&wcls, 0, sizeof(wcls));
  wcls.style = CS_BYTEALIGNCLIENT;
  wcls.lpfnWndProc = &display_proc;
  wcls.cbWndExtra = sizeof(DisplayObject*);
  //wcls.hInstance = HINSTANCE;
  wcls.hCursor = LoadCursor(0, IDC_ARROW);
  wcls.lpszClassName = CLASSNAME;
  RegisterClass(&wcls);

  /* Create the window */
  self->win = CreateWindowEx(0, CLASSNAME, NULL,
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                             WS_MINIMIZEBOX | WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             width + 2*GetSystemMetrics(SM_CXFIXEDFRAME), 
			     height + 2*GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION),
                             (HWND) NULL, (HMENU) NULL,
                             /*HINSTANCE*/ 0, (LPVOID) NULL);
  if (self->win == (HWND) NULL) goto err2;
  SetWindowLong(self->win, 0, (long) self);

  /* Create DC */
  self->dc = GetDC(self->win);
  if (!self->dc) goto err2;
  self->bpp = bpp = GetDeviceCaps(self->dc, BITSPIXEL);
  if (bpp == 8)
  {
	struct {
		WORD         palVersion; 
		WORD         palNumEntries; 
		PALETTEENTRY palPalEntry[255];
	} pal;
	pal.palNumEntries = GetSystemPaletteEntries(self->dc, 0, 255, pal.palPalEntry);
	if (pal.palNumEntries != 0)
	{
		int i;
		pal.palVersion = 0x300;
		self->hpal = CreatePalette((LOGPALETTE*)(&pal));
		self->screenbmpinfo.bmiHeader.biClrUsed = pal.palNumEntries;
		self->hprevpal = SelectPalette(self->dc, self->hpal, FALSE);
		for (i=0; i<pal.palNumEntries; i++) {
			self->screenbmpinfo.bmiIndices[i] = i;
		}
	}
  }
  if (bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32 && !self->hpal)
  {
	bpp = 24;   /* default */
	fprintf(stderr, "WARNING: a hi/true color screen mode of 15, 16, 24 or 32 bits per pixels\n");
	fprintf(stderr, "         is highly recommended !\n");
  }

  /* Allocate screen bitmaps */
  bytes = (bpp+7)/8;          /* per pixel */
  bytes = (bytes*width+3)&~3; /* per scan line */
  self->screenscanline = -bytes;
  bytes = bytes*height;       /* for the whole screen */
  self->screenbmpdata = PyMem_Malloc(bytes);
  self->screenfirstline = self->screenbmpdata + bytes + self->screenscanline;
  if (self->screenbmpdata == NULL)
  {
	PyErr_NoMemory();
	goto err2;
  }
  self->screenbmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  self->screenbmpinfo.bmiHeader.biWidth = self->width;
  self->screenbmpinfo.bmiHeader.biHeight = self->height;
  self->screenbmpinfo.bmiHeader.biPlanes = 1;
  self->screenbmpinfo.bmiHeader.biBitCount = (bpp+7)&~7;
  if (bpp == 16)
  {
	self->screenbmpinfo.bmiHeader.biCompression = BI_BITFIELDS;
	self->screenbmpinfo.bmiMask[0] = 0xF800;
	self->screenbmpinfo.bmiMask[1] = 0x07E0;
	self->screenbmpinfo.bmiMask[2] = 0x001F;
  }

  flush(self);
  return (PyObject*) self;

 err2:
  display_close(self);
  Py_DECREF(self);
  if (!PyErr_Occurred())
	PyErr_SetString(PyExc_IOError, "cannot open window");
  return NULL;
}

static void display_dealloc(DisplayObject* self)
{
  display_close(self);
  Py_XDECREF(self->keyevents);
  Py_XDECREF(self->mouseevents);
  Py_XDECREF(self->motionevent);
  PyMem_Free(self->screenbmpdata);
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
  if (self->win)
    return 1;
  //PyErr_SetString(PyExc_IOError, "the window was closed");
  PyErr_SetString(PyExc_SystemExit, "window closed.");
  return 0;
}

static PyObject* display_clear1(DisplayObject* self, PyObject* args)
{
  if (!checkopen(self))
    return NULL;
  memset(self->screenbmpdata, 0, (-self->screenscanline) * self->height);
  Py_INCREF(Py_None);
  return Py_None;
}

static void pack_pixel(DisplayObject* self, unsigned char *data, int r, int g, int b,
		       int depth)
{
  unsigned short pixel;
  switch( depth )
    {
    case 8:
      data[0] = GetNearestPaletteIndex(self->hpal, (b<<16) | (g<<8) | r);
      break;
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
      if( 1 )
	{
	  data[0] = b;
	  data[1] = g;
	  data[2] = r;
	  break;
	}
    case 32:
      *((long *)data) = (r<<16) | (g<<8) | b;
      break;
    }
}

static PyObject* display_pixmap1(DisplayObject* self, PyObject* args)
{
  int w,h;
  int length;
  unsigned char* input = NULL;
  long keycol = -1;

  if (!checkopen(self))
    return NULL;
  if (!PyArg_ParseTuple(args, "ii|s#l", &w, &h, &input, &length, &keycol))
    return NULL;

  if (1)     /* SHM */
    {
      int x, y;
      unsigned char *dst;
      int size, bytes_per_pixel;
      long packed_keycol = keycol;
      PyObject* result;
      PyObject* str;

      if (input == NULL )
        {
          Py_INCREF(Py_None);
          return Py_None;
        }

      bytes_per_pixel = self->screenbmpinfo.bmiHeader.biBitCount/8;
      size = bytes_per_pixel*w*h;

      if( 3*w*h != length )
	{
	  PyErr_SetString(PyExc_TypeError, "bad string length");
	  return NULL;
	}
      /* Create a new string and fill it with the correctly packed image */
      str = PyString_FromStringAndSize(NULL, size);
      if (!str)
        return NULL;
      if (keycol >= 0)
	switch( self->bpp )
	  {
	  case 8:
	    packed_keycol = 0xFF;
	    break;
	  case 15:
	    packed_keycol = (1 << 10) | (1 << 5) | 1;
	    break;
	  case 16:
	    packed_keycol = (1 << 11) | (1 << 5) | 1;
	    break;
	  default:
	    packed_keycol = keycol;
	    break;
	  }
      result = Py_BuildValue("iiOl", w, h, str, packed_keycol);
      Py_DECREF(str);  /* one ref left in 'result' */
      if (!result)
        return NULL;
      dst = (unsigned char*) PyString_AS_STRING(str);
      memset(dst,0,size);

      for( y=0; y<h; y++ )
	for( x=0; x<w; x++, input+=3, dst += bytes_per_pixel )
	  {
	    int r = input[0];
	    int g = input[1];
	    int b = input[2];
	    if( ((r<<16)|(g<<8)|b) == keycol )
	      for( b=0; b<bytes_per_pixel; b++ )
		dst[b] = ((unsigned char *)&packed_keycol)[b];
	    else
	      pack_pixel(self, dst, r, g, b, self->bpp);
	  }
      return result;
    }
}

static PyObject* display_putppm1(DisplayObject* self, PyObject* args)
{
  if (!checkopen(self))
    return NULL;

  if (1)     /* SHM */
    {
      int x,y,w,h,scanline;
      int clipx=0, clipy=0, clipw=65536, cliph=65536;
      unsigned char* src;
      int length;
      long keycol;
      int bytes_per_pixel = self->screenbmpinfo.bmiHeader.biBitCount/8;
      unsigned char* data = self->screenfirstline;
      if (!PyArg_ParseTuple(args, "ii(iis#l)|(iiii)",
                            &x, &y, &w, &h, &src, &length, &keycol,
                            &clipx, &clipy, &clipw, &cliph) || !data)
        return NULL;
  
      scanline = bytes_per_pixel*w;
      if (scanline*h != length)
        {
          PyErr_SetString(PyExc_TypeError, "bad string length");
          return NULL;
        }
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
      if (x<clipx) { src+=(clipx-x)*bytes_per_pixel; w+=x-clipx; x=clipx; }
      if (y<clipy) { src+=(clipy-y)*scanline; h+=y-clipy; y=clipy; }
      if (x+w > clipw) w = clipw-x;
      if (y+h > cliph) h = cliph-y;
      data += bytes_per_pixel*x+y*self->screenscanline;
      while (h>0)
        {
          int i;
	  int b;
          unsigned char* src0 = src;
	  unsigned char* data0 = data;
          if (keycol < 0)
            for (i=0; i<w; i++)
	      for (b=0; b<bytes_per_pixel; b++)
		*data++ = *src++;
          else
	    {
	      unsigned char *keycol_bytes = (unsigned char *)&keycol;
	      for (i=0; i<w; i++)
		{
		  int transparent = 1;
		  for( b=0; b<bytes_per_pixel; b++ )
		    transparent = transparent && (keycol_bytes[b] == src[b]);

		  if (!transparent)
		    for( b=0; b<bytes_per_pixel; b++ )
		      *data++ = *src++;
		  else
		    {
		      data += bytes_per_pixel;
		      src += bytes_per_pixel;
		    }
		}
	    }
          src = src0 + scanline;
          data = data0 + self->screenscanline;
          h--;
        }
    }
  
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* display_getppm1(DisplayObject* self, PyObject* args)
{
  if (!checkopen(self))
    return NULL;

  if (1)     /* SHM */
    {
      int x,y,w,h,scanline;
      int clipx=0, clipy=0, clipw=self->width, cliph=self->height;
      unsigned char* dst;
      int length;
      PyObject* ignored;
      PyObject* result;
      PyObject* str;
      int bytes_per_pixel = self->screenbmpinfo.bmiHeader.biBitCount/8;
      unsigned char* data = self->screenfirstline;
      if (!PyArg_ParseTuple(args, "(iiii)|O", &x, &y, &w, &h,
                            &ignored) || !data)
        return NULL;

      scanline = bytes_per_pixel*w;
      length = scanline*h;
      str = PyString_FromStringAndSize(NULL, length);
      if (!str)
        return NULL;
      result = Py_BuildValue("iiOl", w, h, str, -1);
      Py_DECREF(str);  /* one ref left in 'result' */
      if (!result)
        return NULL;
      dst = (unsigned char*) PyString_AS_STRING(str);

      if (x<clipx) { dst+=(clipx-x)*bytes_per_pixel; w+=x-clipx; x=clipx; }
      if (y<clipy) { dst+=(clipy-y)*scanline; h+=y-clipy; y=clipy; }
      if (x+w > clipw) w = clipw-x;
      if (y+h > cliph) h = cliph-y;
      data += bytes_per_pixel*x+y*self->screenscanline;
      while (h>0)
        {
          int i;
	  int b;
          unsigned char* dst0 = dst;
	  unsigned char* data0 = data;
          for (i=0; i<w; i++)
            {
	      for( b=0; b<bytes_per_pixel; b++ )
		*dst++ = *data++;
            }
          dst = dst0 + scanline;
          data = data0 + self->screenscanline;
          h--;
        }
      return result;
    }
}

static int readXevents(DisplayObject* self)
{
  MSG Msg;
  while (PeekMessage(&Msg, (HWND) NULL, 0, 0, PM_REMOVE))
    {
      DispatchMessage(&Msg);
      if (PyErr_Occurred())
        return 0;
    }
  return checkopen(self);
}

#define ENABLE_EVENTS(mask)    do { } while (0)   /* nothing */

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
  int r;
  if (!checkopen(self))
    return NULL;

  if (self->hpal)
	  RealizePalette(self->dc);

  r = SetDIBitsToDevice(self->dc, 0, 0, self->width, self->height, 0, 0,
	  0, self->height, self->screenbmpdata, (BITMAPINFO*)(&self->screenbmpinfo),
	  DIB_PAL_COLORS);
  if (!r)
  {
	  PyErr_SetString(PyExc_IOError, "SetDIBitsToDevice failed");
	  return NULL;
  }

  flush(self);
  if (!readXevents(self))
    return NULL;
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* display_shmmode(DisplayObject* self, PyObject *args)
{
  return PyInt_FromLong(0);
}

static PyObject* display_settitle(DisplayObject* self, PyObject* args)
{
  char* title;
  if (!checkopen(self))
    return NULL;
  if (!PyArg_ParseTuple(args, "s", &title))
    return NULL;
  SetWindowText(self->win, title);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef display_methods[] = {
  {"close",    (PyCFunction)display_close1,    METH_VARARGS,  NULL},
  {"flip",     (PyCFunction)display_flip1,     METH_VARARGS,  NULL},
  {"clear",    (PyCFunction)display_clear1,    METH_VARARGS,  NULL},
  {"pixmap",   (PyCFunction)display_pixmap1,   METH_VARARGS,  NULL},
  {"putppm",   (PyCFunction)display_putppm1,   METH_VARARGS,  NULL},
  {"getppm",   (PyCFunction)display_getppm1,   METH_VARARGS,  NULL},
  {"keyevents",(PyCFunction)display_keyevents1,METH_VARARGS,  NULL},
  {"mouseevents",(PyCFunction)display_mouseevents1,METH_VARARGS,NULL},
  {"pointermotion",(PyCFunction)display_pointermotion1,METH_VARARGS,NULL},
  {"shmmode",  (PyCFunction)display_shmmode,   METH_VARARGS,  NULL},
  {"settitle", (PyCFunction)display_settitle,  METH_VARARGS,  NULL},
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


 /************************** AUDIO PART ***************************/

#define NUM_WAVE_HDR    2

typedef struct {
	PyObject_HEAD
	HWAVEOUT waveOut;
	HANDLE doneEvent;
	WAVEHDR waveHdr[NUM_WAVE_HDR];
	int waveHdrCount, waveHdrNext;
} AudioObject;

#define AudioObject_Check(v)	((v)->ob_type == &Audio_Type)
staticforward PyTypeObject Audio_Type;


static PyObject* new_audio(PyObject* dummy, PyObject* args)
{
	WAVEFORMATEX wf;
	int channels, freq, bits, err, bufsize;
	AudioObject* self;
	if (!PyArg_ParseTuple(args, "iiii", &channels, &freq, &bits, &bufsize))
		return NULL;

	self = PyObject_New(AudioObject, &Audio_Type);
	if (self == NULL)
		return NULL;

	self->waveHdrCount = 0;
	self->waveHdrNext  = 0;
	self->waveOut      = 0;
	self->doneEvent    = CreateEvent(NULL, FALSE, TRUE, NULL);

	memset(&wf, 0, sizeof(wf));
	wf.wFormatTag      = WAVE_FORMAT_PCM;
	wf.nChannels       = channels;
	wf.nSamplesPerSec  = freq;
	wf.wBitsPerSample  = bits;
	wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	err = waveOutOpen(&self->waveOut, WAVE_MAPPER, &wf, (DWORD) self->doneEvent, 0, CALLBACK_EVENT);
	if (err != MMSYSERR_NOERROR || self->doneEvent == NULL) {
		Py_DECREF(self);
		PyErr_SetString(PyExc_IOError, "cannot open audio device");
		return NULL;
	}

	while (self->waveHdrCount < NUM_WAVE_HDR) {
		WAVEHDR* wh = &self->waveHdr[self->waveHdrCount];
		wh->lpData         = PyMem_Malloc(bufsize);
		wh->dwBufferLength = bufsize;
		wh->dwFlags        = 0;
		if (wh->lpData == NULL || 
		    waveOutPrepareHeader(self->waveOut, wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			Py_DECREF(self);
			return NULL;
		}
		wh->dwFlags |= WHDR_DONE;
		self->waveHdrCount++;
	}
	return (PyObject*) self;
}

static void audio_close(AudioObject* self)
{
	if (self->waveOut != 0) {
		waveOutReset(self->waveOut);
		while (self->waveHdrCount > 0) {
			WAVEHDR* wh = &self->waveHdr[--self->waveHdrCount];
			waveOutUnprepareHeader(self->waveOut, wh, sizeof(WAVEHDR));
			PyMem_Free(wh->lpData);
		}
		waveOutClose(self->waveOut);
		self->waveOut = 0;
	}
}

static void audio_dealloc(AudioObject* self)
{
	audio_close(self);
	PyObject_Del(self);
}

static PyObject* audio_wait1(AudioObject* self, PyObject* args)
{
	float delay = -1.0;
	if (!PyArg_ParseTuple(args, "|f", &delay))
		return NULL;
	if (self->waveHdrNext >= self->waveHdrCount) {
		PyErr_SetString(PyExc_IOError, "audio device not ready");
		return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
	while (!(self->waveHdr[self->waveHdrNext].dwFlags & WHDR_DONE)) {
		WaitForSingleObject(self->doneEvent, delay<0.0?INFINITE:(DWORD)(delay*1000.0));
	}
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* audio_ready1(AudioObject* self, PyObject* args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return PyInt_FromLong(self->waveHdrNext < self->waveHdrCount &&
			      (self->waveHdr[self->waveHdrNext].dwFlags & WHDR_DONE));
}

static PyObject* audio_write1(AudioObject* self, PyObject* args)
{
	WAVEHDR* wh;
	char* buffer;
	int bufsize;
	if (!PyArg_ParseTuple(args, "s#", &buffer, &bufsize))
		return NULL;
	if (self->waveHdrNext >= self->waveHdrCount) {
		PyErr_SetString(PyExc_IOError, "audio device not ready");
		return NULL;
	}
	wh = &self->waveHdr[self->waveHdrNext];
	if (!(wh->dwFlags & WHDR_DONE)) {
		PyErr_SetString(PyExc_IOError, "audio device would block");
		return NULL;
	}
	if ((DWORD) bufsize != wh->dwBufferLength) {
		PyErr_SetString(PyExc_ValueError, "bufsize mismatch");
		return NULL;
	}
	wh->dwFlags &= ~WHDR_DONE;
	memcpy(wh->lpData, buffer, bufsize);
	if (waveOutWrite(self->waveOut, wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		PyErr_SetString(PyExc_IOError, "audio device write error");
		return NULL;
	}
	self->waveHdrNext++;
	if (self->waveHdrNext >= self->waveHdrCount)
		self->waveHdrNext = 0;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* audio_close1(AudioObject* self, PyObject* args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	audio_close(self);
	Py_INCREF(Py_None);
	return Py_None;
}


static PyMethodDef audio_methods[] = {
  {"ready",    (PyCFunction)audio_ready1,      METH_VARARGS,  NULL},
  {"wait",     (PyCFunction)audio_wait1,       METH_VARARGS,  NULL},
  {"write",    (PyCFunction)audio_write1,      METH_VARARGS,  NULL},
  {"close",    (PyCFunction)audio_close1,      METH_VARARGS,  NULL},
  {NULL,		NULL}		/* sentinel */
};

static PyObject* audio_getattr(AudioObject* self, char* name)
{
  return Py_FindMethod(audio_methods, (PyObject*)self, name);
}


statichere PyTypeObject Audio_Type = {
	PyObject_HEAD_INIT(NULL)
	0,			/*ob_size*/
	"Audio",		/*tp_name*/
	sizeof(AudioObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)audio_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)audio_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
};


static PyMethodDef WinMethods[] = {
           {"Display",  new_display,  METH_VARARGS},
           {"Audio",    new_audio,  METH_VARARGS},
           {NULL,       NULL}         /* Sentinel */
       };

void initwingame(void)
{
  Display_Type.ob_type = &PyType_Type;
  Audio_Type.ob_type = &PyType_Type;
  Py_InitModule("wingame", WinMethods);
}
