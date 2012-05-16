/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/ 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* utility */
#include "genlist.h"
#include "log.h"
#include "mem.h"
#include "shared.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "gui_stuff.h"
#define CLASSNAME "freecivLayoutWindow"
extern HINSTANCE freecivhinst;

struct fcwin_box
{
  int horiz; /* Set to true if the box is horizontally arranged */
  int same_size;
  RECT rc;
  POINT minsize;
  POINT biggest_minsize;
  int num_variable_size;
  HWND owner;
  struct genlist *item_list;  
};

struct fcwin_box_item {
  struct fcwin_box_item *next;
  POINT min;
  POINT biggest_min;
  RECT realrect;
  bool expand;
  bool fill;
  int padding;
  t_fcsetsize setsize;
  t_fcminsize minsize;
  t_fcdelwidget del;
  void *data;
};

struct fcwin_win_data {
  WNDPROC user_wndproc;
  HWND parent;
  struct fcwin_box *full;
  bool size_set;
  enum childwin_mode child_mode;
  bool is_child;
  struct genlist *childs;
  void *user_data;
};

struct tab_data
{
  HWND win;
  struct genlist *tabslist;
};

struct list_data
{
  HWND win;
  int rows;
};

struct edit_data
{
  HWND win;
  int chars;
};

struct groupbox_data
{
  HWND win;
  struct fcwin_box *content;
  int text_height;
};

static void win_text_minsize(HWND hWnd,POINT *rcmin,char *text);
/**************************************************************************

**************************************************************************/
void * fcwin_get_user_data(HWND hWnd)
{
  struct fcwin_win_data *win_data=(struct fcwin_win_data *)
    GetWindowLong(hWnd,GWL_USERDATA);
  if (!win_data)
    return NULL;
  return win_data->user_data;
}
/**************************************************************************

**************************************************************************/
void fcwin_set_user_data(HWND hWnd,void *user_data)
{
  struct fcwin_win_data *win_data=(struct fcwin_win_data *)
    GetWindowLong(hWnd,GWL_USERDATA);
  if (win_data)
    win_data->user_data=user_data;
}
/**************************************************************************

**************************************************************************/
void fcwin_redo_layout(HWND hWnd)
{
  RECT rc;
  POINT pt;
  struct fcwin_win_data *win_data=(struct fcwin_win_data *)
    GetWindowLong(hWnd,GWL_USERDATA);
  if (!win_data)
    return;
  if (!win_data->full)
    return;
  GetClientRect(hWnd,&rc);
  fcwin_box_calc_sizes(win_data->full,&pt);
  win_data->size_set=1;
  if ((rc.right<pt.x)||(rc.bottom<pt.y))
    {
      int new_w,new_h,border_w,border_h;
      new_w=MAX(pt.x,rc.right);
      new_h=MAX(pt.y,rc.bottom);
      my_get_win_border(hWnd,&border_w,&border_h);
      new_w+=border_w;
      new_h+=border_h;
      GetWindowRect(hWnd,&rc);
      MoveWindow(hWnd,rc.left,rc.top,new_w,new_h,TRUE);
      GetClientRect(hWnd,&rc);
    }
  fcwin_box_do_layout(win_data->full,&rc);
      
}

/**************************************************************************

**************************************************************************/
void fcwin_set_box(HWND hWnd,struct fcwin_box *fcb)
{
  struct fcwin_win_data *win_data=(struct fcwin_win_data *)
    GetWindowLong(hWnd,GWL_USERDATA);
  win_data->full=fcb;
  if (!win_data->is_child)
    fcwin_redo_layout(hWnd);
}

/**************************************************************************

**************************************************************************/
static struct genlist *get_childlist(HWND win)
{
  char classname[80];
  struct fcwin_win_data *win_data; 
  GetClassName(win, classname, sizeof(classname));
  if (strcmp(classname, CLASSNAME)==0) {
    win_data=(struct fcwin_win_data *)
      GetWindowLong(win, GWL_USERDATA);
    if (win_data) {
      return win_data->childs; 
    }
  }
  return NULL;
}

