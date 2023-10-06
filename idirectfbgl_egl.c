/*
   This file is part of DirectFB.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include <directfbgl.h>
#include <display/idirectfbsurface.h>
#include <EGL/egl.h>

D_DEBUG_DOMAIN( DFBGL_EGL, "DirectFBGL/EGL", "DirectFBGL EGL Implementation" );

static DFBResult Probe    ( void             *ctx );

static DFBResult Construct( IDirectFBGL      *thiz,
                            IDirectFBSurface *surface,
                            IDirectFB        *idirectfb );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBGL, EGL )

/**********************************************************************************************************************/

typedef struct {
     int                    ref;        /* reference counter */

     EGLDisplay             eglDisplay;
     EGLConfig              eglConfig;
     EGLSurface             eglSurface;
     EGLContext             eglContext;

     DFBSurfaceCapabilities caps;
} IDirectFBGL_EGL_data;

static DFBResult
egl_flip_func( void *ctx )
{
     IDirectFBGL_EGL_data *data = ctx;

     eglSwapBuffers( data->eglDisplay, data->eglSurface );

     return DFB_OK;
}

/**********************************************************************************************************************/

static void
IDirectFBGL_EGL_Destruct( IDirectFBGL *thiz )
{
     IDirectFBGL_EGL_data *data = thiz->priv;

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     eglDestroyContext( data->eglDisplay, data->eglContext );

     eglDestroySurface( data->eglDisplay, data->eglSurface );

     eglTerminate( data->eglDisplay );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DirectResult
IDirectFBGL_EGL_AddRef( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_EGL );

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     data->ref++;

     return DFB_OK;
}

static DirectResult
IDirectFBGL_EGL_Release( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_EGL )

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (--data->ref == 0)
          IDirectFBGL_EGL_Destruct( thiz );

     return DFB_OK;
}

static DFBResult
IDirectFBGL_EGL_Lock( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_EGL );

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (!eglMakeCurrent( data->eglDisplay, data->eglSurface, data->eglSurface, data->eglContext )) {
          D_ERROR( "DirectFBGL/EGL: eglMakeCurrent() failed: 0x%x!\n", (unsigned int) eglGetError() );
          return DFB_FAILURE;
     }

     return DFB_OK;
}

static DFBResult
IDirectFBGL_EGL_Unlock( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_EGL );

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (!eglMakeCurrent( data->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )) {
          D_ERROR( "DirectFBGL/EGL: eglMakeCurrent() failed: 0x%x!\n", (unsigned int) eglGetError() );
          return DFB_FAILURE;
     }

     return DFB_OK;
}

static DFBResult
IDirectFBGL_EGL_GetAttributes( IDirectFBGL     *thiz,
                               DFBGLAttributes *ret_attributes )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_EGL );

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (!ret_attributes)
          return DFB_INVARG;

     memset( ret_attributes, 0, sizeof(DFBGLAttributes) );

     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_BUFFER_SIZE,  &ret_attributes->buffer_size );
     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_DEPTH_SIZE,   &ret_attributes->depth_size );
     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_STENCIL_SIZE, &ret_attributes->stencil_size );
     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_RED_SIZE,     &ret_attributes->red_size );
     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_GREEN_SIZE,   &ret_attributes->green_size );
     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_BLUE_SIZE,    &ret_attributes->blue_size );
     eglGetConfigAttrib( data->eglDisplay, data->eglConfig, EGL_ALPHA_SIZE,   &ret_attributes->alpha_size );

     ret_attributes->double_buffer = DFB_TRUE;

     return DFB_OK;
}

static DFBResult
IDirectFBGL_EGL_GetProcAddress( IDirectFBGL  *thiz,
                                const char   *name,
                                void        **ret_address )
{
     void *handle;

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (!name)
          return DFB_INVARG;

     if (!ret_address)
          return DFB_INVARG;

     handle = eglGetProcAddress( name );
     if (!handle)
          return DFB_FAILURE;

     *ret_address = handle;

     return DFB_OK;
}

#if DIRECTFBGL_INTERFACE_VERSION > 1
static DFBResult
IDirectFBGL_EGL_SwapBuffers( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_EGL );

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (!(data->caps & DSCAPS_GL))
          return DFB_OK;

     eglSwapBuffers( data->eglDisplay, data->eglSurface );

     return DFB_OK;
}
#endif

