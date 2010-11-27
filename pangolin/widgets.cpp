#include "widgets.h"

#include <iostream>
#include "display_internal.h"

using namespace std;

namespace pangolin
{

extern __thread PangolinGl* context;

const static int border = 1;
const static int tab_w = 15;
const static int tab_h = 20;
const static int tab_p = 5;
const static float colour_s1[4] = {0.2, 0.2, 0.2, 1.0};
const static float colour_s2[4] = {0.6, 0.6, 0.6, 1.0};
const static float colour_bg[4] = {0.9, 0.9, 0.9, 1.0};
const static float colour_fg[4] = {1.0, 1.0 ,1.0, 1.0};
const static float colour_tx[4] = {0.0, 0.0, 0.0, 1.0};
const static float colour_hl[4] = {0.9, 0.9, 0.9, 1.0};
const static float colour_dn[4] = {1.0, 0.7 ,0.7, 1.0};
static void* font = GLUT_BITMAP_HELVETICA_12;
static int text_height = 8; //glutBitmapHeight(font) * 0.7;
static float cb_height = text_height * 1.5;

void glRect(Viewport v)
{
  glRectf(v.l,v.t()-1,v.r()-1,v.b);
}

void glRect(Viewport v, int inset)
{
  glRectf(v.l+inset,v.t()-inset-1,v.r()-inset-1,v.b+inset);
}

void DrawShadowRect(Viewport& v)
{
  glColor4fv(colour_s2);
  glBegin(GL_LINE_STRIP);
    glVertex2f(v.l,v.b);
    glVertex2f(v.l,v.t());
    glVertex2f(v.r(),v.t());
    glVertex2f(v.r(),v.b);
    glVertex2f(v.l,v.b);
  glEnd();
}

void DrawShadowRect(Viewport& v, bool pushed)
{
  glColor4fv(pushed ? colour_s1 : colour_s2);
  glBegin(GL_LINE_STRIP);
    glVertex2f(v.l,v.b);
    glVertex2f(v.l,v.t());
    glVertex2f(v.r(),v.t());
  glEnd();

  glColor3fv(pushed ? colour_s2 : colour_s1);
  glBegin(GL_LINE_STRIP);
    glVertex2f(v.r(),v.t());
    glVertex2f(v.r(),v.b);
    glVertex2f(v.l,v.b);
  glEnd();
}

Panal::Panal()
{
  handler = &StaticHandler;
  layout = LayoutVertical;
}

Panal::Panal(const std::string& auto_register_var_prefix)
{
  handler = &StaticHandler;
  layout = LayoutVertical;
  RegisterNewVarCallback(&Panal::AddVariable,(void*)this,auto_register_var_prefix);
}

void Panal::AddVariable(void* data, const std::string& name, _Var& var)
{
  Panal* thisptr = (Panal*)data;

  const string& title = var.meta_friendly;

  if( var.type_name == typeid(bool).name() )
  {
    thisptr->views.push_back(
        var.meta_flags ? (View*)new Checkbox(title,var) : (View*)new Button(title,var)
    );
  }else if( var.type_name == typeid(double).name() || var.type_name == typeid(int).name() )
  {
    thisptr->views.push_back( new Slider(title,var) );
  }

}

void Panal::Render()
{
  OpenGlRenderState::ApplyWindowCoords();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

//  cout << v.l << "," << v.b << "," << v.w << "," << v.h << endl;

  glColor4fv(colour_s2);
  glRect(v);
  glColor4fv(colour_bg);
  glRect(v,border);

  RenderChildren();
}

View& CreatePanal(const std::string& name)
{
  Panal* v = new Panal(name);
  context->all_views[name] = v;
  context->base.views.push_back(v);
  return *v;
}

Button::Button(string title, _Var& tv)
  : Var<bool>(tv), title(title), down(false)
{
  top = 1.0; bottom = -20;
  left = 0; right = 1.0;
  hlock = LockLeft;
  vlock = LockBottom;
  handler = this;
  text_width = glutBitmapLength(font,(unsigned char*)title.c_str());
}

void Button::Mouse(View&, int button, int state, int x, int y)
{
  down = !state;
  if( state > 0 ) a->Set(!a->Get());
}

void Button::Render()
{
  DrawShadowRect(v, down);
  glColor4fv(colour_fg );
  glRect(vinside);
  glColor4fv(colour_tx);
  glRasterPos2f(raster[0],raster[1]-down);
  glutBitmapString(font,(unsigned char*)title.c_str());
}

void Button::ResizeChildren()
{
  raster[0] = v.l + (v.w-text_width)/2.0;
  raster[1] = v.b + (v.h-text_height)/2.0;
  vinside = v.Inset(border);
}

Checkbox::Checkbox(std::string title, _Var& tv)
  :Var<bool>(tv), title(title)
{
  top = 1.0; bottom = -20;
  left = 0; right = 1.0;
  hlock = LockLeft;
  vlock = LockBottom;
  handler = this;
}

void Checkbox::Mouse(View&, int button, int state, int x, int y)
{
  if( state == 0 )
    a->Set(!a->Get());
}

void Checkbox::ResizeChildren()
{
  raster[0] = v.l + cb_height + 4;
  raster[1] = v.b + (v.h-text_height)/2.0;
  const float h = v.h;
  const float t = (h-cb_height) / 2.0;
  vcb = Viewport(v.l,v.b+t,cb_height,cb_height);
}

void Checkbox::Render()
{
  const bool val = a->Get();

  DrawShadowRect(vcb, val);
  if( val )
  {
    glColor4fv(colour_dn);
    glRect(vcb);
  }
  glColor4fv(colour_tx);
  glRasterPos2fv( raster );
  glutBitmapString(font,(unsigned char*)title.c_str());
}


Slider::Slider(std::string title, _Var& tv)
  :Var<double>(tv), title(title+":")
{
  top = 1.0; bottom = -20;
  left = 0; right = 1.0;
  hlock = LockLeft;
  vlock = LockBottom;
  handler = this;
}

void Slider::Mouse(View& view, int button, int state, int x, int y)
{
  MouseMotion(view,x,y);
}

void Slider::MouseMotion(View&, int x, int y)
{
  if( var->meta_range[0] != var->meta_range[1] )
  {
    const double frac = max(0.0,min(1.0,(double)(x - v.l)/(double)v.w));
    const double val = frac * (var->meta_range[1] - var->meta_range[0]) + var->meta_range[0];
    a->Set(val);
  }
}


void Slider::ResizeChildren()
{
  raster[0] = v.l+2;
  raster[1] = v.b + (v.h-text_height)/2.0;
}

void Slider::Render()
{
  const double val = a->Get();

  if( var->meta_range[0] != var->meta_range[1] )
  {
    DrawShadowRect(v);
    glColor4fv(colour_fg);
    glRect(v);
    glColor4fv(colour_dn);
    const double norm_val = max(0.0,min(1.0,(val - var->meta_range[0]) / (var->meta_range[1] - var->meta_range[0])));
    glRect(Viewport(v.l,v.b,v.w*norm_val,v.h));
  }

  glColor4fv(colour_tx);
  glRasterPos2fv( raster );
  glutBitmapString(font,(unsigned char*)title.c_str());

  string str = boost::lexical_cast<string>(val);
  const unsigned int max_chars = 8;
  if( str.length() > max_chars ) str[max_chars] = '\0';
  const int l = glutBitmapLength(font,(unsigned char*)str.c_str()) + 2;
  glRasterPos2f( v.l + v.w - l, raster[1] );
  glutBitmapString(font,(unsigned char*)str.c_str());
}


}