/**************************************************************************

**************************************************************************/
static LONG APIENTRY layout_wnd_proc(HWND hWnd,
				     UINT message,
				     WPARAM wParam,
				     LPARAM lParam)
{
  LONG ret;
  struct fcwin_win_data *win_data;
  win_data=(struct fcwin_win_data *)GetWindowLong(hWnd,GWL_USERDATA);
  switch (message) 
    {
    case WM_CREATE:
      
      win_data=(struct fcwin_win_data *)
	(((LPCREATESTRUCT)lParam)->lpCreateParams);
      SetWindowLong(hWnd,GWL_USERDATA,(LONG)win_data);
      
      break;
    case WM_GETMINMAXINFO: 
      if ((win_data)&&(!win_data->is_child)) {
	POINT pt;
        LPMINMAXINFO minmax;
        minmax=(LPMINMAXINFO)lParam;
        if (win_data->full)     /* Why can this be called before WM_CREATE? */
          {
	    int border_w,border_h;
            fcwin_box_calc_sizes(win_data->full,&pt);
	    my_get_win_border(hWnd,&border_w,&border_h);
	    minmax->ptMinTrackSize.x=pt.x+border_w;
            minmax->ptMinTrackSize.y=pt.y+border_h;
          }
	
      }
      break;
    case WM_SIZE:
      if ((win_data)&&(!win_data->is_child)) {
        RECT rc;
	rc.top=0;
        rc.left=0;
        rc.right=LOWORD(lParam);
        rc.bottom=HIWORD(lParam);
        if ((win_data)&&(win_data->size_set)&&(win_data->full))
	  fcwin_box_do_layout(win_data->full,&rc);
      }
      break;
    default:
      if (!win_data)
	return DefWindowProc(hWnd,message,wParam,lParam);
	  
	
    }
  if (!win_data)
    return 0;
  ret=win_data->user_wndproc(hWnd,message,wParam,lParam);
  if (message==WM_DESTROY)
    {
      struct genlist *childs;
      if (win_data->full)
	fcwin_box_free(win_data->full);
      if (win_data->parent!=NULL) {
	childs=get_childlist(win_data->parent);
	if (childs)
	  genlist_unlink(childs,hWnd);
	if (win_data->child_mode==FAKE_CHILD)
	  SetFocus(win_data->parent);
      }
      fcwin_close_all_childs(hWnd);
      free(win_data);
      SetWindowLong(hWnd,GWL_USERDATA,0);
    }
  return ret;
}

/**************************************************************************

**************************************************************************/
void init_layoutwindow(void)
{
  WNDCLASS *wndclass;
  wndclass=fc_malloc(sizeof(WNDCLASS));
  wndclass->style=0;
  wndclass->cbClsExtra=0;
  wndclass->cbWndExtra=0;
  wndclass->lpfnWndProc=(WNDPROC) layout_wnd_proc;
  wndclass->hIcon=LoadIcon(GetModuleHandle(NULL), "SDL_app");
  wndclass->hCursor=LoadCursor(NULL,IDC_ARROW);
  wndclass->hInstance=freecivhinst;
  wndclass->hbrBackground=CreateSolidBrush(GetSysColor(15));
  wndclass->lpszClassName=CLASSNAME;
  wndclass->lpszMenuName=(LPSTR)NULL;
  if (!RegisterClass(wndclass))
    exit(EXIT_FAILURE);
}

/**************************************************************************
  This closes all windows which were opened without WS_CHILD but with 
  hWndParent != NULL, fcwin_create_layouted_window does not pass that value
  to CreateWindow in that case, so those windows won't be closed if the
  parent is closed.
  If the child would be passed to CreateWindow, the childs could not be
  behind the parent which is really annoying
**************************************************************************/
void fcwin_close_all_childs(HWND win)
{ 
  struct fcwin_win_data *win_data=(struct fcwin_win_data *)
    GetWindowLong(win, GWL_USERDATA);
  if (!win_data)
    return;
  while(genlist_size(win_data->childs)) {
    DestroyWindow((HWND)genlist_get(win_data->childs, 0));
  }
}

/**************************************************************************

**************************************************************************/
HWND fcwin_create_layouted_window(WNDPROC user_wndproc,
				  LPCTSTR lpWindowName,
				  DWORD dwStyle, 
				  int x,
				  int y, 
				  HWND hWndParent,
				  HMENU hMenu,
				  enum childwin_mode child_mode,
				  void *user_data)
{

  HWND win;
  struct fcwin_win_data *win_data;
  win_data=fc_malloc(sizeof(struct fcwin_win_data));
  win_data->user_data=user_data;
  win_data->user_wndproc=user_wndproc;
  win_data->parent=hWndParent;
  win_data->full=NULL;
  win_data->size_set=0;
  win_data->child_mode = child_mode;
  win_data->is_child=dwStyle&WS_CHILD;
  win_data->childs = genlist_new();
  if ((!win_data->is_child) && (win_data->child_mode != REAL_CHILD)) {
    hWndParent=NULL;
  }
  win=CreateWindow(CLASSNAME,lpWindowName,dwStyle,
		   x,y,40,40,
		   hWndParent,
		   hMenu,freecivhinst,win_data);
  if ((win_data->parent)&&(!win_data->is_child)) {
    struct genlist *childs;
    childs=get_childlist(win_data->parent);
    if (childs) {
      genlist_append(childs, win);
    }
  }
  return win;
}

/**************************************************************************

**************************************************************************/
static struct fcwin_box * fcwin_box_new(int horiz, HWND owner, int same_size)
{
  struct fcwin_box *fcb;
  fcb=fc_malloc(sizeof(struct fcwin_box));
  fcb->horiz=horiz;
  fcb->same_size=same_size;
  fcb->num_variable_size=0;
  fcb->owner=owner;
  fcb->item_list = genlist_new();
  return fcb;
}

/**************************************************************************

**************************************************************************/
struct fcwin_box *fcwin_hbox_new(HWND owner,int same_size)
{
  return fcwin_box_new(TRUE,owner,same_size);
}

/**************************************************************************

**************************************************************************/
struct fcwin_box *fcwin_vbox_new(HWND owner,int same_size)
{
  return fcwin_box_new(FALSE,owner,same_size);
}

