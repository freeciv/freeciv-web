## Process this file with automake to produce Makefile.in

if FTWL
FTWLLIB=libftwl.la
else
FTWLLIB=
endif

noinst_LTLIBRARIES = $(FTWLLIB)

AM_CPPFLAGS = -I. -I$(top_srcdir)/utility $(FTWL_CFLAGS)

BACKEND_COMMON_FILES=		\
	be_common_32.c		\
	be_common_32.h


ALL_BACKEND_X11_FILES=		\
	$(BACKEND_COMMON_FILES) \
	be_common_pixels.c	\
	be_x11_ximage_32.c

ALL_BACKEND_SDL_FILES=		\
	$(BACKEND_COMMON_FILES)	\
	be_sdl.c 		\
	be_common_pixels.c	\
	be_sdl_pixels_32.c

ALL_BACKEND_SDL_OPENGL_FILES=	\
	$(BACKEND_COMMON_FILES)	\
	be_sdl.c		\
	be_common_opengl.c	\
	be_sdl_opengl.c



BACKEND_COMMON_CAIRO32_FILES=	\
	be_common_cairo_32.c	\
	be_common_cairo_32.h

ALL_BACKEND_X11_CAIRO32_FILES=	\
	$(BACKEND_COMMON_CAIRO32_FILES) \
	be_common_pixels.c	\
	be_x11_cairo_32.c

ALL_BACKEND_SDL_CAIRO32_FILES=	\
	$(BACKEND_COMMON_CAIRO32_FILES)	\
	be_sdl.c 		\
	be_common_pixels.c	\
	be_sdl_pixels_cairo_32.c


ALL_OTHER_FILES=	\
	back_end.h	\
	common_types.c	\
	common_types.h	\
	widget_p.h	\
	widget.h	\
	widget_timeout.c	\
	widget.c	\
	widget_button.c	\
	widget_edit.c	\
	widget_label.c	\
	widget_list.c	\
	widget_slider.c	\
	text_renderer.c \
	text_renderer.h	\
	widget_core.c	\
	widget_window.c \
	theme_engine.c	\
	theme_engine.h

if FTWL_X11
BACKEND_X11_FILES=$(ALL_BACKEND_X11_CAIRO32_FILES)
endif
if FTWL_SDL
BACKEND_SDL_FILES=$(ALL_BACKEND_SDL_CAIRO32_FILES)
endif
if FTWL
OTHER_FILES=$(ALL_OTHER_FILES)
endif

libftwl_la_SOURCES = $(BACKEND_X11_FILES) \
	$(BACKEND_SDL_FILES) $(OTHER_FILES)