/**********************************************************************************************************************/

static DFBResult
Probe( void *ctx )
{
     return DFB_OK;
}

static DFBResult
Construct( IDirectFBGL      *thiz,
           IDirectFBSurface *surface,
           IDirectFB        *idirectfb )
{
     DFBResult              ret = DFB_FAILURE;
     IDirectFBSurface_data *surface_data;
     EGLint                 num_config;
     EGLint                 config_attr[]  = { EGL_DEPTH_SIZE,      16,
                                               EGL_RED_SIZE,        1,
                                               EGL_GREEN_SIZE,      1,
                                               EGL_BLUE_SIZE,       1,
                                               EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                               EGL_NONE };
     EGLint                 context_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2,
                                               EGL_NONE };

     DIRECT_ALLOCATE_INTERFACE_DATA( thiz, IDirectFBGL_EGL );

     D_DEBUG_AT( DFBGL_EGL, "%s( %p )\n", __FUNCTION__, thiz );

     data->ref = 1;

     setenv( "EGL_PLATFORM", "directfb", 1 );

     data->eglDisplay = eglGetDisplay( (EGLNativeDisplayType) idirectfb );
     if (data->eglDisplay == EGL_NO_DISPLAY) {
          D_ERROR( "DirectFBGL/EGL: eglGetDisplay() failed: 0x%x!\n", (unsigned int) eglGetError() );
          goto error;
     }

     if (!eglInitialize( data->eglDisplay, NULL, NULL )) {
          D_ERROR( "DirectFBGL/EGL: eglInitialize() failed: 0x%x!\n", (unsigned int) eglGetError() );
          goto error;
     }

     if (!eglChooseConfig( data->eglDisplay, config_attr, &data->eglConfig, 1, &num_config ) || (num_config != 1)) {
          D_ERROR( "DirectFBGL/EGL: eglChooseConfig() failed: 0x%x!\n", (unsigned int) eglGetError() );
          goto error;
     }

     data->eglSurface = eglCreateWindowSurface( data->eglDisplay, data->eglConfig, (EGLNativeWindowType) surface, NULL );
     if (!data->eglSurface) {
          D_ERROR( "DirectFBGL/EGL: eglCreateWindowSurface() failed: 0x%x!\n", (unsigned int) eglGetError() );
          goto error;
     }

     data->eglContext = eglCreateContext( data->eglDisplay, data->eglConfig, EGL_NO_CONTEXT, context_attr );
     if (!data->eglContext) {
          D_ERROR( "DirectFBGL/EGL: eglCreateContext() failed: 0x%x!\n", (unsigned int) eglGetError() );
          goto error;
     }

     ret = surface->GetCapabilities( surface, &data->caps );
     if (ret) {
          D_DERROR( ret, "DirectFBGL/EGL: Failed to get surface capabilities!\n" );
          goto error;
     }

     if (!(data->caps & DSCAPS_GL)) {
          surface_data = surface->priv;
          if (!surface_data) {
               ret = DFB_DEAD;
               goto error;
          }

          surface_data->flip_func     = egl_flip_func;
          surface_data->flip_func_ctx = data;
     }

     thiz->AddRef         = IDirectFBGL_EGL_AddRef;
     thiz->Release        = IDirectFBGL_EGL_Release;
     thiz->Lock           = IDirectFBGL_EGL_Lock;
     thiz->Unlock         = IDirectFBGL_EGL_Unlock;
     thiz->GetAttributes  = IDirectFBGL_EGL_GetAttributes;
     thiz->GetProcAddress = IDirectFBGL_EGL_GetProcAddress;
#if DIRECTFBGL_INTERFACE_VERSION > 1
     thiz->SwapBuffers    = IDirectFBGL_EGL_SwapBuffers;
#endif

     return DFB_OK;

error:
     if (data->eglContext)
          eglDestroyContext( data->eglDisplay, data->eglContext );

     if (data->eglSurface)
          eglDestroySurface( data->eglDisplay, data->eglSurface );

     if (data->eglDisplay)
          eglTerminate( data->eglDisplay );

     DIRECT_DEALLOCATE_INTERFACE( thiz );

     return ret;
}