/**************************************************************************

**************************************************************************/
void fcwin_box_add_win_default(struct fcwin_box *box,HWND win)
{
  fcwin_box_add_win(box,win,0,0,3);
}

/**************************************************************************

**************************************************************************/
void fcwin_box_free(struct fcwin_box *box)
{
  while(genlist_size(box->item_list)) {
    fcwin_box_freeitem(box,0);
  }
  free(box);
}

/**************************************************************************

**************************************************************************/
void fcwin_box_freeitem(struct fcwin_box *box, int n)
{
  struct fcwin_box_item *fbi;
  fbi = genlist_get(box->item_list, n);
  if (!fbi) return;
  if (fbi->expand)
    box->num_variable_size--;
  genlist_unlink(box->item_list, fbi);
  if (fbi->del)
    fbi->del(fbi->data);
  free(fbi);
  return;
}


/**************************************************************************

**************************************************************************/
void fcwin_box_add_generic(struct fcwin_box *box,
			   t_fcminsize minsize,
			   t_fcsetsize setsize,
			   t_fcdelwidget del,
			   void *data, 
			   bool expand, 
			   bool fill, 
			   int padding)
{
  struct fcwin_box_item *fbi_new;
  fbi_new=fc_malloc(sizeof(struct fcwin_box_item));
  if (expand)
    box->num_variable_size++;
  fbi_new->next=NULL;
  fbi_new->expand=expand;
  fbi_new->fill=fill;
  fbi_new->padding=padding;
  fbi_new->setsize=setsize;
  fbi_new->minsize=minsize;
  fbi_new->del=del;
  fbi_new->data=data;
  fbi_new->min.x=0;
  fbi_new->min.y=0;
  fbi_new->biggest_min.x=0;
  fbi_new->biggest_min.y=0;
  memset(&(fbi_new->realrect),0,sizeof(RECT));
  genlist_append(box->item_list, fbi_new);
}

/**************************************************************************

**************************************************************************/
static void win_text_minsize(HWND hWnd,POINT *rcmin,char *text)
{
  HDC hdc;
  HFONT old;
  HFONT font;
  RECT textrect;
  RECT rcclient;
  RECT rcwin;
  font=NULL;
  old=NULL; /* silence gcc, will only be used if font!=NULL */
  hdc=GetDC(hWnd);
  if ((font=GetWindowFont(hWnd)))
    old=SelectObject(hdc,font);
  textrect.top=0;
  textrect.right=640;
  textrect.left=0;
  textrect.bottom=480;

  /* Calculation is not done if text is zero length */
  if (strlen(text) == 0) {
    textrect.right = 0;
    textrect.bottom = 0;
  } else {
    DrawText(hdc, text, strlen(text), &textrect, DT_CALCRECT | DT_WORDBREAK);
  }

  if (font)
    SelectObject(hdc,old);
  ReleaseDC(hWnd,hdc);
  
  GetWindowRect(hWnd,&rcwin);
  GetClientRect(hWnd,&rcclient);
  rcmin->x=textrect.right-textrect.left+rcwin.right-rcwin.left+
    rcclient.left-rcclient.right+4;
  
  rcmin->y=textrect.bottom-textrect.top+
    rcwin.bottom-rcwin.top+rcclient.top-rcclient.bottom;  
}

/**************************************************************************

**************************************************************************/
static void win_minsize(POINT *rcmin,void *data)
{
  HWND hWnd;
 
  char wintext[1024];
  hWnd=(HWND)data;
 
  if (!GetWindowText(hWnd,wintext,sizeof(wintext)))
    {
      rcmin->x=10;
      rcmin->y=10;
      return;
    }
  else
    {
      win_text_minsize(hWnd,rcmin,wintext);
    }
}

/**************************************************************************

**************************************************************************/
static void checkbox_minsize(LPPOINT rcmin, void *data)
{
  win_minsize(rcmin,data);
  rcmin->x+=rcmin->y;
}

/**************************************************************************

**************************************************************************/
static void win_setsize(LPRECT newsize,void *data)
{
  POINT p;
  HWND hWnd;
  RECT rc;
  hWnd=(HWND)data;
  p.x=0;
  p.y=0;
  ClientToScreen(GetParent(hWnd),&p);
  GetWindowRect(hWnd,&rc);
  rc.right-=p.x;
  rc.left-=p.x;
  rc.top-=p.y;
  rc.bottom-=p.y;
  if ((rc.top!=newsize->top)||
      (rc.left!=newsize->left)||
      (rc.bottom!=newsize->bottom)||
      (rc.right!=newsize->right))
    MoveWindow(hWnd,newsize->left,
	       newsize->top,
	       newsize->right-newsize->left,
	       newsize->bottom-newsize->top,
	       TRUE);
}

/**************************************************************************

**************************************************************************/
static void win_del(void *data)
{
  DestroyWindow((HWND)data);
}

/**************************************************************************

**************************************************************************/
void fcwin_box_add_win(struct fcwin_box *box, 
		    HWND win,
		    bool expand,
		    bool fill,
		    int padding)
{
  fcwin_box_add_generic(box,win_minsize,win_setsize,win_del,(void *)win,
		     expand,fill,padding);
}

/**************************************************************************

**************************************************************************/
static void button_minsize(LPPOINT minsize, void *data)
{
  win_minsize(minsize,data);
  minsize->x=minsize->x+4;
  minsize->y=minsize->y*3/2+2;
}

/**************************************************************************

**************************************************************************/

HWND fcwin_box_add_checkbox(struct fcwin_box *box, const char *txt, int id, int style,
		       bool expand, bool fill, int padding)
{
  HWND win;
  win=CreateWindow("BUTTON",txt,
		   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | style,
		   0,0,0,0,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  fcwin_box_add_generic(box,checkbox_minsize,win_setsize,win_del,(void *)win,
			expand,fill,padding);
  return win;
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_radiobutton(struct fcwin_box *box, const char *txt, int id,
			       int style, bool expand, bool fill, int padding)
{
  HWND win;
  win=CreateWindow("BUTTON",txt,
		   WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | style,
		   0,0,0,0,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  fcwin_box_add_generic(box,checkbox_minsize,win_setsize,win_del,(void *)win,
			expand,fill,padding);
  return win;
}

/**************************************************************************

**************************************************************************/
static void groupbox_setsize(LPRECT rc, void *data)
{
  RECT boxsize;
  struct groupbox_data *gb=(struct groupbox_data *)data;
  win_setsize(rc,gb->win);
  boxsize.top=rc->top+gb->text_height+3;
  boxsize.bottom=rc->bottom-3;
  boxsize.left=rc->left+3;
  boxsize.right=rc->right-3;
  fcwin_box_do_layout(gb->content,&boxsize);
}

/**************************************************************************

**************************************************************************/
static void groupbox_minsize(LPPOINT minsize, void *data)
{
  POINT boxsize;
  struct groupbox_data *gb=(struct groupbox_data *)data;
  char buf[80];
  GetWindowText(gb->win,buf,sizeof(buf));
  win_text_minsize(gb->win,minsize,buf);
  gb->text_height=minsize->y;
  fcwin_box_calc_sizes(gb->content,&boxsize);
  minsize->x=MAX(minsize->x,boxsize.x);
  minsize->y+=boxsize.y;
  minsize->x+=6;
  minsize->y+=6;
}

/**************************************************************************

**************************************************************************/
static void groupbox_del(void *data)
{
  struct groupbox_data *gb=(struct groupbox_data *)data;
  DestroyWindow(gb->win);
  fcwin_box_free(gb->content);
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_groupbox(struct fcwin_box *box,const char *txt,
			    struct fcwin_box *box_add, int style,
			    bool expand, bool fill,
			    int padding)
{
  struct groupbox_data *gb=fc_malloc(sizeof(struct groupbox_data));
  gb->content=box_add;
  gb->win=CreateWindow("BUTTON",txt,
		       WS_CHILD | WS_VISIBLE | BS_GROUPBOX | style,
		       0,0,0,0,
		       box->owner,NULL,freecivhinst,NULL);
  fcwin_box_add_generic(box,groupbox_minsize,groupbox_setsize,groupbox_del,
			gb,expand,fill,padding);
  return gb->win;
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_static(struct fcwin_box *box, const char *txt,
			  int id, int style,
			  bool expand, bool fill, int padding)
{
  HWND win;
  win=CreateWindow("STATIC",txt,WS_CHILD | WS_VISIBLE | style,
		   0,0,0,0,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  if (!win) return NULL;
  fcwin_box_add_win(box,win,expand,fill,padding);
  return win;
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_button(struct fcwin_box *box, const char *txt,
			  int id, int style,
			  bool expand, bool fill, int padding)
{
  HWND win;
  win=CreateWindow("BUTTON",txt,WS_CHILD | WS_VISIBLE | style,
		   0,0,5,5,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  if (!win) return NULL;
  fcwin_box_add_generic(box,button_minsize,win_setsize,win_del,win,
			expand,fill,padding);
  return win;
}

/**************************************************************************

**************************************************************************/
static void listview_minsize(LPPOINT minsize,void *data)
{
  struct list_data *ld=data;
  LV_COLUMN lvc;
  char buf[64];
  int i;
  minsize->x=0;
  i=0;
  while(1) {
    lvc.pszText=buf;
    lvc.cchTextMax=sizeof(buf);
    lvc.mask=LVCF_TEXT | LVCF_WIDTH;
    buf[0]='\0';
    if (!ListView_GetColumn(ld->win,i,&lvc))
      break;
    if (!strlen(buf))
      break;
    minsize->x+=lvc.cx;
    i++;
  }
  if (!minsize->x)
    minsize->x=400;     /* Just a dummy value */
 
  minsize->y=200; /* Also a dummy value, need 
                     some check for height of an item */
}
/**************************************************************************

**************************************************************************/
static void list_minsize(LPPOINT minsize,void *data)
{
  char buf[256];
  int i;
  int n;
  int w,h;
  HDC hdc;
  HFONT old;
  HFONT font;
  RECT rc;
  struct list_data *ld;
  ld=(struct list_data *)data;
  old=NULL; /* silence gcc, will only be used if font!=NULL */
  minsize->x=0;
  minsize->y=ListBox_GetItemHeight(ld->win,0)*ld->rows;
  n=ListBox_GetCount(ld->win);
  font=NULL;
  hdc=GetDC(ld->win);
  if ((font=GetWindowFont(ld->win)))
    old=SelectObject(hdc,font);
  for (i=0;i<n;i++) {
    if (ListBox_GetTextLen(ld->win,i)<sizeof(buf)) {
      ListBox_GetText(ld->win,i,buf);
      memset(&rc,0,sizeof(RECT));
      DrawText(hdc,buf,strlen(buf),&rc,DT_CALCRECT);
      minsize->x=MAX(minsize->x,rc.right-rc.left);
    }
  }
  if (font)
    SelectObject(hdc,old);
  ReleaseDC(ld->win,hdc);
  minsize->x+=20;
  my_get_win_border(ld->win,&w,&h);
  minsize->x+=w;
  minsize->y+=h;
}

/**************************************************************************

**************************************************************************/
static void list_setsize(LPRECT rc,void *data)
{
  struct list_data *ld;
  ld=(struct list_data *)data;
  win_setsize(rc,ld->win);
}
/**************************************************************************

**************************************************************************/

static void list_del(void *data)
{
  struct list_data *ld;
  ld=(struct list_data *)data;
  DestroyWindow(ld->win);
  free(ld);
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_list(struct fcwin_box *box,
			int rows,
			int id,
			int style,
			bool expand, bool fill, int padding)
{
  struct list_data *ld;
  HWND win;
  win=CreateWindow("LISTBOX",NULL,LBS_HASSTRINGS | WS_CHILD |
		   WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_HASSTRINGS | style,
		   0,0,0,0,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  if (!win) return NULL;
  ld=fc_malloc(sizeof(struct list_data));
  ld->win=win;
  ld->rows=rows;
  fcwin_box_add_generic(box,list_minsize,list_setsize,list_del,ld,
			expand,fill,padding);
  return win;
}

/**************************************************************************

 *************************************************************************/
static void tab_del(void *data)
{
  struct tab_data *td=data;
  DestroyWindow(td->win);
  while(genlist_size(td->tabslist)) {
    HWND win;
    win = (HWND)genlist_get(td->tabslist, 0);
    DestroyWindow((HWND)win);
    genlist_unlink(td->tabslist, win);
  }
  free(td);
}

/**************************************************************************

 *************************************************************************/
static void tab_setsize(RECT *size, void *data)
{
  HWND win;
  RECT rc;
  RECT rcclient;
  struct fcwin_win_data *wd;
  struct tab_data *td=data;
  const struct genlist_link *myiter;
  MoveWindow(td->win,size->left,size->top,size->right-size->left,
	     size->bottom-size->top,TRUE);
  rc.left=size->left;
  rc.right=size->right;
  rc.top=size->top;
  rc.bottom=size->bottom;
  TabCtrl_AdjustRect(td->win,FALSE,&rc);
  myiter = genlist_head(td->tabslist);
  for(;genlist_link_data(myiter);myiter=genlist_link_next(myiter)) {
    win=(HWND)genlist_link_data(myiter);
    wd=(struct fcwin_win_data *)GetWindowLong(win,GWL_USERDATA);
    MoveWindow(win,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
    if (wd) {
      if (wd->full) {
	GetClientRect(win,&rcclient);
	fcwin_box_do_layout(wd->full,&rcclient);
      }
    }
  }
}
/**************************************************************************

 *************************************************************************/
static void tab_minsize(POINT *min,void *data)
{
  RECT rc;
  HWND win;
  struct fcwin_win_data *wd;
  const struct genlist_link *myiter;
  struct tab_data *td=data;
  min->x=0;
  min->y=0;
  myiter = genlist_head(td->tabslist);
  for(;genlist_link_data(myiter);myiter=genlist_link_next(myiter)) {
    POINT box_min;
    win=(HWND)genlist_link_data(myiter);
    wd=(struct fcwin_win_data *)GetWindowLong(win,GWL_USERDATA);
    if (wd) {
      if (wd->full) {
	fcwin_box_calc_sizes(wd->full,&box_min);
	min->x=MAX(min->x,box_min.x);
	min->y=MAX(min->y,box_min.y);
      }
    }
  }
  rc.left=0;
  rc.right=min->x;
  rc.bottom=min->y;
  rc.top=0;
  TabCtrl_AdjustRect(td->win,TRUE,&rc);
  min->x=rc.right-rc.left;
  min->y=rc.bottom-rc.top;
}

/**************************************************************************

 *************************************************************************/
HWND fcwin_box_add_tab(struct fcwin_box *box,
		       WNDPROC *wndprocs,
		       HWND *wnds,
		       char **titles,
		       void **user_data, int n,
		       int id,int style,
		       bool expand, bool fill, int padding)
{
  struct tab_data *td;

  int i;
  td=fc_malloc(sizeof(struct tab_data));
  td->tabslist = genlist_new();
  td->win=CreateWindow(WC_TABCONTROL,"",
		       WS_CHILD | WS_VISIBLE | style,
		       0,0,300,50,box->owner,NULL,freecivhinst,NULL);
  for(i=0;i<n;i++) {
    TC_ITEM tci;
    tci.mask=TCIF_TEXT | TCIF_PARAM;
    tci.pszText=titles[i];
    wnds[i]=fcwin_create_layouted_window(wndprocs[i],NULL,WS_CHILD,
					 0,0,box->owner,NULL,REAL_CHILD,
					 user_data[i]);
    tci.lParam=(LPARAM)wnds[i];
    TabCtrl_InsertItem(td->win,i,&tci);
    genlist_append(td->tabslist, wnds[i]);
  }
  fcwin_box_add_generic(box,tab_minsize,tab_setsize,tab_del,td,
			expand,fill,padding);
  return td->win;
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_listview(struct fcwin_box *box,
			    int rows,
			    int id,
			    int style,
			    bool expand, bool fill, int padding)
{
    struct list_data *ld;
  HWND win;
  win=CreateWindow(WC_LISTVIEW,NULL,WS_CHILD |
		   WS_VISIBLE | style,
		   0,0,0,0,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  if (!win) return NULL;
  ld=fc_malloc(sizeof(struct list_data));
  ld->win=win;
  ld->rows=rows;
  fcwin_box_add_generic(box,listview_minsize,list_setsize,list_del,ld,
			expand,fill,padding);
  return win;
}
/**************************************************************************

**************************************************************************/
static void combo_minsize(POINT *minsize,void *data)
{
  int n,i;
  RECT rc; 
  HDC hdc;
  HFONT old;
  HFONT font;
  char buf[256];
  struct list_data *ld=data;
  old=NULL; /* silence gcc, will only be used if font!=NULL */
  n=ComboBox_GetCount(ld->win);
  minsize->x=0;
  hdc=GetDC(ld->win);
  if ((font=GetWindowFont(ld->win)))
    old=SelectObject(hdc,font);
  minsize->y=ComboBox_GetItemHeight(ld->win)+8;
  for(i=0;i<n;i++) {
    if (ComboBox_GetLBTextLen(ld->win,i)<sizeof(buf)) {
      ComboBox_GetLBText(ld->win,i,buf);
      memset(&rc,0,sizeof(RECT));
      DrawText(hdc,buf,strlen(buf),&rc,DT_CALCRECT);
      minsize->x=MAX(minsize->x,rc.right-rc.left);
    }
  }
  if (font)
    SelectObject(hdc,old);
  ReleaseDC(ld->win,hdc);
  minsize->x+=minsize->y;
}
/**************************************************************************

**************************************************************************/
static void combo_setsize(LPRECT size, void *data)
{
  struct list_data *ld=data;
  MoveWindow(ld->win,size->left,size->top,
	     size->right-size->left,
	     ComboBox_GetItemHeight(ld->win)*ld->rows,TRUE);
}
/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_combo(struct fcwin_box *box,
			 int rows,
			 int id,
			 int style,
			 bool expand, bool fill, int padding)
{
  struct list_data *ld;
  HWND win;
  win=CreateWindow("COMBOBOX",NULL,WS_CHILD |
		   WS_VISIBLE | style,
		   0,0,0,0,
		   box->owner,
		   (HMENU)id,
		   freecivhinst,
		   NULL);
  if (!win) return NULL;
  ld=fc_malloc(sizeof(struct list_data));
  ld->win=win;
  ld->rows=rows;
  fcwin_box_add_generic(box,combo_minsize,combo_setsize,list_del,ld,
			expand,fill,padding);
  return win;
}
/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_button_default(struct fcwin_box *box, const char *txt, 
				  int id, int style)
{
  return fcwin_box_add_button(box,txt,id,style,FALSE,FALSE,5);
}

/**************************************************************************

**************************************************************************/
HWND fcwin_box_add_static_default(struct fcwin_box *box, const char *txt, 
				  int id, int style)
{
  return fcwin_box_add_static(box,txt,id,style,FALSE,FALSE,5);
}

/**************************************************************************

**************************************************************************/
static void box_minsize(POINT *pt,void *data)
{
  fcwin_box_calc_sizes((struct fcwin_box *)data,pt);
}

/**************************************************************************

**************************************************************************/
static void box_setsize(LPRECT rc,void *data)
{
  fcwin_box_do_layout((struct fcwin_box *)data,rc);
}

/**************************************************************************

**************************************************************************/
static void box_del(void *data)
{
  fcwin_box_free((struct fcwin_box *)data);
}
/**************************************************************************

*************************************************************************/
static void edit_minsize(POINT *minsize,void *data)
{
  char text[1024];
  int i;
  struct edit_data *edit_d=data;
  for(i=0;i<edit_d->chars;i++) {
    text[i]='X';
  }
  text[i]=0;
  win_text_minsize(edit_d->win,minsize,text);
  minsize->y+=4;
}
/**************************************************************************

 *************************************************************************/
static void edit_setsize(RECT *minsize, void *data)
{
  struct edit_data *edit_d=data;
  MoveWindow(edit_d->win,minsize->left,minsize->top,
	     minsize->right-minsize->left,
	     minsize->bottom-minsize->top,TRUE);
}
/**************************************************************************

 *************************************************************************/
static void edit_del(void *data)
{
  struct edit_data *edit_d=data;
  DestroyWindow(edit_d->win);
  free(edit_d);
}
/**************************************************************************

 *************************************************************************/
HWND fcwin_box_add_edit(struct fcwin_box *box, const char *txt, int maxchars,
			int id,int style,
			bool expand, bool fill, int padding)
{
  struct edit_data *edit_d;
  edit_d=fc_malloc(sizeof(struct edit_data ));
  edit_d->win=CreateWindowEx(WS_EX_CLIENTEDGE,
			     "Edit",txt,
			     WS_CHILD | WS_TABSTOP | WS_VISIBLE |
			     style,
			     0,0,0,0,
			     box->owner,
			     (HMENU)id,
			     freecivhinst,
			     NULL);
  fcwin_box_add_generic(box,
			edit_minsize,
			edit_setsize,
			edit_del,
			edit_d,
			expand,fill,padding);
  
  edit_d->chars=maxchars;
  return edit_d->win;
}
/**************************************************************************

**************************************************************************/
void fcwin_box_add_box(struct fcwin_box *box, struct fcwin_box *box_to_add,
		       bool expand, bool fill, int padding)
{
  fcwin_box_add_generic(box,box_minsize,box_setsize,box_del,
		     (void *)box_to_add,expand,fill,padding);
}

/**************************************************************************

**************************************************************************/
void fcwin_box_calc_sizes(struct fcwin_box *box, POINT *minsize)
{
  int i=0;
  POINT biggest_minsize;
  minsize->x=0;
  minsize->y=0;   
  biggest_minsize.x=0;
  biggest_minsize.y=0;
  TYPED_LIST_ITERATE(struct fcwin_box_item, box->item_list, fbi) {
    fbi->minsize(&fbi->min,fbi->data);
    if (fbi->biggest_min.x<fbi->min.x)
      fbi->biggest_min.x=fbi->min.x;
    if (fbi->biggest_min.y<fbi->min.y)
      fbi->biggest_min.y=fbi->min.y;
    if (box->same_size)
      {
	i++;
	minsize->y=MAX(minsize->y,
		       fbi->min.y);
	minsize->x=MAX(minsize->x,
		       fbi->min.x);
	biggest_minsize.x=MAX(biggest_minsize.x,fbi->biggest_min.x);
	biggest_minsize.y=MAX(biggest_minsize.y,fbi->biggest_min.y);
      }
    else if (box->horiz)
      {
	minsize->y=MAX(minsize->y,
		       fbi->min.y);
	biggest_minsize.y=MAX(biggest_minsize.y,fbi->biggest_min.y);
	minsize->x+=fbi->min.x+fbi->padding;
	biggest_minsize.x+=fbi->biggest_min.x+fbi->padding;
      }
    else
      {
	biggest_minsize.x=MAX(biggest_minsize.x,fbi->biggest_min.x);
	biggest_minsize.y+=fbi->biggest_min.y+fbi->padding;
	minsize->x=MAX(minsize->x,fbi->min.x);
	minsize->y+=fbi->min.y+fbi->padding;
      }
    
  } LIST_ITERATE_END;
  if (box->same_size)
    {
      TYPED_LIST_ITERATE(struct fcwin_box_item, box->item_list, fbi) {
	fbi->min.x=minsize->x;
	fbi->min.y=minsize->y;
	fbi->biggest_min.x=biggest_minsize.x;
	fbi->biggest_min.y=biggest_minsize.y;
      } LIST_ITERATE_END;
      if (box->horiz) {
	minsize->x*=i;
	biggest_minsize.x*=i;
      } else {
	minsize->y*=i;
	biggest_minsize.y*=i;
      }
      TYPED_LIST_ITERATE(struct fcwin_box_item, box->item_list, fbi) {
	if (box->horiz) {
	  minsize->x+=fbi->padding;
	  biggest_minsize.x+=fbi->padding;
	} else {
	  minsize->y+=fbi->padding;
	  biggest_minsize.y+=fbi->padding;
	}
      } LIST_ITERATE_END;
      
    }
  box->minsize.x=minsize->x;
  box->minsize.y=minsize->y;
  box->biggest_minsize.x=biggest_minsize.x;
  box->biggest_minsize.y=biggest_minsize.y;
}

/**************************************************************************

**************************************************************************/
static int fcwin_box_layoutitem(struct fcwin_box *box,
				struct fcwin_box_item *fbi,
				int r,int dr)
{
  RECT rc;
  if (box->horiz)
    {
      rc.left=r;
      rc.top=box->rc.top;
      rc.right=r;
      if (fbi->fill)
	rc.right+=dr;
      rc.right+=fbi->biggest_min.x;
      rc.bottom=box->rc.bottom;
      fbi->setsize(&rc,fbi->data);
      memcpy(&(fbi->realrect),&rc,sizeof(RECT));
      return r+dr+fbi->biggest_min.x+fbi->padding;
    }
  else
    {
      rc.top=r;
      rc.left=box->rc.left;
      rc.bottom=r;
      if (fbi->fill)
	rc.bottom+=dr;
      rc.bottom+=fbi->biggest_min.y;
      rc.right=box->rc.right;
      fbi->setsize(&rc,fbi->data);
      memcpy(&(fbi->realrect),&rc,sizeof(RECT));
      return r+dr+fbi->biggest_min.y+fbi->padding; 
    }
}


/**************************************************************************

**************************************************************************/
void fcwin_box_do_layout(struct fcwin_box *box, LPRECT size)
{
  int r;     
  int akku;       /* Doing some kind of 
                     Bresenhams line drawing algorithm */
  int expandsize;
  int reminder;
  box->rc.top=size->top;
  box->rc.bottom=size->bottom;
  box->rc.right=size->right;
  box->rc.left=size->left;
  expandsize=0;
  reminder=0;
  if ((size->right-size->left<box->biggest_minsize.x)
      ||(size->bottom-size->top<box->biggest_minsize.y)) {
    box->biggest_minsize.x=box->minsize.x;
    box->biggest_minsize.y=box->minsize.y;
    TYPED_LIST_ITERATE(struct fcwin_box_item, box->item_list, fbi) {
      fbi->biggest_min.x=fbi->min.x;
      fbi->biggest_min.y=fbi->min.y;
    } LIST_ITERATE_END;
  }
  if (box->num_variable_size>0)
    {
      if (box->horiz)
	expandsize=size->right-size->left-box->biggest_minsize.x;
      else
	expandsize=size->bottom-size->top-box->biggest_minsize.y;
      reminder=expandsize%box->num_variable_size;
      expandsize/=box->num_variable_size;
    }
  if (box->horiz)
    r=size->left;
  else
    r=size->top;
  akku=0;
  TYPED_LIST_ITERATE(struct fcwin_box_item, box->item_list, fbi) {
    if (fbi->expand) {
      akku+=reminder;
      if (akku>reminder) {
	r=fcwin_box_layoutitem(box,fbi,r,expandsize+1);
	akku-=box->num_variable_size;
      } else {
	r=fcwin_box_layoutitem(box,fbi,r,expandsize);
      }
    }
    else {
      r=fcwin_box_layoutitem(box,fbi,r,0);
    } 
  } LIST_ITERATE_END;
}

/**************************************************************************

**************************************************************************/
void fcwin_box_calc_layout(struct fcwin_box *box, int x, int y)
{
  POINT pt;
  RECT rc;
  fcwin_box_calc_sizes(box,&pt);
  rc.top=y;
  rc.left=x;
  rc.bottom=y+pt.y;
  rc.right=x+pt.x;
  fcwin_box_do_layout(box,&rc);
}

/**************************************************************************

**************************************************************************/
void mydrawrect(HDC hdc, int x, int y,int w,int h)
{
  MoveToEx(hdc,x,y,NULL);
  LineTo(hdc,x+w,y);
  LineTo(hdc,x+w,y+h);
  LineTo(hdc,x,y+h);
  LineTo(hdc,x,y);
}

/**************************************************************************

**************************************************************************/
void my_get_win_border(HWND hWnd,int *w,int *h)
{
  RECT rcwin;
  RECT rcclient;
  GetWindowRect(hWnd,&rcwin);
  GetClientRect(hWnd,&rcclient);
  *w=rcwin.right-rcwin.left+rcclient.left-rcclient.right;
  *h=rcwin.bottom-rcwin.top+rcclient.top-rcclient.bottom;   
}

/**************************************************************************

**************************************************************************/
char *convertnl2crnl(const char *str)
{
  int i;
  const char *old;
  char *buf;
  char *newbuf;
  old=str;
  i=0;
  while((old=strchr(old,'\n')))
    {
      i++;
      old=old+1;
    }
  buf=fc_malloc(strlen(str)*2+1);
  i=0;
  old=str;
  newbuf=buf;
  while((str=strchr(old,'\n')))
    {
      int len;
      str=str+1;
      len=str-old;
     
      strncpy(newbuf,old,len-1);
      newbuf=newbuf+len-1;
      newbuf[0]='\r';
      newbuf++;
      newbuf[0]='\n';
      newbuf++;
      newbuf[0]=0;
      old=str;
    }
  strcpy(newbuf,old);
  return buf;
}
/**************************************************************************

**************************************************************************/
int fcwin_listview_add_row(HWND lv, int row_nr, int columns, char **row)
{
  LV_ITEM lvi;
  int i,id;
  lvi.mask=LVIF_TEXT;
  lvi.iItem=row_nr;
  lvi.iSubItem=0;
  lvi.pszText=row[0];
  id=ListView_InsertItem(lv,&lvi);
  for(i=1;i<columns;i++) {
    lvi.iSubItem=i;
    lvi.iItem=id;
    lvi.pszText=row[i];
    ListView_SetItem(lv,&lvi);
  }
  return id;
}
